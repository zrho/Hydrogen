.PHONY: all build link headers clean

TARGET			:= hydrogen
TARGET_BIN		:= $(TARGET).bin

SOURCE_DIR		:= ./src
BUILD_DIR		:= ./build
INC_DIR			:= ./inc
PREFIX			:= ./../build

OBJECTS			:= $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SOURCE_DIR) -name "*.c"))
OBJECTS			+= $(patsubst $(SOURCE_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SOURCE_DIR) -name "*.s"))

CC				:= gcc
AS				:= nasm
LD				:= ld

CFLAGS			:= -m64 -ffreestanding -Wall -Werror -mcmodel=large -I$(INC_DIR)
ASFLAGS			:= -f elf64
LDFLAGS			:= -z max-page-size=0x1000

LDFILE			:= link.ld

all: build link headers

build: $(OBJECTS)

$(BUILD_DIR)/%.s.o: $(SOURCE_DIR)/%.s
	@ echo " AS " $<
	@ mkdir -p $(@D)
	@ $(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.c.o: $(SOURCE_DIR)/%.c
	@ echo " CC " $<
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $< -o $@
	
link:
	@ echo " LD " $(TARGET_BIN)
	@ mkdir -p $(PREFIX)
	@ $(LD) $(LDFLAGS) -T $(LDFILE) -o $(PREFIX)/$(TARGET_BIN) $(OBJECTS)
	
headers:
	@ mkdir -p $(PREFIX)
	@ cp $(INC_DIR)/hydrogen.h $(PREFIX)/
	
clean:
	@ rm -R $(BUILD_DIR)/*