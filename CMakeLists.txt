cmake_minimum_required(VERSION 3.22)
project(ore_outline LANGUAGES CXX)

if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "Use the Android NDK with arm64-v8a. Run 'make test' for the host core.")
endif()
if(NOT ANDROID_STL STREQUAL "c++_shared")
    message(FATAL_ERROR "Set ANDROID_STL=c++_shared to match the LeviLauncher SDK ABI.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the same SDK revision referenced by the launcher for link-time checking.
# Only our module is packaged; the device supplies Preloader and libc++_shared.
include(FetchContent)
FetchContent_Declare(
    preloader_sdk
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG cc08b40a31471d3e2dedb267f5f17b28da2b0604
)
FetchContent_MakeAvailable(preloader_sdk)

add_library(ore_outline SHARED src/launcher/hud_preview.cpp src/ore_outline/core.cpp)
target_include_directories(ore_outline PRIVATE include)
target_link_libraries(ore_outline PRIVATE preloader)
target_compile_options(ore_outline PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(ore_outline PRIVATE -Wl,--no-undefined -Wl,-z,max-page-size=16384)
set_target_properties(ore_outline PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    SKIP_BUILD_RPATH YES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mod"
)
