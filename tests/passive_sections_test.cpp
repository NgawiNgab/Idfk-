#include "ore_outline/passive_sections.hpp"

#include <cassert>
#include <iostream>
#include <limits>
#include <set>

using namespace ore_outline;
using namespace ore_outline::passive;

namespace {

constexpr WorldKey kWorld{1, 0};
constexpr std::string_view kDiamond = "minecraft:diamond_ore";
constexpr std::string_view kGold = "minecraft:gold_ore";
constexpr std::string_view kStone = "minecraft:stone";
constexpr std::string_view kAir = "minecraft:air";

Settings enabled() {
    Settings filters;
    filters.enabled = true;
    filters.categories.set();
    return filters;
}

BuildTicket begin(Sections& sections, SectionPos section) {
    const auto ticket = sections.begin(section);
    assert(ticket);
    return *ticket;
}

template<class Fixture>
void visit_section(BuildSamples& samples, SectionPos base, Fixture fixture) {
    // Produce the same scoped callback shape for every cell. The collector receives only
    // its section and the streamed values, not the fixture's ore positions.
    for (std::int32_t y = 0; y < 16; ++y) {
        for (std::int32_t z = 0; z < 16; ++z) {
            for (std::int32_t x = 0; x < 16; ++x) {
                const BlockPos position{16 * base.x + x, 16 * base.y + y, 16 * base.z + z};
                samples.record(position, classify(fixture(position)));
            }
        }
    }
}

template<class Fixture>
BuildSamples stream(BuildTicket ticket, Fixture fixture, End end = End::Finished) {
    BuildSamples samples(ticket);
    visit_section(samples, ticket.section, fixture);
    samples.finish(end);
    return samples;
}

void lifecycle_test() {
    constexpr SectionPos west{-1, 6, 0}, east{0, 6, 0};
    constexpr BlockPos west_ore{-3, 109, 13}, east_ore{9, 111, 4}, focus{0, 102, 0};
    const auto fixture = [west_ore, east_ore](BlockPos p) {
        return p == west_ore ? kDiamond : p == east_ore ? kGold : kStone;
    };
    const auto empty = [](BlockPos) { return kStone; };
    Sections sections;
    assert(!sections.begin(west));
    sections.reset(kWorld);
    const auto west_ticket = begin(sections, west);
    const auto east_ticket = begin(sections, east);
    const auto west_samples = stream(west_ticket, fixture);
    const auto east_samples = stream(east_ticket, fixture);
    assert(west_samples.complete() && east_samples.complete());
    assert(!sections.publish(west_samples)); // Finished on the worker, not yet adopted.
    assert(sections.coverage(west) == Coverage::Unobserved);
    assert(sections.snapshot(focus, enabled())->blocks.empty());

    assert(sections.adopt(west_ticket) && sections.adopt(east_ticket));
    assert(sections.coverage(east) == Coverage::Unobserved); // Adoption is not coverage either.
    assert(sections.publish(west_samples) && sections.publish(east_samples));
    assert(!sections.publish(west_samples)); // Only one publication for the current build.
    auto snapshot = sections.snapshot(focus, enabled());
    assert(snapshot->world == kWorld && snapshot->blocks.size() == 2);
    assert(snapshot->blocks[0].position == east_ore && snapshot->blocks[0].category == Category::Gold);
    assert(snapshot->blocks[1].position == west_ore && snapshot->blocks[1].category == Category::Diamond);

    auto diamonds_only = enabled();
    diamonds_only.categories.reset(static_cast<std::size_t>(Category::Gold));
    snapshot = sections.snapshot(focus, diamonds_only, 1);
    assert(snapshot->blocks.size() == 1 && snapshot->blocks.front().position == west_ore);
    assert(sections.coverage(east) == Coverage::ObservedOres); // Filter changes don't resample.

    sections.forget(west); // Content change, before a replacement lands.
    assert(!sections.adopt(west_ticket) && !sections.publish(west_samples));
    assert(sections.coverage(west) == Coverage::Unobserved);
    assert(sections.snapshot(focus, enabled())->blocks.size() == 1);

    const auto superseded = begin(sections, west);
    const auto old_report = stream(superseded, fixture);
    const auto replacement = begin(sections, west);
    assert(!sections.adopt(superseded) && !sections.publish(old_report));
    assert(sections.adopt(replacement));
    const auto after_edit = stream(replacement, empty);
    assert(sections.publish(after_edit));
    assert(sections.coverage(west) == Coverage::ObservedEmpty);
    assert(sections.snapshot(focus, enabled())->blocks.size() == 1);

    sections.forget(east); // Out of range even if the game still owns a geometry object.
    assert(!sections.publish(east_samples));
    assert(sections.coverage(east) == Coverage::Unobserved);
    const auto reentry = begin(sections, east);
    assert(reentry.serial != east_ticket.serial);
    assert(!sections.adopt(east_ticket) && !sections.publish(east_samples));
    assert(sections.snapshot(focus, enabled())->blocks.empty());
    assert(sections.adopt(reentry) && sections.publish(stream(reentry, fixture)));

    for (const auto next : {WorldKey{1, 1}, WorldKey{2, 0}, kWorld}) {
        sections.reset(next); // Dimension change, world change; reuse the same coordinates.
        assert(sections.snapshot(focus, enabled())->blocks.empty());
        const auto current = begin(sections, east);
        assert(sections.adopt(current));
        assert(!sections.publish(east_samples));
    }
    sections.reset(std::nullopt);
    assert(!sections.snapshot(focus, enabled()) && !sections.begin(east));
    sections.reset(kWorld);
    assert(sections.adopt(begin(sections, east)));
    assert(!sections.publish(east_samples));
    sections.reset(WorldKey{0, 0});
    assert(!sections.snapshot(focus, enabled()));
}

void coverage_test() {
    constexpr SectionPos section{0, 0, -1};
    constexpr BlockPos position{0, 0, -16};
    Sections sections;
    sections.reset(kWorld);
    for (const auto end : {End::Clipped, End::Canceled}) {
        const auto ticket = begin(sections, section);
        auto report = stream(ticket, [](BlockPos) { return kDiamond; }, end);
        assert(report.visited() == kCellsPerSection && !report.complete());
        report.finish(End::Finished); // Late completion cannot reopen a sealed report.
        // Even a mistakenly reported owner-side adoption cannot turn partial work into a result.
        assert(sections.adopt(ticket) && !sections.publish(report));
        assert(sections.coverage(section) == Coverage::Unobserved);
    }
    {
        const auto ticket = begin(sections, section);
        BuildSamples report(ticket);
        for (std::size_t i = 0; i < kCellsPerSection; ++i) report.record(position, classify(kAir));
        report.record({0, 0, -17}, classify(kDiamond)); // Neighbor, not part of the section.
        assert(report.visited() == 1);
        report.finish(End::Finished);
        assert(!report.complete());
        assert(sections.adopt(ticket) && !sections.publish(report));
    }
    {
        const auto ticket = begin(sections, section);
        const auto report = stream(ticket, [position](BlockPos p) -> std::optional<std::string_view> {
            return p == position ? std::nullopt : std::optional{kAir}; // Unavailable, not loaded air.
        });
        assert(!report.complete());
        assert(sections.adopt(ticket) && !sections.publish(report));
        assert(sections.coverage(section) == Coverage::Unobserved);
    }
    {
        const auto ticket = begin(sections, section);
        BuildSamples report(ticket);
        visit_section(report, section, [](BlockPos) { return kDiamond; });
        report.record(position, classify(kAir)); // Conflicting copies despite full cell coverage.
        report.finish(End::Finished);
        assert(report.visited() == kCellsPerSection && !report.complete());
        assert(sections.adopt(ticket) && !sections.publish(report));
    }
    const auto ticket = begin(sections, section);
    auto report = stream(ticket, [](BlockPos) { return kAir; });
    report.record(position, classify(kDiamond)); // Shut after the worker returns.
    assert(sections.adopt(ticket) && sections.publish(report));
    assert(sections.coverage(section) == Coverage::ObservedEmpty);
    assert(sections.snapshot(position, enabled())->blocks.empty());
    assert(!sections.snapshot(position, Settings{}));
}

void scale_and_limits_test() {
    Sections sections;
    sections.reset(kWorld);
    const auto sparse = [](BlockPos p) {
        return local_coordinate(p.x) == 11 && local_coordinate(p.y) == 3 && local_coordinate(p.z) == 8 ? kDiamond : kStone;
    };
    // Grow the set 5 -> 22 -> 31 columns. Use two stacked sections in each column.
    for (std::int32_t columns : {5, 22, 31}) {
        for (std::int32_t x = 0; x < columns; ++x) {
            for (std::int32_t y : {-1, 0}) {
                const SectionPos section{x, y, -1};
                if (sections.coverage(section) != Coverage::Unobserved) continue;
                const auto ticket = begin(sections, section);
                const auto report = stream(ticket, sparse);
                assert(report.complete() && sections.adopt(ticket) && sections.publish(report));
            }
        }
        const auto snapshot = sections.snapshot({0, 0, -1}, enabled());
        assert(snapshot->blocks.size() == static_cast<std::size_t>(2 * columns));
        std::set<SectionPos> found;
        for (const auto& hit : snapshot->blocks) {
            assert(hit.category == Category::Diamond && sparse(hit.position) == kDiamond);
            const auto section = section_containing(hit.position);
            assert(0 <= section.x && section.x < columns && -1 <= section.y && section.y <= 0 && section.z == -1);
            assert(found.insert(section).second);
        }
    }
    for (std::int32_t x = 5; x < 31; ++x) {
        for (std::int32_t y : {-1, 0}) sections.forget({x, y, -1});
    }
    assert(sections.snapshot({0, 0, -1}, enabled())->blocks.size() == 10);

    sections.reset(kWorld);
    for (std::int32_t x : {40, 30, 20, 10, 0}) {
        const auto ticket = begin(sections, {x, 0, 0});
        assert(sections.adopt(ticket) && sections.publish(stream(ticket, [](BlockPos) { return kDiamond; })));
        assert(sections.coverage(ticket.section) == Coverage::Truncated);
    }
    const auto snapshot = sections.snapshot({0, 0, 0}, enabled(), std::numeric_limits<std::size_t>::max());
    assert(snapshot->blocks.size() == kMaxTrackedBlocks);
    for (const auto& hit : snapshot->blocks) assert(section_containing(hit.position).x < 40);
    assert(sections.snapshot({640, 0, 0}, enabled(), 1)->blocks[0].position.x == 640);
    assert(sections.snapshot({0, 0, 0}, enabled(), 0)->blocks.empty());

    sections.reset(kWorld);
    for (std::size_t i = 0; i < kMaxSections; ++i) {
        assert(sections.begin({static_cast<std::int32_t>(i), 0, 0}));
    }
    assert(!sections.begin({-1, 0, 0}));
    const auto current = begin(sections, {0, 0, 0});
    sections.forget(current.section);
    assert(!sections.adopt(current) && sections.begin({-1, 0, 0}));

    sections.reset(kWorld);
    const auto min = std::numeric_limits<std::int32_t>::min();
    const auto max = std::numeric_limits<std::int32_t>::max();
    const BlockPos corner{min, max, min};
    const auto extreme = begin(sections, section_containing(corner));
    assert(!sections.begin({max, min, min})); // Not a representable block-section coordinate.
    const auto result = stream(extreme, [corner](BlockPos p) { return p == corner ? kDiamond : kAir; });
    assert(sections.adopt(extreme) && sections.publish(result));
    const auto extreme_snapshot = sections.snapshot({max, min, max}, enabled());
    assert(extreme_snapshot->blocks.size() == 1 && extreme_snapshot->blocks[0].position == corner);
}

} // namespace

int main() {
    lifecycle_test();
    coverage_test();
    scale_and_limits_test();
    std::cout << "passive sections: 5 -> 22 -> 31 -> 5 columns, adoption, replacement, retirement and limits passed\n";
}
