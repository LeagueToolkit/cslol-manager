#pragma once
#include <cinttypes>
#include <cstdio>
#include <cstring>

struct Config {
    uint32_t checksum = 0;
    uintptr_t off_fp = 0;
    uintptr_t off_pmeth = 0;
    uint32_t enable_fp = 1;
    uint32_t enable_wad = 1;
    bool needsave;

    void print() const {
        printf("Checksum: 0x%08X\n", checksum);
        printf("FileProvider(%u): 0x%08X\n", enable_fp,off_fp);
        printf("Wad(%u): 0x%08X\n",enable_wad, off_pmeth);
    }

    void save() const {
        if(FILE* file = nullptr;
                !fopen_s(&file, "lolskinmod.txt", "w") && file) {
            fprintf_s(file, "0x%08X 0x%08X 0x%08X %u %u\n",
                      checksum, off_fp, off_pmeth, enable_fp, enable_wad);
            fclose(file);
        }
    }

    bool load() {
        if(FILE* file = nullptr;
                !fopen_s(&file, "lolskinmod.txt", "r") && file) {
            fscanf_s(file, "0x%08X 0x%08X 0x%08X %u %u\n",
                      &checksum, &off_fp, &off_pmeth, &enable_fp, &enable_wad);
            fclose(file);
            return true;
        }
        return false;
    }
};
