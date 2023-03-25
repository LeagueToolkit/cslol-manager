%include 'defines.asm'

call back
back:   pop rax
        and rax, 0xFFFFFFFFFFFFF000
mov rax, QWORD payload_find_ret_addr[rax]
push rax
; following two lines are just shorter way to do: mov rax, 1
xor rax,rax
inc rax
ret
