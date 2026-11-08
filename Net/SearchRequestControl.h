#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>

// Local coordination only; no change to the server protocol.
struct SearchRequestControl {
    static constexpr std::uint64_t largeResultRows = 100000;
    std::atomic_bool cancelled{false}, done{false}, largeApproved{false}, paused{false};
    std::atomic_uint64_t rows{0}, bytes{0};

    void checkpoint() const {
        while (paused.load() && !cancelled.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        if (cancelled.load()) throw std::runtime_error("Search cancelled");
    }
    void record() {
        rows.fetch_add(1);
        while (rows.load() >= largeResultRows && !largeApproved.load()) {
            checkpoint();
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        checkpoint();
    }
};

class SearchWatchdog {
public:
    using Clock = std::chrono::steady_clock;
    enum class Notice { None, Slow, Large };
    explicit SearchWatchdog(Clock::time_point now = Clock::now()) : last_(now) {}
    Notice due(const SearchRequestControl& control, Clock::time_point now = Clock::now()) const {
        if (control.cancelled.load()) return Notice::None;
        if (control.rows.load() >= SearchRequestControl::largeResultRows && !control.largeApproved.load())
            return Notice::Large;
        return now - last_ >= std::chrono::seconds(20) ? Notice::Slow : Notice::None;
    }
    void continued(Clock::time_point now = Clock::now()) { last_ = now; }
private:
    Clock::time_point last_;
};
