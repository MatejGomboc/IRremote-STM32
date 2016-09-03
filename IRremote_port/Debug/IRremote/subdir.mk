################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../IRremote/IRremote.c \
../IRremote/irRecv.c \
../IRremote/irSend.c \
../IRremote/ir_RC5_RC6.c \
../IRremote/ir_Sony.c 

OBJS += \
./IRremote/IRremote.o \
./IRremote/irRecv.o \
./IRremote/irSend.o \
./IRremote/ir_RC5_RC6.o \
./IRremote/ir_Sony.o 

C_DEPS += \
./IRremote/IRremote.d \
./IRremote/irRecv.d \
./IRremote/irSend.d \
./IRremote/ir_RC5_RC6.d \
./IRremote/ir_Sony.d 


# Each subdirectory must supply rules for building sources it contributes
IRremote/%.o: ../IRremote/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo %cd%
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F411xE -I"C:/Users/user/Desktop/nucleo_workspace/IRremote_port/Inc" -I"C:/Users/user/Desktop/nucleo_workspace/IRremote_port/Drivers/STM32F4xx_HAL_Driver/Inc" -I"C:/Users/user/Desktop/nucleo_workspace/IRremote_port/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"C:/Users/user/Desktop/nucleo_workspace/IRremote_port/Drivers/CMSIS/Include" -I"C:/Users/user/Desktop/nucleo_workspace/IRremote_port/Drivers/CMSIS/Device/ST/STM32F4xx/Include"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


