################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HAL/INTERNAL_RTC/INTERNAL_RTC.c 

OBJS += \
./HAL/INTERNAL_RTC/INTERNAL_RTC.o 

C_DEPS += \
./HAL/INTERNAL_RTC/INTERNAL_RTC.d 


# Each subdirectory must supply rules for building sources it contributes
HAL/INTERNAL_RTC/%.o HAL/INTERNAL_RTC/%.su HAL/INTERNAL_RTC/%.cyclo: ../HAL/INTERNAL_RTC/%.c HAL/INTERNAL_RTC/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DSTM32 -DSTM32F407G_DISC1 -DSTM32F4 -DSTM32F407VGTx -c -I../Inc -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/INTERNAL_RTC" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/INTERNAL_RTC" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/EEPROM" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/RELAY" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/APP/App" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/RTC" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/FP" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/FP" -I../USB_HOST/App -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/TASKS" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/include" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/portable/GCC" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/portable/GCC/ARM_CM4F" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/portable/MemMang" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/MCAL/USART" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/MCAL/I2C" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/MCAL/System" -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-HAL-2f-INTERNAL_RTC

clean-HAL-2f-INTERNAL_RTC:
	-$(RM) ./HAL/INTERNAL_RTC/INTERNAL_RTC.cyclo ./HAL/INTERNAL_RTC/INTERNAL_RTC.d ./HAL/INTERNAL_RTC/INTERNAL_RTC.o ./HAL/INTERNAL_RTC/INTERNAL_RTC.su

.PHONY: clean-HAL-2f-INTERNAL_RTC

