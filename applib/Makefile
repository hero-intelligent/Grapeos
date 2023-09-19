all:api.o

api.o:api.asm
	nasm api.asm -f elf32 -o api.o

clean:
	rm -rf api.o	