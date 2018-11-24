#pragma once

#include <chrono>

namespace zero_switch {

class Stop_watch {
public:
    std::chrono::milliseconds lap() const noexcept
    {
        const auto e = std::chrono::steady_clock::now() - start_;
        return std::chrono::duration_cast<std::chrono::milliseconds>(e);
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
};

}