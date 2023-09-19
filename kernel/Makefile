.PHONY:all clean

#定义变量#
diskName=GrapeOS.img
diskPath=/media/VMShare/$(diskName)
#共享文件夹中的软盘镜像文件mount后，向其复制较大文件时会会使Linux奔溃重启。在非共享文件夹中复制一个代理镜像进行mount操作。
diskAgent=/root/$(diskName)
mountPath=/mnt/
ENTRY_POINT=0x2000
SRC_DIR=src
BUILD_DIR=build
C_FILES=$(wildcard $(SRC_DIR)/*.c)
ASM_FILES=$(wildcard $(SRC_DIR)/*.asm)
C_O_FILES=$(patsubst $(SRC_DIR)%.c,$(BUILD_DIR)%.o,$(C_FILES))
ASM_O_FILES=$(patsubst $(SRC_DIR)%.asm,$(BUILD_DIR)%.o,$(ASM_FILES))
ELF_FILE=$(BUILD_DIR)/kernel.elf
BIN_FILE=$(BUILD_DIR)/kernel.bin
AS=nasm
CC=gcc
LD=ld
ASFLAGS=-f elf32 -g # -g产生调试信息
#-fno-builtin 不采用C语言的内建函数，解决与内建函数重名问题。
#不要被-Wall的名字迷惑，它并没有开启所有告警，-Wextra用于启用一些未由-Wall启用的额外警告标志。 （此选项过去称为-W ，旧名称仍然受支持，但更新的名称更具描述性。
# -g产生调试信息
CFLAGS=-m32 -c -fno-builtin -g -std=c99 -Wall -Wextra 
#-Map顺便生成map文件，可以查看符号的内存布局等信息。-Ttext设置.text节的地址。
LDFLAGS=-m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map

all: $(BIN_FILE)

# $<表示规则中依赖文件中的第1个文件
# $@表示规则中的目标文件名集合
#c文件转.o文件
$(BUILD_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

#汇编文件转.o文件
$(BUILD_DIR)/%.o:$(SRC_DIR)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

#链接所有目标文件，生成可执行文件。#链接时必须把main.o文件放在第一个，源代码中main函数也必须是main.c中的第1个函数。
$(ELF_FILE): build/main.o $(ASM_O_FILES) $(C_O_FILES)
	$(LD) $(LDFLAGS) $^ -o $@	
	objdump -s -d $(ELF_FILE) >$(ELF_FILE).dis -M intel

#从ELF格式文件中抽取出纯二进制文件 
$(BIN_FILE):$(ELF_FILE)
	objcopy -I elf32-i386 -O binary $(ELF_FILE) $(BIN_FILE)	
	rm -f $(diskAgent)
	cp $(diskPath) $(diskAgent)
	mount $(diskAgent) $(mountPath) -t msdos -o loop
	cp $(BIN_FILE) $(mountPath)
	sync
	umount $(mountPath)
	rm -f $(diskPath)
	cp $(diskAgent) $(diskPath)

clean:
	rm -rf $(BUILD_DIR)/*