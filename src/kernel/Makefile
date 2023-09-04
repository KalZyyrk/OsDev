BUILD_DIR?=build/
ASM?=nasm

.PHONY: all kernel clean

all: kernel

stage1: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin:
	$(ASM) boot.asm -f bin -o $(BUILD_DIR)/kernel.bin

clean:
	rm -f $(BUILD_DIR)/kernel.bin