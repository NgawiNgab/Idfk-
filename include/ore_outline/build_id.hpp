#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace ore_outline::build_id {

// Fingerprint of the separate Drive sample, not a permission to call Minecraft code.
inline constexpr std::string_view kDriveBuildId = "c60fa32908019eecae39ebfe7bac5755dc35cde2";
inline constexpr std::size_t kMaxNoteBytes = 64 * 1024;

struct BuildId {
    std::array<std::uint8_t, 64> bytes{};
    std::size_t size = 0;
    bool operator==(const BuildId&) const = default;
    [[nodiscard]] std::string hex() const;
};

struct NoteScan {
    bool well_formed;
    std::optional<BuildId> id;
};

// Inspect a bounded, little-endian PT_NOTE payload. No pointers escape the scan.
[[nodiscard]] NoteScan scan_notes(std::span<const std::uint8_t> bytes,
                                  std::size_t alignment) noexcept;

enum class Status { NotLoaded, MultipleModules, Unavailable, Found };
struct Result {
    Status status = Status::NotLoaded;
    BuildId id{};
};

// Inspect only the linker's existing mappings in this process. Does not load a
// library or access a game object, function, cache file, or world.
[[nodiscard]] Result query_loaded_game() noexcept;

struct CodeSpan {
    std::uintptr_t elf_address;
    std::span<const std::uint8_t> expected_bytes;
};
struct RelroSlot {
    std::uintptr_t elf_address;
    std::uintptr_t target_elf_address;
};
enum class CodeStatus {
    NotLoaded, MultipleModules, Unavailable, DifferentBuild, DifferentCode, DifferentSlot, Match
};
struct CodeMatch {
    CodeStatus status = CodeStatus::NotLoaded;
    std::uintptr_t load_bias = 0; // Valid only on Match; does not pin the module.
};

// Require a unique module, a matching note, and exact bytes in file-backed R-X
// ranges of its existing mapping. If specified, also check one relocated pointer
// in a file-backed non-executable GNU_RELRO range. Does not write or pin memory.
[[nodiscard]] CodeMatch verify_loaded_game_code(std::string_view expected_id,
                                                std::span<const CodeSpan> spans,
                                                std::optional<RelroSlot> slot = {}) noexcept;

} // namespace ore_outline::build_id
