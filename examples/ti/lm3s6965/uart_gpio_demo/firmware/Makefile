# TI Stellaris LM3S6965 uart_gpio_demo - hand-written bare-metal build, no
# vendor SDK/generator (see target.toml/README.md). Same arm-none-eabi-gcc
# toolchain every other chip in this repo already uses - no TI-specific
# toolchain dependency.
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m3 -mthumb
# No -mfpu/-mfloat-abi=hard: Cortex-M3 has no FPU. Deliberately
# -mfloat-abi=soft (not the default for this target triple on every
# distro) so no hardware-float instruction is ever emitted by accident.
CFLAGS = $(CPU) -mfloat-abi=soft -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc -MMD -MP

LDFLAGS = $(CPU) -mfloat-abi=soft -specs=nano.specs -Tlm3s6965.ld \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

SOURCES = Src/startup.c Src/main.c
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@
	$(OBJCOPY) -O ihex $@ $(BUILD_DIR)/$(TARGET).hex
	$(OBJCOPY) -O binary -S $@ $(BUILD_DIR)/$(TARGET).bin

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

-include $(OBJECTS:.o=.d)
