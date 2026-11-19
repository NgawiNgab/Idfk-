#pragma once

#include "ore_outline/core.hpp"

namespace ore_outline::passive {

inline constexpr std::size_t kCellsPerSection = 16 * 16 * 16;
inline constexpr std::size_t kMaxSections = 1024;
inline constexpr std::size_t kMaxOresPerSection = 64;

struct BuildTicket {
    WorldKey world;
    SectionPos section;
    // Issued by the observer, not a pointer, frame index, or wrapping engine counter.
    std::uint64_t serial;
    bool operator==(const BuildTicket&) const = default;
};

enum class End { Finished, Clipped, Canceled };

// Value-only scratch for one build and one producer. No allocation or engine access in record().
// The caller must prove that a sample is valid before reporting Air/Other/Ore. In particular,
// the game's air fallback must be reported as Unavailable unless actual block data was verified.
class BuildSamples {
public:
    explicit BuildSamples(BuildTicket ticket) noexcept : ticket_(ticket) {}
    void record(BlockPos position, Classification sample) noexcept;
    void finish(End end) noexcept;
    [[nodiscard]] std::size_t visited() const noexcept { return visited_; }
    [[nodiscard]] bool complete() const noexcept;

private:
    friend class Sections;
    static constexpr std::uint8_t kNonOre = 1, kFirstOre = 2;
    BuildTicket ticket_;
    std::array<std::uint8_t, kCellsPerSection> cells_{};
    std::size_t visited_ = 0;
    std::optional<End> end_;
    bool ambiguous_ = false;
};

// Truncated means full coverage with more positives than the retention limit.
enum class Coverage { Unobserved, ObservedEmpty, ObservedOres, Truncated };

// Host model; none of these events are wired to the game. Use one owning thread for this
// object's entire lifetime. Route reports only to the issuing instance. A future adapter must
// transfer sealed copies with synchronization and bounded storage, never share live BuildSamples
// or queue game pointers.
class Sections {
public:
    Sections() = default;
    Sections(const Sections&) = delete;
    Sections& operator=(const Sections&) = delete;

    // Clear on disable, context loss, or world/dimension replacement (even at identical XYZ).
    // This starts a fresh observation scope, including if the same WorldKey is passed again.
    void reset(std::optional<WorldKey> world);
    // Enroll before observing a verified build. Conservatively drop earlier observations at
    // that position. If not admitted, do not record any samples for this build.
    [[nodiscard]] std::optional<BuildTicket> begin(SectionPos section);
    // Owner-side signal: exactly this build has been adopted and the section is render-ready
    // in the selected view/range. Do not call at worker completion or for any guessed event.
    [[nodiscard]] bool adopt(BuildTicket ticket);
    // Only after adopt(). An early, incomplete or stale report isn't retained for later use.
    [[nodiscard]] bool publish(const BuildSamples& samples);
    // On content invalidation, retirement, or leaving the selected range. A later build at
    // the same XYZ requires a new ticket even if the engine reuses the same instance.
    void forget(SectionPos section);

    [[nodiscard]] Coverage coverage(SectionPos section) const;
    // Produce a bounded value snapshot, ordering sections nearest to the focus first. Filters
    // don't affect observation. This does not compute a camera or submit draw work.
    [[nodiscard]] std::optional<Snapshot> snapshot(
        BlockPos focus, const Settings& filters, std::size_t limit = kMaxTrackedBlocks) const;

private:
    struct Entry {
        std::uint64_t serial = 0;
        bool adopted = false;
        Coverage coverage = Coverage::Unobserved;
        std::vector<OutlineBlock> ores;
    };
    using Entries = std::map<SectionPos, Entry>;
    [[nodiscard]] Entry* current(BuildTicket ticket);
    std::optional<WorldKey> world_;
    std::uint64_t serial_ = 0; // Not reset: late tickets from earlier windows must not match.
    Entries entries_;
};

} // namespace ore_outline::passive
