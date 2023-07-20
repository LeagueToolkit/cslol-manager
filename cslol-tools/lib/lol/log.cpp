#include <fmtlog-inl.h>

#include <lol/common.hpp>
#include <lol/log.hpp>

void lol::init_logging_thread() {
    fmtlogDetailWrapper<>::impl.stopPollingThread();
    fmtlogDetailWrapper<>::impl.threadRunning = true;
    fmtlogDetailWrapper<>::impl.thr = std::thread([]() {
        while (fmtlogDetailWrapper<>::impl.threadRunning) {
            fmtlogDetailWrapper<>::impl.poll(false);
            // Why are we using our own polling thread instead of builtin one:
            // This chinese logging library uses nanoseconds to poll.
            // As it turns out windows is utterly incapable of sleeping below 1ms.
            // As a consequence std::this_thread_sleep sleeps by doing a busy wait on high precision system clock.
            // This absolutely destroys CPU performance on certain machines as it pins the core to 100%.
            lol::sleep_ms(1);
        }
        fmtlogDetailWrapper<>::impl.poll(true);
    });
}
