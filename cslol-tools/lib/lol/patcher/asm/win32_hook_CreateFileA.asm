[bits 64]

; args
%define arg_lpFileName 16
%define arg_dwDesiredAccess 24
%define arg_dwShareMode 32
%define arg_lpSecurityAttributes 40
%define arg_dwCreationDisposition 48
%define arg_dwFlagsAndAttributes 56
%define arg_hTemplateFile 72

; locals
%define local_buffer_size 0x1000
%define local_buffer -local_buffer_size

; prologue
enter local_buffer_size, 0
push rbx
push rdi
push rsi
sub rsp, 72

; use original functions shadow stack to preserve original args
mov arg_lpSecurityAttributes[rbp], r9
mov arg_dwShareMode[rbp], r8
mov arg_dwDesiredAccess[rbp], rdx
mov arg_lpFileName[rbp], rcx

; GENERIC_READ
cmp dword arg_dwDesiredAccess[rbp], 0x80000000
jne original

; FILE_SHARE_READ
cmp dword arg_dwShareMode[rbp], 1
jne original

; OPEN_EXISTING
cmp dword arg_dwCreationDisposition[rbp], 0x3
jne original

; FILE_ATTRIBUTE_NORMAL
cmp dword arg_dwFlagsAndAttributes[rbp], 0x80
jne original

; only DATA
cmp byte [rcx], 'D'
jne original
cmp byte [rcx+1], 'A'
jne original
cmp byte [rcx+2], 'T'
jne original
cmp byte [rcx+3], 'A'
jne original

; prepare buffer for writing
lea rdi, QWORD local_buffer[rbp]

; write prefix
lea rsi, QWORD [rel prefix]
write_prefix:   lodsw
                stosw
                test ax, ax
                jne write_prefix

; remove null terminator
sub rdi, 2

; write path
mov rsi, arg_lpFileName[rbp]
mov ah, 0
write_path: lodsb
            cmp al, 47
            jne write_path_skip
            mov al, 92
write_path_skip:
            stosw
            test al, al
            jne write_path

; call wide with modified buffer
mov rax, arg_hTemplateFile[rbp]
mov [rsp+48], rax
mov rax, arg_dwFlagsAndAttributes[rbp]
mov [rsp+40], rax
mov rax, arg_dwCreationDisposition[rbp]
mov [rsp+32], rax
mov r9, arg_lpSecurityAttributes[rbp]
mov r8, arg_dwShareMode[rbp]
mov rdx, arg_dwDesiredAccess[rbp]
lea rcx, QWORD local_buffer[rbp]
call QWORD [rel ptr_CreateFileW]

; restore original arguments
; check for success
cmp rax, -1
jne done

; just call original
original:
    mov rax, arg_hTemplateFile[rbp]
    mov [rsp+48], rax
    mov rax, arg_dwFlagsAndAttributes[rbp]
    mov [rsp+40], rax
    mov rax, arg_dwCreationDisposition[rbp]
    mov [rsp+32], rax
    mov r9, arg_lpSecurityAttributes[rbp]
    mov r8, arg_dwShareMode[rbp]
    mov rdx, arg_dwDesiredAccess[rbp]
    mov rcx, arg_lpFileName[rbp]
    call QWORD [rel ptr_CreateFileA]

; epilogue
done: add rsp, 72
      pop rsi
      pop rdi
      pop rbx
      leave
      ret

; runtime data comes after
align 0x100

ptr_CreateFileA:
    dq   0x11223344556677

ptr_CreateFileW:
    dq   0x11223344556677

prefix:
    dq   0x11223344556677
