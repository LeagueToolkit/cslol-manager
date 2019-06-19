#pragma once
#include <cinttypes>
#include <cstdio>
#include <cstring>

struct Config {
    uint32_t checksum = 0;
    uintptr_t off_fp = 0;
    uintptr_t off_pmeth = 0;

    void print() const {
        printf("Checksum: 0x%08X\n", checksum);
        printf("FileProvider: 0x%08X\n", off_fp);
        printf("PMeth: 0x%08X\n", off_pmeth);
    }

    void save() const {
        if(FILE* file = nullptr;
                !fopen_s(&file, "lolskinmod.txt", "w") && file) {
            fprintf_s(file, "0x%08X 0x%08X 0x%08X\n",
                      checksum, off_fp, off_pmeth);
            fclose(file);
        }
    }

    void load() {
        if(FILE* file = nullptr;
                !fopen_s(&file, "lolskinmod.txt", "r") && file) {
            fscanf_s(file, "0x%08X 0x%08X 0x%08X\n",
                      &checksum, &off_fp, &off_pmeth);
            fclose(file);
        }
    }
};
