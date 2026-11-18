BUILD_DIR ?= build
CXXFLAGS ?= -O2 -g
HOST_TEST := $(BUILD_DIR)/ore-outline-core-test
CORE := src/ore_outline/core.cpp
TEST := tests/core_test.cpp

.PHONY: test
test: $(HOST_TEST)
	"$(HOST_TEST)"

$(HOST_TEST): $(CORE) $(TEST) include/ore_outline/core.hpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Wpedantic -Iinclude $(CORE) $(TEST) $(LDFLAGS) $(LDLIBS) -o "$@"
