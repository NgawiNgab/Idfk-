#include "ore_outline/passive_sections.hpp"

#include <algorithm>
#include <limits>

namespace ore_outline::passive {
namespace {

constexpr auto kMinSection = chunk_coordinate(std::numeric_limits<std::int32_t>::min());
constexpr auto kMaxSection = chunk_coordinate(std::numeric_limits<std::int32_t>::max());

bool valid_section(SectionPos section) noexcept {
    const auto in_range = [](std::int32_t n) { return kMinSection <= n && n <= kMaxSection; };
    return in_range(section.x) && in_range(section.y) && in_range(section.z);
}

std::size_t cell_index(BlockPos pos) noexcept {
    const auto x = static_cast<std::size_t>(local_coordinate(pos.x));
    const auto y = static_cast<std::size_t>(local_coordinate(pos.y));
    const auto z = static_cast<std::size_t>(local_coordinate(pos.z));
    return x + 16 * (y + 16 * z);
}

BlockPos position_at(SectionPos section, std::size_t index) noexcept {
    // Only called for a validated section and 0 <= index < 4096.
    return {16 * section.x + static_cast<std::int32_t>(index % 16),
            16 * section.y + static_cast<std::int32_t>((index / 16) % 16),
            16 * section.z + static_cast<std::int32_t>(index / 256)};
}

std::int64_t distance_squared(SectionPos a, SectionPos b) noexcept {
    // Each delta is bounded by the range of signed block coordinates divided by 16.
    const auto x = std::int64_t{a.x} - b.x;
    const auto y = std::int64_t{a.y} - b.y;
    const auto z = std::int64_t{a.z} - b.z;
    return x * x + y * y + z * z;
}

} // namespace

void BuildSamples::record(BlockPos position, Classification sample) noexcept {
    if (end_ || ambiguous_ || section_containing(position) != ticket_.section) return;
    std::uint8_t value;
    if ((sample.kind == BlockKind::Air || sample.kind == BlockKind::Other) && !sample.category) {
        value = kNonOre;
    } else if (sample.kind == BlockKind::Ore && sample.category &&
               static_cast<std::size_t>(*sample.category) < kCategoryCount) {
        value = static_cast<std::uint8_t>(kFirstOre + static_cast<std::uint8_t>(*sample.category));
    } else {
        ambiguous_ = true;
        return;
    }
    auto& cell = cells_[cell_index(position)];
    if (cell == 0) {
        cell = value;
        ++visited_;
    } else if (cell != value) {
        ambiguous_ = true;
    }
}

void BuildSamples::finish(End end) noexcept {
    if (!end_) end_ = end;
}

bool BuildSamples::complete() const noexcept {
    return end_ == End::Finished && !ambiguous_ && visited_ == cells_.size();
}

void Sections::reset(std::optional<WorldKey> world) {
    entries_.clear();
    world_ = world && world->epoch != 0 ? world : std::nullopt;
}

std::optional<BuildTicket> Sections::begin(SectionPos section) {
    if (!world_ || !valid_section(section)) return std::nullopt;
    if (serial_ == std::numeric_limits<std::uint64_t>::max()) {
        reset(std::nullopt); // Refuse to reuse serials, even across disabled windows.
        return std::nullopt;
    }
    auto it = entries_.find(section);
    if (it == entries_.end()) {
        if (entries_.size() >= kMaxSections) return std::nullopt;
        it = entries_.try_emplace(section).first;
    }
    it->second = Entry{++serial_, false, Coverage::Unobserved, {}};
    return BuildTicket{*world_, section, serial_};
}

Sections::Entry* Sections::current(BuildTicket ticket) {
    if (!world_ || ticket.world != *world_ || ticket.serial == 0) return nullptr;
    const auto it = entries_.find(ticket.section);
    return it != entries_.end() && it->second.serial == ticket.serial ? &it->second : nullptr;
}

bool Sections::adopt(BuildTicket ticket) {
    auto* entry = current(ticket);
    if (!entry) return false;
    entry->adopted = true;
    return true;
}

bool Sections::publish(const BuildSamples& samples) {
    auto* entry = current(samples.ticket_);
    if (!entry || !entry->adopted || entry->coverage != Coverage::Unobserved || !samples.complete()) return false;

    std::vector<OutlineBlock> ores;
    bool truncated = false;
    for (std::size_t i = 0; i < samples.cells_.size(); ++i) {
        if (samples.cells_[i] < BuildSamples::kFirstOre) continue;
        if (ores.size() == kMaxOresPerSection) {
            truncated = true;
            break;
        }
        ores.push_back({position_at(samples.ticket_.section, i),
                        static_cast<Category>(samples.cells_[i] - BuildSamples::kFirstOre)});
    }
    entry->coverage = truncated ? Coverage::Truncated : ores.empty() ? Coverage::ObservedEmpty : Coverage::ObservedOres;
    entry->ores.swap(ores);
    return true;
}

void Sections::forget(SectionPos section) { entries_.erase(section); }

Coverage Sections::coverage(SectionPos section) const {
    const auto it = entries_.find(section);
    return it == entries_.end() ? Coverage::Unobserved : it->second.coverage;
}

std::optional<Snapshot> Sections::snapshot(BlockPos focus, const Settings& filters, std::size_t limit) const {
    if (!world_ || !filters.enabled) return std::nullopt;
    Snapshot result{*world_, {}};
    limit = std::min(limit, kMaxTrackedBlocks);
    if (!limit || filters.categories.none()) return result;

    std::vector<const Entries::value_type*> ordered;
    for (const auto& entry : entries_) {
        if (!entry.second.ores.empty()) ordered.push_back(&entry);
    }
    const auto origin = section_containing(focus);
    std::sort(ordered.begin(), ordered.end(), [origin](const auto* lhs, const auto* rhs) {
        const auto dl = distance_squared(lhs->first, origin);
        const auto dr = distance_squared(rhs->first, origin);
        return dl == dr ? lhs->first < rhs->first : dl < dr;
    });
    for (const auto* entry : ordered) {
        for (const auto& ore : entry->second.ores) {
            if (!filters.allows(ore.category)) continue;
            result.blocks.push_back(ore);
            if (result.blocks.size() == limit) return result;
        }
    }
    return result;
}

} // namespace ore_outline::passive
