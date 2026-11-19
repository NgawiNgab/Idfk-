#include "../src/launcher/relro_slot.hpp"

#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>

using ore_outline::detail::exchange_readonly_slot;
using ore_outline::detail::SlotSwap;

#define CHECK(expression)                                                                      \
    do {                                                                                       \
        if (!(expression)) {                                                                   \
            std::cerr << __LINE__ << ": " << #expression << '\n';                              \
            std::exit(EXIT_FAILURE);                                                           \
        }                                                                                      \
    } while (false)

int main() {
    const auto page_size = sysconf(_SC_PAGESIZE);
    CHECK(page_size > 0);
    const auto bytes = static_cast<std::size_t>(page_size);
    void* page = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(page != MAP_FAILED);
    auto* slot = static_cast<std::uintptr_t*>(page);
    constexpr std::uintptr_t original = 0x1000, replacement = 0x2000, sentinel = 0x1234;
    slot[0] = original;
    slot[1] = sentinel;
    const auto address = reinterpret_cast<std::uintptr_t>(slot);
    CHECK(exchange_readonly_slot(address, original, replacement) == SlotSwap::NotReadOnly);
    CHECK(mprotect(page, bytes, PROT_READ) == 0);
    CHECK(exchange_readonly_slot(address + 1, original, replacement) == SlotSwap::NotReadOnly);
    CHECK(exchange_readonly_slot(address, replacement, original) == SlotSwap::Unexpected);
    CHECK(exchange_readonly_slot(address, original, replacement) == SlotSwap::Changed);
    CHECK(slot[0] == replacement && slot[1] == sentinel);
    CHECK(exchange_readonly_slot(address, replacement, original) == SlotSwap::Changed);
    CHECK(slot[0] == original && slot[1] == sentinel);
    CHECK(munmap(page, bytes) == 0);
    CHECK(exchange_readonly_slot(address, original, replacement) == SlotSwap::NotReadOnly);

    std::cout << "RELRO slot: reject writable/absent/unaligned; checked pointer exchange and restoration passed\n";
}
