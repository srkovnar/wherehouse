################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/display.c \
../src/epd.c \
../src/main.c \
../src/misc.c \
../src/stm32f0xx_it.c \
../src/syscalls.c \
../src/system_stm32f0xx.c 

OBJS += \
./src/display.o \
./src/epd.o \
./src/main.o \
./src/misc.o \
./src/stm32f0xx_it.o \
./src/syscalls.o \
./src/system_stm32f0xx.o 

C_DEPS += \
./src/display.d \
./src/epd.d \
./src/main.d \
./src/misc.d \
./src/stm32f0xx_it.d \
./src/syscalls.d \
./src/system_stm32f0xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F0 -DSTM32F091RCTx -DSTM32F091xC -DUSE_HAL_DRIVER -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/HAL_Driver/Inc/Legacy" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/inc" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/CMSIS/device" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/CMSIS/core" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/HAL_Driver/Inc" -O3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


