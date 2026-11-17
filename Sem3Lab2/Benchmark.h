#pragma once

#include <chrono>
#include <utility>

namespace lab2 {

    template <typename F>
    double timeMs(F&& f) {
        auto t0 = std::chrono::high_resolution_clock::now();
        f();
        auto t1 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

} // namespace lab2