#include "ore_outline/build_id.hpp"

#include <dlfcn.h>

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include <vector>

namespace probe = ore_outline::build_id;

#define CHECK(expr) do { if (!(expr)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": " << #expr << '\n'; \
    std::exit(EXIT_FAILURE); \
} } while (false)

namespace {
using Bytes = std::vector<std::uint8_t>;

void append_word(Bytes& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void append_note(Bytes& bytes, std::uint32_t type, std::string_view owner,
                 std::initializer_list<std::uint8_t> desc, std::size_t alignment = 4) {
    append_word(bytes, static_cast<std::uint32_t>(owner.size() + 1));
    append_word(bytes, static_cast<std::uint32_t>(desc.size()));
    append_word(bytes, type);
    bytes.insert(bytes.end(), owner.begin(), owner.end());
    bytes.push_back(0);
    while (bytes.size() % alignment != 0) bytes.push_back(0);
    bytes.insert(bytes.end(), desc.begin(), desc.end());
    while (bytes.size() % alignment != 0) bytes.push_back(0);
}

void test_note_parser() {
    Bytes notes;
    append_note(notes, 3, "NotGNU", {1});
    append_note(notes, 1, "Android", {28, 0, 0, 0});
    CHECK(probe::scan_notes(notes, 4).well_formed && !probe::scan_notes(notes, 4).id);
    append_note(notes, 3, "GNU", {0, 0x10, 0xab, 0xff, 2});
    const auto found = probe::scan_notes(notes, 4);
    CHECK(found.well_formed && found.id && found.id->hex() == "0010abff02");
    append_note(notes, 3, "GNU", {0, 0x10, 0xab, 0xff, 2});
    CHECK(probe::scan_notes(notes, 4).id == found.id); // Identical duplicate.
    append_note(notes, 3, "GNU", {9});
    CHECK(!probe::scan_notes(notes, 4).well_formed); // Conflicting duplicate.

    notes.clear();
    append_note(notes, 3, "GNU", {0x42}, 8);
    const auto aligned = probe::scan_notes(notes, 8);
    CHECK(aligned.well_formed && aligned.id && aligned.id->hex() == "42");
    CHECK(!probe::scan_notes(notes, 16).well_formed);
    notes.pop_back();
    CHECK(!probe::scan_notes(notes, 8).well_formed); // Missing alignment padding.
    notes.resize(12);
    CHECK(!probe::scan_notes(notes, 8).well_formed); // Missing owner and descriptor.
    notes.assign(12, 0xff);
    CHECK(!probe::scan_notes(notes, 4).well_formed);
    notes.assign(probe::kMaxNoteBytes + 1, 0);
    CHECK(!probe::scan_notes(notes, 4).well_formed);
    notes.clear();
    append_note(notes, 3, "GNU", {});
    CHECK(!probe::scan_notes(notes, 4).well_formed); // Empty ID is not a result.
}

void* load_fixture(const char* path) {
    auto* handle = dlopen(path, RTLD_LOCAL | RTLD_NOW);
    if (!handle) {
        std::cerr << dlerror() << '\n';
        std::exit(EXIT_FAILURE);
    }
    return handle;
}

void test_code_gate(const char* matching_path, const char* other_path, const char* missing_path) {
    using enum probe::CodeStatus;
    CHECK(probe::verify_loaded_game_code(probe::kDriveBuildId, {}).status == NotLoaded);
    auto* matching = load_fixture(matching_path);
    const auto function = dlsym(matching, "ore_outline_build_id_fixture");
    const auto slot = dlsym(matching, "ore_outline_build_id_fixture_slot");
    Dl_info image{};
    CHECK(function && slot && dladdr(function, &image) != 0);
    const auto bias = reinterpret_cast<std::uintptr_t>(image.dli_fbase);
    const auto entry = reinterpret_cast<std::uintptr_t>(function) - bias;
    const auto field = reinterpret_cast<std::uintptr_t>(slot) - bias;
    const std::array intact{*static_cast<const std::uint8_t*>(function)};
    const std::array changed{static_cast<std::uint8_t>(intact[0] ^ 0xff)};
    const std::array guard{probe::CodeSpan{entry, intact}};
    const auto verify = [&](std::span<const probe::CodeSpan> code,
                            probe::RelroSlot pointer = {}) {
        return probe::verify_loaded_game_code(probe::kDriveBuildId, code,
                                             pointer.elf_address ? std::optional{pointer} : std::nullopt);
    };
    const auto match = verify(guard, {field, entry});
    CHECK(match.status == Match && match.load_bias == bias);
    const auto check = [](probe::CodeMatch result, probe::CodeStatus expected) {
        CHECK(result.status == expected && result.load_bias == 0);
    };
    check(verify(std::array{probe::CodeSpan{entry, changed}}), DifferentCode);
    check(verify(std::array{probe::CodeSpan{field, intact}}), DifferentCode);
    check(verify(std::array{probe::CodeSpan{UINTPTR_MAX, intact}}), DifferentCode);
    check(verify({}), DifferentCode);
    check(verify(guard, {field, entry + 1}), DifferentSlot);
    check(verify(guard, {field + 1, entry}), DifferentSlot);
    check(verify(guard, {entry, entry}), DifferentSlot);
    check(verify(guard, {field, UINTPTR_MAX}), DifferentSlot);

    auto* other = load_fixture(other_path);
    check(verify(guard, {field, entry}), MultipleModules);
    CHECK(dlclose(matching) == 0);
    check(verify(guard, {field, entry}), DifferentBuild);
    CHECK(dlclose(other) == 0);
    auto* missing = load_fixture(missing_path);
    check(verify(guard), Unavailable);
    CHECK(dlclose(missing) == 0);
}

void test_loaded_modules(const char* matching_path, const char* other_path, const char* missing_path) {
    CHECK(probe::query_loaded_game().status == probe::Status::NotLoaded);
    auto* matching = load_fixture(matching_path);
    const auto match = probe::query_loaded_game();
    CHECK(match.status == probe::Status::Found && match.id.hex() == probe::kDriveBuildId);

    auto* other = load_fixture(other_path);
    CHECK(probe::query_loaded_game().status == probe::Status::MultipleModules);
    CHECK(dlclose(matching) == 0);
    const auto different = probe::query_loaded_game();
    CHECK(different.status == probe::Status::Found &&
          different.id.hex() == "00112233445566778899aabbccddeeff00112233");
    CHECK(dlclose(other) == 0);

    auto* no_note = load_fixture(missing_path);
    CHECK(probe::query_loaded_game().status == probe::Status::Unavailable);
    CHECK(dlclose(no_note) == 0);
    CHECK(probe::query_loaded_game().status == probe::Status::NotLoaded);
}
} // namespace

int main(int argc, char** argv) {
    CHECK(argc == 4);
    test_note_parser();
    test_loaded_modules(argv[1], argv[2], argv[3]);
    test_code_gate(argv[1], argv[2], argv[3]);
    std::cout << "loaded Build ID: note bounds, unique module, exact code and relocated RELRO gate passed\n";
}
