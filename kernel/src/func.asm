[bits 32]
global asm_hlt,asm_cli,asm_sti,asm_stihlt
global io_in8,io_in16
global io_out8,io_out16
global get_eflags,set_eflags
global load_gdtr,load_idtr,load_tr
global farjmp,open_page
global do_invlpg
global asm_inthandler20,asm_inthandler21,asm_inthandler2c,asm_inthandler30

extern inthandler_common

section .text
asm_hlt: ;void asm_hlt(void);
hlt
ret

asm_cli: ;void asm_cli(void);
cli
ret

asm_sti: ;void asm_sti(void);
sti
ret

asm_stihlt: ;void asm_stihlt(void);
sti
hlt
ret

io_in8: ;int io_in8(int port);
mov edx,[esp+4]
mov eax,0
in al,dx
ret

io_in16: ;int io_in16(int port);
mov edx,[esp+4]
mov eax,0
in ax,dx
ret

io_out8: ;void io_out8(int port,int data);
mov edx,[esp+4]
mov al,[esp+8]
out dx,al
ret

io_out16: ;void io_out16(int port,int data);
mov edx,[esp+4]
mov ax,[esp+8]
out dx,ax
ret

get_eflags: ;int get_eflags(void);
pushfd ;push eflags
pop eax
ret

set_eflags: ;void io_stroe_eflags(int eflags);
mov eax,[esp+4]
push eax
popfd ;pop eflags
ret

load_gdtr: ; void load_gdtr(int limit, int addr);
mov ax,[esp+4] ;limit
mov [esp+6],ax ;limit只需两个字节。
lgdt [esp+6]
ret		

load_idtr: ;void load_idtr(int limit,int addr);
mov ax,[esp+4] ;limit
mov [esp+6],ax 
lidt [esp+6]
ret

load_tr: ; void load_tr(int tr);
ltr [esp+4] ;tr
ret		

farjmp: ;void farjmp(int eip,int cs);
jmp far [esp+4] ;eip,cs
ret

open_page: ;void open_page(int *pg_dir);打开分页
;把页目录地址赋值给cr3
mov eax,[esp+4] ;pg_dir
mov cr3,eax
;打开cr0的pg位(第31位)表示开启分页
mov eax,cr0
or eax,0x80000000
mov cr0,eax
ret

do_invlpg: ;void do_invlpg(void * vaddr);
;mov eax,[esp+4] ;vaddr
invlpg [esp+4]
ret

asm_inthandler_common: ;通用中断处理函数【不考虑有错误码的情况】
;以下是保存上下文环境
push gs
push fs
push es
push ds
push edi
push esi
push ebp
push edx
push ecx
push ebx
push eax
mov eax,[esp+44] ;中断号
push eax
cmp eax,0x30
je .n1
mov eax,0
jmp .n2
.n1:
mov eax,[esp+64] ;esp_old
.n2:
push eax 
call inthandler_common
add esp,8 ;栈平衡 esp_old、中断号
pop eax
pop ebx
pop ecx
pop edx
pop ebp
pop esi
pop edi
pop ds
pop es
pop fs
pop gs
add esp,4 ;栈平衡 中断号
iretd

;中断发生时栈中的内容：
;如果特权级有变化，CPU会改用TSS中的特权栈ss0:esp0存放以下内容。
;如果特权级没有变化，则继续使用当前的栈。
;ss_old  ;特权级变化时有
;esp_old ;特权级变化时有
;eflags
;cs_old
;eip_old
;error_code ;部分异常中断有

asm_inthandler20: ;时钟中断处理函数 
push 0x20 ;中断号
jmp asm_inthandler_common

asm_inthandler21: ;键盘中断处理函数 
push 0x21 ;中断号
jmp asm_inthandler_common

asm_inthandler2c: ;鼠标中断处理函数 
push 0x2c ;中断号
jmp asm_inthandler_common

asm_inthandler30: ;系统调用
push 0x30 ;中断号
jmp asm_inthandler_common