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

; prepare buffer for writing
lea edi, DWORD local_buffer[ebp]

; write prefix
mov esi, DWORD payload_prefix_open_ptr[ebx]
write_prefix:   lodsw
                stosw
                test ax, ax
                jne write_prefix

; remove null terminator
sub edi, 2

; write path
mov esi, DWORD arg_lpFileName[ebp]
mov ah, 0
write_path: lodsb
            cmp al, 47
            jne write_path_skip
            mov al, 92
write_path_skip:
            stosw
            test al, al
            jne write_path

; call original with modified buffer
lea eax, DWORD local_buffer[ebp]
push DWORD arg_hTemplateFile[ebp]
push DWORD arg_dwFlagsAndAttributes[ebp]
push DWORD arg_dwCreationDisposition[ebp]
push DWORD arg_lpSecurityAttributes[ebp]
push DWORD arg_dwShareMode[ebp]
push DWORD arg_dwDesiredAccess[ebp]
push eax
call DWORD payload_wopen[ebx]

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
call DWORD payload_org_open_ptr[ebx]

; epilogue
done: pop esi
      pop edi
      pop ebx
      leave
      ret 28
