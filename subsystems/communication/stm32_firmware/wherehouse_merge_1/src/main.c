
//===========================================================================
// ECE 362 lab experiment 10 -- Asynchronous Serial Communication
//===========================================================================

#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "fifo.h"
#include "tty.h"
#include <string.h> // for memset()
#include <stdio.h> // for printf()


#define COMM_DELAY 20 // Number of seconds to wait between commands

#define LIL_INTERVAL 10
#define BIG_INTERVAL 60

#define VERBOSE 1 // Uncomment this declaration to enable verbose mode.



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

int comm_status;
const int LIL_READY     = (1 << 0); // UNUSED
const int BIG_READY     = (1 << 1); // UNUSED
const int TRIGGERED     = (1 << 2);
const int WIFI_STATUS   = (1 << 3);
const int RESET_START   = (1 << 4);
const int RESET_END     = (1 << 5);
// ^ This is for saving space on my "status" variable


char global_buffer[100];
int global_buffer_index = 0;

char g_item_name[32];
char g_item_code[32];
char g_unit_weight[32]; // Convert this to int when needed
char g_tare_weight[32]; // Convert this to int when needed

int global_stock = 10;

int comm_counter;

// Write your subroutines below.


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
	NVIC->ISER[0] |= 1 << 29; //don't know the cmsis symbol for that
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
//
// Prescaler can be anywhere from 1 to 65536.
// (I can't find ARR's range in the Family Reference Manual,
// but I know it can be at least 10000.)
//=====================================================================
void setup_tim6(void) {
    // Enable RCC to timer 6
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    // In Timer 6 Control Register (CR)...
    TIM6->CR1 &= ~TIM_CR1_CEN;
    TIM6->PSC = 47999; // Prescaler = 48000
    TIM6->ARR = 999; // Auto-Reload Register = 1000
    // Should give us a timer that goes off once per second.
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;

    //Unmask in NVIC_ISER to enable interrupt (FRM page 217)
    NVIC->ISER[0] = 1 << TIM6_DAC_IRQn;
}

void TIM6_DAC_IRQHandler(void) {
    // This just toggles two LEDs.
    // One is toggled once per second, the other is toggled
    // once per minute. This functionality can be replaced
    // once the actual functionality is done.
    int led1;
    int led2;
    int respval;

    int buffer_index;

    // Acknowledge interrupt
    TIM6->SR &= ~TIM_SR_UIF;

    // Toggle PC6 (Connected to the rightmost LED
    led1 = GPIOC->ODR & (1<<6);
    if (led1) {
        // If on, turn it off
        GPIOC->ODR &= ~(1<<6);
    }
    else {
        // If off, turn it on
        GPIOC->ODR |= (1<<6);
    }

    // ^ Above code worked on March 28, 2023

    // Big stuff below
    comm_counter = (comm_counter + 1) % COMM_DELAY; // This counts seconds.
    if (comm_counter == 0) {
        // Raise flag to activate Wi-Fi communication
        comm_status |= TRIGGERED;

        // Toggle LED
        led2 = GPIOC->ODR & (1<<7);
        if (led2) {
            GPIOC->ODR &= ~(1<<7);
        }
        else {
            GPIOC->ODR |= (1<<7);
        }
    }
}

//=====================================================================
// Timer 14 setup and interrupt handler.
// (Not currently in use.)
//=====================================================================
void setup_tim14(void)
{
    // No idea why these all say TIM6instead of tim14...
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
	TIM6->CR1 &= ~TIM_CR1_CEN;
	TIM6->PSC = 9599; //9600
	TIM6->ARR = 9999; //10000, will take it to 0.5 Hz
	TIM6->DIER |= TIM_DIER_UIE;
	TIM6->CR1 |= TIM_CR1_CEN;
	NVIC->ISER[0] = 1<<TIM14_IRQn;
}


void TIM14_IRQHandler(void)
{
    TIM14->SR &= ~TIM_SR_UIF; //Acknowledge
    advance_fattime();
}


//=====================================================================
// UART stuff (DEFINITELY used right now
//=====================================================================
int simple_putchar(int x) //Tested, working for single values
{
	while((USART5->ISR & USART_ISR_TXE) == 0);
	USART5->TDR = x;
	return x;
}

int simple_getchar(void)
{
	while((USART5->ISR & USART_ISR_RXNE) == 0);
	return USART5->RDR;
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

//Step 2.5
int better_getchar(void)
{
	while((USART5->ISR & USART_ISR_RXNE) == 0);
	if (USART5->RDR == '\r')
		return '\n';
	return USART5->RDR;
}

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

    printf(cmd);
    while((status == 0) && (try < attempts)) {
        fgets(cbuf, 99, stdin);
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

    if (status == 1) { // Get the value after ACK
        strcpy(store_here, cbuf+3);
    }
    else if (status == -1) {
        strcpy(store_here, cbuf+4);
    }

    // v THIS IS FOR TESTING
    //printf(store_here);
    // ^ This is for testing!
    return status;
}


//const char *cmd_list[] = {
//        "WF+PING!\n",
//        "WF+CONN!\n",
//        "WF+CONN?\n",
//        "WF+CFG!\n",
//        "WF+DCON!\n"
//};
//size_t cmd_list_size = sizeof(cmd_list) / sizeof(cmd_list[0]);
//// Stackoverflow says this works even if the strings in the list are different lengths
//
//const int checks = {1, 0, 1, 0};
//// ^ This isn't being used yet, but I'll eventually use it to decide between
//// whether to move back on a NACK or to repeat the previous command.
//
//
//// TODO: Need variable delay. CONN should be followed immediately by CFG, followed immediately by DCON.
//// Then it's a longer delay.
//
//
//
//// This works for running one command at a time.
//int next_command(int state) {
//    int next_state = state;
//    char f_buffer[32]; // Local buffer for storing strings
//    f_buffer[31] = '\0';
//
//    char current_char;
//
//    //int result = wait_for_response(cmd_list[state]);
//    int result = char_response(cmd_list[state], global_buffer);
//
//    if (result == 1) {
//        next_state = (state + result) % cmd_list_size;
//    }
//    // ACK returns 1, NACK returns -1.
//    // If we get an ACK, move on.
//    // If we get a NACK, try the same command again.
//    // This will need more sophistication in the future.
//
//
//    if (state == 3) {
//        // This is the CFG command. If we get this, we need to parse it.
//        // Format for CFG response: ". $item . | . $name . | . $weight . | . $tare ."
//
//        int k = 0;
//        int n = 0;
//        char *strplist[] = {g_item_code, g_item_name, g_unit_weight, g_tare_weight};
//        // ^ So that I don't have to do a bunch of if statements.
//
//        for (int i = 0; i <= strlen(global_buffer); i++) {
//            if (global_buffer[i] == '|') {
//                strcpy(strplist[k], f_buffer);
//                n = 0;
//                k++;
//            }
//            // Maybe we should exclude '.'?
//            else {
//                f_buffer[n] = global_buffer[i];
//                f_buffer[n+1] = '\0';
//                n++;
//            }
//        }
//        //printf("New item code: %s\n", g_item_code);
//        //printf("New item name: %s\n", g_item_name);
//        //printf("New unit weight: %s\n", g_unit_weight);
//        //printf("New tare weight: %s\n", g_tare_weight);
//
//        //bb|test_name|10|3
//    }
//
//
//    return next_state;
//}
//
//
//
//const char *setup_sequence[] = {
//        "WF+PING!\n",
//        "WF+CONN?\n",
//        "WF+CONN!\n",
//        "WF+AP!\n",// Go into access point mode if connection doesn't work right away.
//        "WF+DCON!\n"// Just testing that the wifi works
//};
//const int setup_ack_incr[] = {
//        1,
//        3,// Skip to disconnect if already connected
//        2,// Skip to disconnect if CONN works
//        2,// Skip to end once in access point mode. If this happens, we should run the sequence again.
//        1//Once disconnected, exit sequence
//};
//const int setup_nack_incr[] = {
//        0,
//        1,
//        0,
//        0,
//        0//Wait for disconnect
//};
//
//const char *cfg_sequence[] = {
//        "WF+CONN?\n",
//        "WF+CONN!\n",
//        "WF+CFG!\n",
//        "WF+DCON!\n"
//};
//const int cfg_ack_incr[] = {
//        2,// If already connected, skip the connection process.
//        1,
//        1,
//        1
//};
//const int cfg_nack_incr[] = {
//        1,
//        0,
//        0,
//        0
//};
//
//
//const char *stock_sequence[] = {
//        "WF+CONN?\n",
//        "WF+CONN!\n",
//        "WF+STOCK!\n",
//        "WF+DCON!\n"
//};
//const int stock_ack_incr[] = {
//        2,
//        1,
//        1,
//        1// Causes code to exit sequence
//};
//const int stock_nack_incr[] = {
//        1,
//        0,
//        0,
//        0
//};
//// ^ These are implemented in the below function.
//
//// FUNCTION BELOW IS NOT DONE YET. This is to resolve big delays between linked commands
//// (i.e. WF+CFG! should happen directly after WF+CONN is checked.
//int run_sequence(char** sequence, int* ack_increment, int* nack_increment) {
//    size_t sequence_length = sizeof(sequence) / sizeof(sequence[0]);
//    int sequence_index = 0;
//    int next_index = 0;
//
//    char f_buffer[32];
//
//    int local_result;
//
//    int update_config = 0;
//
//
//    int try = 0;
//    int timeout = sequence_length * 2;
//    //for (int i = 0; i < sequence_length; i++) {
//    while ((next_index < sequence_length) && (try < timeout)) {
//        // Store any extra stuff in the global buffer
//        local_result = char_response(sequence[sequence_index], global_buffer);
//
//        // Increment.
//        if (local_result == 1) {
//            //ACK
//            next_index = ack_increment[sequence_index];
//        }
//        else if (local_result == -1) {
//            //NACK
//            next_index = nack_increment[sequence_index];
//        }
//
//        // Decide if we need to update the configuration
//        update_config = strcmp(sequence[sequence_index], "WF+CFG!\n");
//        if (update_config) {
//            int k = 0;
//            int n = 0;
//            char *strplist[] = {g_item_code, g_item_name, g_unit_weight, g_tare_weight};
//            // ^ So that I don't have to do a bunch of if statements.
//
//            for (int i = 0; i <= strlen(global_buffer); i++) {
//                if (global_buffer[i] == '|') {
//                    strcpy(strplist[k], f_buffer);
//                    n = 0;
//                    k++;
//                }
//                // Maybe we should exclude '.'?
//                else {
//                    f_buffer[n] = global_buffer[i];
//                    f_buffer[n+1] = '\0';
//                    n++;
//                }
//            }
//        }
//        try++; // Increment this to detect a timeout
//    }
//
//    if (try < timeout) {
//        return 1;
//    }
//    return -1;
//}
//
//
//const char **sequences[] = {
//        setup_sequence,
//        cfg_sequence,
//        stock_sequence
//};
//const int *sequences_ack_incr[] = {
//        cfg_ack_incr,
//        stock_ack_incr
//};
//const int *sequences_nack_incr[] = {
//        cfg_nack_incr,
//        stock_nack_incr
//};



//=====================================================================
// Parse raw text containing "code|item name|unit weight|tare weight"
// into the respective global variables.
//=====================================================================
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


//=====================================================================
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
//=====================================================================
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

    //GPIOC->ODR |= (1<<9);

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

    sprintf(cmd, "%s%ld\n", set__stock, global_stock); //Convert int stock to string
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

//=====================================================================
// Send PING command; wait for response. Return 0 if we receive ACK;
// otherwise, return 1 to indicate not ready.
//=====================================================================
int check_sequence(void) {
    int cmd_response;
    int resp2;
    int errors = 0;

    char f_buffer[32];
    f_buffer[31] = '\0';

    //cmd_response = char_response(run__ping, f_buffer);
    //if (cmd_response != 1) {
    //    errors++;
    //}

    // PING has incorrect functionality right now, so I'm using a workaround.

    cmd_response = char_response(get__connect, f_buffer);
    if (cmd_response != 1) { // NACK = not connected
        cmd_response = char_response(run__connect, f_buffer);
        if (cmd_response != 1) { // NACK again = failed to connect
            errors++;
        }
        cmd_response = char_response(run__disconnect, f_buffer);
    }

    if (errors != 0) {
        return 1;
    }
    else {
        return 0;
    }
}



int main()
{
    // === SETUP GPIO PINS FOR ESP8266 ===
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
    config_pin(GPIOC, 6, 1); // Dev board LED 1 (red)
    config_pin(GPIOC, 7, 1); // Dev board LED 2 (orange)
    config_pin(GPIOC, 8, 1); // Dev board LED 3 (green)
    config_pin(GPIOC, 9, 1); // Dev board LED 4 (blue)

    // Now we can set values to be outputted by the GPIO pins.
    GPIOC->ODR &= ~ESP_RST;
    GPIOC->ODR |= ESP_CH_EN;
    GPIOC->ODR |= ESP_GPIO0;
    GPIOC->ODR |= ESP_GPIO2; // Testing using this as CH_EN
    GPIOC->ODR |= ESP_RST;
    //GPIOC->ODR &= ~ESP_RST; // Reset the chip
    //GPIOC->ODR |= ESP_RST;
    //GPIOC->ODR |= ESP_GPIO2;

    comm_status |= RESET_START; // Signal that reset has been started

    setup_tim6();

    setup_usart5();

    // Uncomment these when you're asked to...
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    enable_tty_interrupt();

    comm_status = 0;
    global_buffer[99] = '\0';

    /*
    if (comm_status & TRIGGERED) {
        comm_status |= RESET_END;
        GPIOC->ODR  |= ESP_RST;
        comm_status &= ~TRIGGERED;
    }
    */

    GPIOC->ODR |= ESP_RST;


    // === MAIN LOOP TEST ===
    int cstate = 0; //Currently unused
    int result;

    for (;;) {
        // Check if module is in station mode and connected to wifi.
        // If not, ping module on every comm cycle until it's ready.

        if (comm_status & TRIGGERED) {
            GPIOC->ODR |= (1<<8);
/*
            if (!(comm_status & RESET_END)) {
                GPIOC->ODR  |= ESP_RST;
                comm_status |= RESET_END;
            }
            else */if (comm_status & WIFI_STATUS) {
                result = wifi_sequence();
                if (result != 0) {
                    comm_status &= ~WIFI_STATUS;
                }
            }
            else {
                result = check_sequence();
                if (result == 0) {
                    comm_status |= WIFI_STATUS;
                }
            }

            comm_status &= ~TRIGGERED;
            GPIOC->ODR &= ~(1<<8);
        }
    }

    //return 0;
}