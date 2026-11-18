#include "ore_outline/build_id.hpp"

#include <link.h>

#include <algorithm>
#include <bit>
#include <limits>

namespace ore_outline::build_id {
namespace {

constexpr std::string_view kGameLibrary = "libminecraftpe.so";
constexpr std::size_t kMaxProgramHeaders = 1024;
static_assert(std::endian::native == std::endian::little);

std::uint32_t read_u32le(std::span<const std::uint8_t> bytes) noexcept {
    return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8) |
           (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
}

std::size_t align_up(std::size_t value, std::size_t alignment) noexcept {
    // Callers bound value to kMaxNoteBytes before aligning.
    return (value + alignment - 1) & ~(alignment - 1);
}

bool within_readable_load(const ElfW(Phdr)& note,
                          std::span<const ElfW(Phdr)> headers) noexcept {
    for (const auto& load : headers) {
        if (load.p_type != PT_LOAD || !(load.p_flags & PF_R) || load.p_filesz > load.p_memsz ||
            note.p_vaddr < load.p_vaddr || note.p_offset < load.p_offset) continue;
        const auto offset = note.p_vaddr - load.p_vaddr;
        if (offset == note.p_offset - load.p_offset && offset <= load.p_filesz &&
            note.p_filesz <= load.p_filesz - offset) return true;
    }
    return false;
}

std::optional<BuildId> read_module(const dl_phdr_info& module) noexcept {
    if (!module.dlpi_phdr || module.dlpi_phnum == 0 || module.dlpi_phnum > kMaxProgramHeaders) {
        return std::nullopt;
    }
    const std::span headers(module.dlpi_phdr, module.dlpi_phnum);
    std::optional<BuildId> id;
    constexpr auto max_address = std::numeric_limits<std::uintptr_t>::max();
    for (const auto& note : headers) {
        if (note.p_type != PT_NOTE || note.p_filesz == 0) continue;
        if (note.p_filesz > kMaxNoteBytes || note.p_filesz > note.p_memsz ||
            note.p_vaddr > max_address - module.dlpi_addr || !within_readable_load(note, headers)) {
            return std::nullopt;
        }
        const auto address = module.dlpi_addr + note.p_vaddr;
        if (address == 0 || note.p_filesz > max_address - address) return std::nullopt;

        const auto alignment = note.p_align < 4 ? 4 : note.p_align;
        if (alignment != 4 && alignment != 8) return std::nullopt;
        const auto notes = std::span(reinterpret_cast<const std::uint8_t*>(address),
                                     static_cast<std::size_t>(note.p_filesz));
        const auto parsed = scan_notes(notes, static_cast<std::size_t>(alignment));
        if (!parsed.well_formed || (id && parsed.id && *id != *parsed.id)) return std::nullopt;
        if (parsed.id) id = parsed.id;
    }
    return id;
}

int visit_module(dl_phdr_info* info, std::size_t size, void* context) noexcept {
    if (size < offsetof(dl_phdr_info, dlpi_phnum) + sizeof(info->dlpi_phnum) || !info->dlpi_name) {
        return 0;
    }
    const std::string_view path(info->dlpi_name);
    const auto slash = path.find_last_of('/');
    const auto name = slash == std::string_view::npos ? path : path.substr(slash + 1);
    if (name != kGameLibrary) return 0;

    auto& result = *static_cast<Result*>(context);
    if (result.status != Status::NotLoaded) {
        result = {Status::MultipleModules, {}};
        return 1;
    }
    const auto id = read_module(*info);
    result = id ? Result{Status::Found, *id} : Result{Status::Unavailable, {}};
    return 0;
}

} // namespace

std::string BuildId::hex() const {
    constexpr char digits[] = "0123456789abcdef";
    if (size > bytes.size()) return {};
    std::string out;
    out.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        out.push_back(digits[bytes[i] >> 4]);
        out.push_back(digits[bytes[i] & 0xf]);
    }
    return out;
}

NoteScan scan_notes(std::span<const std::uint8_t> bytes, std::size_t alignment) noexcept {
    constexpr std::size_t header_size = 12; // namesz, descsz, type (uint32_t each)
    if (bytes.size() > kMaxNoteBytes || (alignment != 4 && alignment != 8)) return {false, {}};

    std::optional<BuildId> found;
    while (!bytes.empty()) {
        if (bytes.size() < header_size) return {false, {}};
        const auto name_size = read_u32le(bytes);
        const auto desc_size = read_u32le(bytes.subspan(4));
        const auto type = read_u32le(bytes.subspan(8));
        if (name_size > bytes.size() - header_size) return {false, {}};
        const auto desc = align_up(header_size + name_size, alignment);
        if (desc > bytes.size() || desc_size > bytes.size() - desc) return {false, {}};
        const auto next = align_up(desc + desc_size, alignment);
        if (next > bytes.size()) return {false, {}};

        if (type == NT_GNU_BUILD_ID && name_size == 4 && bytes[12] == 'G' &&
            bytes[13] == 'N' && bytes[14] == 'U' && bytes[15] == '\0') {
            BuildId id;
            if (desc_size == 0 || desc_size > id.bytes.size()) return {false, {}};
            id.size = desc_size;
            std::copy_n(bytes.begin() + desc, desc_size, id.bytes.begin());
            if (found && *found != id) return {false, {}};
            found = id;
        }
        bytes = bytes.subspan(next);
    }
    return {true, found};
}

Result query_loaded_game() noexcept {
    Result result;
    dl_iterate_phdr(visit_module, &result);
    return result;
}

} // namespace ore_outline::build_id
