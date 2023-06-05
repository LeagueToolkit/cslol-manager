#!/bin/sh
nasm -f bin -Ox -o win32_hook_CreateFileA.bin win32_hook_CreateFileA.asm
nasm -f bin -Ox -o win32_hook_CRYPTO_free.bin win32_hook_CRYPTO_free.asm
nasm -f bin -Ox -o macos_hook.bin macos_hook.asm

xxd -i -c 8 win32_hook_CreateFileA.bin
xxd -i -c 8 win32_hook_CRYPTO_free.bin
xxd -i -c 8 macos_hook.bin

rm -f win32_hook_CreateFileA.bin
rm -f win32_hook_CRYPTO_free.bin
rm -f macos_hook.bin
