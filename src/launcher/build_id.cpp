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

bool is_game_module(const dl_phdr_info& info, std::size_t size) noexcept {
    if (size < offsetof(dl_phdr_info, dlpi_phnum) + sizeof(info.dlpi_phnum) || !info.dlpi_name) {
        return false;
    }
    const std::string_view path(info.dlpi_name);
    const auto slash = path.find_last_of('/');
    const auto name = slash == std::string_view::npos ? path : path.substr(slash + 1);
    return name == kGameLibrary;
}

int visit_module(dl_phdr_info* info, std::size_t size, void* context) noexcept {
    if (!is_game_module(*info, size)) return 0;
    auto& result = *static_cast<Result*>(context);
    if (result.status != Status::NotLoaded) {
        result = {Status::MultipleModules, {}};
        return 1;
    }
    const auto id = read_module(*info);
    result = id ? Result{Status::Found, *id} : Result{Status::Unavailable, {}};
    return 0;
}

bool contains_file_span(const ElfW(Phdr)& header, std::uintptr_t address,
                        std::size_t size) noexcept {
    if (!size || header.p_filesz > header.p_memsz || address < header.p_vaddr) return false;
    const auto offset = address - header.p_vaddr;
    return offset < header.p_filesz && size <= header.p_filesz - offset;
}

std::uintptr_t loaded_address(const dl_phdr_info& info, std::uintptr_t relative,
                              std::size_t size) noexcept {
    constexpr auto max = std::numeric_limits<std::uintptr_t>::max();
    if (relative > max - info.dlpi_addr || relative + info.dlpi_addr == 0 ||
        size > max - (relative + info.dlpi_addr)) return 0;
    return relative + info.dlpi_addr;
}

bool id_matches(const BuildId& id, std::string_view text) noexcept {
    constexpr char digits[] = "0123456789abcdef";
    if (text.size() != id.size * 2) return false;
    for (std::size_t i = 0; i < id.size; ++i) {
        if (text[2 * i] != digits[id.bytes[i] >> 4] || text[2 * i + 1] != digits[id.bytes[i] & 0xf]) {
            return false;
        }
    }
    return true;
}

bool matches_slot(const dl_phdr_info& info, std::span<const ElfW(Phdr)> headers,
                 RelroSlot slot) noexcept {
    constexpr auto size = sizeof(std::uintptr_t);
    const auto address = loaded_address(info, slot.elf_address, size);
    const auto target = loaded_address(info, slot.target_elf_address, 1);
    if (!address || address % alignof(std::uintptr_t) != 0 || !target) return false;
    bool in_relro = false, in_data = false, target_in_text = false;
    for (const auto& header : headers) {
        const auto flags = header.p_flags & (PF_R | PF_W | PF_X);
        const bool contains_slot = contains_file_span(header, slot.elf_address, size);
        in_relro |= header.p_type == PT_GNU_RELRO && contains_slot;
        in_data |= header.p_type == PT_LOAD && flags == (PF_R | PF_W) && contains_slot;
        target_in_text |= header.p_type == PT_LOAD && flags == (PF_R | PF_X) &&
                          contains_file_span(header, slot.target_elf_address, 1);
    }
    if (!in_relro || !in_data || !target_in_text) return false;
    return __atomic_load_n(reinterpret_cast<const std::uintptr_t*>(address), __ATOMIC_ACQUIRE) == target;
}

struct CodeQuery {
    std::string_view expected_id;
    std::span<const CodeSpan> spans;
    std::optional<RelroSlot> slot;
    CodeMatch result;
};

int visit_code(dl_phdr_info* info, std::size_t size, void* context) noexcept {
    if (!is_game_module(*info, size)) return 0;
    auto& query = *static_cast<CodeQuery*>(context);
    if (query.result.status != CodeStatus::NotLoaded) {
        query.result = {CodeStatus::MultipleModules, 0};
        return 1;
    }
    const auto id = read_module(*info);
    if (!id) {
        query.result.status = CodeStatus::Unavailable;
        return 0;
    }
    if (!id_matches(*id, query.expected_id)) {
        query.result.status = CodeStatus::DifferentBuild;
        return 0;
    }
    const std::span headers(info->dlpi_phdr, info->dlpi_phnum);
    query.result.status = CodeStatus::DifferentCode;
    if (query.spans.empty() || query.spans.size() > 16) return 0;
    for (const auto& span : query.spans) {
        const auto bytes = span.expected_bytes;
        if (bytes.empty() || bytes.size() > 256) return 0;
        const auto address = loaded_address(*info, span.elf_address, bytes.size());
        if (!address || !std::any_of(headers.begin(), headers.end(), [&](const auto& header) {
            return header.p_type == PT_LOAD && (header.p_flags & (PF_R | PF_W | PF_X)) == (PF_R | PF_X) &&
                   contains_file_span(header, span.elf_address, bytes.size());
        })) return 0;
        if (!std::equal(bytes.begin(), bytes.end(), reinterpret_cast<const std::uint8_t*>(address))) return 0;
    }
    if (query.slot && !matches_slot(*info, headers, *query.slot)) {
        query.result.status = CodeStatus::DifferentSlot;
        return 0;
    }
    query.result = {CodeStatus::Match, info->dlpi_addr};
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
            std::copy_n(bytes.data() + desc, desc_size, id.bytes.begin());
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

CodeMatch verify_loaded_game_code(std::string_view expected_id, std::span<const CodeSpan> spans,
                                  std::optional<RelroSlot> slot) noexcept {
    CodeQuery query{expected_id, spans, slot, {}};
    dl_iterate_phdr(visit_code, &query);
    return query.result;
}

} // namespace ore_outline::build_id
