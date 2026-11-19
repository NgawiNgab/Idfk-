#pragma once

#include "ore_outline/core.hpp"

#include <span>
#include <string>

namespace ore_outline::esp {

constexpr std::size_t kMaxIds = 16;
constexpr std::size_t kMaxIdBytes = 96;
constexpr std::size_t kMaxTargets = 8;
constexpr std::size_t kMaxTrianglesPerTarget = 20;
constexpr std::string_view kDiamondIds = "minecraft:diamond_ore, minecraft:deepslate_diamond_ore";

// Exact type names, not prefixes, runtime IDs or block-state predicates.
struct IdentifierList {
    std::array<std::string, kMaxIds> names;
    std::size_t size{};
    [[nodiscard]] std::optional<std::size_t> match(std::string_view id) const noexcept;
};
[[nodiscard]] bool valid_identifier(std::string_view id) noexcept;
[[nodiscard]] std::optional<IdentifierList> parse_identifiers(std::string_view text);

struct BlockTarget {
    BlockPos position;
    Color color;
};
[[nodiscard]] Color block_color(std::string_view identifier, std::size_t filter_index) noexcept;

struct Point {
    double x, y;
    bool operator==(const Point&) const = default;
};
struct Triangle {
    std::array<Point, 3> vertices;
    Color color;
};
struct DrawOptions {
    bool lines = true;
    bool chams = true;
};
struct Batch {
    std::vector<HudLine> lines;
    std::vector<Triangle> fills;
};

// Paint filled projected cube silhouettes first, then tracers. Depth is used to
// clip the geometry (not for terrain occlusion). The caller orders selected
// targets back to front for fill compositing. No producer pointers are kept.
[[nodiscard]] Batch project(WorldKey world, std::span<const BlockTarget> targets,
                            const CameraFrame& frame, DrawOptions options);

struct RelativeCamera {
    std::array<float, 3> origin;
    std::array<float, 16> view;
    std::array<float, 16> projection; // Column-major, full viewport GL perspective.
};
// Device trial only. Refuse unrecognized matrices and mismatched aspect ratios;
// this is a sanity check, not proof of the producer's render pass or viewport.
[[nodiscard]] std::optional<CameraFrame> make_fullscreen_frame(WorldKey world,
    const RelativeCamera& camera, double width, double height) noexcept;

} // namespace ore_outline::esp
