BUILD_DIR ?= build
CXXFLAGS ?= -O2 -g
HOST_FLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
HOST_TEST := $(BUILD_DIR)/ore-outline-core-test
SCAN_TEST := $(BUILD_DIR)/ore-outline-chunk-scan-test
BUILD_ID_TEST := $(BUILD_DIR)/ore-outline-build-id-test
CAPTURE_TEST := $(BUILD_DIR)/ore-outline-terrain-capture-test
SLOT_TEST := $(BUILD_DIR)/ore-outline-relro-slot-test
CORE := src/ore_outline/core.cpp
CORE_TEST := tests/core_test.cpp
SCAN := src/ore_outline/chunk_scan.cpp
SCAN_TEST_SRC := tests/chunk_scan_test.cpp
PROBE := src/launcher/build_id.cpp
PROBE_TEST := tests/build_id_test.cpp
FIXTURE_MATCH := $(BUILD_DIR)/fixtures/build-id-match/libminecraftpe.so
FIXTURE_OTHER := $(BUILD_DIR)/fixtures/build-id-other/libminecraftpe.so
FIXTURE_NO_ID := $(BUILD_DIR)/fixtures/build-id-absent/libminecraftpe.so
FIXTURES := $(FIXTURE_MATCH) $(FIXTURE_OTHER) $(FIXTURE_NO_ID)
WORLD_PROBE := $(BUILD_DIR)/world-probe/ore-outline-one-block-probe.mcpack
WORLD_PROBE_FILES := probes/one-block/manifest.json probes/one-block/scripts/main.js probes/one-block/scripts/probe.js

.PHONY: test test-world-probe world-probe
test: $(HOST_TEST) $(SCAN_TEST) $(BUILD_ID_TEST) $(FIXTURES) $(CAPTURE_TEST) $(SLOT_TEST)
	"$(HOST_TEST)"
	"$(SCAN_TEST)"
	"$(BUILD_ID_TEST)" "$(abspath $(FIXTURE_MATCH))" "$(abspath $(FIXTURE_OTHER))" "$(abspath $(FIXTURE_NO_ID))"
	"$(CAPTURE_TEST)"
	"$(SLOT_TEST)"

$(HOST_TEST): $(CORE) $(CORE_TEST) include/ore_outline/core.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) $(CORE) $(CORE_TEST) $(LDFLAGS) $(LDLIBS) -o "$@"

$(SCAN_TEST): $(CORE) $(SCAN) $(SCAN_TEST_SRC) include/ore_outline/core.hpp include/ore_outline/chunk_scan.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) $(CORE) $(SCAN) $(SCAN_TEST_SRC) $(LDFLAGS) $(LDLIBS) -o "$@"

$(BUILD_ID_TEST): $(PROBE) $(PROBE_TEST) include/ore_outline/build_id.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) $(PROBE) $(PROBE_TEST) $(LDFLAGS) $(LDLIBS) -ldl -o "$@"

$(CAPTURE_TEST): tests/terrain_capture_test.cpp include/ore_outline/terrain_capture.hpp include/ore_outline/core.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) -pthread "$<" $(LDFLAGS) $(LDLIBS) -o "$@"

$(SLOT_TEST): src/launcher/relro_slot.cpp src/launcher/relro_slot.hpp tests/relro_slot_test.cpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOST_FLAGS) "$<" tests/relro_slot_test.cpp $(LDFLAGS) $(LDLIBS) -o "$@"

# These are tiny host fixtures, not Minecraft binaries. They aren't packaged.
$(FIXTURE_MATCH): NOTE_ID := 0xc60fa32908019eecae39ebfe7bac5755dc35cde2
$(FIXTURE_OTHER): NOTE_ID := 0x00112233445566778899aabbccddeeff00112233
$(FIXTURE_NO_ID): NOTE_ID := none
$(FIXTURES): tests/build_id_fixture.cpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(HOST_FLAGS) -fPIC -shared -Wl,-z,relro,-z,now,--build-id=$(NOTE_ID) "$<" -o "$@"

test-world-probe:
	node --experimental-default-type=module --test tests/world_probe_test.mjs

world-probe: $(WORLD_PROBE)

$(WORLD_PROBE): $(WORLD_PROBE_FILES) scripts/package_world_probe.py
	python3 scripts/package_world_probe.py "$@"
