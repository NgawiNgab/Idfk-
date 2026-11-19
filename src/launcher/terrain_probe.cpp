#include "ore_outline/terrain_capture.hpp"
#include "relro_slot.hpp"
#include "terrain_layout.hpp"

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>

#if !defined(__ANDROID__) || !defined(__aarch64__)
#error The passive terrain binding is only for Android ARM64.
#endif

namespace {
using namespace ore_outline;
namespace capture = terrain_capture;
namespace layout = terrain_layout;
namespace menu = pl::modmenu;
using detail::SlotSwap;

constexpr auto kModule = "idfk.passive-terrain-probe";
static_assert(sizeof(BlockPos) == 12 && offsetof(BlockPos, y) == 4 && offsetof(BlockPos, z) == 8);
static_assert(std::atomic<std::uintptr_t>::is_always_lock_free && std::atomic<capture::Capture::Token>::is_always_lock_free);

// Kept alive (including during detach) by the game handle and the probe's
// DF_1_NODELETE. Neither render workers nor menu callbacks own a game object.
constinit std::atomic<std::uintptr_t> original_lookup{0};
constinit capture::Capture samples;
using Lookup = const void* (*)(const void* source, const void* position);

std::optional<std::uint64_t> copy_key(const void* block) noexcept {
    // The very next instructions at the scoped caller read this same chain.
    if (!block) return {};
    const std::byte* type = nullptr;
    std::memcpy(&type, static_cast<const std::byte*>(block) + layout::kBlockType, sizeof(type));
    if (!type) return {};
    std::uint64_t key;
    std::memcpy(&key, type + layout::kTypeKey, sizeof(key));
    return key;
}

[[gnu::noinline]] const void* observe_lookup(const void* source, const void* position) {
    // Pair with the slot publisher even though the engine loads the slot using
    // an ordinary, naturally aligned LDR rather than an acquire instruction.
    std::atomic_thread_fence(std::memory_order_acquire);
    const auto original = original_lookup.load(std::memory_order_acquire);
    const auto caller = reinterpret_cast<std::uintptr_t>(
        __builtin_extract_return_addr(__builtin_return_address(0)));
    const auto window = caller == original - layout::kLookup + layout::kBuilderReturn ? samples.token() : 0;

    // This is the game's existing request, not an additional lookup. All other
    // call sites are straight pass-through, including temporaries/other worlds.
    const auto* result = reinterpret_cast<Lookup>(original)(source, position);
    if (window) {
        if (position) {
            BlockPos xyz;
            std::memcpy(&xyz, position, sizeof(xyz));
            if (xyz == capture::kExposed || xyz == capture::kEnclosed) {
                samples.record(window, xyz, copy_key(result));
                return result;
            }
        }
        samples.record(window);
    }
    return result;
}

std::string_view guard_status(build_id::CodeStatus status) noexcept {
    using enum build_id::CodeStatus;
    switch (status) {
        case NotLoaded: return "Game library not loaded";
        case MultipleModules: return "More than one game library";
        case Unavailable: return "Loaded Build ID unavailable";
        case DifferentBuild: return "Unsupported Build ID (not attached)";
        case DifferentCode: return "Code check failed (not attached)";
        case DifferentSlot: return "Dispatch slot check failed (not attached)";
        case Match: break;
    }
    return "Probe not attached";
}

std::string_view swap_status(SlotSwap status) noexcept {
    switch (status) {
        case SlotSwap::Changed: break;
        case SlotSwap::Unexpected: return "Dispatch slot changed";
        case SlotSwap::NotReadOnly: return "Dispatch page is not private read-only";
        case SlotSwap::ProtectFailed: return "Dispatch page cannot be made writable";
        case SlotSwap::RolledBack: return "Page protection error (change rolled back)";
        case SlotSwap::RestoreFailed: return "Page protection error; exit Minecraft";
    }
    return "Probe not attached";
}

std::string_view marker(capture::Match match) noexcept {
    switch (match) {
        case capture::Match::NoVisit: return "not visited";
        case capture::Match::UnknownKey: return "unknown block key";
        case capture::Match::AirKeyOrFallback: return "air key / fallback";
        case capture::Match::OtherKey: return "other block key";
        case capture::Match::DiamondKey: return "diamond marker";
    }
    return "unknown";
}

class TerrainProbe {
public:
    bool load() const { return true; }

    bool enable(pl::mod::ModContext& context) {
        std::lock_guard lock(mutex_);
        if (registered_) return true;
        const auto generation = ++generation_;
        menu::ModuleInfo info{};
        info.moduleId = kModule;
        info.displayName = "Passive Terrain Probe";
        info.description = "Diagnostic for a disposable offline world. Shows a report, not ore outlines.";
        info.modId = context.id();
        info.defaultEnabled = false;
        info.hideInHudEditor = true;
        info.configs = {
            {"arm", "Start capture", menu::ConfigType::Button, "", "", "", ""},
            {"freeze", "Freeze and show report", menu::ConfigType::Button, "", "", "", ""},
        };
        info.onToggle = [this, generation](std::string_view id, bool on) {
            std::lock_guard lock(mutex_);
            if (!registered_ || generation != generation_ || id != kModule) return;
            if (visible_ == on) return;
            visible_ = on;
            if (!visible_) clear();
            redraw();
        };
        info.onConfigChanged = [this, generation](std::string_view id, std::string_view key, std::string_view value) {
            std::lock_guard lock(mutex_);
            if (!registered_ || !visible_ || generation != generation_ || id != kModule || value != "true") return;
            if (key == "arm") start_capture();
            else if (key == "freeze") freeze();
            else return;
            redraw();
        };
        registered_ = menu::registerModule(info);
        return registered_;
    }

    bool disable() {
        std::lock_guard lock(mutex_);
        clear();
        if (registered_) {
            registered_ = visible_ = false;
            menu::submitDrawCommands(kModule, {});
            menu::unregisterModule(kModule);
        }
        return true;
    }

    bool unload() { return disable(); }

private:
    std::mutex mutex_; // Only UI/lifecycle calls take this lock.
    std::uint64_t generation_ = 0;
    bool registered_ = false, visible_ = false, restart_required_ = false;
    void* game_pin_ = nullptr; // RTLD_NOLOAD reference deliberately never closed.
    std::uintptr_t slot_ = 0;
    std::string_view status_ = "Off: press Start capture to attach";
    std::optional<capture::Report> report_;

    bool pin_game(std::uintptr_t load_bias) noexcept {
        if (game_pin_) return original_lookup.load(std::memory_order_relaxed) == load_bias + layout::kLookup;
        Dl_info module{};
        if (!dladdr(reinterpret_cast<const void*>(load_bias + layout::kLookup), &module) || !module.dli_fname ||
            reinterpret_cast<std::uintptr_t>(module.dli_fbase) != load_bias) return false;
        auto* pin = dlopen(module.dli_fname, RTLD_NOLOAD | RTLD_NOW);
        if (!pin) return false;
        // Verify the NOLOAD result refers to this link-map entry, not another namespace.
        constexpr auto kResizeSymbol = "Java_com_mojang_minecraftpe_MainActivity_nativeResize";
        constexpr std::uintptr_t kResizeAddress = 0x0889c618;
        const auto symbol = reinterpret_cast<std::uintptr_t>(dlsym(pin, kResizeSymbol));
        const auto check = build_id::verify_loaded_game_code(build_id::kDriveBuildId, layout::kCode, layout::kSlot);
        if (symbol != load_bias + kResizeAddress || check.status != build_id::CodeStatus::Match ||
            check.load_bias != load_bias) {
            dlclose(pin); // Nothing was published or hooked yet.
            return false;
        }
        game_pin_ = pin;
        original_lookup.store(load_bias + layout::kLookup, std::memory_order_release);
        return true;
    }

    void start_capture() noexcept {
        if (restart_required_ || slot_) return;
        report_.reset();
        const auto check = build_id::verify_loaded_game_code(build_id::kDriveBuildId, layout::kCode, layout::kSlot);
        if (check.status != build_id::CodeStatus::Match) {
            status_ = guard_status(check.status);
            return;
        }
        if (!pin_game(check.load_bias)) {
            status_ = "Could not keep game library loaded (not attached)";
            return;
        }
        const auto address = check.load_bias + layout::kLookupSlot;
        const auto swap = detail::exchange_readonly_slot(address, original_lookup.load(std::memory_order_relaxed),
                                                        reinterpret_cast<std::uintptr_t>(&observe_lookup));
        if (swap != SlotSwap::Changed) {
            status_ = swap_status(swap);
            restart_required_ = swap == SlotSwap::RestoreFailed;
            return;
        }
        slot_ = address;
        samples.arm();
        status_ = "Capturing ordinary terrain builds";
    }

    bool detach() noexcept {
        samples.stop(); // A late call may still forward; it cannot add to this report.
        if (!slot_) return !restart_required_;
        const auto swap = detail::exchange_readonly_slot(slot_, reinterpret_cast<std::uintptr_t>(&observe_lookup),
                                                        original_lookup.load(std::memory_order_relaxed));
        // Never overwrite a foreign destination. Pinned code keeps all in-flight calls safe.
        if (swap == SlotSwap::Changed || swap == SlotSwap::Unexpected) slot_ = 0;
        restart_required_ = swap != SlotSwap::Changed;
        if (restart_required_) status_ = "Could not detach cleanly; exit Minecraft";
        return !restart_required_;
    }

    void clear() noexcept {
        if (detach()) status_ = "Off: press Start capture to attach";
        report_.reset();
    }

    void freeze() noexcept {
        if (!slot_) return;
        if (detach()) {
            report_ = samples.report();
            status_ = report_->builder_seen ? "Frozen: builder visits observed" : "Frozen: no builder visits observed";
        }
    }

    void redraw() const {
        const auto surface = menu::getHudSurfaceSize();
        if (!visible_ || !std::isfinite(surface.width) || !std::isfinite(surface.height) ||
            surface.width <= 0 || surface.height <= 0) {
            menu::submitDrawCommands(kModule, {});
            return;
        }
        const float size = std::min({24.0f, surface.width * 0.023f, surface.height * 0.04f});
        const float spacing = 1.5f * size;
        std::array<menu::DrawCommand, 3> commands{};
        std::size_t count = 0;
        const auto append = [&](std::string_view title, std::string_view detail = {}) {
            auto& line = commands[count++];
            line.type = menu::DrawCommandType::Text;
            line.x = 0.025f * surface.width;
            line.y = 0.16f * surface.height + static_cast<float>(count - 1) * spacing;
            line.w = 0.95f * surface.width;
            line.h = spacing;
            line.size = size;
            line.color = 0xffffffffu;
            line.text = title;
            line.text += detail;
        };
        append(status_);
        if (report_) {
            append("Exposed 20, 100, -12: ", marker(report_->exposed));
            append("Enclosed 28, 100, -4: ", marker(report_->enclosed));
        }
        menu::submitDrawCommands(kModule, std::span(commands).first(count));
    }
};

TerrainProbe& probe() {
    static TerrainProbe instance;
    return instance;
}
} // namespace

PL_REGISTER_MOD(TerrainProbe, probe())
