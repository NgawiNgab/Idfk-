#include "ore_outline/chunk_scan.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>

using namespace ore_outline;

namespace {

#define CHECK(...) do { if (!(__VA_ARGS__)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": " << #__VA_ARGS__ << '\n'; std::abort(); \
} } while (false)

constexpr WorldKey kWorld{1, 0};
constexpr BlockPos kPlayer{3, 100, 7};
constexpr ReadySection kHome{{0, 6, 0}, 21};
constexpr ReadySection kWest{{-1, 6, 0}, 41};
constexpr std::array kSingle{kHome};

ScanOptions options(std::size_t budget = 256) {
    ScanOptions result;
    result.filters.enabled = true;
    result.filters.categories.set(static_cast<std::size_t>(Category::Diamond));
    result.filters.categories.set(static_cast<std::size_t>(Category::Gold));
    result.vertical_sections = 0;
    result.reads_per_update = budget;
    return result;
}

ClientScanFrame frame(std::span<const ReadySection> sections, std::int32_t low = 96, std::int32_t high = 111) {
    return {kWorld, kPlayer, low, high, sections};
}

struct FixtureReader final : ExistingSectionReader {
    WorldKey active_world = kWorld;
    std::map<SectionPos, std::uint64_t> present;
    std::map<BlockPos, std::string> blocks;
    std::vector<SectionPos> checks;
    std::vector<BlockPos> reads;
    std::optional<BlockPos> fail_at;
    std::optional<BlockPos> drop_after;
    std::string default_type = "minecraft:stone";

    void set_present(ReadySection stamp) { present[stamp.position] = stamp.revision; }

    bool still_readable(WorldKey world, ReadySection stamp) override {
        checks.push_back(stamp.position);
        last_check.reset();
        const auto it = present.find(stamp.position);
        if (world != active_world || it == present.end() || it->second != stamp.revision) return false;
        last_check = stamp;
        return true;
    }

    std::optional<std::string> read_existing(WorldKey world, ReadySection stamp, BlockPos pos) override {
        CHECK(world == active_world);
        CHECK(last_check && last_check->position == stamp.position && last_check->revision == stamp.revision);
        CHECK(section_containing(pos) == stamp.position);
        last_check.reset();
        reads.push_back(pos);
        if (fail_at == pos) return std::nullopt;
        if (drop_after == pos) present.erase(stamp.position);
        const auto it = blocks.find(pos);
        return it == blocks.end() ? default_type : it->second;
    }

private:
    std::optional<ReadySection> last_check;
};

Snapshot snapshot(const ChunkScanner& scan, std::size_t count) {
    auto copy = scan.snapshot();
    CHECK(copy && copy->blocks.size() == count);
    return *copy;
}

bool has_outline(const Snapshot& shot, BlockPos position, Category category) {
    return std::ranges::any_of(shot.blocks, [&](const OutlineBlock& block) {
        return block.position == position && block.category == category;
    });
}

void discovers_neighbors_without_coordinates() {
    ChunkScanner scan;
    FixtureReader source;
    constexpr ReadySection too_far{{2, 6, 0}, 51};
    constexpr ReadySection data_not_render_ready{{0, 6, 1}, 61};
    constexpr std::array feed{too_far, kWest, kHome}; // No placement positions in the feed.
    for (auto stamp : feed) source.set_present(stamp);
    source.set_present(data_not_render_ready);

    constexpr BlockPos diamond{13, 104, 9};
    constexpr BlockPos gold{-11, 101, 3};
    source.blocks = {{diamond, "minecraft:deepslate_diamond_ore"}, {gold, "minecraft:gold_ore"},
                     {{0, 100, 23}, "minecraft:diamond_ore"}, {{36, 100, 1}, "minecraft:diamond_ore"}};
    constexpr std::size_t per_section = 16 * 16 * 16;
    constexpr std::size_t budget = 243; // Doesn't divide a section, so the cursor must cross ticks.
    auto settings = options(budget);
    for (std::size_t remaining = 2 * per_section; remaining > 0;) {
        const auto step = scan.step(frame(feed), settings, source);
        CHECK(step.ready_sections == 2 && step.block_reads == std::min(remaining, budget));
        CHECK(step.readiness_checks == 2 + step.block_reads);
        remaining -= step.block_reads;
        CHECK((step.pending_sections == 0) == (remaining == 0));
    }
    CHECK(source.reads.front() == BlockPos{0, 96, 0}); // Player's section first despite feed order.
    CHECK(source.reads[per_section] == BlockPos{-16, 96, 0});
    CHECK(std::set<BlockPos>(source.reads.begin(), source.reads.end()).size() == 2 * per_section);
    CHECK(std::ranges::none_of(source.checks, [&](SectionPos pos) {
        return pos == too_far.position || pos == data_not_render_ready.position;
    }));
    const auto shot = snapshot(scan, 2);
    CHECK(has_outline(shot, diamond, Category::Diamond));
    CHECK(has_outline(shot, gold, Category::Gold));
    const CameraFrame synthetic{kWorld,
                                {0.04, 0, 0.018, 0, 0, 0.04, 0.014, -4, 0, 0, 0.04, 0, 0, 0, 0, 1},
                                {0, 0, 600, 400}};
    CHECK(project_outlines(shot, synthetic).size() == 24);

    auto moved = frame(feed);
    moved.player.x = 10; // Still in the same section; don't restart empty work.
    CHECK(scan.step(moved, settings, source).block_reads == 0);
    CHECK(source.reads.size() == 2 * per_section);
}

void invalidates_revisions_range_and_filters() {
    ChunkScanner scan;
    FixtureReader source;
    std::array feed{kHome, kWest};
    for (auto stamp : feed) source.set_present(stamp);
    constexpr BlockPos obsolete{3, 100, 9};
    constexpr BlockPos added{4, 100, 4};
    constexpr BlockPos west{-1, 100, 10};
    source.blocks = {{obsolete, "minecraft:diamond_ore"}, {west, "minecraft:diamond_ore"}};
    auto current = frame(feed, 100, 100);
    auto settings = options(1024);
    CHECK(scan.step(current, settings, source).block_reads == 512);
    CHECK(has_outline(snapshot(scan, 2), obsolete, Category::Diamond));

    // A new stamp dirties one section only. Discard the old value before reading any replacement.
    ++feed[0].revision;
    source.set_present(feed[0]);
    source.blocks[obsolete] = "minecraft:stone";
    source.blocks[added] = "minecraft:gold_ore";
    settings.reads_per_update = 0;
    const auto changed = scan.step(current, settings, source);
    CHECK(changed.pending_sections == 1 && changed.block_reads == 0);
    CHECK(has_outline(snapshot(scan, 1), west, Category::Diamond));
    settings.reads_per_update = 1024;
    CHECK(scan.step(current, settings, source).block_reads == 256);
    CHECK(has_outline(snapshot(scan, 2), added, Category::Gold));

    // Air in a live section also removes an ore; the other section doesn't have to be rescanned.
    ++feed[1].revision;
    source.set_present(feed[1]);
    source.blocks[west] = "minecraft:air";
    CHECK(scan.step(current, settings, source).block_reads == 256);
    CHECK(has_outline(snapshot(scan, 1), added, Category::Gold));

    settings.reads_per_update = 0;
    settings.filters.categories.reset(static_cast<std::size_t>(Category::Gold));
    CHECK(scan.step(current, settings, source).block_reads == 0);
    snapshot(scan, 0);
    settings.filters.categories.set(static_cast<std::size_t>(Category::Gold));
    CHECK(scan.step(current, settings, source).pending_sections == 2);
    snapshot(scan, 0); // Re-enabling never revives stale cache entries.
    settings.reads_per_update = 1024;
    CHECK(scan.step(current, settings, source).block_reads == 512);
    CHECK(has_outline(snapshot(scan, 1), added, Category::Gold));

    current.ready_sections = std::span<const ReadySection>(feed).subspan(1);
    CHECK(scan.step(current, settings, source).block_reads == 0);
    snapshot(scan, 0); // A section removed from the render-ready feed is immediately dropped.
    current.ready_sections = feed;
    CHECK(scan.step(current, settings, source).block_reads == 256);
    CHECK(has_outline(snapshot(scan, 1), added, Category::Gold));
    current.player.x = 32; // Home is now two sections away, outside our radius of one.
    CHECK(scan.step(current, settings, source).ready_sections == 0);
    snapshot(scan, 0);
}

void refuses_unavailable_reads() {
    ChunkScanner scan;
    FixtureReader source;
    source.set_present({kHome.position, kHome.revision - 1});
    constexpr BlockPos first{0, 100, 0};
    source.blocks[first] = "minecraft:diamond_ore";
    const auto current = frame(kSingle, 100, 100);
    auto settings = options();
    auto result = scan.step(current, settings, source);
    CHECK(result.ready_sections == 0 && result.readiness_checks == 1 && source.reads.empty());
    snapshot(scan, 0);

    source.set_present(kHome);
    CHECK(scan.step(current, settings, source).block_reads == 256);
    CHECK(has_outline(snapshot(scan, 1), first, Category::Diamond));
    source.present.clear();
    CHECK(scan.step(current, settings, source).block_reads == 0);
    snapshot(scan, 0); // A stale feed cannot keep an old outline from a completed section.

    source.set_present(kHome);
    source.drop_after = first;
    source.reads.clear();
    result = scan.step(current, settings, source);
    CHECK(result.block_reads == 1 && result.ready_sections == 0 && result.readiness_checks == 3);
    CHECK(source.reads == std::vector<BlockPos>{first}); // Lost readiness before second lookup.
    snapshot(scan, 0);

    source.set_present(kHome);
    source.drop_after.reset();
    source.fail_at = {1, 100, 0};
    result = scan.step(current, settings, source);
    CHECK(result.block_reads == 2 && result.ready_sections == 0);
    snapshot(scan, 0); // nullopt isn't air and ends work on this invalid section for this update.
}

void bounds_and_lifecycle() {
    ChunkScanner scan;
    FixtureReader source;
    auto settings = options(std::numeric_limits<std::size_t>::max());
    settings.horizontal_sections = std::numeric_limits<unsigned>::max();
    settings.vertical_sections = std::numeric_limits<unsigned>::max();
    constexpr auto lo = std::numeric_limits<std::int32_t>::min();
    constexpr auto hi = std::numeric_limits<std::int32_t>::max();
    constexpr BlockPos extreme{hi, lo, hi};
    constexpr auto pos = section_containing(extreme);
    constexpr ReadySection edge{pos, 9};
    constexpr ReadySection far{{pos.x - 5, pos.y, pos.z}, 12};
    constexpr ReadySection unknown{{pos.x - 1, pos.y, pos.z}, 0};
    constexpr std::array edge_feed{edge, far, unknown};
    for (auto stamp : edge_feed) source.set_present(stamp);
    source.default_type = "minecraft:diamond_ore";
    ClientScanFrame edge_frame{kWorld, extreme, lo + 5, lo + 10, edge_feed};
    const auto s1 = scan.step(edge_frame, settings, source);
    CHECK(s1.ready_sections == 1 && s1.block_reads == kMaxReadsPerUpdate && s1.pending_sections == 1);
    CHECK(source.reads.front() == BlockPos{hi - 15, lo + 5, hi - 15});
    const auto s2 = scan.step(edge_frame, settings, source);
    CHECK(s2.ready_sections == 1 && s2.block_reads == 6 * 256 - kMaxReadsPerUpdate && s2.pending_sections == 0);
    CHECK(source.reads.back() == BlockPos{hi, lo + 10, hi});
    CHECK(std::ranges::all_of(source.checks, [&](SectionPos checked) { return checked == pos; }));
    snapshot(scan, kMaxTrackedBlocks);

    settings.reads_per_update = 0;
    edge_frame.world = {2, 0}; // New world with the same coordinates and stamps.
    source.active_world = edge_frame.world;
    CHECK(scan.step(edge_frame, settings, source).pending_sections == 1);
    CHECK(snapshot(scan, 0).world == edge_frame.world);
    settings.filters.enabled = false;
    source.checks.clear();
    CHECK(scan.step(edge_frame, settings, source).block_reads == 0);
    CHECK(!scan.snapshot() && source.checks.empty());
    settings.filters.enabled = true;
    settings.reads_per_update = 1;
    CHECK(scan.step(edge_frame, settings, source).block_reads == 1);
    snapshot(scan, 1);
    CHECK(scan.step(std::nullopt, settings, source).block_reads == 0);
    CHECK(!scan.snapshot());
}

void rejects_ambiguous_and_invalid_frames() {
    ChunkScanner scan;
    FixtureReader source;
    source.set_present(kHome);
    source.default_type = "minecraft:diamond_ore";
    const auto settings = options(1);
    CHECK(scan.step(frame(kSingle), settings, source).block_reads == 1);
    snapshot(scan, 1);
    source.checks.clear();
    source.reads.clear();
    const std::array duplicate{kHome, ReadySection{kHome.position, 0}};
    const std::vector<ReadySection> oversized(kMaxReadySectionFeed + 1, kHome);
    auto invalid_world = frame(kSingle);
    invalid_world.world.epoch = 0;
    for (const auto bad : {frame(duplicate), frame(oversized), frame(kSingle, 101, 100), invalid_world}) {
        CHECK(scan.step(bad, settings, source).readiness_checks == 0);
        CHECK(!scan.snapshot() && source.checks.empty() && source.reads.empty());
    }
    auto no_categories = settings;
    no_categories.filters.categories.reset();
    CHECK(scan.step(frame(kSingle), no_categories, source).readiness_checks == 0);
    CHECK(!scan.snapshot() && source.checks.empty() && source.reads.empty());
}

} // namespace

int main() {
    discovers_neighbors_without_coordinates();
    invalidates_revisions_range_and_filters();
    refuses_unavailable_reads();
    bounds_and_lifecycle();
    rejects_ambiguous_and_invalid_frames();
    std::cout << "chunk scanner: guarded two-chunk discovery, budgets, revisions, and lifetime passed\n";
}
