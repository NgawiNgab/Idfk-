#pragma once

#include <cstdint>

namespace ore_outline::detail {

enum class SlotSwap { Changed, Unexpected, NotReadOnly, ProtectFailed, RolledBack, RestoreFailed };

// Caller verifies this is a relocated pointer in a non-executable GNU_RELRO
// segment and pins both images. Never makes a page executable. Serialized
// writers only; engine readers may see either intact pointer during exchange.
[[nodiscard]] SlotSwap exchange_readonly_slot(std::uintptr_t address, std::uintptr_t from,
                                               std::uintptr_t to) noexcept;

} // namespace ore_outline::detail
