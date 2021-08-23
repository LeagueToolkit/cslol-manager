#include "modoverlay.hpp"
#include <thread>
#include <chrono>

using namespace LCS;

void LCS::SleepMS(uint32_t time) noexcept {
    std::this_thread::sleep_for(std::chrono::milliseconds(time));
}

void ModOverlay::save(std::filesystem::path const& filename) const noexcept {
    FILE* file = {};
#ifdef WIN32
    _wfopen_s(&file, filename.c_str(),  L"wb");
#else
    file = fopen(filename.c_str(), "wb");
#endif
    if (file) {
        auto str = to_string();
        fwrite(str.data(), 1, str.size(), file);
        fclose(file);
    }
}

void ModOverlay::load(std::filesystem::path const& filename) noexcept {
    FILE* file = {};
#ifdef WIN32
    _wfopen_s(&file, filename.c_str(),  L"rb");
#else
    file = fopen(filename.c_str(), "rb");
#endif
    if (file) {
        auto buffer = std::string();
        fseek(file, 0, SEEK_END);
        auto end = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer.resize((size_t)end);
        fread(buffer.data(), 1, buffer.size(), file);
        from_string(buffer);
    }
}
