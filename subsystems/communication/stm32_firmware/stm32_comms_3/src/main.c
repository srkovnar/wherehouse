#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "fifo.h"
#include "tty.h"
#include <string.h> // for memset()
#include <stdio.h> // for printf()


#define COMM_DELAY 20 // Number of seconds to wait between commands

void advance_fattime(void);
void command_shell(void);

const int ESP_CH_EN = (1 << 0);
const int ESP_RST   = (1 << 1);
const int ESP_GPIO0 = (1 << 2);
const int ESP_GPIO2 = (1 << 3);

const char* run__ping       = "WF+PING!\n";
const char* run__connect    = "WF+CONN!\n";
const char* get__connect    = "WF+CONN?\n";
const char* run__config     = "WF+CNFG!\n";
const char* set__stock      = "WF+STCK=";
const char* run__send       = "WF+SEND!\n";
const char* run__disconnect = "WF+DCON!\n";

int comm_status = 0;
const int CF_TRIGGER    = (1 << 0); // Big trigger, raised once every COMM_DELAY seconds.
const int CF_TRIGGER2   = (1 << 1); // Small trigger, raised every second.

const int CF_WFSTATUS   = (1 << 3); // If HIGH, ESP8266 is in station mode
const int CF_WAITING    = (1 << 4); // If HIGH, a command has been sent and we're waiting for response. (not implemented)
const int RESET_START   = (1 << 5); // UNUSED
const int RESET_END     = (1 << 6); // UNUSED
// ^ This is for saving space on my "status" variable


char global_buffer[100];
int global_buffer_index = 0;

char g_item_name[32];
char g_item_code[32];
char g_unit_weight[32]; // Convert this to int as needed
char g_tare_weight[32]; // Convert this to int as needed

int g_unit_value;
int g_tare_value;

int global_stock = 10;

int comm_counter; // Increments with 1-Hz timer interrupt up to COMM_DELAY






volatile int display_update_flag = 1;
int previous_quantity = 0;
int quantity = 0;






//=====================================================================
// Configure GPIO Pin mode.
// x = Pin number (0, 1, 2, etc.)
// mode = The mode with which to configure the pin
//=====================================================================
void config_pin(GPIO_TypeDef *g, uint8_t x, uint8_t mode)
{
	//INPUT = 00, OUTPUT = 01, ALTERNATE = 10, ANALOG = 11
	g->MODER &= ~(3 << (x * 2));
	g->MODER |= mode << (x * 2);
}

//=====================================================================
// AFR = Alternate Function Register (? I think, it's been a while).
// Need this for enabling UART.
//=====================================================================
void config_afr(GPIO_TypeDef *g, uint8_t x, uint8_t afr)
{
	int addr = x * 4;
	int reg = 0;
	if (x > 8)
	{
		reg = 1;
		addr = (x - 8) * 4;
	}
	g->AFR[reg] &= ~(0xF << addr);
	g->AFR[reg] |= afr << addr;
}



//=====================================================================
// UART setup.
//=====================================================================
void setup_usart5(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN; // Enable clock to GPIO port C
	RCC->AHBENR |= RCC_AHBENR_GPIODEN; // Enable clock to GPIO port B

	config_pin(GPIOC, 12, 2); //Set PC12 to AFR2
	config_afr(GPIOC, 12, 2);

	config_pin(GPIOD, 2, 2); //Set PD2 to AFR2
	config_afr(GPIOD, 2, 2);

	RCC->APB1ENR	|= RCC_APB1ENR_USART5EN;
	USART5->CR1 	&= ~USART_CR1_UE;	//Disable before configuring
	USART5->CR1 	&= ~((1<<12) | (1<<28) | USART_CR1_PCE | USART_CR1_OVER8);
	USART5->CR2 	&= ~(USART_CR2_STOP_0 | USART_CR2_STOP_1);
	USART5->BRR  	 = 0x1A1;
	USART5->CR1		|= USART_CR1_TE | USART_CR1_RE;
	USART5->CR1 	|= USART_CR1_UE;	//Enable after configuring

	while((USART5->ISR & USART_ISR_REACK) == 0);
	while((USART5->ISR & USART_ISR_TEACK) == 0);
}


//=====================================================================
// Configure the UART peripheral to raise an interrupt every time that
// a new character arrives.
//=====================================================================
void enable_tty_interrupt(void)
{
	USART5->CR1 |= USART_CR1_RXNEIE;
	NVIC->ISER[0] |= 1 << USART3_8_IRQn;
}


//=====================================================================
// This is the timer that controls WiFi transactions.
//
// Note that TIM6 is generally used with the DAC; at some point,
// it would be good to switch to using a different timer that has a
// more generalized purpose.
//
// Timer calculation example:
// Prescaler = 48000 -> Divide 48MHz system clock by 48000
// Auto-reload register = 500
// 48 MHz * (1/48000) * (1 / 500) = 2 Hz
// -> Interrupt will occur 2 times per second.
// and interrupt handler
// Prescaler can be anywhere from 1 to 65536.
// (I can't find ARR's range in the Family Reference Manual,
// but I know it can be at least 10000.)
//=====================================================================
void setup_tim3(void) {
    // Enable RCC to timer 3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // In Timer 3 Control Register (CR)...
    TIM3->CR1 &= ~TIM_CR1_CEN;
    TIM3->PSC = 47999; // Prescaler = 48000
    TIM3->ARR = 999; // Auto-Reload Register = 1000
    // Should give us a timer that goes off once per second.
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;

    //Unmask in NVIC_ISER to enable interrupt (FRM page 217)
    NVIC->ISER[0] = 1 << TIM3_IRQn;
}

void TIM3_IRQHandler(void) {
    // Acknowledge interrupt
    TIM3->SR &= ~TIM_SR_UIF;

    // Toggle LED 1
    if (GPIOC->ODR & (1<<7)) {
        GPIOC->ODR &= ~(1<<7);
    }
    else {
        GPIOC->ODR |= (1<<7);
    }

    // Increment secondary counter
    comm_counter = (comm_counter + 1) % COMM_DELAY;// This counts seconds
    if (comm_counter == 0) {
        // Raise flag
        comm_status |= CF_TRIGGER;

        // Toggle LED 2
        if (GPIOC->ODR & (1<<9)) {
            GPIOC->ODR &= ~(1<<9);
        }
        else {
            GPIOC->ODR |= (1<<9);
        }
    }
}


//Step 2.5
int better_putchar(int x)
{
	if (x == '\n')
	{
		while((USART5->ISR & USART_ISR_TXE) == 0);
		USART5->TDR = '\r';
	}
	while((USART5->ISR & USART_ISR_TXE) == 0);
	USART5->TDR = x;
	return x;
}

////Step 2.5
//int better_getchar(void)
//{
//	while((USART5->ISR & USART_ISR_RXNE) == 0);
//	if (USART5->RDR == '\r')
//		return '\n';
//	return USART5->RDR;
//}

//Step 2.7
int interrupt_getchar(void)
{
	USART_TypeDef *u = USART5;
	// Wait for a newline to complete the buffer.


	while(fifo_newline(&input_fifo) == 0) {
	    asm volatile ("wfi");
	}


	// Return a character from the line buffer.
	char ch = fifo_remove(&input_fifo);
	return ch;
}


//=====================================================================
// Pre-defined "weak" method used in printf() and putchar(),
// among other system functions.
//=====================================================================
int __io_putchar(int ch)
{
	//return simple_putchar(ch); //Step 2.4
	return better_putchar(ch); //Step 2.5
}


//=====================================================================
// Pre-defined "weak" method used in fgets() and getchar(),
// among other system functions.
//=====================================================================
int __io_getchar(void)
{
	//return simple_getchar(); //Step 2.4
	//return better_getchar(); //Step 2.5
	//return line_buffer_getchar(); //Step 2.6
	return interrupt_getchar(); //Step 2.7
}


//=====================================================================
// Used in Part 2.7
//
// Interrupt handler for USART5. For details about the interrupt,
// see enable_tty_interrupt(); that's where the interrupt is configured.
// Basically, every time a new character is received over UART,
// this handler is triggered.
//
// (I think, it's been a while since I wrote this code).
//=====================================================================
void USART3_4_5_6_7_8_IRQHandler(void)
{
	USART_TypeDef *u = USART5;
	// If we missed reading some characters, clear the overrun flag.
	if (u->ISR & USART_ISR_ORE)
		u->ICR |= USART_ICR_ORECF;
	int x = u->RDR;
	if (fifo_full(&input_fifo))
		return;
	insert_echo_char(x); // From tty.c
}


// Write your subroutines above.


//=====================================================================
// Wait for either "ACK" or "NACK" from device.
//
// This should really be done in a non-blocking way...
//
// Status:
//    0 = not done yet (or timeout)
//    1 = ACK
//   -1 = NACK
//=====================================================================
int wait_for_response(const char* cmd) {
    int status = 0;
    char cbuf[100];

    int try = 0;
    int attempts = 3;

    printf(cmd);
    while ((status == 0) && (try < attempts)) {
        fgets(cbuf, 99, stdin);
        cbuf[99] = '\0';
        if (strcmp(cbuf, "ACK\n") == 0) {
            status = 1;
        }
        else if (strcmp(cbuf, "NACK\n") == 0) {
            status = -1;
        }
        try++;
    }
    return status;
}

int char_response(const char* cmd, char* store_here) {
    int status = 0;
    char cbuf[100];

    char atest[4];
    char ntest[5];

    int try = 0;
    int attempts = 3;

    // Send command over UART
    printf(cmd);

    // Wait for response
    while((status == 0) && (try < attempts)) {
        fgets(cbuf, 99, stdin); // uses __io_getchar, which uses interrupt_getchar to wait for interrupt on \r
        cbuf[99] = '\0';

        strncpy(atest, cbuf, 3);
        atest[3] = '\0';
        strncpy(ntest, cbuf, 4);
        ntest[4] = '\0';

        if (strcmp(atest, "ACK") == 0) {
            status = 1;
        }
        if (strcmp(ntest, "NACK") == 0) {
            status = -1;
        }

        try++;
    }

    // Move the rest of the string into store_here[]
    if (status == 1) { // Get the value after ACK
        strcpy(store_here, cbuf+3);
    }
    else if (status == -1) {
        strcpy(store_here, cbuf+4);
    }

    // Return 1 if ACK, -1 if NACK, 0 if no response.
    return status;
}


//---------------------------------------------------------------------
// Parse raw text containing "code|item name|unit weight|tare weight"
// into the respective global variables.
//---------------------------------------------------------------------
void parse_configuration(const char* raw_text) {
    int k = 0; // Storage location counter
    int n = 0; // Buffer index

    char delimiter = '|';

    char p_buffer[32];
    p_buffer[31] = '\0'; // Just in case

    char* strplist[] = {g_item_code, g_item_name, g_unit_weight, g_tare_weight};
    // ^ Avoids if-statements
    char ignore_list[] = {':'}; // Not implemented

    //for (int i = 0; i <= strlen(raw_text); i++) {
    for (int i = 1; i <= strlen(raw_text); i++) { //Ignore the first character, it's a colon.
        if (raw_text[i] == delimiter) {
            strcpy(strplist[k], p_buffer);
            n = 0;
            k++;
        }
        // Maybe we should exclude '.'...
        else {
            p_buffer[n] = raw_text[i];
            p_buffer[n+1] = '\0';
            n++;
        }
    }
}


//---------------------------------------------------------------------
// Preliminary function to perform all of the typical wifi interactions
// one after another.
// 1. Send WF+CONN!
// 2. Send WF+CNFG!
// 3. Parse configuration string
// 4. Send WF+STCK=stock
// 5. Send WF+SEND!
// 6. Send WF+DCON!
//
// If at any point in the process we receive a NACK, return a nonzero
// number. This will put the STM32 back in waiting mode, where it just
// repeatedly pings the ESP8266 to see if it's ready and connected
// to WiFi.
//---------------------------------------------------------------------
int wifi_sequence(void) {
    int cmd_response;
    int errors = 0;
    char f_buffer[32];
    f_buffer[31] = '\0'; // Just in case
    char cmd[32];
    cmd[31] = '\0';

    char temp[32];
    temp[31] = '\0';

    int stock = 0;

    cmd_response = char_response(run__connect, f_buffer);
    if (cmd_response != 1) {
        errors++;
    }

    cmd_response = char_response(run__config, f_buffer);
    if (cmd_response != 1) {
        errors++;
    }
    parse_configuration(f_buffer);

    // Stock calculations would happen here

    sprintf(cmd, "%s%ld\n", set__stock, quantity); //Convert int stock to string
    cmd_response = char_response(cmd, f_buffer);
    if (cmd_response != 1) {
        errors++;
    }

    cmd_response = char_response(run__send, f_buffer);
    if (cmd_response != 1) {
        errors++;
    }

    cmd_response = char_response(run__disconnect, f_buffer);
    if (cmd_response != 1) {
        errors++;
    }

    if (errors != 0) {
        return 1;
    }
    else {
        return 0;
    }
}

//---------------------------------------------------------------------
// Send PING command; wait for response. Return 0 if we receive ACK;
// otherwise, return 1 to indicate not ready.
//---------------------------------------------------------------------
int ping_sequence(void) {
    int cmd_response;
    int resp2;
    int errors = 0;

    char f_buffer[32];
    f_buffer[31] = '\0';

    cmd_response = char_response(run__ping, f_buffer);
    if (cmd_response != 1) {
        errors++;
    }

    if (errors != 0) {
        return 1;
    }
    else {
        return 0;
    }
}

//=====================================================================
//                              MERGE
//=====================================================================


void display_ui()
{
    int failed = 0;
    char qty_str[12];

    DISPLAY_clear(0);

    DISPLAY_drawChar(10, 10, '2', 1);
    DISPLAY_drawRect(0, 0, 264, 176, 1);
    DISPLAY_drawRect(5, 5, 254, 166, 0);
    DISPLAY_drawRect(0, 126, 230, 50, 1);
    DISPLAY_drawRect(5, 131, 220, 40, 0);

    DISPLAY_print(9, 133, 1, "Shelf ID: \0");
    DISPLAY_print(9, 80, 1, "Item: 100mH \0");
    DISPLAY_print(9, 40, 1, "Quantity: \0");

    if ( (failed = dtostr(quantity, qty_str, 12) )  == -1 || failed == 0)
    {
        DISPLAY_clear(0);
        DISPLAY_print(4, 40, 1, "ERR: Failed to get string rep\0");
    }
    else
    {
        //DISPLAY_clear(0);
        DISPLAY_print(160,40, 1, qty_str);
        //DISPLAY_print(20,150, 1, qty_str);
        //dtostr(123, qty_str, 12);
        //DISPLAY_print(5,30, 1, qty_str);
        //dtostr(1234, qty_str, 12);
        //DISPLAY_print(5,60, 1, qty_str);
        //dtostr(12345, qty_str, 12);
        //DISPLAY_print(5,90, 1, qty_str);
        //dtostr(123456, qty_str, 12);
        //DISPLAY_print(5,120, 1, qty_str);
        //dtostr(0, qty_str, 12);
        //DISPLAY_print(5,150, 1, qty_str);
    }

    DISPLAY_update();
}


//

void init_tim6()
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->CR1 &= ~TIM_CR1_CEN;

    TIM6->PSC = 48000 - 1;
    TIM6->ARR = 1000 - 1;
    TIM6->DIER |= 0x1;

    TIM6->CR1 |= TIM_CR1_CEN;

    NVIC->ISER[0] |= 0x1 << 17;
}

void TIM6_DAC_IRQHandler ()
{
    TIM6->SR &= ~0x1;
    if (quantity != previous_quantity)
    {
        display_update_flag = 1;
    }
}

void init_sensors()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOB->MODER |= 0<<(2*14);
    GPIOB->ODR |= 1<<14;
    GPIOB->MODER |= 0<<(2*12);
    GPIOB->ODR |= 1<<12;
    GPIOB->MODER |= 0<<(2*11);
    GPIOB->ODR |= 1<<11;
    GPIOB->MODER |= 0<<(2*10);
    GPIOB->ODR |= 1<<10;
    GPIOB->MODER |= 0<<(2*9);
    GPIOB->ODR |= 1<<9;
    GPIOB->MODER |= 0<<(2*8);
    GPIOB->ODR |= 1<<8;
    GPIOB->MODER &= ~(3<<(2*13));
    GPIOB->MODER |= 1<<(2*13);
    GPIOC->MODER |= 1<<(2*6);
    GPIOB->BRR = GPIO_ODR_13;
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH4 | GPIO_AFRH_AFRH5 | GPIO_AFRH_AFRH7);
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    SPI2->CR1 |= SPI_CR1_BIDIOE | SPI_CR1_BIDIMODE | SPI_CR1_MSTR | 0x38;   //SPI_CR1_BR;
    SPI2->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI2->CR2 = SPI_CR2_DS_3 | SPI_CR2_DS_0 | SPI_CR2_SSOE | SPI_CR2_NSSP;
    SPI2->CR1 |= SPI_CR1_SPE;
}



int main()
{
    // === DISPLAY SETUP ===
    uint32_t value;
    uint32_t fixer;
    uint32_t checker;
    int offset;
    int avg;
    int esp8266_weight = 154;
    int item_weight = 5;
    avg = 0;
    fixer = 0x800000;
    offset = 0;


    init_sensors();
    init_tim6();
    DISPLAY_init();


    // === COMMUNICATION SETUP ===
    // PC0 -> ESP_CH_EN     (STM32 output)
    // PC1 -> ESP_RST       (STM32 output, active low?)
    // PC2 -> ESP_GPIO0     (STM32 output, active low?)
    // PC3 -> ESP_GPIO2     (nothing for now, reserved for future use)

    // The UART pins are being setup by the setup_usart5() function.
    // PC12 -> UART TX (ESP RX)
    // PD2 -> UART RX (ESP TX)

    // Step 1) Enable clocks to GPIO registers in use (I'm only using C for GPIO)
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // Step 2) Configure GPIO pins as input or output (or analog or alternate function)
    config_pin(GPIOC, 0, 1);
    config_pin(GPIOC, 1, 1);
    config_pin(GPIOC, 2, 1);
    config_pin(GPIOC, 3, 1); // I'm not actually using GPIO2 right now, so I'm gonna
    // link it to an LED to demo that my timer works.
    config_pin(GPIOC, 7, 1); // Dev board LED 2 (orange)
    config_pin(GPIOC, 8, 1); // Dev board LED 3 (green)
    config_pin(GPIOC, 9, 1); // Dev board LED 4 (blue)


    // Now we can set values to be outputted by the GPIO pins.
    GPIOC->ODR &= ~ESP_RST;
    GPIOC->ODR |= ESP_CH_EN;
    GPIOC->ODR |= ESP_GPIO0;
    GPIOC->ODR |= ESP_GPIO2; // Testing using this as CH_EN

    comm_status |= RESET_START; // Signal that reset has been started

    //setup_tim6();
    setup_tim3();

    setup_usart5();

    // Uncomment these when you're asked to...
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    enable_tty_interrupt();

    global_buffer[99] = '\0';

    GPIOC->ODR |= ESP_RST;
    comm_status |= RESET_END;


    // === MAIN LOOP TEST ===
    int cstate = 0; //Currently unused
    int result;

    for (;;) {
        // Check if module is in station mode and connected to wifi.
        // If not, ping module on every comm cycle until it's ready.

        if (comm_status & CF_TRIGGER) {
            GPIOC->ODR |= (1<<8);// Turn LED on; comm handling is starting

            if (comm_status & CF_WFSTATUS) {
                result = wifi_sequence();
                if (result != 0) {
                    comm_status &= ~CF_WFSTATUS;
                }
            }
            else {
                result = ping_sequence();
                if (result == 0) {
                    comm_status |= CF_WFSTATUS;
                }
            }

            GPIOC->ODR &= ~(1<<8); // Turn LED off; comm handling is done
            comm_status &= ~CF_TRIGGER; // Lower TRIGGERED flag
        }

        offset = 0;
        checker = (GPIOB->IDR);


        // LOOP 1
        while (GPIOB->IDR & (1<<14)){}
        //for (int x = 0; x < 100; x++){
        value = 0;
        for (int i = 0; i <24; i++){
            wait(5);
            GPIOB->BSRR = GPIO_ODR_13;
            value = value << 1;
            GPIOB->BRR = GPIO_ODR_13;
            if (GPIOB->IDR & (1<<14)){
                value += 1;
            }
            //GPIOB->BSRR = GPIO_ODR_13;
            //GPIOB->BRR = GPIO_ODR_13;
        }


        value = value ^ fixer;
        //    avg += value;
        //}
        //avg = avg / 100;
        //int show = value;
        int balance = (value / 15000) - offset;

        //Buttons
        if ((GPIOB->IDR & (1<<12))){GPIOC->BSRR = GPIO_ODR_6;}
        if (!(GPIOB->IDR & (1<<12))){GPIOC->BRR = GPIO_ODR_6;}
        int holder = GPIOB->IDR & (1<<12);

        // LOOP 2

        while (!(holder & (1<<12))){
            if (!((GPIOB->IDR & (1<<11)))){
                balance += 1;
            }
            if (!((GPIOB->IDR & (1<<11)))){
                balance -= 1;
            }
            if (!((GPIOB->IDR & (1<<9)))){
                balance = 0;
            }
            if (!((GPIOB->IDR & (1<<8)))){
                offset = value / 15000;
            }
            if (!((GPIOB->IDR & (1<<12)))){
                holder = (1<<12);
            }
        }




        //avg = 0;

        quantity =   ((value)/450 - 18480)/item_weight;
        if ( display_update_flag == 1) {
            display_ui();

            display_update_flag = 0;
            previous_quantity = quantity;
        }
    }
}
