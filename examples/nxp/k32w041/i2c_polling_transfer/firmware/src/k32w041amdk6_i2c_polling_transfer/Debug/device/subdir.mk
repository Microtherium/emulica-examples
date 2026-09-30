################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../device/system_K32W041AM.c 

C_DEPS += \
./device/system_K32W041AM.d 

OBJS += \
./device/system_K32W041AM.o 


# Each subdirectory must supply rules for building sources it contributes
device/%.o: ../device/%.c device/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__NEWLIB__ -DCPU_K32W041AMZ -DCPU_K32W041AMZ_cm4 -DK32W041AMDK6 -DDK6 -DCPU_JN518X -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0  -D__USE_CMSIS -DDEBUG -I"..\source" -I".." -I"..\drivers" -I"..\device" -I"..\utilities" -I"..\component\serial_manager" -I"..\component\lists" -I"..\component\uart" -I"..\CMSIS" -I"..\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage --specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-device

clean-device:
	-$(RM) ./device/system_K32W041AM.d ./device/system_K32W041AM.o

.PHONY: clean-device

