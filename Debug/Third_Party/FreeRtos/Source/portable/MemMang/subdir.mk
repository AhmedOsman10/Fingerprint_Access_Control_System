################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Third_Party/FreeRtos/Source/portable/MemMang/heap_4.c 

OBJS += \
./Third_Party/FreeRtos/Source/portable/MemMang/heap_4.o 

C_DEPS += \
./Third_Party/FreeRtos/Source/portable/MemMang/heap_4.d 


# Each subdirectory must supply rules for building sources it contributes
Third_Party/FreeRtos/Source/portable/MemMang/%.o Third_Party/FreeRtos/Source/portable/MemMang/%.su Third_Party/FreeRtos/Source/portable/MemMang/%.cyclo: ../Third_Party/FreeRtos/Source/portable/MemMang/%.c Third_Party/FreeRtos/Source/portable/MemMang/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DSTM32 -DSTM32F407G_DISC1 -DSTM32F4 -DSTM32F407VGTx -c -I../Inc -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/EEPROM" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/RELAY" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/APP/App" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/RTC" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/FP" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/HAL/FP" -I../USB_HOST/App -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/TASKS" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/include" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/portable/GCC" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/portable/GCC/ARM_CM4F" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/Third_Party/FreeRtos/Source/portable/MemMang" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/MCAL/USART" -I"E:/Learning_WorkSpace - 2/Fingerprint_Access_Control_System/MCAL/System" -I../USB_HOST/Target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Third_Party-2f-FreeRtos-2f-Source-2f-portable-2f-MemMang

clean-Third_Party-2f-FreeRtos-2f-Source-2f-portable-2f-MemMang:
	-$(RM) ./Third_Party/FreeRtos/Source/portable/MemMang/heap_4.cyclo ./Third_Party/FreeRtos/Source/portable/MemMang/heap_4.d ./Third_Party/FreeRtos/Source/portable/MemMang/heap_4.o ./Third_Party/FreeRtos/Source/portable/MemMang/heap_4.su

.PHONY: clean-Third_Party-2f-FreeRtos-2f-Source-2f-portable-2f-MemMang

