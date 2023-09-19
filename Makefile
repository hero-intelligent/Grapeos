all:	
	make -C ./bootloader
	make -C ./kernel
	make -C ./applib
	make -C ./apps/notepad

clean:
	make clean -C ./bootloader	
	make clean -C ./kernel	
	make clean -C ./applib
	make clean -C ./apps/notepad

newdisk:
	rm -f /media/VMShare/GrapeOS.img
	dd if=/dev/zero of=/media/VMShare/GrapeOS.img bs=1M count=4