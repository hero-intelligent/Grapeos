.PHONY:all clean

#定义变量#
libPath=../../applib
diskName=GrapeOS.img
diskPath=/media/VMShare/$(diskName)
#共享文件夹中的软盘镜像文件mount后，向其复制较大文件时会会使Linux奔溃重启。在非共享文件夹中复制一个代理镜像进行mount操作。
diskAgent=/root/$(diskName)
mountPath=/mnt/

all:disk

disk:notepad.exe

# -I 表示include头文件的目录
main.o:main.c makefile
	gcc main.c -m32 -c -I $(libPath) -g -std=c99 -Wall

#api.o必须放到链接列表最前面，且_start必须是api.asm中的第一个函数。
notepad.elf:main.o $(libPath)/api.o
	ld -m elf_i386 -Ttext 0x400000 -o notepad.elf -Map notepad.map $(libPath)/api.o main.o 
	objdump -s -d notepad.elf > notepad.elf.dis -M intel

notepad.exe:notepad.elf
	objcopy -I elf32-i386 -O binary notepad.elf notepad.exe
	rm -f $(diskAgent)
	cp $(diskPath) $(diskAgent)
	mount $(diskAgent) $(mountPath) -t msdos -o loop
	cp notepad.exe $(mountPath)
	sync
	umount $(mountPath)
	rm -f $(diskPath)
	cp $(diskAgent) $(diskPath)

clean:
	rm -rf *.exe *.elf *.o *.dis *.map