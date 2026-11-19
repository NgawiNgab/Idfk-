#pragma once

#include "ore_outline/core.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>

namespace ore_outline::terrain_capture {

// Exact-build keys verified by the initializer trace. Not runtime block IDs.
inline constexpr std::uint64_t kDiamondKey = 0x64ac5906db894391;
inline constexpr std::uint64_t kDeepslateDiamondKey = 0xf720faa144e50745;
inline constexpr std::size_t kMaxSampledSections = 4096;
inline constexpr std::size_t kMaxExamples = 4;

enum class Match : std::uint8_t { None, Diamond, DeepslateDiamond };

struct Hit {
    BlockPos position{};
    Match match = Match::None;
};

struct Report {
    bool builder_seen = false;
    bool contention = false;
    bool limited = false;
    std::size_t sections = 0, columns = 0, matched_sections = 0;
    std::array<Hit, kMaxExamples> examples{};
    std::size_t example_count = 0;
};

// Historical samples within one manual capture window, never a ready-section inventory.
// Control methods run on a serialized UI/lifecycle path. Workers retain only the token
// from before the game's original call; they never retain source/block objects.
class Capture {
public:
    using Token = std::uint64_t;

    [[nodiscard]] bool arm();
    void stop() noexcept { state_.fetch_and(~kArmed, std::memory_order_acq_rel); }

    // Count scoped call entries, then carry this identity through the original lookup.
    [[nodiscard]] Token visit() noexcept {
        const auto state = state_.load(std::memory_order_acquire);
        if (!(state & kArmed)) return 0;
        const auto window = state & kWindow;
        if (!(state & kVisited)) flag(window, kVisited, state);
        return window;
    }

    void record(Token window, BlockPos position, std::optional<std::uint64_t> key) noexcept {
        if (!window) return;
        const auto match = key == kDiamondKey ? Match::Diamond
            : key == kDeepslateDiamondKey ? Match::DeepslateDiamond : Match::None;
        // Sample ordinary visits to section origins, plus positives anywhere. A
        // clipped loop can skip its origin. This is not a test of valid-cell coverage.
        if (match == Match::None && ((position.x | position.y | position.z) & 15) != 0) return;
        record_sample(window, position, match);
    }

    // Call after stop(). Waits only for a short copy into our own table, never a game lookup.
    [[nodiscard]] Report report() const;

private:
    struct Entry {
        SectionPos section{};
        Hit first_hit{};
        bool used = false;
    };
    static constexpr Token kArmed = 1, kVisited = 2, kContention = 4, kFlags = 7;
    static constexpr Token kWindow = ~kFlags | kArmed;
    static constexpr std::size_t kBucketCount = 2 * kMaxSampledSections;
    static constexpr std::size_t kMaxProbes = 16;

    void flag(Token window, Token bit, Token before) noexcept;
    void record_sample(Token window, BlockPos position, Match match) noexcept;

    std::atomic<Token> state_{0};
    mutable std::mutex mutex_;
    std::array<Entry, kBucketCount> entries_{};
    std::size_t section_count_ = 0;
    bool limited_ = false;
};

} // namespace ore_outline::terrain_capture
