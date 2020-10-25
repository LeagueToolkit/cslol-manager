%include 'defines.asm'

; config
%define config_scan_size 0x2C
%define config_result_offset -4

; args
%define arg_str 8
%define arg_file 12
%define arg_line 16

; prologue
enter 0, 0
push ebx
push esi
call back
back:   pop ebx
        and ebx, 0xFFFFF000
        add ebx, data_skip_size

; scanning the stack
mov esi, DWORD data_ret_addr[ebx]
mov eax, ebp
add eax, config_scan_size
scan:   sub eax, 4
        cmp eax, ebp
        je done
        cmp esi, DWORD [eax]
        jne scan
        lea eax, DWORD config_result_offset[eax]
        mov DWORD [eax], 1

; fetch resume address
done:   mov eax, DWORD data_org_free_addr[ebx]

; epilogue
pop esi
pop ebx
leave
jmp eax
