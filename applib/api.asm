[bits 32]

global _start,api_openwin,api_exit,api_fill_rectangle,api_get_sys_msg,api_print_string,api_switch_cursor 
global api_move_cursor,api_print_text,api_change_color,api_open_popwin
global api_umalloc,api_ufree,api_create_file,api_save_file,
global api_get_should_open_filename,api_open_file,api_read_file

extern main

_start:
call main
jmp api_exit

api_openwin:
mov eax,1
int 0x30
ret

api_exit: 
mov eax,2
int 0x30

api_fill_rectangle: 
mov eax,3
int 0x30
ret

api_get_sys_msg: 
mov eax,4
int 0x30
ret

api_print_string: 
mov eax,5
int 0x30
ret

api_switch_cursor: 
mov eax,6
int 0x30
ret

api_move_cursor: 
mov eax,7
int 0x30
ret

api_print_text: 
mov eax,8
int 0x30
ret

api_change_color: 
mov eax,9
int 0x30
ret

api_open_popwin: 
mov eax,10
int 0x30
ret

api_umalloc: 
mov eax,13
int 0x30
ret

api_ufree: 
mov eax,14
int 0x30
ret

api_create_file: 
mov eax,15
int 0x30
ret

api_save_file: 
mov eax,16
int 0x30
ret

api_get_should_open_filename: 
mov eax,18
int 0x30
ret

api_open_file: 
mov eax,19
int 0x30
ret

api_read_file: 
mov eax,20
int 0x30
ret