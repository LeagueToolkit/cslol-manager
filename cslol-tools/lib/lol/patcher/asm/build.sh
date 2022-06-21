#!/bin/sh
nasm -f bin -Ox -o file_hook.bin file_hook.asm
nasm -f bin -Ox -o up_hook.bin up_hook.asm
nasm -f bin -Ox -o ret_hook.bin ret_hook.asm
nasm -f bin -Ox -o macos_hook.bin macos_hook.asm

xxd -i -c 8 file_hook.bin
xxd -i -c 8 up_hook.bin
xxd -i -c 8 ret_hook.bin
xxd -i -c 8 macos_hook.bin

rm file_hook.bin
rm up_hook.bin
rm ret_hook.bin
rm macos_hook.bin
