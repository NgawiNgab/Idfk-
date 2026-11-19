#include "relro_slot.hpp"

#include <sys/mman.h>
#include <unistd.h>

#include <cinttypes>
#include <cstdio>
#include <limits>
#include <string_view>

namespace ore_outline::detail {
namespace {

bool read_only_mapping(std::uintptr_t page, std::size_t size) noexcept {
    auto* maps = std::fopen("/proc/self/maps", "re");
    if (!maps) return false;
    bool found = false;
    char line[512];
    while (std::fgets(line, sizeof(line), maps)) {
        std::uintptr_t left{}, right{};
        char permissions[5]{};
        if (std::sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4s", &left, &right, permissions) == 3 &&
            left <= page && page < right) {
            found = right - page >= size && std::string_view(permissions) == "r--p";
            break;
        }
        // Skip any continuation of a long pathname; never parse it as a new entry.
        if (std::string_view(line).back() != '\n') {
            for (int ch; (ch = std::fgetc(maps)) != '\n' && ch != EOF;) {}
        }
    }
    std::fclose(maps);
    return found;
}

} // namespace

SlotSwap exchange_readonly_slot(std::uintptr_t address, std::uintptr_t from, std::uintptr_t to) noexcept {
    static_assert(__atomic_always_lock_free(sizeof(std::uintptr_t), nullptr));
    const auto size = sysconf(_SC_PAGESIZE);
    if (size < static_cast<long>(sizeof(std::uintptr_t)) || size > 65536 || (size & (size - 1)) != 0 ||
        !address || address % alignof(std::uintptr_t) != 0 || !from || !to || from == to) {
        return SlotSwap::NotReadOnly;
    }
    const auto bytes = static_cast<std::size_t>(size);
    const auto page = address & ~(static_cast<std::uintptr_t>(bytes) - 1);
    if (page > std::numeric_limits<std::uintptr_t>::max() - bytes || !read_only_mapping(page, bytes)) {
        return SlotSwap::NotReadOnly;
    }

    auto* slot = reinterpret_cast<std::uintptr_t*>(address);
    auto* mapping = reinterpret_cast<void*>(page);
    if (__atomic_load_n(slot, __ATOMIC_ACQUIRE) != from) return SlotSwap::Unexpected;
    if (mprotect(mapping, bytes, PROT_READ | PROT_WRITE) != 0) return SlotSwap::ProtectFailed;
    auto expected = from;
    const bool exchanged = __atomic_compare_exchange_n(slot, &expected, to, false, __ATOMIC_RELEASE,
                                                       __ATOMIC_RELAXED);
    if (mprotect(mapping, bytes, PROT_READ) == 0) {
        return exchanged ? SlotSwap::Changed : SlotSwap::Unexpected;
    }

    // If protection restoration failed, undo our change, never a third party's.
    // Both destinations must remain resident even after a rollback (in-flight callers).
    if (exchanged) {
        expected = to;
        __atomic_compare_exchange_n(slot, &expected, from, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
    }
    return mprotect(mapping, bytes, PROT_READ) == 0 ? SlotSwap::RolledBack : SlotSwap::RestoreFailed;
}

} // namespace ore_outline::detail
