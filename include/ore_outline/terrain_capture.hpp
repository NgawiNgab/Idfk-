#pragma once

#include "ore_outline/core.hpp"

#include <atomic>
#include <cstdint>
#include <optional>

namespace ore_outline::terrain_capture {

// Both points lie in section (1, 6, -1); X and Z also test world vs local XYZ.
inline constexpr BlockPos kExposed{20, 100, -12};
inline constexpr BlockPos kEnclosed{28, 100, -4};
// Stored first words identified in the exact build, not runtime block IDs.
inline constexpr std::uint64_t kDiamondKey = 0x64ac5906db894391;
inline constexpr std::uint64_t kAirKey = 0xbd584baf00003448;

enum class Match { NoVisit, UnknownKey, AirKeyOrFallback, OtherKey, DiamondKey };
struct Report {
    bool armed;
    bool builder_seen;
    Match exposed, enclosed;
};

// Only a copy of two test positions' results crosses the mesh-worker boundary.
// Control methods (arm, stop) run on a serialized UI/lifecycle path. A worker
// holding a token from an old window cannot publish into the next one.
class Capture {
public:
    using Token = std::uint64_t;

    void arm() noexcept {
        const auto previous = state_.load(std::memory_order_relaxed);
        state_.store(((previous & ~kFlags) + (kFlags + 1)) | kArmed, std::memory_order_release);
    }

    void stop() noexcept { state_.fetch_and(~kArmed, std::memory_order_acq_rel); }

    [[nodiscard]] Token token() const noexcept {
        const auto state = state_.load(std::memory_order_acquire);
        return state & kArmed ? state & kWindow : 0;
    }

    // nullopt position means a verified builder visit outside the two test points.
    void record(Token window, std::optional<BlockPos> position = {},
                std::optional<std::uint64_t> key = {}) noexcept {
        if (!window) return;
        auto before = state_.load(std::memory_order_relaxed);
        while ((before & kWindow) == window) {
            auto after = before | kBuilder;
            const unsigned shift = position == kExposed ? kExposedShift : position == kEnclosed ? kEnclosedShift : 0;
            if (shift) {
                const auto match = !key || *key == 0 ? Match::UnknownKey
                    : *key == kDiamondKey ? Match::DiamondKey
                    : *key == kAirKey ? Match::AirKeyOrFallback : Match::OtherKey;
                const auto value = static_cast<Token>(match);
                if (value > ((before >> shift) & kResultMask)) {
                    after = (after & ~(kResultMask << shift)) | (value << shift);
                }
            }
            if (before == after || state_.compare_exchange_strong(before, after, std::memory_order_relaxed)) break;
        }
    }

    [[nodiscard]] Report report() const noexcept {
        const auto state = state_.load(std::memory_order_acquire);
        return {bool(state & kArmed), bool(state & kBuilder),
                static_cast<Match>((state >> kExposedShift) & kResultMask),
                static_cast<Match>((state >> kEnclosedShift) & kResultMask)};
    }

private:
    static constexpr Token kArmed = 1, kBuilder = 2, kFlags = 0xff, kResultMask = 7;
    static constexpr unsigned kExposedShift = 2, kEnclosedShift = 5;
    static constexpr Token kWindow = ~kFlags | kArmed;
    std::atomic<Token> state_{0};
};

} // namespace ore_outline::terrain_capture
