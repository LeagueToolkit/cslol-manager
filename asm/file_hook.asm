%include 'defines.asm'

; args
%define arg_lpFileName 8
%define arg_dwDesiredAccess 12
%define arg_dwShareMode 16
%define arg_lpSecurityAttributes 20
%define arg_dwCreationDisposition 24
%define arg_dwFlagsAndAttributes 28
%define arg_hTemplateFile 32

; locals
%define local_buffer_size 0x1000
%define local_buffer -local_buffer_size

; prologue
enter local_buffer_size, 0
push ebx
push edi
push esi

; get pointer to data after function
call back
back:   pop ebx
        and ebx, 0xFFFFF000
        add ebx, data_skip_size

; prepare buffer for writing
lea edi, DWORD local_buffer[ebp]

; write prefix
lea esi, DWORD data_prefix[ebx]
write_prefix:   lodsb
                stosb
                test al, al
                jne write_prefix

; remove null terminator
dec edi

; write path
mov esi, DWORD arg_lpFileName[ebp]
write_path: lodsb
            stosb
            test al, al
            jne write_path

; call original with modified buffer
push DWORD arg_hTemplateFile[ebp]
push DWORD arg_dwFlagsAndAttributes[ebp]
push DWORD arg_dwCreationDisposition[ebp]
push DWORD arg_lpSecurityAttributes[ebp]
push DWORD arg_dwShareMode[ebp]
push DWORD arg_dwDesiredAccess[ebp]
lea eax, DWORD local_buffer[ebp]
push eax
lea eax, DWORD data_org_open_shellcode[ebx]
call eax

; check for success
cmp eax, -1
jne done

; just call original
push DWORD arg_hTemplateFile[ebp]
push DWORD arg_dwFlagsAndAttributes[ebp]
push DWORD arg_dwCreationDisposition[ebp]
push DWORD arg_lpSecurityAttributes[ebp]
push DWORD arg_dwShareMode[ebp]
push DWORD arg_dwDesiredAccess[ebp]
push DWORD arg_lpFileName[ebp]
lea eax, DWORD data_org_open_shellcode[ebx]
call eax

;epilogue 
done: pop esi
      pop edi
      pop ebx
      leave
      ret 28
