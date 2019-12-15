#include <inttypes.h>
#include <stddef.h>

// Win32: VectorDeleter
// GNU: Destructor + Deleter
// g++ -O2 -fno-stack-protector -march=native modoverlay_shellcode.cpp -nostdlib -oshellcode.o
// objdump -d -C shellcode.o
// for i in $(objdump -d -C shellcode.o |grep "^ " |cut -f2); do echo -n '0x'$i', '; done | fold -w48 | paste -sd'\n' -

// Copies a null terminated string to a buffer including null terminator
// Returns the position after null terminator
static inline char* small_copy(char* dst, char const* src) {
    char const* src_iter;
    char* dest_iter;
    int tmp;
    asm volatile(  
        "1:\tlodsb\n\t"
        "stosb\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b"
        : "=&S" (src_iter), "=&D" (dest_iter), "=&a" (tmp)
        : "0" (src), "1" (dst) 
        : "memory");
    return dest_iter;
}

[[gnu::used]] char const* __cdecl PrefixFn(char* buffer, char const* src, char const* prefix) noexcept {
    small_copy(small_copy(buffer, prefix) - 1, src);
    return buffer;
}

[[gnu::used]] char const* __cdecl PrefixFn_WadOnly(char* buffer, char const* src, char const* prefix) noexcept {
    char const* end = small_copy(small_copy(buffer, prefix) - 1, src) - 4;
    do {
        if (*reinterpret_cast<uint32_t const*>(end) == 0x6461772E) {
            return buffer;
        }
        --end;
    } while(end != buffer);
    return src;
}

[[gnu::used]] int __cdecl Verify() { return 1; }

struct FileProvider {
    [[gnu::used]] virtual void* Open(char const* src, char const* mode) {
        char buffer[512];
        return list->arr[2]->Open(prefixFn(buffer, src, prefix), mode);
    }
    
    [[gnu::used]] virtual void* CheckAccess(char const* src, uint32_t mode) {
        char buffer[512];
        return list->arr[2]->CheckAccess(prefixFn(buffer, src, prefix), mode);
    }
    
    [[gnu::used]] virtual void* CreateIterator(char const*, uint32_t) { return nullptr; }
    
    [[gnu::used]] virtual FileProvider* VectorDeleter(uint32_t) { return this; }

    [[gnu::used]] virtual void Destructor() {}
    
    [[gnu::used]] virtual void Deleter() {}

    [[gnu::used]] virtual bool IsRads() { return false; }
    
    struct List {
        FileProvider* arr[4];
        uint32_t size;
    }* list = {};
    decltype(&PrefixFn) prefixFn;
    char prefix[256];
};
