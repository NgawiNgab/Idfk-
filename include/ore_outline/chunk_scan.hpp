#pragma once

#include "ore_outline/core.hpp"

#include <span>
#include <string>

namespace ore_outline {

struct ReadySection {
    SectionPos position;
    // Nonzero content stamp; change on reload or block changes, including hidden blocks.
    // A renderer mesh revision on its own need not cover these changes.
    std::uint64_t revision;
};

constexpr std::size_t kMaxReadySectionFeed = 1024;
constexpr unsigned kMaxHorizontalSectionRadius = 4;
constexpr unsigned kMaxVerticalSectionRadius = 3;
constexpr std::size_t kMaxReadsPerUpdate = 1024;

struct ClientScanFrame {
    WorldKey world;
    BlockPos player;
    std::int32_t min_y, max_y; // Inclusive valid block heights for this dimension.
    // Authoritative client render-ready sections for the scan window this update, not
    // a server-loaded list or the visible mesh faces. No duplicates within the window.
    // The adapter prefilters large lists. The span is used only during step().
    std::span<const ReadySection> ready_sections;
};

struct ScanOptions {
    Settings filters{};
    unsigned horizontal_sections = 1; // In either direction from the player's section.
    unsigned vertical_sections = 1;
    std::size_t reads_per_update = 256;
};

class ExistingSectionReader {
public:
    virtual ~ExistingSectionReader() = default;
    // A non-generating, existing-and-current check. An unknown or stale stamp is false.
    virtual bool still_readable(WorldKey world, ReadySection section) = 0;
    // Called only after a successful check. The adapter must keep validity through the
    // copy (e.g. with a verified owning update scope), or fail without a fallback read.
    // No load/request/generation. nullopt is unavailable; "minecraft:air" is actual air.
    // Return an owned value, never a game pointer or a view into engine data.
    virtual std::optional<std::string> read_existing(WorldKey world, ReadySection section, BlockPos position) = 0;
};

struct ScanStep {
    std::size_t readiness_checks{};
    std::size_t block_reads{}; // Attempted calls to read_existing, including failed copies.
    std::size_t ready_sections{};
    std::size_t pending_sections{};
};

// One owner thread. Call from a verified update context and publish snapshots with
// caller-provided synchronization. This class has no Minecraft bindings; neither
// the reader nor the input span is kept between updates.
class ChunkScanner {
public:
    ScanStep step(std::optional<ClientScanFrame> frame, const ScanOptions& options, ExistingSectionReader& reader);
    void clear();
    [[nodiscard]] std::optional<Snapshot> snapshot() const;

private:
    struct Context {
        WorldKey world;
        std::int32_t min_y, max_y;
        bool operator==(const Context&) const = default;
    };
    struct Progress {
        std::uint64_t revision;
        unsigned cursor = 0;
    };
    using Sections = std::map<SectionPos, Progress>;

    void discard_stale(const Sections& next);

    std::optional<Context> context_;
    Settings filters_{};
    Sections sections_;
    OutlineCache cache_;
};

} // namespace ore_outline
