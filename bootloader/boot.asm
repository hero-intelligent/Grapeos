;FAT16目录项结构(32B)：
;名称		    偏移	    长度	描述
DIR_Name	    equ 0x00	;11	文件名8B，扩展名3B
DIR_Attr	    equ 0x0b	;1	文件属性
;保留	        0x0c	    10	保留位
DIR_WrtTime     equ 0x16	;2	最后一次写入时间
DIR_WrtDate     equ 0x18	;2	最后一次写入日期
DIR_FstClus     equ 0x1a	;2	起始簇号
DIR_FileSize    equ 0x1c	;4	文件大小

BOOT_ADDRESS equ 0x7c00
LOADER_ADDRESS equ 0x1000
STACK_BOTTOM equ LOADER_ADDRESS
VIDEO_SEGMENT_ADDRESS equ 0xb800 ;显存段地址（默认显示模式为25行80列字符模式）
VIDEO_CHAR_MAX_COUNT equ 2000 ;默认屏幕最多显示字符数。
DISK_BUFFER equ 0x7e00 ;读磁盘临时存放数据用的缓存区，放到boot程序之后。
DISK_SIZE_M equ 4 ;磁盘容量，单位M。
FAT1_SECTORS equ 32 ;FAT1占用扇区数
ROOT_DIR_SECTORS equ 32 ;根目录占用扇区数
SECTOR_NUM_OF_FAT1_START equ 1 ;FAT1起始扇区号
SECTOR_NUM_OF_ROOT_DIR_START equ 33 ;根目录起始扇区号
SECTOR_NUM_OF_DATA_START equ 65 ;数据区起始扇区号，对应簇号为2。
SECTOR_CLUSTER_BALANCE equ 63 ;簇号加上该值正好对应扇区号。
FILE_NAME_LENGTH equ 11 ;文件名8字节加扩展名3字节共11字节。
DIR_ENTRY_SIZE equ 32 ;目录项为32字节。
DIR_ENTRY_PER_SECTOR equ 16 ;每个扇区能存放目录项的数目。

org BOOT_ADDRESS
jmp short boot_start
nop

BS_OEMName 		db 'GrapeOS1'   ;生产厂商名8字节
BPB_BytesPerSec dw 0x0200		;每扇区字节数
BPB_SecPerClus	db 0x01			;每簇扇区数
BPB_RsvdSecCnt	dw 0x0001		;保留扇区数
BPB_NumFATs		db 0x01			;FAT表的份数
BPB_RootEntCnt	dw 0x0200		;根目录可容纳的目录项数
BPB_TotSec16	dw 0x2000 		;总扇区数
BPB_Media		db 0xf8 		;介质描述符（0xF8表示硬盘，0xF0表示高密度的3.5寸软盘。）
BPB_FATSz16		dw 0x0020		;每FAT扇区数
BPB_SecPerTrk	dw 0x0020		;每磁道扇区数
BPB_NumHeads	dw 0x0040		;磁头数
BPB_hiddSec		dd 0x00000000	;隐藏扇区数
BPB_TotSec32	dd 0x00000000	;如果BPB_TotSec16值为0，则由这个值记录扇区数
BS_DrvNum		db 0x80			;int 13h的驱动器号（软盘0x00，硬盘0x80）
BS_Reserved1	db 0x00			;未使用
BS_BootSig		db 0x29			;扩展引导标记
BS_VolID		dd 0x0	        ;卷序列号
BS_VolLab		db 'Grape OS   ';卷标11字节
BS_FileSysType	db 'FAT16   '	;文件系统类型8字节
;引导扇区   1个扇区     扇区0           0x0000~0x01ff
;FAT1       32个扇区    扇区1~32        0x0200~0x41ff   可容纳8K个簇
;FAT2       无
;根目录     32个扇区    扇区33~64       0x4200~0x81ff   可容纳512个目录项
;数据区     8127个扇区  扇区65~0x1fff   0x8200~0x3fffff

boot_start:
;初始化寄存器
mov ax,cs
mov ds,ax
mov es,ax
mov fs,ax
mov ss,ax
mov sp,STACK_BOTTOM
mov ax,VIDEO_SEGMENT_ADDRESS
mov gs,ax ;本程序中gs专用于指向显存段

;清屏
call func_clear_screen

;打印字符串："boot start"
mov si,boot_start_string
mov di,0 ;屏幕第1行显示
call func_print_string

;读取根目录的第1个扇区(1个扇区可以存放16个目录项)【计划根目录目录项个数不能超过16个】
mov esi,SECTOR_NUM_OF_ROOT_DIR_START 
mov di,DISK_BUFFER
call func_read_one_sector16

;查找loader.bin文件
cld ;cld告诉程序si，di向地址增加的方向移动，std相反。
mov bx,0 ;用bx记录遍历第几个目录项。
next_dir_entry:
mov si,bx
shl si,5 ;乘以32（目录项的大小）
add si,DISK_BUFFER
mov di,loader_file_name_string
mov cx,FILE_NAME_LENGTH ;字符比较次数为FAT16文件名长度，每比较一个字符，cx会自动减一。
repe cmpsb ;如果相等则比较下一个字节，直到ZF为0或CX为0，ZF为0表示发现不相等的字节，CX为0表示这一串字节相等。
jcxz loader_found
inc bx
cmp bx,DIR_ENTRY_PER_SECTOR
jl next_dir_entry
jmp loader_not_found

loader_found:
;打印字符串："loader found"
mov si,loader_found_string
mov di,80 ;屏幕第2行显示
call func_print_string

;获取loader起始簇号
shl bx,5 ;乘以32
add bx,DISK_BUFFER
mov bx,[bx+DIR_FstClus] ;保存loader的起始簇号

;读取FAT1的第1个扇区【计划所有文件只用到该扇区中的簇号】
mov esi,SECTOR_NUM_OF_FAT1_START 
mov di,DISK_BUFFER ;放到boot程序之后
call func_read_one_sector16

;按簇读loader
mov bp,LOADER_ADDRESS ;目标地址初值
read_loader:
xor esi,esi
mov si,bx
add esi,SECTOR_CLUSTER_BALANCE
mov di,bp
call func_read_one_sector16
add bp,512 ;下一个目标地址

;获取下一个簇号（每个FAT表项为2字节）
shl bx,1 ;乘2，每个FAT表项占2个字节
mov bx,[bx+DISK_BUFFER]
cmp bx,0xfff8 ;大于等于0xfff8表示文件的最后一个簇
jb read_loader ;jb无符号小于则跳转，jl有符号小于则跳转。

read_loader_finish:
jmp LOADER_ADDRESS ;跳转到loader

loader_not_found:
;打印字符串："loader not found"
mov si,loader_not_found_string
mov di,80 ;屏幕第2行显示
call func_print_string

stop:
hlt
jmp stop

;打印字符串函数
;输入参数：ds:si，di。
;输出参数：无。
;si 表示字符串起始地址，以0为结束符。
;di 表示字符串在屏幕上显示的起始位置（0~1999）
func_print_string:
mov ah,0x07 ;ah 表示字符属性 黑底白字
shl di,1 ;乘2（屏幕上每个字符对应2个字节，高字节为属性，低字节为ASCII）
.start_char:
mov al,[si]
cmp al,0
jz .end_print
mov [gs:di],ax
inc si
add di,2
jmp .start_char
.end_print:
ret

;清屏函数（将屏幕写满空格就实现了清屏）
;输入参数：无。
;输出参数：无。
func_clear_screen:
push bx
mov ah,0;
mov al,' '
mov cx,VIDEO_CHAR_MAX_COUNT
.start_blank:
mov bx,cx ;bx=(cx-1)*2
dec bx
shl bx,1
mov [gs:bx],ax
loop .start_blank
pop bx
ret

;读取硬盘1个扇区函数（主通道主盘）【经测试，一次读多个扇区会有各种问题，需要分开一个扇区一个扇区的读。】
;输入参数：esi，ds:di。
;输出参数：无。
;esi 起始LBA扇区号
;di 将数据写入的内存地址
func_read_one_sector16:
;第1步：设置要读取的扇区数
mov dx,0x1f2
mov al,1
out dx,al ;读取的扇区数
;第2步：将LBA地址存入0x1f3~0x1f6
mov eax,esi
;LBA地址7~0位写入端口0x1f3
mov dx,0x1f3
out dx,al
;LBA地址15~8位写入端口写入0x1f4
shr eax,8
mov dx,0x1f4
out dx,al
;LBA地址23~16位写入端口0x1f5
shr eax,8
mov dx,0x1f5
out dx,al
;LBA28模式
shr eax,8
and al,0x0f ;LBA第24~27位
or al,0xe0 ;设置7~4位为1110，表示LBA模式，主盘
mov dx,0x1f6
out dx,al
;第3步：向0x1f7端口写入读命令0x20
mov dx,0x1f7
mov al,0x20
out dx,al
;第4步：检测硬盘状态【该方法IDE硬盘正常，但SATA硬盘会表现为一直忙，无限循环卡在这里。】
.not_ready:
nop
in al,dx ;读0x1f7端口
and al,0x88 ;第3位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙。
cmp al,0x08
jnz .not_ready ;若未准备好，继续等
;第5步：从0x1f0端口读数据
mov cx,256 ;每次读取2字节，一个扇区需要读256次。
mov dx,0x1f0
.go_on_read:
in ax,dx
mov [di],ax
add di,2
loop .go_on_read
ret

boot_start_string:db "boot start",0
loader_file_name_string:db "LOADER  BIN",0
loader_not_found_string:db "loader not found",0
loader_found_string:db "loader found",0

times 510-($-$$) db 0
db 0x55,0xaa