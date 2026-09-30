################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := k32w041amdk6_i2c_polling_transfer
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: k32w041amdk6_i2c_polling_transfer.axf

# Tool invocations
k32w041amdk6_i2c_polling_transfer.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="k32w041amdk6_i2c_polling_transfer.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mthumb -T k32w041amdk6_i2c_polling_transfer_Debug.ld -o "k32w041amdk6_i2c_polling_transfer.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) k32w041amdk6_i2c_polling_transfer.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "k32w041amdk6_i2c_polling_transfer.axf"; echo "win32" | grep -q win* && py -3 "../scripts/dk6_image_tool.py" -s 294912  k32w041amdk6_i2c_polling_transfer.axf || python3 "../scripts/dk6_image_tool.py" -s 294912  k32w041amdk6_i2c_polling_transfer.axf; arm-none-eabi-objcopy -v -O binary "k32w041amdk6_i2c_polling_transfer.axf" "k32w041amdk6_i2c_polling_transfer.bin"
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
