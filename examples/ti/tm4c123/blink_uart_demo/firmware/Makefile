# TI Tiva C TM4C123GH6PM blink_uart_demo - hand-written bare-metal build,
# no vendor SDK/generator (see target.toml/README.md). Same arm-none-eabi-gcc
# toolchain every other chip in this repo already uses - no TI-specific
# toolchain dependency.
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m4 -mthumb
# No -mfpu/-mfloat-abi=hard: this demo does no floating point at all, so
# soft-float (valid on any Cortex-M, including this chip's real M4F core)
# avoids FPU complexity entirely rather than exercising it needlessly -
# same minimalism LM3S6965's own demo uses for its (real, FPU-less)
# Cortex-M3 core, just a deliberate choice here rather than the only
# option.
CFLAGS = $(CPU) -mfloat-abi=soft -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc -MMD -MP

LDFLAGS = $(CPU) -mfloat-abi=soft -specs=nano.specs -Ttm4c123.ld \
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
