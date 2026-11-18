#include "ore_outline/build_id.hpp"
#include "ore_outline/core.hpp"

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace ore_outline;
namespace menu = pl::modmenu;

constexpr std::string_view kModuleId = "idfk.ore-outline-hud-test";
constexpr std::string_view kBuildIdModuleId = "idfk.minecraft-build-id";
constexpr ScanScope kFixture{{1, 0}, {-2, 0, 0}, {1, 0, 0}};
struct Sample {
    Category category;
    std::string_view key, title, name;
    BlockPos position;
};
constexpr std::array kSamples{
    Sample{Category::Diamond, "diamond", "Synthetic Diamond", "minecraft:diamond_ore", {-2, 0, 0}},
    Sample{Category::Gold, "gold", "Synthetic Nether Gold", "minecraft:nether_gold_ore", {1, 0, 0}},
};

constexpr std::uint32_t argb(Color c) noexcept {
    return (std::uint32_t{c.a} << 24) | (std::uint32_t{c.r} << 16) |
           (std::uint32_t{c.g} << 8) | std::uint32_t{c.b};
}

bool valid_surface(menu::HudSurfaceSize size) noexcept {
    return std::isfinite(size.width) && std::isfinite(size.height) &&
           size.width > 0 && size.height > 0;
}

void show_loaded_build_id() {
    const auto surface = menu::getHudSurfaceSize();
    if (!valid_surface(surface)) {
        menu::submitDrawCommands(kBuildIdModuleId, {});
        return;
    }

    // One snapshot of the current link map, only when the user enables this check.
    const auto result = build_id::query_loaded_game();
    const auto [width, height] = surface;
    const float textSize = std::min({26.0f, width * 0.029f, height * 0.045f});
    const float lineHeight = 1.5f * textSize;
    std::vector<menu::DrawCommand> commands;
    const auto append = [&](std::string_view text) {
        menu::DrawCommand line{};
        line.type = menu::DrawCommandType::Text;
        line.x = 0.05f * width;
        line.y = 0.16f * height + static_cast<float>(commands.size()) * lineHeight;
        line.w = 0.9f * width;
        line.h = lineHeight;
        line.size = textSize;
        line.color = 0xffffffffu;
        line.text = text;
        commands.push_back(std::move(line));
    };
    switch (result.status) {
        case build_id::Status::NotLoaded:
            append("Minecraft library not loaded");
            break;
        case build_id::Status::MultipleModules:
            append("Multiple Minecraft libraries");
            break;
        case build_id::Status::Unavailable:
            append("Build ID unavailable");
            break;
        case build_id::Status::Found: {
            const auto hex = result.id.hex();
            append(hex == build_id::kDriveBuildId ? "Build ID: MATCH" : "Build ID: DIFFERENT");
            const std::string_view digits(hex);
            for (std::size_t i = 0; i < digits.size(); i += 20) {
                append(digits.substr(i, 20));
            }
            break;
        }
    }
    menu::submitDrawCommands(kBuildIdModuleId, commands);
}

class HudPreview {
public:
    bool load() { return true; }

    bool enable(pl::mod::ModContext& context) {
        std::lock_guard lock(mutex_);
        if (registered_) return true;
        const auto generation = ++generation_;
        settings_ = {};
        menu::ModuleInfo info{};
        info.moduleId = kModuleId;
        info.displayName = "Ore Outlines HUD Test";
        info.description = "Synthetic boxes only. No Minecraft world data is read.";
        info.modId = context.id();
        info.defaultEnabled = false;
        info.hideInHudEditor = true;
        for (const auto& sample : kSamples) {
            settings_.categories.set(static_cast<std::size_t>(sample.category));
            menu::ConfigEntry entry{};
            entry.key = sample.key;
            entry.displayName = sample.title;
            entry.type = menu::ConfigType::Toggle;
            entry.defaultValue = "true";
            info.configs.push_back(entry);
        }
        info.onToggle = [this, generation](std::string_view id, bool enabled) {
            std::lock_guard lock(mutex_);
            if (!current(id, generation)) return;
            settings_.enabled = enabled;
            redraw();
        };
        info.onConfigChanged = [this, generation](std::string_view id, std::string_view key,
                                                  std::string_view value) {
            const bool enabled = value == "true" || value == "1";
            if (!enabled && value != "false" && value != "0") return;
            std::lock_guard lock(mutex_);
            if (!current(id, generation)) return;
            for (const auto& sample : kSamples) {
                if (sample.key != key) continue;
                settings_.categories.set(static_cast<std::size_t>(sample.category), enabled);
                redraw();
                return;
            }
        };
        menu::ModuleInfo identity{};
        identity.moduleId = kBuildIdModuleId;
        identity.displayName = "Minecraft Build ID Check";
        identity.description = "Read the loaded library fingerprint. No world reads or game hooks.";
        identity.modId = context.id();
        identity.defaultEnabled = false;
        identity.hideInHudEditor = true;
        identity.onToggle = [this, generation](std::string_view id, bool enabled) {
            std::lock_guard lock(mutex_);
            if (!registered_ || id != kBuildIdModuleId || generation != generation_) return;
            if (enabled) show_loaded_build_id();
            else menu::submitDrawCommands(kBuildIdModuleId, {});
        };
        if (!menu::registerModule(info)) return false;
        if (!menu::registerModule(identity)) {
            menu::unregisterModule(kModuleId);
            return false;
        }
        registered_ = true;
        return true;
    }

    bool disable() {
        std::lock_guard lock(mutex_);
        if (registered_) {
            registered_ = false;
            settings_ = {};
            menu::submitDrawCommands(kModuleId, {});
            menu::unregisterModule(kModuleId);
            menu::submitDrawCommands(kBuildIdModuleId, {});
            menu::unregisterModule(kBuildIdModuleId);
        }
        return true;
    }

    bool unload() { return disable(); }

private:
    // Menu callbacks are called outside the bridge lock. Serialize our settings and
    // submit/clear operations; ignore callbacks copied out by an older registration.
    std::mutex mutex_;
    Settings settings_;
    std::uint64_t generation_ = 0;
    bool registered_ = false;

    bool current(std::string_view id, std::uint64_t generation) const noexcept {
        return registered_ && id == kModuleId && generation == generation_;
    }

    void redraw() const {
        if (!settings_.enabled) {
            menu::submitDrawCommands(kModuleId, {});
            return;
        }
        const auto surface = menu::getHudSurfaceSize();
        if (!valid_surface(surface)) {
            menu::submitDrawCommands(kModuleId, {});
            return;
        }
        const auto [width, height] = surface;
        const double side = 0.9 * std::min(width, height);
        const CameraFrame camera{
            kFixture.world,
            {0.34, 0, 0.13, -0.065,
             0, 0.38, 0.16, -0.27,
             0, 0, 0.24, 0,
             0, 0, 0, 1},
            {(width - side) * 0.5, (height - side) * 0.5, side, side},
        };
        OutlineCache fixture;
        fixture.configure(kFixture, settings_);
        for (const auto& sample : kSamples) {
            fixture.observe(kFixture.world, sample.position, sample.name);
        }
        const auto lines = project_outlines(fixture.snapshot(), camera, 12 * kSamples.size());
        std::vector<menu::DrawCommand> commands;
        commands.reserve(lines.size());
        for (const auto& line : lines) {
            menu::DrawCommand command{};
            command.type = menu::DrawCommandType::Line;
            command.x = static_cast<float>(line.x0);
            command.y = static_cast<float>(line.y0);
            command.w = static_cast<float>(line.x1 - line.x0);
            command.h = static_cast<float>(line.y1 - line.y0);
            command.size = 3.0f;
            command.color = argb(line.color);
            commands.push_back(std::move(command));
        }
        menu::submitDrawCommands(kModuleId, commands);
    }
};

HudPreview& preview() {
    static HudPreview instance;
    return instance;
}
} // namespace

PL_REGISTER_MOD(HudPreview, preview())
