################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ADC.c \
../src/App.c \
../src/Control.c \
../src/Dac.c \
../src/LCD.c \
../src/Menu.c \
../src/PWM.c \
../src/Rf.c \
../src/Serial.c \
../src/Teclado.c \
../src/TerminalMenu.c \
../src/global.c \
../src/main.c \
../src/syscalls.c \
../src/system_stm32f4xx.c 

OBJS += \
./src/ADC.o \
./src/App.o \
./src/Control.o \
./src/Dac.o \
./src/LCD.o \
./src/Menu.o \
./src/PWM.o \
./src/Rf.o \
./src/Serial.o \
./src/Teclado.o \
./src/TerminalMenu.o \
./src/global.o \
./src/main.o \
./src/syscalls.o \
./src/system_stm32f4xx.o 

C_DEPS += \
./src/ADC.d \
./src/App.d \
./src/Control.d \
./src/Dac.d \
./src/LCD.d \
./src/Menu.d \
./src/PWM.d \
./src/Rf.d \
./src/Serial.d \
./src/Teclado.d \
./src/TerminalMenu.d \
./src/global.d \
./src/main.d \
./src/syscalls.d \
./src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F407VETx -DDEBUG -DSTM32F40XX -DUSE_STDPERIPH_DRIVER -I"C:/Users/MODDE/Desktop/TD2/TP6_2026_BELOSSI/StdPeriph_Driver/inc" -I"C:/Users/MODDE/Desktop/TD2/TP6_2026_BELOSSI/inc" -I"C:/Users/MODDE/Desktop/TD2/TP6_2026_BELOSSI/CMSIS/device" -I"C:/Users/MODDE/Desktop/TD2/TP6_2026_BELOSSI/CMSIS/core" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


