# TI SimpleLink CC2640R2F gpio_uart_i2c_demo - hand-written bare-metal
# build, no vendor SDK/generator (see target.toml/README.md). Same
# arm-none-eabi-gcc toolchain every other chip in this repo already uses.
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m3 -mthumb
# No -mfpu/-mfloat-abi=hard: real Cortex-M3 has no FPU. Deliberately
# -mfloat-abi=soft (matches every other Cortex-M3 demo in this repo).
CFLAGS = $(CPU) -mfloat-abi=soft -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc -MMD -MP

LDFLAGS = $(CPU) -mfloat-abi=soft -specs=nano.specs -Tcc2640r2f.ld \
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
