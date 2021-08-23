#!/bin/sh
nasm -f bin -Ox -o file_hook.bin file_hook.asm
nasm -f bin -Ox -o up_hook.bin up_hook.asm

xxd -i -C -c 8 file_hook.bin
xxd -i -C -c 8 up_hook.bin

rm file_hook.bin
rm up_hook.bin
