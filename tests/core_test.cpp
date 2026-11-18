#include "ore_outline/core.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>

using namespace ore_outline;

#define CHECK(expr) do { if (!(expr)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": " << #expr << '\n'; \
    std::exit(EXIT_FAILURE); \
} } while (false)

namespace {
constexpr WorldKey kWorld{1, 0};
constexpr ScanScope kScope{kWorld, {-512, -64, -512}, {512, 319, 512}};
constexpr std::string_view kDiamond = "minecraft:diamond_ore";
constexpr std::string_view kGold = "minecraft:nether_gold_ore";

Settings enabled(std::initializer_list<Category> categories) {
    Settings s;
    s.enabled = true;
    for (const auto c : categories) s.categories.set(static_cast<std::size_t>(c));
    return s;
}

void expect_count(const OutlineCache& cache, std::size_t count) {
    const auto snapshot = cache.snapshot();
    CHECK(snapshot && snapshot->blocks.size() == count);
}

void test_names_and_coordinates() {
    CHECK(classify(std::nullopt).kind == BlockKind::Unavailable);
    CHECK(classify("minecraft:air").kind == BlockKind::Air);
    CHECK(classify("minecraft:stone").kind == BlockKind::Other);
    CHECK(classify("tile.diamond_ore").kind == BlockKind::Other);
    CHECK(classify("example:diamond_ore").kind == BlockKind::Other);
    CHECK(classify("minecraft:diamond_block").kind == BlockKind::Other);
    CHECK(classify("minecraft:diamond_ore_extra").kind == BlockKind::Other);
    CHECK(classify("minecraft:raw_gold_block").kind == BlockKind::Other);
    CHECK(classify("minecraft:barrel").kind == BlockKind::Other);
    struct OreFixture { std::string_view name; Category category; };
    constexpr OreFixture ores[] = {
        {"minecraft:coal_ore", Category::Coal},
        {"minecraft:deepslate_coal_ore", Category::Coal},
        {"minecraft:copper_ore", Category::Copper},
        {"minecraft:deepslate_copper_ore", Category::Copper},
        {"minecraft:iron_ore", Category::Iron},
        {"minecraft:deepslate_iron_ore", Category::Iron},
        {"minecraft:gold_ore", Category::Gold},
        {"minecraft:deepslate_gold_ore", Category::Gold},
        {kGold, Category::Gold},
        {"minecraft:redstone_ore", Category::Redstone},
        {"minecraft:lit_redstone_ore", Category::Redstone},
        {"minecraft:deepslate_redstone_ore", Category::Redstone},
        {"minecraft:lit_deepslate_redstone_ore", Category::Redstone},
        {"minecraft:lapis_ore", Category::Lapis},
        {"minecraft:deepslate_lapis_ore", Category::Lapis},
        {kDiamond, Category::Diamond},
        {"minecraft:deepslate_diamond_ore", Category::Diamond},
        {"minecraft:emerald_ore", Category::Emerald},
        {"minecraft:deepslate_emerald_ore", Category::Emerald},
        {"minecraft:quartz_ore", Category::Quartz},
        {"minecraft:ancient_debris", Category::AncientDebris},
    };
    for (const auto& [name, category] : ores) {
        const auto sample = classify(name);
        CHECK(sample.kind == BlockKind::Ore && sample.category == category);
        CHECK(color_for(category).a == 255);
    }
    CHECK((color_for(Category::Diamond) == Color{0, 255, 255, 255}));
    CHECK((color_for(Category::Gold) == Color{255, 255, 0, 255}));
    CHECK(color_for(Category::Count).a == 0);

    constexpr auto lo = std::numeric_limits<std::int32_t>::min();
    constexpr auto hi = std::numeric_limits<std::int32_t>::max();
    CHECK(chunk_coordinate(-17) == -2 && local_coordinate(-17) == 15);
    CHECK(chunk_coordinate(-16) == -1 && local_coordinate(-16) == 0);
    CHECK(chunk_coordinate(-1) == -1 && local_coordinate(-1) == 15);
    CHECK(chunk_coordinate(16) == 1 && local_coordinate(16) == 0);
    for (const auto n : {lo, lo + 1, -257, -17, -16, -15, -1, 0, 1, 15, 16, hi}) {
        const auto local = local_coordinate(n);
        CHECK(local >= 0 && local < 16);
        CHECK(static_cast<std::int64_t>(chunk_coordinate(n)) * 16 + local == n);
    }
}

void test_cache_and_filters() {
    OutlineCache cache;
    constexpr BlockPos here{-1, -16, 15};
    constexpr BlockPos there{16, -16, 15};
    cache.configure(kScope, {});
    cache.observe(kWorld, here, kDiamond);
    CHECK(!cache.snapshot());
    cache.configure(kScope, enabled({}));
    cache.observe(kWorld, here, kDiamond);
    expect_count(cache, 0);

    const auto two_categories = enabled({Category::Diamond, Category::Gold});
    cache.configure(kScope, two_categories);
    {
        // The caller's string storage may go away immediately after observe().
        std::string copied{"minecraft:deepslate_diamond_ore"};
        cache.observe(kWorld, here, copied);
        copied.clear();
    }
    const auto first = cache.snapshot();
    CHECK(first && first->blocks.size() == 1 && first->blocks[0].category == Category::Diamond);
    cache.observe(kWorld, here, kGold);
    expect_count(cache, 1);
    CHECK(cache.snapshot()->blocks[0].category == Category::Gold);
    CHECK(first->blocks[0].category == Category::Diamond);
    cache.observe(kWorld, here, "minecraft:stone");
    expect_count(cache, 0);
    cache.observe(kWorld, here, kDiamond);
    cache.observe(kWorld, here, std::nullopt);
    expect_count(cache, 0);
    cache.observe(kWorld, here, kDiamond);
    cache.observe(kWorld, here, "minecraft:air");
    expect_count(cache, 0);

    cache.observe(kWorld, here, kDiamond);
    cache.observe(kWorld, there, kGold);
    cache.observe(kWorld, {513, 0, 0}, kDiamond);
    expect_count(cache, 2);
    cache.configure(kScope, enabled({Category::Diamond}));
    expect_count(cache, 1);
    cache.configure(kScope, two_categories);
    expect_count(cache, 1); // Re-enabling a filter does not revive the old observation.

    auto shifted = kScope;
    shifted.min.x = 0;
    cache.configure(shifted, two_categories);
    expect_count(cache, 0);
    cache.observe(kWorld, there, kGold);
    expect_count(cache, 1);
    auto nether = shifted;
    nether.world.dimension = 1;
    cache.configure(nether, two_categories);
    cache.observe(kWorld, there, kGold); // Old dimension.
    expect_count(cache, 0);
    cache.observe(nether.world, there, kGold);
    expect_count(cache, 1);
    ++nether.world.epoch;
    cache.configure(nether, two_categories);
    cache.observe({1, 1}, there, kGold); // Old world in the same dimension.
    expect_count(cache, 0);
    cache.observe(nether.world, there, kGold);
    cache.configure(nether, {});
    CHECK(!cache.snapshot());
    cache.configure(nether, two_categories);
    expect_count(cache, 0);
    cache.observe(nether.world, there, kGold);
    cache.configure(std::nullopt, two_categories); // World exit.
    CHECK(!cache.snapshot());
    auto invalid = kScope;
    invalid.min.x = invalid.max.x + 1;
    cache.configure(invalid, two_categories);
    CHECK(!cache.snapshot());
}

void test_capacity() {
    OutlineCache cache;
    cache.configure(kScope, enabled({Category::Diamond, Category::Gold}));
    for (std::size_t i = 0; i < kMaxTrackedBlocks + 10; ++i) {
        cache.observe(kWorld, {static_cast<std::int32_t>(i), 0, 0}, kDiamond);
    }
    expect_count(cache, kMaxTrackedBlocks);
    cache.observe(kWorld, {0, 0, 0}, kGold); // Can still update a tracked block at capacity.
    CHECK(cache.snapshot()->blocks.front().category == Category::Gold);
    cache.observe(kWorld, {0, 0, 0}, std::nullopt);
    expect_count(cache, kMaxTrackedBlocks - 1);
    cache.observe(kWorld, {300, 0, 0}, kDiamond);
    expect_count(cache, kMaxTrackedBlocks);
}

constexpr CameraFrame kOblique{
    kWorld,
    {0.25, 0, 0.15, -0.3,
     0, 0.25, 0.1, -0.2,
     0, 0, 0.2, 0,
     0, 0, 0, 1},
    {10, 20, 240, 160},
};

bool in_view(const HudLine& line, const Viewport& v) {
    const auto point = [&](double x, double y) {
        return std::isfinite(x) && std::isfinite(y) && x >= v.left && y >= v.top &&
               x <= v.left + v.width && y <= v.top + v.height;
    };
    return point(line.x0, line.y0) && point(line.x1, line.y1);
}

void test_projection_and_budgets() {
    const Snapshot pair{kWorld, {{{0, 0, 0}, Category::Diamond}, {{1, 0, 0}, Category::Gold}}};
    const auto lines = project_outlines(pair, kOblique);
    CHECK(lines.size() == 20); // Two boxes, four common edges drawn only once.
    CHECK(std::all_of(lines.begin(), lines.end(), [](const auto& line) {
        return in_view(line, kOblique.viewport);
    }));
    CHECK(std::any_of(lines.begin(), lines.end(), [](const auto& line) {
        return line.color == color_for(Category::Gold);
    }));
    CHECK(project_outlines(pair, kOblique, 5).size() == 5);
    CHECK(project_outlines(pair, kOblique, 0).empty());
    CHECK(project_outlines(std::nullopt, kOblique).empty());
    CHECK(project_outlines(pair, std::nullopt).empty());

    auto invalid = kOblique;
    ++invalid.world.epoch;
    CHECK(project_outlines(pair, invalid).empty());
    invalid = kOblique;
    invalid.viewport.width = 0;
    CHECK(project_outlines(pair, invalid).empty());
    invalid = kOblique;
    invalid.viewport.left = -1;
    CHECK(project_outlines(pair, invalid).empty());
    invalid = kOblique;
    invalid.world_to_clip[0] = std::numeric_limits<double>::quiet_NaN();
    CHECK(project_outlines(pair, invalid).empty());
    invalid = kOblique;
    invalid.world_to_clip[15] = 0;
    CHECK(project_outlines(pair, invalid).empty());

    Snapshot many{kWorld, {}};
    for (std::size_t i = 0; i < kMaxTrackedBlocks + 10; ++i) {
        many.blocks.push_back({{static_cast<std::int32_t>(i * 2), 0, 0}, Category::Diamond});
    }
    auto wide = kOblique;
    wide.world_to_clip[0] = 0.001;
    CHECK(project_outlines(many, wide).size() == kDefaultLineLimit);
    CHECK(project_outlines(many, wide, std::numeric_limits<std::size_t>::max()).size() == kMaxLineLimit);
}

void test_frame_lifecycle() {
    OutlineCache cache;
    const auto selected = enabled({Category::Diamond});
    cache.configure(kScope, selected);
    cache.observe(kWorld, {0, 0, 0}, kDiamond);
    const auto lines = project_outlines(cache.snapshot(), kOblique);
    CHECK(lines.size() == 12);
    CHECK(std::all_of(lines.begin(), lines.end(), [](const auto& line) {
        return line.color == color_for(Category::Diamond);
    }));

    // Y goes up in clip space and down in HUD pixels (including viewport origin).
    const auto close = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    CHECK(std::any_of(lines.begin(), lines.end(), [&](const auto& line) {
        return close(line.x0, 94) && close(line.y0, 116) &&
               close(line.x1, 94) && close(line.y1, 96);
    }));

    cache.configure(kScope, {});
    CHECK(project_outlines(cache.snapshot(), kOblique).empty());
    cache.configure(kScope, selected);
    CHECK(project_outlines(cache.snapshot(), kOblique).empty());

    auto new_scope = kScope;
    ++new_scope.world.epoch;
    cache.configure(new_scope, selected);
    cache.observe(new_scope.world, {0, 0, 0}, kDiamond);
    CHECK(project_outlines(cache.snapshot(), kOblique).empty()); // Camera from old world.
    auto new_camera = kOblique;
    new_camera.world = new_scope.world;
    CHECK(project_outlines(cache.snapshot(), new_camera).size() == 12);
    cache.configure(std::nullopt, selected);
    CHECK(project_outlines(cache.snapshot(), new_camera).empty());
}

void test_clip_planes() {
    constexpr double n = 0.5, f = 10.0;
    constexpr CameraFrame perspective{
        kWorld,
        {0.25, 0, 0, 0,
         0, 0.25, 0, 0,
         0, 0, (f + n) / (f - n), -2 * f * n / (f - n),
         0, 0, 1, 0},
        {0, 0, 200, 200},
    };
    const Snapshot near{kWorld, {{{0, 0, 0}, Category::Diamond}}};
    const auto lines = project_outlines(near, perspective);
    // This depth edge crosses z=0.5 at x=1,y=0; both the clipped and original
    // endpoints have deterministic screen coordinates under this fixture.
    const auto close = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    CHECK(std::any_of(lines.begin(), lines.end(), [&](const auto& line) {
        return close(line.x0, 150) && close(line.y0, 100) &&
               close(line.x1, 125) && close(line.y1, 100);
    }));
    CHECK(std::all_of(lines.begin(), lines.end(), [&perspective](const auto& line) {
        return in_view(line, perspective.viewport);
    }));
    const Snapshot behind{kWorld, {{{0, 0, -2}, Category::Diamond}}};
    const Snapshot beyond{kWorld, {{{0, 0, 11}, Category::Diamond}}};
    CHECK(project_outlines(behind, perspective).empty());
    CHECK(project_outlines(beyond, perspective).empty());

    auto side = kOblique;
    side.world_to_clip[3] = 0.875;
    const auto clipped = project_outlines(near, side);
    CHECK(std::any_of(clipped.begin(), clipped.end(), [&](const auto& line) {
        return close(line.x1, side.viewport.left + side.viewport.width);
    }));
    CHECK(std::all_of(clipped.begin(), clipped.end(), [&](const auto& line) { return in_view(line, side.viewport); }));

    constexpr auto max = std::numeric_limits<std::int32_t>::max();
    const Snapshot far_x{kWorld, {{{max, 0, 0}, Category::Diamond}}};
    auto recentered = kOblique;
    recentered.world_to_clip[3] -= 0.25 * max;
    CHECK(project_outlines(far_x, recentered).size() == 12);
}
} // namespace

int main() {
    test_names_and_coordinates();
    test_cache_and_filters();
    test_capacity();
    test_projection_and_budgets();
    test_frame_lifecycle();
    test_clip_planes();
    std::cout << "ore outline core: all host checks passed\n";
}
