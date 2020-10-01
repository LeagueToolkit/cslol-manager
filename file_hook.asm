; This assumes:
; - shellcode fits inside 0x80
; - allocation is aligned to 0x80
; - there is following data after shellcode:
;   0. 64 bytes of original shellcode
;   1. char prefix[...];
; - total size of merged string does not go over 0x10000

; args
lpFileName = 8
dwDesiredAccess = 12
dwShareMode = 16
lpSecurityAttributes = 20
dwCreationDisposition = 24
dwFlagsAndAttributes = 28
hTemplateFile = 32

; locals
buffer_size = 0x1000
buffer = -buffer_size

; globals
data_org_fn = 0
data_prefix = 64

; prologue
enter buffer_size, 0
push ebx
push edi
push esi

; get pointer to data after function
call SHORT back
back:
pop ebx
and ebx, 0xFFFFFF80
add ebx, 0x80

; prepare buffer for writing
lea edi, DWORD PTR buffer[ebp]

; write prefix
lea esi, DWORD PTR data_prefix[ebx]
write_prefix:
lodsb
stosb
testb al, al
jne SHORT write_prefix

; remove null terminator
dec edi

; write path
mov esi, DWORD PTR lpFileName[ebp]
write_path:
lodsb
stosb
testb al, al
jne SHORT write_path

; call original with modified buffer
push DWORD PTR hTemplateFile[ebp]
push DWORD PTR dwFlagsAndAttributes[ebp]
push DWORD PTR dwCreationDisposition[ebp]
push DWORD PTR lpSecurityAttributes[ebp]
push DWORD PTR dwShareMode[ebp]
push DWORD PTR dwDesiredAccess[ebp]
lea eax, DWORD PTR buffer[ebp]
push eax
lea eax, DWORD PTR data_org_fn[ebx]
call eax

; check for success
cmp eax, -1
jne SHORT done

; just call original
push DWORD PTR hTemplateFile[ebp]
push DWORD PTR dwFlagsAndAttributes[ebp]
push DWORD PTR dwCreationDisposition[ebp]
push DWORD PTR lpSecurityAttributes[ebp]
push DWORD PTR dwShareMode[ebp]
push DWORD PTR dwDesiredAccess[ebp]
push DWORD PTR lpFileName[ebp]
lea eax, DWORD PTR data_org_fn[ebx]
call eax

done:
; epilogue 
pop esi
pop edi
pop ebx
leave
ret 28
