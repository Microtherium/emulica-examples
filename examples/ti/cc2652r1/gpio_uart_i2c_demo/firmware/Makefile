# TI SimpleLink CC2652R1 gpio_uart_i2c_demo - hand-written bare-metal
# build, no vendor SDK/generator (see target.toml/README.md). Same
# arm-none-eabi-gcc toolchain every other chip in this repo already uses.
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m4 -mthumb
# No -mfpu/-mfloat-abi=hard: this bare-metal demo never uses floating
# point and links no FreeRTOS port code (the port.c that actually forces
# real FPU flags on an M4F build - see freertos_demo's own Makefile) -
# -mfloat-abi=soft is safe and simpler here, same convention TM4C123's
# own bare-metal blink_uart_demo already uses despite also being a real
# Cortex-M4F part.
CFLAGS = $(CPU) -mfloat-abi=soft -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc -MMD -MP

LDFLAGS = $(CPU) -mfloat-abi=soft -specs=nano.specs -Tcc2652r1.ld \
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
