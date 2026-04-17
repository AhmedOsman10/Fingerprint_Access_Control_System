################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HAL/RTC/RTC.c 

OBJS += \
./HAL/RTC/RTC.o 

C_DEPS += \
./HAL/RTC/RTC.d 


# Each subdirectory must supply rules for building sources it contributes
HAL/RTC/%.o HAL/RTC/%.su HAL/RTC/%.cyclo: ../HAL/RTC/%.c HAL/RTC/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DSTM32 -DSTM32F407G_DISC1 -DSTM32F4 -DSTM32F407VGTx -c -I../Inc -I"E:/Learning_WorkSpace - 2/Fingerprint/APP/App" -I"E:/Learning_WorkSpace - 2/Fingerprint/HAL/RTC" -I"E:/Learning_WorkSpace - 2/Fingerprint/HAL/FP" -I"E:/Learning_WorkSpace - 2/Fingerprint/HAL/FP" -I../USB_HOST/App -I"E:/Learning_WorkSpace - 2/Fingerprint/TASKS" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party/FreeRtos" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party/FreeRtos/Source" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party/FreeRtos/Source/include" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party/FreeRtos/Source/portable/GCC" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party/FreeRtos/Source/portable/GCC/ARM_CM4F" -I"E:/Learning_WorkSpace - 2/Fingerprint/Third_Party/FreeRtos/Source/portable/MemMang" -I"E:/Learning_WorkSpace - 2/Fingerprint/MCAL/USART" -I"E:/Learning_WorkSpace - 2/Fingerprint/MCAL/System" -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-HAL-2f-RTC

clean-HAL-2f-RTC:
	-$(RM) ./HAL/RTC/RTC.cyclo ./HAL/RTC/RTC.d ./HAL/RTC/RTC.o ./HAL/RTC/RTC.su

.PHONY: clean-HAL-2f-RTC

