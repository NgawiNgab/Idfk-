#include "ore_outline/terrain_capture.hpp"

#include <algorithm>
#include <limits>
#include <span>
#include <utility>

namespace ore_outline::terrain_capture {
namespace {

std::uint64_t section_hash(SectionPos section) noexcept {
    const auto x = static_cast<std::uint32_t>(section.x);
    const auto y = static_cast<std::uint32_t>(section.y);
    const auto z = static_cast<std::uint32_t>(section.z);
    auto hash = (x * std::uint64_t{0x9e3779b97f4a7c15}) ^ (y * std::uint64_t{0xd6e8feb86659fd93}) ^
                (z * std::uint64_t{0xa0761d6478bd642f});
    hash ^= hash >> 32;
    hash *= 0xd6e8feb86659fd93;
    return hash ^ (hash >> 32);
}

using Column = std::pair<std::int32_t, std::int32_t>;

} // namespace

bool Capture::arm() {
    std::lock_guard lock(mutex_);
    const auto before = state_.load(std::memory_order_relaxed);
    const auto serial = before & ~kFlags;
    if ((before & kArmed) || serial > std::numeric_limits<Token>::max() - (kFlags + 1)) return false;
    entries_.fill(Entry{});
    section_count_ = 0;
    limited_ = false;
    state_.store((serial + (kFlags + 1)) | kArmed, std::memory_order_release);
    return true;
}

void Capture::flag(Token window, Token bit, Token before) noexcept {
    // Only two flags can change within this window, so strong CAS can't spin on traffic.
    while ((before & kWindow) == window && !(before & bit) &&
           !state_.compare_exchange_strong(before, before | bit, std::memory_order_relaxed)) {}
}

void Capture::record_sample(Token window, BlockPos position, Match match) noexcept {
    const auto state = state_.load(std::memory_order_relaxed);
    if ((state & kWindow) != window) return;
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock) {
        if (!(state & kContention)) flag(window, kContention, state);
        return;
    }
    // A call for a stopped window might have been delayed until after a new arm().
    if ((state_.load(std::memory_order_relaxed) & kWindow) != window) return;

    const auto section = section_containing(position);
    const auto bucket = static_cast<std::size_t>(section_hash(section) % kBucketCount);
    for (std::size_t i = 0; i < kMaxProbes; ++i) {
        auto& entry = entries_[(bucket + i) % kBucketCount];
        if (!entry.used) {
            if (section_count_ == kMaxSampledSections) break;
            entry.section = section;
            entry.used = true;
            ++section_count_;
        } else if (entry.section != section) {
            continue;
        }
        if (match != Match::None && entry.first_hit.match == Match::None) {
            entry.first_hit = {position, match};
        }
        return;
    }
    limited_ = true;
}

Report Capture::report() const {
    std::lock_guard lock(mutex_);
    const auto state = state_.load(std::memory_order_acquire);
    Report result;
    result.builder_seen = (state & kVisited) != 0;
    result.contention = (state & kContention) != 0;
    result.limited = limited_;
    result.sections = section_count_;

    std::array<Column, kMaxSampledSections> columns;
    std::array<Column, kMaxExamples> example_columns;
    auto end = columns.begin();
    for (const auto& entry : entries_) {
        if (!entry.used) continue;
        const Column column{entry.section.x, entry.section.z};
        *end++ = column;
        if (entry.first_hit.match == Match::None) continue;
        ++result.matched_sections;
        // Prefer different columns so one large vein doesn't fill the HUD examples.
        if (result.example_count < kMaxExamples) {
            const auto shown = std::span(example_columns).first(result.example_count);
            if (std::find(shown.begin(), shown.end(), column) == shown.end()) {
                example_columns[result.example_count] = column;
                result.examples[result.example_count++] = entry.first_hit;
            }
        }
    }
    std::sort(columns.begin(), end);
    result.columns = static_cast<std::size_t>(std::unique(columns.begin(), end) - columns.begin());

    if (result.example_count < kMaxExamples && result.example_count < result.matched_sections) {
        for (const auto& entry : entries_) {
            if (entry.first_hit.match == Match::None) continue;
            const auto shown = std::span(result.examples).first(result.example_count);
            if (std::none_of(shown.begin(), shown.end(), [&](const Hit& hit) {
                    return hit.position == entry.first_hit.position;
                })) {
                result.examples[result.example_count++] = entry.first_hit;
                if (result.example_count == kMaxExamples) break;
            }
        }
    }
    return result;
}

} // namespace ore_outline::terrain_capture
