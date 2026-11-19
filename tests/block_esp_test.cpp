#include "ore_outline/block_esp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>

using namespace ore_outline;
using namespace ore_outline::esp;

#define CHECK(expr) do { if (!(expr)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": " << #expr << '\n'; \
    std::exit(EXIT_FAILURE); \
} } while (false)

namespace {
constexpr WorldKey kWorld{42, 0};
constexpr Color kCyan{0, 255, 255, 255};
constexpr Color kAmber{255, 172, 77, 255};
constexpr Color kViolet{213, 144, 249, 255};
constexpr CameraFrame kOrtho{kWorld, {0.25, 0, 0, 0, 0, 0.25, 0, 0, 0, 0, 0.25, 0, 0, 0, 0, 1},
    {100, 50, 800, 400}};
constexpr RelativeCamera kRelative{{0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {0.5f, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1.2f, -1, 0, 0, -1.65f, 0}};

void identifiers() {
    const auto defaults = parse_identifiers(kDiamondIds);
    CHECK(defaults && defaults->size == 2);
    CHECK(defaults->match("minecraft:diamond_ore") == 0);
    CHECK(defaults->match("minecraft:deepslate_diamond_ore") == 1);
    CHECK(!defaults->match("minecraft:diamond"));
    CHECK(!defaults->match("minecraft:diamond_ore_extra"));
    CHECK(!defaults->match("minecraft:diamond_ore[foo=true]"));
    CHECK(!defaults->match("example:diamond_ore"));
    CHECK(!defaults->match("minecraft:ancient_debris"));
    const auto custom = parse_identifiers("\t addons.ext:ore/blue, minecraft:ancient_debris,\naddons.ext:ore/blue  ");
    CHECK(custom && custom->size == 2 && custom->match("addons.ext:ore/blue") == 0);
    CHECK(custom->match("minecraft:ancient_debris") == 1);
    CHECK(!custom->match("addons.ext:ore/Blue"));
    CHECK(block_color("minecraft:ancient_debris", 1) == kAmber);
    CHECK(block_color("minecraft:diamond_ore", 0) == kCyan);
    CHECK(block_color("addons.ext:ore/blue", 0).a == 255);
    CHECK(parse_identifiers(" \r\n\t")->size == 0);
    for (auto invalid : {"diamond_ore", "1337", "Minecraft:diamond_ore", "minecraft:*", "minecraft:stone[foo=1]",
                         "minecraft:stone,", ",minecraft:stone", "minecraft:stone,,minecraft:air", "minecraft:bad id",
                         "minecraft:", ":diamond_ore", "a:b:c", "a/b:ore", "a:ore; b:ore", "a:b@10,20,30"}) {
        CHECK(!parse_identifiers(invalid));
    }
    CHECK(!parse_identifiers(std::string_view{"a:b\0c", 5}));
    auto largestId = "a:" + std::string(kMaxIdBytes - 2, 'x');
    CHECK(parse_identifiers(largestId).has_value());
    CHECK(!parse_identifiers(largestId + "x"));
    std::string full;
    for (std::size_t i = 0; i < kMaxIds; ++i) {
        if (i > 0) full += ",";
        full += "example:ore_" + std::to_string(i);
    }
    CHECK(parse_identifiers(full)->size == kMaxIds);
    CHECK(!parse_identifiers(full + ",example:one_more"));
}

void expect_in_view(const Batch& batch, const Viewport& viewport) {
    const auto point = [&](double x, double y) {
        CHECK(std::isfinite(x) && std::isfinite(y));
        CHECK(x >= viewport.left && x <= viewport.left + viewport.width);
        CHECK(y >= viewport.top && y <= viewport.top + viewport.height);
    };
    for (const auto& line : batch.lines) {
        point(line.x0, line.y0);
        point(line.x1, line.y1);
    }
    for (const auto& tri : batch.fills) {
        for (auto p : tri.vertices) point(p.x, p.y);
    }
}

double area(const Triangle& triangle) {
    const auto& [a, b, c] = triangle.vertices;
    return ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) * 0.5;
}

void multiple_styles() {
    constexpr BlockTarget fixtures[]{{{-2, -1, 0}, kCyan}, {{1, 0, 0}, kAmber}, {{0, 1, 0}, kViolet}};
    const auto both = project(kWorld, fixtures, kOrtho, {true, true});
    CHECK(both.lines.size() == 3 && both.fills.size() == 6);
    expect_in_view(both, kOrtho.viewport);
    for (std::size_t i = 0; i < std::size(fixtures); ++i) {
        CHECK(both.lines[i].color == fixtures[i].color);
        CHECK(both.fills[2 * i].color == fixtures[i].color && both.fills[2 * i + 1].color == fixtures[i].color);
        CHECK(both.lines[i].x0 == 500 && std::abs(both.lines[i].y0 - 286) < 1e-9);
        // A 100x50 rectangle with no extra overdraw from the far or side faces.
        CHECK(area(both.fills[2 * i]) + area(both.fills[2 * i + 1]) == 5000);
    }
    CHECK(both.lines[0].x1 == 350 && both.lines[1].x1 == 650);
    const auto lines = project(kWorld, fixtures, kOrtho, {true, false});
    CHECK(lines.lines.size() == 3 && lines.fills.empty());
    const auto fills = project(kWorld, fixtures, kOrtho, {false, true});
    CHECK(fills.lines.empty() && fills.fills.size() == 6);
    const auto hidden = project(kWorld, fixtures, kOrtho, {false, false});
    CHECK(hidden.lines.empty() && hidden.fills.empty());
    const auto otherWorld = project({kWorld.epoch + 1, kWorld.dimension}, fixtures, kOrtho, {});
    CHECK(otherWorld.lines.empty() && otherWorld.fills.empty());
    const auto otherDimension = project({kWorld.epoch, 1}, fixtures, kOrtho, {});
    CHECK(otherDimension.lines.empty() && otherDimension.fills.empty());
}

void clipping_and_limits() {
    auto camera = make_fullscreen_frame(kWorld, kRelative, 800, 400);
    CHECK(camera);
    constexpr BlockTarget blocks[]{{{-1, -1, 2}, kCyan}, {{500, 0, -2}, kAmber}, {{0, 0, -20}, kViolet}};
    const auto outside = project(kWorld, blocks, *camera, {});
    CHECK(outside.lines.empty() && outside.fills.empty()); // Behind, offscreen, beyond far plane.
    constexpr BlockTarget crossNear{{0, 0, -1}, kCyan};
    const auto near = project(kWorld, std::span(&crossNear, 1), *camera, {});
    CHECK(near.lines.empty() && !near.fills.empty()); // Near plane occludes center, not the whole cube.
    expect_in_view(near, camera->viewport);
    CHECK(std::all_of(near.fills.begin(), near.fills.end(), [](const Triangle& t) { return area(t) > 0; }));

    constexpr BlockTarget atSide{{2, 0, -2}, kAmber};
    const auto side = project(kWorld, std::span(&atSide, 1), *camera, {});
    CHECK(!side.fills.empty() && side.lines.size() == 1);
    expect_in_view(side, camera->viewport);
    CHECK(std::any_of(side.fills.begin(), side.fills.end(), [](auto t) {
        return std::any_of(t.vertices.begin(), t.vertices.end(), [](Point p) { return p.x == 800; });
    }));

    std::array<BlockTarget, 100> many;
    many.fill({{0, 0, -2}, kViolet});
    const auto capped = project(kWorld, many, *camera, {});
    CHECK(capped.lines.size() == kMaxTargets);
    CHECK(capped.fills.size() >= kMaxTargets && capped.fills.size() <= kMaxTargets * kMaxTrianglesPerTarget);
    camera->world_to_clip[12] = std::numeric_limits<double>::quiet_NaN();
    const auto invalidMatrix = project(kWorld, many, *camera, {});
    CHECK(invalidMatrix.lines.empty() && invalidMatrix.fills.empty());
    camera = kOrtho;
    camera->viewport.left = -1;
    const auto invalidScreen = project(kWorld, many, *camera, {});
    CHECK(invalidScreen.lines.empty() && invalidScreen.fills.empty());
    constexpr BlockTarget extreme{{std::numeric_limits<std::int32_t>::max(), 0,
        std::numeric_limits<std::int32_t>::min()}, kCyan};
    const auto offscreen = project(kWorld, std::span(&extreme, 1), kOrtho, {});
    CHECK(offscreen.lines.empty() && offscreen.fills.empty());
}

void relative_camera_conversion() {
    auto translated = kRelative;
    translated.origin = {1'000'000, -32, -1'000'000};
    // A 90 degree camera rotation about Y, plus a small walk-bob translation.
    translated.view = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0.1f, 0, 0, 1};
    const auto rotated = make_fullscreen_frame(kWorld, translated, 800, 400);
    CHECK(rotated);
    constexpr BlockTarget target{{1'000'002, -32, -1'000'000}, kCyan};
    const auto batch = project(kWorld, std::span(&target, 1), *rotated, {});
    CHECK(batch.lines.size() == 1 && !batch.fills.empty());
    CHECK(std::abs(batch.lines.front().x1 - 448.0) < 1e-5);
    CHECK(std::abs(batch.lines.front().y1 - 160.0) < 1e-5);
    CHECK(!make_fullscreen_frame(kWorld, translated, 800, 500));
    CHECK(!make_fullscreen_frame(kWorld, translated, 800, 0));
    CHECK(!make_fullscreen_frame({0, 0}, translated, 800, 400));
    translated.projection[11] = 1;
    CHECK(!make_fullscreen_frame(kWorld, translated, 800, 400));
    translated = kRelative;
    translated.view[0] = 0.5f;
    CHECK(!make_fullscreen_frame(kWorld, translated, 800, 400));
    translated = kRelative;
    translated.origin[0] = std::numeric_limits<float>::infinity();
    CHECK(!make_fullscreen_frame(kWorld, translated, 800, 400));
}
}

int main() {
    identifiers();
    multiple_styles();
    clipping_and_limits();
    relative_camera_conversion();
    std::cout << "block ESP tests passed\n";
}
