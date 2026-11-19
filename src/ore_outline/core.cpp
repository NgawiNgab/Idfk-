#include "ore_outline/core.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace ore_outline {
namespace {

struct Name {
    std::string_view id;
    Category category;
};
constexpr std::array kOreNames{
    Name{"minecraft:coal_ore", Category::Coal},
    Name{"minecraft:deepslate_coal_ore", Category::Coal},
    Name{"minecraft:copper_ore", Category::Copper},
    Name{"minecraft:deepslate_copper_ore", Category::Copper},
    Name{"minecraft:iron_ore", Category::Iron},
    Name{"minecraft:deepslate_iron_ore", Category::Iron},
    Name{"minecraft:gold_ore", Category::Gold},
    Name{"minecraft:deepslate_gold_ore", Category::Gold},
    Name{"minecraft:nether_gold_ore", Category::Gold},
    Name{"minecraft:redstone_ore", Category::Redstone},
    Name{"minecraft:deepslate_redstone_ore", Category::Redstone},
    Name{"minecraft:lit_redstone_ore", Category::Redstone},
    Name{"minecraft:lit_deepslate_redstone_ore", Category::Redstone},
    Name{"minecraft:lapis_ore", Category::Lapis},
    Name{"minecraft:deepslate_lapis_ore", Category::Lapis},
    Name{"minecraft:diamond_ore", Category::Diamond},
    Name{"minecraft:deepslate_diamond_ore", Category::Diamond},
    Name{"minecraft:emerald_ore", Category::Emerald},
    Name{"minecraft:deepslate_emerald_ore", Category::Emerald},
    Name{"minecraft:quartz_ore", Category::Quartz},
    Name{"minecraft:ancient_debris", Category::AncientDebris},
};
// RGB values from the Apollon ore palette, stored without ImGui's packing convention.
constexpr std::array kColors{
    Color{0, 0, 0, 255},       // Coal
    Color{184, 115, 51, 255},  // Copper
    Color{255, 165, 0, 255},   // Iron
    Color{255, 255, 0, 255},   // Gold
    Color{255, 0, 0, 255},     // Redstone
    Color{0, 0, 255, 255},     // Lapis
    Color{0, 255, 255, 255},   // Diamond
    Color{0, 255, 0, 255},     // Emerald
    Color{255, 255, 255, 255}, // Quartz
    Color{83, 83, 83, 255},    // Ancient debris
};
static_assert(kColors.size() == kCategoryCount);

bool contains(const ScanScope& scope, BlockPos pos) noexcept {
    return scope.min.x <= pos.x && pos.x <= scope.max.x &&
           scope.min.y <= pos.y && pos.y <= scope.max.y &&
           scope.min.z <= pos.z && pos.z <= scope.max.z;
}

using Vertex = std::array<std::int64_t, 3>;
using Edge = std::pair<Vertex, Vertex>;
using ClipPoint = std::array<double, 4>;
constexpr double kMinimumW = 1e-8;

bool valid_viewport(const Viewport& v) noexcept {
    constexpr auto max_hud_coord = std::numeric_limits<float>::max();
    return std::isfinite(v.left) && std::isfinite(v.top) &&
           std::isfinite(v.width) && std::isfinite(v.height) &&
           v.left >= 0 && v.top >= 0 && v.width > 0 && v.height > 0 &&
           v.left + v.width <= max_hud_coord && v.top + v.height <= max_hud_coord;
}

ClipPoint world_to_clip(const std::array<double, 16>& m, const Vertex& v) noexcept {
    ClipPoint p{};
    for (std::size_t i = 0; i < p.size(); ++i) {
        const auto row = i * 4;
        p[i] = m[row] * static_cast<double>(v[0]) +
               m[row + 1] * static_cast<double>(v[1]) +
               m[row + 2] * static_cast<double>(v[2]) + m[row + 3];
    }
    return p;
}

std::array<double, 7> clip_distances(const ClipPoint& p) noexcept {
    const auto [x, y, z, w] = p;
    return {w - kMinimumW, w + x, w - x, w + y, w - y, w + z, w - z};
}

// Intersect the parameter interval of the edge with each homogeneous clip plane.
// In particular, clip depth before dividing by w so a near-plane crossing remains visible.
bool clip_segment(ClipPoint& a, ClipPoint& b) noexcept {
    const auto da = clip_distances(a);
    const auto db = clip_distances(b);
    double begin = 0;
    double end = 1;
    for (std::size_t i = 0; i < da.size(); ++i) {
        const auto fa = da[i];
        const auto fb = db[i];
        if (!std::isfinite(fa) || !std::isfinite(fb) || (fa < 0 && fb < 0)) return false;
        if ((fa < 0) == (fb < 0)) continue;

        const auto delta = fa - fb;
        if (!std::isfinite(delta)) return false;
        const auto t = fa / delta;
        if (fa < 0) begin = std::max(begin, t);
        else end = std::min(end, t);
        if (begin >= end) return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto from = a[i];
        const auto to = b[i];
        a[i] = std::lerp(from, to, begin);
        b[i] = std::lerp(from, to, end);
    }
    return a[3] > 0 && b[3] > 0;
}

std::array<double, 2> to_hud(const ClipPoint& p, const Viewport& v) noexcept {
    const auto nx = std::clamp(p[0] / p[3], -1.0, 1.0);
    const auto ny = std::clamp(p[1] / p[3], -1.0, 1.0);
    return {v.left + (nx + 1) * 0.5 * v.width, v.top + (1 - ny) * 0.5 * v.height};
}

} // namespace

bool Settings::allows(Category category) const noexcept {
    const auto index = static_cast<std::size_t>(category);
    return enabled && index < kCategoryCount && categories[index];
}

Classification classify(std::optional<std::string_view> copied_name) noexcept {
    if (!copied_name) return {BlockKind::Unavailable, std::nullopt};
    if (*copied_name == "minecraft:air") return {BlockKind::Air, std::nullopt};
    for (const auto& [id, category] : kOreNames) {
        if (*copied_name == id) return {BlockKind::Ore, category};
    }
    return {BlockKind::Other, std::nullopt};
}

Color color_for(Category category) noexcept {
    const auto index = static_cast<std::size_t>(category);
    return index < kColors.size() ? kColors[index] : Color{0, 0, 0, 0};
}

void OutlineCache::configure(std::optional<ScanScope> scope, Settings settings) {
    if (scope && (scope->world.epoch == 0 || !contains(*scope, scope->min))) scope.reset();
    if (!scope || !scope_ || scope->world != scope_->world || !settings.enabled) blocks_.clear();

    scope_ = scope;
    settings_ = settings;
    if (scope_) {
        std::erase_if(blocks_, [&](const auto& entry) {
            return !contains(*scope_, entry.first) || !settings_.allows(entry.second);
        });
    }
}

void OutlineCache::observe(WorldKey world, BlockPos position, std::optional<std::string_view> copied_name) {
    if (!settings_.enabled || !scope_ || world != scope_->world || !contains(*scope_, position)) return;
    const auto sample = classify(copied_name);
    if (!sample.category || !settings_.allows(*sample.category)) {
        blocks_.erase(position);
    } else if (auto it = blocks_.find(position); it != blocks_.end()) {
        it->second = *sample.category;
    } else if (blocks_.size() < kMaxTrackedBlocks) {
        blocks_.emplace(position, *sample.category);
    }
}

std::optional<Snapshot> OutlineCache::snapshot() const {
    if (!settings_.enabled || !scope_) return std::nullopt;
    Snapshot out{scope_->world, {}};
    out.blocks.reserve(blocks_.size());
    for (const auto& [pos, category] : blocks_) out.blocks.push_back({pos, category});
    return out;
}

std::vector<HudLine> project_outlines(const std::optional<Snapshot>& snapshot,
                                      const std::optional<CameraFrame>& camera,
                                      std::size_t line_limit) {
    if (!snapshot || !camera || snapshot->world.epoch == 0 || snapshot->world != camera->world ||
        !valid_viewport(camera->viewport) || !std::all_of(camera->world_to_clip.begin(),
            camera->world_to_clip.end(), [](double v) { return std::isfinite(v); })) return {};

    const auto cap = std::min(line_limit, kMaxLineLimit);
    const auto block_count = std::min(snapshot->blocks.size(), kMaxTrackedBlocks);
    std::vector<HudLine> lines;
    lines.reserve(std::min(cap, block_count * 12));
    std::set<Edge> drawn;
    for (std::size_t i = 0; i < block_count && lines.size() < cap; ++i) {
        const auto& [pos, category] = snapshot->blocks[i];
        const auto color = color_for(category);
        if (!color.a) continue;

        std::array<Vertex, 8> vertices;
        std::array<ClipPoint, 8> points;
        for (std::size_t j = 0; j < vertices.size(); ++j) {
            // Widen before adding 1 so the top side of INT32_MAX is representable.
            vertices[j] = {static_cast<std::int64_t>(pos.x) + static_cast<std::int64_t>(j & 1),
                           static_cast<std::int64_t>(pos.y) + static_cast<std::int64_t>((j >> 1) & 1),
                           static_cast<std::int64_t>(pos.z) + static_cast<std::int64_t>((j >> 2) & 1)};
            points[j] = world_to_clip(camera->world_to_clip, vertices[j]);
        }
        for (std::size_t a = 0; a < vertices.size(); ++a) {
            for (std::size_t axis = 1; axis <= 4; axis <<= 1) {
                if ((a & axis) != 0) continue;
                const auto b = a | axis;
                const Edge edge{vertices[a], vertices[b]};
                if (drawn.contains(edge)) continue;

                auto start = points[a];
                auto end = points[b];
                if (!clip_segment(start, end)) continue;
                const auto p0 = to_hud(start, camera->viewport);
                const auto p1 = to_hud(end, camera->viewport);
                if (p0 == p1) continue;
                drawn.insert(edge);
                lines.push_back({p0[0], p0[1], p1[0], p1[1], color});
                if (lines.size() == cap) return lines;
            }
        }
    }
    return lines;
}

} // namespace ore_outline
