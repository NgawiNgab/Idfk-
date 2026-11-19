#include "ore_outline/chunk_scan.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace ore_outline {
namespace {

constexpr int kSide = 16;
constexpr unsigned kPlane = kSide * kSide;
constexpr auto kFirstSection = chunk_coordinate(std::numeric_limits<std::int32_t>::min());
constexpr auto kLastSection = chunk_coordinate(std::numeric_limits<std::int32_t>::max());

std::int32_t first_block(std::int32_t section) noexcept { return section * kSide; }
std::int32_t last_block(std::int32_t section) noexcept { return first_block(section) + (kSide - 1); }

std::int32_t lower_bound(std::int32_t center, unsigned range) noexcept {
    return first_block(std::max(kFirstSection, center - static_cast<std::int32_t>(range)));
}

std::int32_t upper_bound(std::int32_t center, unsigned range) noexcept {
    return last_block(std::min(kLastSection, center + static_cast<std::int32_t>(range)));
}

bool in_window(SectionPos pos, SectionPos lo, SectionPos hi) noexcept {
    return lo.x <= pos.x && pos.x <= hi.x && lo.y <= pos.y && pos.y <= hi.y && lo.z <= pos.z && pos.z <= hi.z;
}

struct Slice {
    std::int32_t x, y, z; // First valid block, clipped vertically to the build bounds.
    unsigned count;

    [[nodiscard]] BlockPos at(unsigned i) const noexcept {
        return {x + static_cast<std::int32_t>(i % kSide), y + static_cast<std::int32_t>(i / kPlane),
                z + static_cast<std::int32_t>((i / kSide) % kSide)};
    }
};

Slice slice(SectionPos section, const ScanScope& scope) noexcept {
    const auto y = std::max(first_block(section.y), scope.min.y);
    const auto top = std::min(last_block(section.y), scope.max.y);
    return {first_block(section.x), y, first_block(section.z), static_cast<unsigned>(top - y + 1) * kPlane};
}

std::int64_t section_distance(SectionPos a, SectionPos b) noexcept {
    // Only nearby sections are passed here, so the squared sum is bounded.
    const auto dx = static_cast<std::int64_t>(a.x) - b.x;
    const auto dy = static_cast<std::int64_t>(a.y) - b.y;
    const auto dz = static_cast<std::int64_t>(a.z) - b.z;
    return dx * dx + dy * dy + dz * dz;
}

} // namespace

ScanStep ChunkScanner::step(std::optional<ClientScanFrame> frame, const ScanOptions& options,
                            ExistingSectionReader& reader) {
    if (!frame || frame->world.epoch == 0 || frame->min_y > frame->max_y ||
        frame->ready_sections.size() > kMaxReadySectionFeed ||
        !options.filters.enabled || options.filters.categories.none()) {
        clear();
        return {};
    }

    const auto world = frame->world;
    const auto center = section_containing(frame->player);
    const auto horizontal = std::min(options.horizontal_sections, kMaxHorizontalSectionRadius);
    const auto vertical = std::min(options.vertical_sections, kMaxVerticalSectionRadius);
    const ScanScope scope{world,
                          {lower_bound(center.x, horizontal), std::max(lower_bound(center.y, vertical), frame->min_y),
                           lower_bound(center.z, horizontal)},
                          {upper_bound(center.x, horizontal), std::min(upper_bound(center.y, vertical), frame->max_y),
                           upper_bound(center.z, horizontal)}};
    if (scope.min.y > scope.max.y) {
        clear();
        return {};
    }
    const auto lo = section_containing(scope.min);
    const auto hi = section_containing(scope.max);

    // Validate the feed before calling the reader. Any ambiguous in-range section
    // invalidates this frame, even if the conflicting stamps are equal or zero.
    Sections next;
    for (const auto section : frame->ready_sections) {
        if (!in_window(section.position, lo, hi)) continue;
        if (!next.emplace(section.position, Progress{section.revision}).second) {
            clear();
            return {};
        }
    }

    const Context context{world, frame->min_y, frame->max_y};
    if (context_ != context) clear();
    context_ = context;

    ScanStep stats;
    const auto check = [&](ReadySection section) {
        ++stats.readiness_checks;
        return reader.still_readable(world, section);
    };
    std::erase_if(next, [&](const auto& entry) {
        const auto& [position, progress] = entry;
        // Recheck completed sections, too, so a stale renderer feed is not sufficient
        // to keep an old outline when the reader can no longer verify the section.
        return progress.revision == 0 || !check({position, progress.revision});
    });

    if (filters_.categories == options.filters.categories) {
        for (auto& [position, progress] : next) {
            const auto old = sections_.find(position);
            if (old != sections_.end() && old->second.revision == progress.revision) {
                progress.cursor = old->second.cursor;
            }
        }
    }
    filters_ = options.filters;
    cache_.configure(scope, filters_);
    discard_stale(next);
    sections_ = std::move(next);

    std::vector<SectionPos> pending;
    pending.reserve(sections_.size());
    for (const auto& [position, progress] : sections_) {
        if (progress.cursor < slice(position, scope).count) pending.push_back(position);
    }
    const auto limit = std::min(options.reads_per_update, kMaxReadsPerUpdate);
    if (limit != 0) {
        std::sort(pending.begin(), pending.end(), [&](SectionPos a, SectionPos b) {
            const auto da = section_distance(a, center);
            const auto db = section_distance(b, center);
            return da != db ? da < db : a < b;
        });
    }

    bool failed = false;
    for (const auto position : pending) {
        if (stats.block_reads == limit) break;
        auto& progress = sections_.at(position);
        const ReadySection section{position, progress.revision};
        const auto blocks = slice(position, scope);
        for (; progress.cursor < blocks.count && stats.block_reads < limit; ++progress.cursor) {
            if (check(section)) {
                const auto block = blocks.at(progress.cursor);
                ++stats.block_reads;
                const auto value = reader.read_existing(world, section, block);
                if (value && !value->empty()) {
                    cache_.observe(world, block, std::string_view(*value));
                    continue;
                }
            }
            // The section may have become invalid during this update. Don't try another
            // position in it or show an earlier result from it this update.
            sections_.erase(position);
            failed = true;
            break;
        }
    }
    if (failed) discard_stale(sections_);

    stats.ready_sections = sections_.size();
    for (const auto& [position, progress] : sections_) {
        if (progress.cursor < slice(position, scope).count) ++stats.pending_sections;
    }
    return stats;
}

void ChunkScanner::discard_stale(const Sections& next) {
    const auto snapshot = cache_.snapshot();
    if (!snapshot) return;
    for (const auto& block : snapshot->blocks) {
        const auto position = section_containing(block.position);
        const auto fresh = next.find(position);
        const auto old = sections_.find(position);
        if (fresh == next.end() || old == sections_.end() || fresh->second.revision != old->second.revision) {
            cache_.observe(snapshot->world, block.position, std::nullopt);
        }
    }
}

void ChunkScanner::clear() {
    context_.reset();
    filters_ = {};
    sections_.clear();
    cache_.configure(std::nullopt, {});
}

std::optional<Snapshot> ChunkScanner::snapshot() const {
    return cache_.snapshot();
}

} // namespace ore_outline
