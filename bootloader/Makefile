diskName=GrapeOS.img
diskPath=/media/VMShare/$(diskName)
#共享文件夹中的软盘镜像文件mount后，向其复制较大文件时会会使Linux奔溃重启，故在非共享文件夹中复制一个代理镜像进行mount操作。
diskAgent=/root/$(diskName)
mountPath=/mnt/

all:boot.bin loader.bin

boot.bin:boot.asm
	nasm boot.asm -o boot.bin -l boot.lst
	dd conv=notrunc if=$@ of=$(diskPath)

loader.bin:loader.asm
	nasm loader.asm -o loader.bin -l loader.lst
	rm -f $(diskAgent)
	cp $(diskPath) $(diskAgent)
	mount $(diskAgent) $(mountPath) -t msdos -o loop
	cp $@ $(mountPath)
	sync
	umount $(mountPath)
	rm -f $(diskPath)
	cp $(diskAgent) $(diskPath)

clean:
	rm -rf *.bin *.lst