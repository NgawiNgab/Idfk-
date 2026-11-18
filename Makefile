BUILD_DIR ?= build
CXXFLAGS ?= -O2 -g
HOST_FLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
HOST_TEST := $(BUILD_DIR)/ore-outline-core-test
BUILD_ID_TEST := $(BUILD_DIR)/ore-outline-build-id-test
CORE := src/ore_outline/core.cpp
CORE_TEST := tests/core_test.cpp
PROBE := src/launcher/build_id.cpp
PROBE_TEST := tests/build_id_test.cpp
FIXTURE_MATCH := $(BUILD_DIR)/fixtures/build-id-match/libminecraftpe.so
FIXTURE_OTHER := $(BUILD_DIR)/fixtures/build-id-other/libminecraftpe.so
FIXTURE_NO_ID := $(BUILD_DIR)/fixtures/build-id-absent/libminecraftpe.so
FIXTURES := $(FIXTURE_MATCH) $(FIXTURE_OTHER) $(FIXTURE_NO_ID)

.PHONY: test
test: $(HOST_TEST) $(BUILD_ID_TEST) $(FIXTURES)
	"$(HOST_TEST)"
	"$(BUILD_ID_TEST)" "$(abspath $(FIXTURE_MATCH))" "$(abspath $(FIXTURE_OTHER))" "$(abspath $(FIXTURE_NO_ID))"

$(HOST_TEST): $(CORE) $(CORE_TEST) include/ore_outline/core.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) $(CORE) $(CORE_TEST) $(LDFLAGS) $(LDLIBS) -o "$@"

$(BUILD_ID_TEST): $(PROBE) $(PROBE_TEST) include/ore_outline/build_id.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) $(PROBE) $(PROBE_TEST) $(LDFLAGS) $(LDLIBS) -ldl -o "$@"

# These are tiny host fixtures, not Minecraft binaries. They aren't packaged.
$(FIXTURE_MATCH): NOTE_ID := 0xc60fa32908019eecae39ebfe7bac5755dc35cde2
$(FIXTURE_OTHER): NOTE_ID := 0x00112233445566778899aabbccddeeff00112233
$(FIXTURE_NO_ID): NOTE_ID := none
$(FIXTURES): tests/build_id_fixture.cpp Makefile
	mkdir -p "$(@D)"
	$(CXX) -fPIC -shared -Wl,--build-id=$(NOTE_ID) "$<" -o "$@"
