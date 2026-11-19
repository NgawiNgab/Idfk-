#pragma once

#include <array>
#include <bitset>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

namespace ore_outline {

enum class Category : std::uint8_t {
    Coal, Copper, Iron, Gold, Redstone, Lapis, Diamond, Emerald, Quartz, AncientDebris, Count,
};
constexpr auto kCategoryCount = static_cast<std::size_t>(Category::Count);
constexpr std::size_t kMaxTrackedBlocks = 256;
constexpr std::size_t kDefaultLineLimit = 1024;
constexpr std::size_t kMaxLineLimit = kMaxTrackedBlocks * 12;

struct Color {
    std::uint8_t r, g, b, a;
    bool operator==(const Color&) const = default;
};

struct Settings {
    bool enabled = false;
    std::bitset<kCategoryCount> categories{};
    [[nodiscard]] bool allows(Category category) const noexcept;
};

enum class BlockKind { Unavailable, Air, Other, Ore };
struct Classification {
    BlockKind kind;
    std::optional<Category> category;
};

// nullopt means that no block was read. Do not use an engine fallback string as a read.
[[nodiscard]] Classification classify(std::optional<std::string_view> copied_name) noexcept;
[[nodiscard]] Color color_for(Category category) noexcept;

struct BlockPos {
    std::int32_t x, y, z;
    auto operator<=>(const BlockPos&) const = default;
};

// Integer floor division, including for positions west/south of the origin.
[[nodiscard]] constexpr std::int32_t chunk_coordinate(std::int32_t block) noexcept {
    return block / 16 - (block % 16 < 0 ? 1 : 0);
}
[[nodiscard]] constexpr std::int32_t local_coordinate(std::int32_t block) noexcept {
    const auto remainder = block % 16;
    return remainder < 0 ? remainder + 16 : remainder;
}

// Logical 16×16×16 section coordinates, not an engine/ABI type.
struct SectionPos {
    std::int32_t x, y, z;
    auto operator<=>(const SectionPos&) const = default;
};

[[nodiscard]] constexpr SectionPos section_containing(BlockPos position) noexcept {
    return {chunk_coordinate(position.x), chunk_coordinate(position.y), chunk_coordinate(position.z)};
}

struct WorldKey {
    std::uint64_t epoch; // Nonzero; change on world replacement, even for the same dimension.
    std::int32_t dimension;
    bool operator==(const WorldKey&) const = default;
};
struct ScanScope {
    WorldKey world;
    BlockPos min;
    BlockPos max; // Inclusive, in block coordinates.
};
struct OutlineBlock {
    BlockPos position;
    Category category;
};
struct Snapshot {
    WorldKey world;
    std::vector<OutlineBlock> blocks;
};

// One owner thread. Observations must be current and bounded by the caller's scan budget.
// Revalidate tracked positions, reporting unavailable ones as nullopt; entries do not expire
// on their own. No engine reference or string view survives observe(). Publish value snapshots
// with caller-provided synchronization and discard them on invalid context or settings changes.
class OutlineCache {
public:
    void configure(std::optional<ScanScope> scope, Settings settings);
    void observe(WorldKey world, BlockPos position, std::optional<std::string_view> copied_name);
    [[nodiscard]] std::optional<Snapshot> snapshot() const;

private:
    std::optional<ScanScope> scope_;
    Settings settings_;
    std::map<BlockPos, Category> blocks_;
};

struct Viewport {
    double left, top, width, height; // A rectangle already mapped into launcher HUD pixels.
};
struct CameraFrame {
    WorldKey world;
    // Portable convention only: row-major matrix times [worldX, worldY, worldZ, 1].
    // Clip coordinates have -w <= x,y,z <= w. A device adapter must normalize to this.
    std::array<double, 16> world_to_clip;
    Viewport viewport;
};
struct HudLine {
    double x0, y0, x1, y1;
    Color color;
};

// No implicit use of a previous frame. An absent snapshot/camera or world mismatch gives
// an empty batch; a HUD bridge must publish that empty batch to clear its previous lines.
[[nodiscard]] std::vector<HudLine> project_outlines(
    const std::optional<Snapshot>& snapshot,
    const std::optional<CameraFrame>& camera,
    std::size_t line_limit = kDefaultLineLimit);

} // namespace ore_outline
