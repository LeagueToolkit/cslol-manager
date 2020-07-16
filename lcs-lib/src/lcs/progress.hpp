#ifndef LCS_PROGRESS_HPP
#define LCS_PROGRESS_HPP
#include "common.hpp"

namespace LCS {
    class Progress {
    public:
        Progress() noexcept;
        virtual ~Progress() noexcept;
        virtual void startItem(fs::path const& path, size_t dataSize) noexcept;
        virtual void consumeData(size_t ammount) noexcept;
        virtual void finishItem() noexcept;
    };

    class ProgressMulti : public Progress {
    public:
        ProgressMulti() noexcept;
        virtual ~ProgressMulti() noexcept;
        virtual void startMulti(size_t itemCount, size_t dataTotal) noexcept;
        virtual void finishMulti() noexcept;
    };
}

#endif // LCS_PROGRESS_HPP
