################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32.s 

OBJS += \
./startup/startup_stm32.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/HAL_Driver/Inc/Legacy" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/inc" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/CMSIS/device" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/CMSIS/core" -I"/home/sam/workspace/wherehouse/subsystems/power-and-output/HAL_Driver/Inc" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


