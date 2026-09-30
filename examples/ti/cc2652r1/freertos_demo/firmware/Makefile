# TI SimpleLink CC2652R1 freertos_demo - hand-written bare-metal build,
# no vendor SDK/generator (see target.toml/README.md). Same arm-none-
# eabi-gcc toolchain every other chip in this repo already uses. Real
# ARM_CM4F port (this chip's genuine core, unlike CC2640R2F's ARM_CM3) -
# reused verbatim from TM4C123's own already-fetched bundle, no new
# fetch needed (both real M4F cores) - see FreeRTOSConfig.h's own
# docstring.
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m4 -mthumb
# Real, load-bearing fact (found the hard way building TM4C123's own
# FreeRTOS demo - see that chip's own Makefile comment for the full
# incident writeup): the real ARM_CM4F port.c contains genuine VFP
# lazy-stacking assembly unconditionally, so the assembler needs
# -mfpu=fpv4-sp-d16 even though this demo's own C code never uses
# floating point.
FPU = -mfpu=fpv4-sp-d16
FLOAT_ABI = -mfloat-abi=hard
CFLAGS = $(CPU) $(FPU) $(FLOAT_ABI) -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc \
         -IMiddlewares/Third_Party/FreeRTOS/Source/include \
         -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
         -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
         -IDrivers/CMSIS/Include \
         -MMD -MP

LDFLAGS = $(CPU) $(FPU) $(FLOAT_ABI) -specs=nano.specs -Tcc2652r1.ld \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

SOURCES = \
	Src/startup.c \
	Src/main.c \
	Src/freertos_hooks.c \
	Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
	Middlewares/Third_Party/FreeRTOS/Source/list.c \
	Middlewares/Third_Party/FreeRTOS/Source/queue.c \
	Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
	Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
	Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c \
	Middlewares/Third_Party/FreeRTOS/Source/timers.c \
	Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
	Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
	Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c

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
