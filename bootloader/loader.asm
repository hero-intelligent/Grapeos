;FAT16目录项结构(32B)：
;名称		    偏移	    长度	描述
DIR_Name	    equ 0x00	;11	文件名8B，扩展名3B
DIR_Attr	    equ 0x0b	;1	文件属性
;保留	        0x0c	        10	保留位
DIR_WrtTime     equ 0x16	;2	最后一次写入时间
DIR_WrtDate     equ 0x18	;2	最后一次写入日期
DIR_FstClus     equ 0x1a	;2	起始簇号
DIR_FileSize    equ 0x1c	;4	文件大小

LOADER_ADDRESS equ 0x1000
KERNEL_ADDRESS equ 0x2000
STACK_BOTTOM equ LOADER_ADDRESS
DISK_BUFFER equ 0x1e00 ;读磁盘用的缓存区，放到kernel之前的512字节。
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

;段描述符高4字节
DESC_CODE_HIGH4 equ  00000000_11001111_10011000_00000000b ;type=1000 Execute-Only
DESC_DATA_HIGH4 equ  00000000_11001111_10010010_00000000b ;type=0010 Read/Write
;段选择子
SELECTOR_CODE equ (0x0001<<3)+000b;
SELECTOR_DATA equ (0x0002<<3)+000b;

org LOADER_ADDRESS
jmp loader_start
vbe_mode_num: dw 0      ;VBE模式号，偏移地址为2B。
gram:dd 0   ;图形地址，偏移地址为4B。
leds:db 0;用来记录键盘Capslock是ON还是OFF等状态，偏移地址为8B。

loader_start:
;进入保护模式前需要做的事：
;切换到图形模式
;确认VBE是否存在
mov di,0x2000 ;保存VbeInfoBlock的地方，占用512B。
mov ax,0x4f00
int 0x10
cmp ax,0x004f
jne scrn320

;检查VBE的版本
mov ax,[es:di+4]
cmp ax,0x0200
jb scrn320 ;if(ax<0x0200)goto scrn320 VBE的版本应该在2.0及以上。

;一个一个检查画面模式，直到找到符合条件的画面模式
mov fs,[0x2000+16]
mov si,[0x2000+14]
mov dx,0 ;循环计数
check_mode_num:
mov di,0x2200 ;保存ModeInfoBlock的地方，占用256B。
mov ax,0x4f01 ;取得画面模式信息
mov cx,[fs:si] ;模式号
int 0x10
cmp ax,0x004f
jne next_mode_num

;画面模式信息的确认
cmp byte [es:di+0x19],32 ;颜色位数
jne next_mode_num
cmp byte [es:di+0x1b],6 ;颜色的指定方法（4是调色板模式，6是Direct Color）
jne next_mode_num
mov ax,[es:di+0x00] ;模式属性
and ax,0x0080
jz next_mode_num      ;模式属性的bit7是0，所以放弃。bit7是线性帧缓存区模式，1为有效，0为无效。
cmp word [es:di+18],1024 ;水平分辨率
jnz next_mode_num
cmp word [es:di+20],768 ;垂直分辨率
jnz next_mode_num

;将符合条件的VBE模式保存
save_vbe_data: 
mov ax,[fs:si] ;保存模式号
mov [vbe_mode_num],ax
mov eax,[es:di+40] ;保存图形地址
mov [gram],eax

; ;打印显存地址
; mov ebx,eax
; mov di,166
; call print_one_byte_by_hex
; shr ebx,8
; mov al,bl
; mov di,164
; call print_one_byte_by_hex
; shr ebx,8
; mov al,bl
; mov di,162
; call print_one_byte_by_hex
; shr ebx,8
; mov al,bl
; mov di,160
; call print_one_byte_by_hex
; jmp stop16

jmp set_vbe

next_mode_num:
add si,2 ;指向下一个模式号
inc dx
cmp dx,111 ;存放模式号的空间共222字节，每个模式号占2字节，所以模式号数量不会超过111个。
jb check_mode_num

;设置VBE
set_vbe:
mov bx,[vbe_mode_num]
add bx,0x4000
mov ax,0x4f02
int 0x10
cmp ax,0x004f ;如果返回值为0x004f说明设置显示模式成功
jnz scrn320

;用BIOS取得键盘上各种LED指示灯的状态
mov ah,0x02
int 0x16 ;keyboard BIOS
mov [leds],al
jmp enter_protected_mode

scrn320:
mov al,0x13 ;VGA显卡，320x200x8位彩色
mov ah,0
int 0x10

stop16:
hlt
jmp stop16

; ;按16进制打印1字节数据。
; ;输入参数：al,di。
; ;输出参数：无。
; ;al 要打印的1字节数据。
; ;di 表示字符串在屏幕上显示的起始位置（0~1999）
; print_one_byte_by_hex:
; .low:
; mov cl,al
; and cl,0x0f
; cmp cl,9
; ja .greater_than_9_L
; add cl,'0'
; jmp .high
; .greater_than_9_L:
; add cl,55

; .high:
; mov ch,al
; shr ch,4
; cmp ch,9
; ja .greater_than_9_H
; add ch,'0'
; jmp .print
; .greater_than_9_H:
; add ch,55

; .print:
; mov ah,0x07
; shl di,1
; mov al,ch
; mov [gs:di],ax
; add di,2
; mov al,cl
; mov [gs:di],ax
; ret

;进入保护模式5步：1.打开A20；2.关中断；3.加载GDT；4.开启保护模式；5.刷新流水线。
enter_protected_mode:
;1.打开A20地址线。
in al,0x92
or al,0000_0010B
out 0x92,al

;2.关中断。在进入保护模式前，如果没有加载IDT，必须关中断，否则虽然在bochs中正常，但在VirtualBox、VMware、QEMU中会报错停机。
;经测试关中断放在打开A20之后，在多种虚拟机中兼容性比较好。
cli

;3.加载GDT。
lgdt [gdt_ptr]

;4.将CR0的PE(Protection Enable)位置1，开启保护模式。
mov eax,cr0
or eax,0x00000001
mov cr0,eax

;5.通过远跳转刷新流水线，同时修改代码段寄存器cs的值。
jmp dword SELECTOR_CODE:protected_mode_start

[bits 32]
protected_mode_start:
mov ax,SELECTOR_DATA
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax
mov ss,ax
mov esp,STACK_BOTTOM

; ;图形测试
; mov ecx,0xc0000 ;=1024*768
; mov eax,[gram]
; draw_test:
; mov [eax],ecx
; add eax,4
; loop draw_test
; jmp stop32

;寻找内核文件
;读取根目录的第1个扇区(1个扇区可以存放16个目录项)【计划根目录目录项个数不能超过16个】
mov esi,SECTOR_NUM_OF_ROOT_DIR_START
mov edi,DISK_BUFFER 
mov ecx,1 ;读取1个扇区
call func_read_one_sector32

;查找kernel.bin文件
cld ;cld告诉程序si，di向地址增加的方向移动，std相反。
mov bx,0 ;bx用来记录遍历的目录项序号(0~15)。
next_dir_entry:
mov si,bx
shl si,5 ;乘以32
add si,DISK_BUFFER ;指向目录项中的文件名
mov di,kernel_file_name_string ;指向kernel文件名
mov cx,FILE_NAME_LENGTH ;文件名长度
repe cmpsb ;如果相等则比较下一个字节，直到ZF为0或CX为0，ZF为0表示发现不相等的字节，CX为0表示这一串字节相等。
jcxz kernel_found ;cx=0
inc bx
cmp bx,DIR_ENTRY_PER_SECTOR
jl next_dir_entry
jmp kernel_not_found

kernel_found:
;获取kernel起始簇号
shl bx,5 ;目录项序号乘以32得到目录项的偏移量
add bx,DISK_BUFFER ;目录项在内存中的地址
mov bx,[bx+DIR_FstClus] ;保存kernel的起始簇号

;读取FAT1的第1个扇区
mov esi,SECTOR_NUM_OF_FAT1_START 
mov edi,DISK_BUFFER 
call func_read_one_sector32

;按簇读kernel
mov ebp,KERNEL_ADDRESS ;目标地址初值
read_kernel:
xor esi,esi
mov si,bx
add esi,SECTOR_CLUSTER_BALANCE
mov edi,ebp
call func_read_one_sector32
add ebp,512 ;下一个目标地址

;获取下一个簇号（每个FAT表项为2字节）
shl bx,1 ;乘2，每个FAT表项占2个字节
mov bx,[bx+DISK_BUFFER]
cmp bx,0xfff8 ;大于等于0xfff8表示文件的最后一个簇
jb read_kernel ;jb无符号小于则跳转，jl有符号小于则跳转。

goto_kernel:
; mov esp,KERNEL_ADDRESS 更改栈后开机桌面显示很慢，关闭窗口的速度也很慢。
jmp KERNEL_ADDRESS ;跳转到kernel

kernel_not_found:
stop32:
hlt
jmp stop32

;读取硬盘1个扇区函数（主通道主盘）【经测试，一次读多个扇区会有各种问题，需要分开一个扇区一个扇区的读。】
;输入参数：esi，edi。
;输出参数：无。
;esi 起始LBA扇区号
;edi 将数据写入的内存地址
func_read_one_sector32:
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
mov [edi],ax
add edi,2
loop .go_on_read
ret

loader_start_string:db "loader start",0
kernel_file_name_string:db"KERNEL  BIN"

;构建临时GDT及描述符
GDT_BASE: dd 0x00000000,0x00000000 ;GDT第1项不允许使用，设为全0。
CODE_DESC: dd 0x0000ffff,DESC_CODE_HIGH4 ;平坦模式
DATA_DESC: dd 0x0000ffff,DESC_DATA_HIGH4 ;平坦模式
GDT_SIZE equ $-GDT_BASE
GDT_LIMIT equ GDT_SIZE-1 ;GDT界限值,相当于GDT的字节大小减1。
gdt_ptr dw GDT_LIMIT
        dd GDT_BASE