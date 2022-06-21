[bits 64]
%define buffer_size       0x800

setup:
    push    rbp
    mov     rbp, rsp
    push    r12
    push    r13
    sub     rsp, buffer_size
    mov     r12, rdi            ; filename
    mov     r13, rsi            ; mode

check_args_not_null:
    test    r12, r12
    je     call_with_filename
    test    r13, r13
    je     call_with_filename

check_mode_eq_rb:
    cmp     byte [r13], 'r'
    jne     call_with_filename
    cmp     byte [r13 + 1], 'b'
    jne     call_with_filename
    cmp     byte [r13 + 2], 0
    jne     call_with_filename

write_prefix:
    mov     rdi, rsp            ; dst = buffer
    lea     rsi, [rel prefix]   ; src = prefix
    write_prefix_continue:
        lodsb
        stosb
        test al, al
        jne write_prefix_continue

write_filename:
    dec     rdi                 ; dst = buffer[strlen(buffer)]
    mov     rsi, r12            ; src = filename
    write_filename_continue:
        lodsb
        stosb
        test al, al
        jne write_filename_continue

call_with_buffer:
    mov     rdi, rsp            ; filename = buffer
    mov     rsi, r13            ; mode = mode
    mov     rax, qword [rel fopen_org_ref]
    call    qword [rax]
    test    rax, rax
    jne     return

call_with_filename:
    mov     rdi, r12            ; filename = filename
    mov     rsi, r13            ; mode = mode
    mov     rax, qword [rel fopen_org_ref]
    call    qword [rax]

return:
    add     rsp, buffer_size
    pop     r13
    pop     r12
    pop     rbp
    ret

align 0x80

fopen_org_ref:
    dq   0x11223344556677  
prefix:
    dq   0x11223344556677
