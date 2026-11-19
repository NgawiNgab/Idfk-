#include "ore_outline/terrain_capture.hpp"

#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
using namespace ore_outline;
using namespace terrain_capture;

#define CHECK(expression)                                                                      \
    do {                                                                                       \
        if (!(expression)) {                                                                   \
            std::cerr << __LINE__ << ": " << #expression << '\n';                              \
            std::exit(EXIT_FAILURE);                                                           \
        }                                                                                      \
    } while (false)

void test_outcomes() {
    Capture capture;
    CHECK(capture.token() == 0);
    capture.record(0, kExposed, kDiamondKey);
    CHECK(!capture.report().armed && !capture.report().builder_seen);

    capture.arm();
    const auto window = capture.token();
    CHECK(window && capture.report().armed && !capture.report().builder_seen);
    capture.record(window, BlockPos{4, 4, 4}, kDiamondKey);
    CHECK(capture.report().builder_seen && capture.report().exposed == Match::NoVisit);
    capture.record(window, kExposed);
    CHECK(capture.report().exposed == Match::UnknownKey);
    capture.record(window, kExposed, kAirKey);
    CHECK(capture.report().exposed == Match::AirKeyOrFallback);
    capture.record(window, kExposed, 1);
    CHECK(capture.report().exposed == Match::OtherKey);
    capture.record(window, kExposed, kDiamondKey);
    capture.record(window, kExposed, kAirKey);
    CHECK(capture.report().exposed == Match::DiamondKey);
    CHECK(capture.report().enclosed == Match::NoVisit);

    capture.stop();
    capture.record(window, kEnclosed, kDiamondKey);
    CHECK(capture.token() == 0 && !capture.report().armed);
    CHECK(capture.report().enclosed == Match::NoVisit);
    capture.arm();
    const auto next = capture.token();
    CHECK(next && next != window);
    capture.record(window, kEnclosed, kDiamondKey);
    CHECK(!capture.report().builder_seen && capture.report().exposed == Match::NoVisit);
    capture.record(next, kEnclosed, 0);
    CHECK(capture.report().enclosed == Match::UnknownKey);
}

void test_concurrent_workers() {
    Capture capture;
    capture.arm();
    const auto window = capture.token();
    std::atomic<bool> start{false};
    const auto record = [&](BlockPos point) {
        while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
        for (int i = 0; i < 1000; ++i) capture.record(window, point, i == 500 ? kDiamondKey : 42);
    };
    std::thread exposed(record, kExposed);
    std::thread enclosed(record, kEnclosed);
    start.store(true, std::memory_order_release);
    exposed.join();
    enclosed.join();
    const auto report = capture.report();
    CHECK(report.builder_seen && report.exposed == Match::DiamondKey && report.enclosed == Match::DiamondKey);
}
} // namespace

int main() {
    test_outcomes();
    test_concurrent_workers();
    std::cout << "terrain capture: empty/unknown/air/other/diamond, freeze, epoch isolation, concurrent workers passed\n";
}
