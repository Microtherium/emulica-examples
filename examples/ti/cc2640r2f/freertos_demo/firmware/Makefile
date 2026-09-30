# TI SimpleLink CC2640R2F freertos_demo - hand-written bare-metal build,
# no vendor SDK/generator (see target.toml/README.md). Same arm-none-eabi-gcc
# toolchain every other chip in this repo already uses. FreeRTOS kernel
# bundle (portable/GCC/ARM_CM3 included) copied verbatim from
# lm3s6965/freertos_demo's own - both chips are genuine, FPU-less
# Cortex-M3 cores, so no new port fetch was needed (see the
# add-mcu-example skill's own Step 5 guidance).
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m3 -mthumb
# No -mfpu/-mfloat-abi=hard: real Cortex-M3 has no FPU. Deliberately
# -mfloat-abi=soft (matches every other Cortex-M3 demo in this repo,
# including this chip's own gpio_uart_i2c_demo sibling).
CFLAGS = $(CPU) -mfloat-abi=soft -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc \
         -IMiddlewares/Third_Party/FreeRTOS/Source/include \
         -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
         -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 \
         -IDrivers/CMSIS/Include \
         -MMD -MP

LDFLAGS = $(CPU) -mfloat-abi=soft -specs=nano.specs -Tcc2640r2f.ld \
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
	Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c

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
