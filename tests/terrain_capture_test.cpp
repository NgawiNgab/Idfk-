#include "ore_outline/terrain_capture.hpp"

#include <cstdlib>
#include <iostream>
#include <latch>
#include <limits>
#include <set>
#include <thread>

namespace {
using namespace ore_outline;
using namespace terrain_capture;

#define CHECK(...)                                                    \
    do {                                                              \
        if (!(__VA_ARGS__)) {                                         \
            std::cerr << __LINE__ << ": " << #__VA_ARGS__ << '\n';     \
            std::exit(EXIT_FAILURE);                                  \
        }                                                             \
    } while (false)

void test_sampled_discovery() {
    constexpr auto fallback_air = std::uint64_t{0xbd584baf00003448};
    constexpr BlockPos west{-19, -48, 13}, east{49, -32, -80}, stacked{51, -46, -65};
    Capture capture;
    CHECK(!capture.visit());
    capture.record(0, west, kDiamondKey);
    CHECK(!capture.report().builder_seen && capture.report().sections == 0);

    CHECK(capture.arm());
    const auto window = capture.visit();
    CHECK(window);
    CHECK(!capture.arm());
    capture.record(window, {7, 2, 1}, 42); // Ordinary non-origin non-match: not sampled.
    capture.record(window, {-32, -48, 0}, fallback_air); // Visited, not proof of loaded air.
    capture.record(window, west, kDiamondKey); // Previously unknown points, not requested reads.
    capture.record(window, west, kDiamondKey);
    capture.record(window, {-18, -48, 13}, kDiamondKey); // Same section: only one sample.
    capture.record(window, west, fallback_air); // Historical sample, not a live ore cache.
    capture.record(window, east, kDeepslateDiamondKey);
    capture.record(window, stacked, kDeepslateDiamondKey); // Same column, different Y section.
    capture.record(window, {32, 0, 32}, std::nullopt); // Unknown at an origin: only a visit.
    capture.stop();

    const auto report = capture.report();
    CHECK(report.builder_seen && !report.contention && !report.limited);
    CHECK(report.sections == 4 && report.columns == 3 && report.matched_sections == 3);
    CHECK(report.example_count == 3);
    const auto first = section_containing(report.examples[0].position);
    const auto second = section_containing(report.examples[1].position);
    CHECK(first.x != second.x || first.z != second.z); // Show distinct columns first.
    bool found_west = false, found_east = false, found_stacked = false;
    for (std::size_t i = 0; i < report.example_count; ++i) {
        const auto& hit = report.examples[i];
        found_west |= hit.position == west && hit.match == Match::Diamond;
        found_east |= hit.position == east && hit.match == Match::DeepslateDiamond;
        found_stacked |= hit.position == stacked && hit.match == Match::DeepslateDiamond;
    }
    CHECK(found_west && found_east && found_stacked);
}

void test_early_stop_and_rearm() {
    constexpr BlockPos point{1000, -41, -99};
    constexpr auto lo = std::numeric_limits<std::int32_t>::min();
    constexpr auto hi = std::numeric_limits<std::int32_t>::max();
    constexpr BlockPos new_point{lo + 1, hi, lo};
    Capture capture;
    CHECK(capture.arm());
    const auto old_window = capture.visit();
    capture.stop();
    capture.record(old_window, point, kDiamondKey);
    const auto stopped = capture.report();
    CHECK(stopped.builder_seen && stopped.sections == 0); // Lookup entered before returning.
    CHECK(!capture.visit());
    CHECK(capture.arm());
    capture.record(old_window, point, kDiamondKey);
    CHECK(!capture.report().builder_seen && capture.report().sections == 0);
    const auto current_window = capture.visit();
    CHECK(current_window && old_window != current_window);
    capture.record(current_window, new_point, kDiamondKey);
    capture.stop();
    const auto current = capture.report();
    CHECK(current.builder_seen && current.matched_sections == 1 && current.example_count == 1);
    CHECK(current.examples[0].position == new_point && current.examples[0].match == Match::Diamond);
}

void test_limit() {
    Capture capture;
    CHECK(capture.arm());
    const auto window = capture.visit();
    capture.record(window, {0, 0, 0}, {});
    for (std::int32_t i = 1; i <= static_cast<std::int32_t>(kMaxSampledSections * 2); ++i) {
        capture.record(window, {16 * i, -32, 0}, {});
    }
    capture.record(window, {1, 2, 3}, kDiamondKey); // Can still enrich an admitted section.
    capture.stop();
    const auto report = capture.report();
    CHECK(report.limited && !report.contention && report.sections == kMaxSampledSections);
    CHECK(report.columns == report.sections && report.matched_sections == 1 && report.example_count == 1);
    CHECK(report.examples[0].position == BlockPos{1, 2, 3});
}

void test_example_cap() {
    Capture capture;
    CHECK(capture.arm());
    const auto window = capture.visit();
    for (std::int32_t x = -3; x <= 3; ++x) {
        for (std::int32_t y : {-62, -41}) capture.record(window, {16 * x + 2, y, -8}, kDeepslateDiamondKey);
    }
    capture.stop();
    const auto report = capture.report();
    CHECK(report.sections == 14 && report.matched_sections == 14 && report.columns == 7);
    CHECK(report.example_count == kMaxExamples);
    std::set<std::int32_t> shown_columns;
    for (const auto& hit : report.examples) {
        CHECK(hit.match == Match::DeepslateDiamond && shown_columns.insert(chunk_coordinate(hit.position.x)).second);
    }
}

void test_concurrent_freeze() {
    Capture capture;
    CHECK(capture.arm());
    std::latch entered(2), record_now(1);
    const auto writer = [&](std::int32_t x) {
        const auto held = capture.visit();
        entered.count_down();
        record_now.wait();
        for (int i = 0; i < 1000; ++i) capture.record(held, {x, -20, 1}, kDiamondKey);
    };
    std::thread west(writer, -7), east(writer, 18);
    entered.wait();
    record_now.count_down();
    capture.stop();
    const auto frozen = capture.report();
    CHECK(frozen.builder_seen && frozen.sections <= 2 && frozen.matched_sections == frozen.sections);
    CHECK(capture.arm());
    const auto next = capture.visit();
    west.join();
    east.join();
    capture.record(next, {101, -1, 0}, kDeepslateDiamondKey);
    capture.stop();
    const auto current = capture.report();
    CHECK(current.sections == 1 && current.example_count == 1);
    CHECK(current.examples[0].position == BlockPos{101, -1, 0});
}
} // namespace

int main() {
    test_sampled_discovery();
    test_early_stop_and_rearm();
    test_limit();
    test_example_cap();
    test_concurrent_freeze();
    std::cout << "terrain discovery: unknown positions, bounded samples, epoch isolation and concurrent freeze passed\n";
}
