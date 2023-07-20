#include <chrono>
#include <lol/common.hpp>
#include <thread>

using namespace lol;

#ifdef _WIN32
extern "C" {
extern void Sleep(unsigned ms);
}
void lol::sleep_ms(std::uint32_t ms) { Sleep(ms); }
#else
void lol::sleep_ms(std::uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds{ms}); }
#endif