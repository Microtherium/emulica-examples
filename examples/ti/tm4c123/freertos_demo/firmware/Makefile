# TI Tiva C TM4C123GH6PM freertos_demo - hand-written bare-metal build,
# no vendor SDK/generator (see target.toml/README.md). Same arm-none-
# eabi-gcc toolchain every other chip in this repo already uses. Real
# ARM_CM4F port (this chip's genuine core, unlike LM3S6965's ARM_CM3) -
# see FreeRTOSConfig.h's own docstring.
TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m4 -mthumb
# Real, load-bearing fact found the hard way (first compile attempt used
# -mfloat-abi=soft with no -mfpu, matching this repo's own M3/M0-class
# demos - failed: `selected FPU does not support instruction --
# 'vstmdbeq r0!,{s16-s31}'`). The real ARM_CM4F port.c contains genuine
# VFP lazy-stacking assembly unconditionally (runtime-gated by FPCCR, not
# compile-time gated by configENABLE_FPU - that flag only controls
# whether the *scheduler* does per-task FPU context switching, not
# whether the port's own assembly exists at all) - the assembler needs
# -mfpu to even recognize those opcodes, regardless of calling
# convention. Matches stm32l476/freertos_demo's own real, already-
# working M4F Makefile exactly (-mfpu=fpv4-sp-d16 -mfloat-abi=hard) -
# safe here too since this demo's own C code uses no floating point,
# hard-float just permits it, it doesn't require it.
FPU = -mfpu=fpv4-sp-d16
FLOAT_ABI = -mfloat-abi=hard
CFLAGS = $(CPU) $(FPU) $(FLOAT_ABI) -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 \
         -IInc \
         -IMiddlewares/Third_Party/FreeRTOS/Source/include \
         -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
         -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
         -IDrivers/CMSIS/Include \
         -MMD -MP

LDFLAGS = $(CPU) $(FPU) $(FLOAT_ABI) -specs=nano.specs -Ttm4c123.ld \
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
