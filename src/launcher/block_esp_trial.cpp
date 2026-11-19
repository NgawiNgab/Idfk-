#include "ore_outline/block_esp.hpp"

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <mutex>

namespace {
using namespace ore_outline;
namespace menu = pl::modmenu;

constexpr std::string_view kModule = "idfk.block-esp-hud-trial";
constexpr WorldKey kPreviewWorld{1, 0}; // Fixture namespace only; never a Minecraft world key.
constexpr struct Sample {
    BlockPos position;
    std::string_view id;
} kSamples[]{
    // Back to front in the synthetic camera, which looks along -Z.
    {{2, 1, -9}, "minecraft:ancient_debris"},
    {{-3, 0, -8}, "minecraft:diamond_ore"},
    {{0, 1, -7}, "minecraft:deepslate_diamond_ore"},
    {{2, -2, -6}, "minecraft:diamond_ore"},
    {{-1, -2, -5}, "example:glowing_ore"},
};
static_assert(std::size(kSamples) <= esp::kMaxTargets);

constexpr std::uint32_t argb(Color c) noexcept {
    return (std::uint32_t{c.a} << 24) | (std::uint32_t{c.r} << 16) |
           (std::uint32_t{c.g} << 8) | c.b;
}

std::optional<bool> toggle_value(std::string_view value) noexcept {
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    return std::nullopt;
}

std::optional<CameraFrame> trial_frame(menu::HudSurfaceSize surface) noexcept {
    const auto [width, height] = surface;
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) return {};
    // Use a bounded central viewport so the fixture also fits on tall displays.
    const auto w = std::min(static_cast<double>(width), 1.8 * height);
    const auto h = std::min(static_cast<double>(height), w);
    constexpr float scale = 1.2f;
    const esp::RelativeCamera fixture{{0, 0, 0},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {static_cast<float>(scale * h / w), 0, 0, 0, 0, scale, 0, 0,
         0, 0, -1.002f, -1, 0, 0, -0.2002f, 0}};
    auto frame = esp::make_fullscreen_frame(kPreviewWorld, fixture, w, h);
    if (frame) frame->viewport = {(width - w) * 0.5, (height - h) * 0.5, w, h};
    return frame;
}

menu::DrawCommand fill_command(const esp::Triangle& fill) {
    const auto& [a, b, c] = fill.vertices;
    menu::DrawCommand command{};
    command.type = menu::DrawCommandType::TriangleFilled;
    command.x = static_cast<float>(a.x);
    command.y = static_cast<float>(a.y);
    // LeviLauncher triangle coordinates are absolute (unlike Line.w/h).
    command.w = static_cast<float>(b.x);
    command.h = static_cast<float>(b.y);
    command.x3 = static_cast<float>(c.x);
    command.y3 = static_cast<float>(c.y);
    command.color = argb(fill.color);
    return command;
}

menu::DrawCommand line_command(const HudLine& line, float width) {
    menu::DrawCommand command{};
    command.type = menu::DrawCommandType::Line;
    command.x = static_cast<float>(line.x0);
    command.y = static_cast<float>(line.y0);
    command.w = static_cast<float>(line.x1 - line.x0);
    command.h = static_cast<float>(line.y1 - line.y0);
    command.size = width;
    command.color = argb(line.color);
    return command;
}

menu::DrawCommand status_command(std::string_view message, menu::HudSurfaceSize surface) {
    menu::DrawCommand command{};
    command.type = menu::DrawCommandType::Text;
    command.x = surface.width * 0.035f;
    command.y = surface.height * 0.17f;
    command.size = std::min({24.0f, surface.width * 0.027f, surface.height * 0.045f});
    command.color = 0xffffffffu;
    command.text = message;
    return command;
}

class BlockEspTrial {
public:
    bool load() const { return true; }

    bool enable(pl::mod::ModContext& context) {
        std::lock_guard lock(mutex_);
        if (registered_) return true;
        const auto generation = ++generation_;
        active_ = false;
        options_ = {false, false};
        identifiers_ = esp::parse_identifiers(esp::kDiamondIds);

        menu::ModuleInfo info{};
        info.moduleId = kModule;
        info.displayName = "Block ESP & Chams (HUD Trial)";
        info.description = "Simulated blocks only. No live camera, world reads or ore detection.";
        info.modId = context.id();
        info.defaultEnabled = false;
        info.hideInHudEditor = true;
        info.configs = {
            {"lines", "ESP Lines", menu::ConfigType::Toggle, "false", "", "", ""},
            {"chams", "Chams", menu::ConfigType::Toggle, "false", "", "", ""},
            {"ids", "Block IDs (comma-separated)", menu::ConfigType::Text, std::string{esp::kDiamondIds}, "", "", ""},
        };
        info.onToggle = [this, generation](std::string_view id, bool on) {
            std::lock_guard lock(mutex_);
            if (!current(id, generation)) return;
            active_ = on;
            redraw();
        };
        info.onConfigChanged = [this, generation](std::string_view id, std::string_view key, std::string_view value) {
            std::lock_guard lock(mutex_);
            if (!current(id, generation)) return;
            if (key == "ids") {
                identifiers_ = esp::parse_identifiers(value);
            } else if (key == "lines" || key == "chams") {
                const auto on = toggle_value(value);
                if (!on) return;
                (key == "lines" ? options_.lines : options_.chams) = *on;
            } else {
                return;
            }
            redraw();
        };
        registered_ = menu::registerModule(info);
        return registered_;
    }

    bool disable() {
        std::lock_guard lock(mutex_);
        if (!registered_) return true;
        registered_ = active_ = false;
        menu::submitDrawCommands(kModule, {});
        menu::unregisterModule(kModule);
        return true;
    }

    bool unload() { return disable(); }

private:
    bool current(std::string_view id, std::uint64_t generation) const noexcept {
        return registered_ && id == kModule && generation == generation_;
    }

    // All changes replace the retained batch, including invalid input and off.
    // Never acquire, cache, or interpret any game object in this trial.
    void redraw() const {
        if (!active_ || (!options_.lines && !options_.chams)) {
            menu::submitDrawCommands(kModule, {});
            return;
        }
        const auto surface = menu::getHudSurfaceSize();
        const auto frame = trial_frame(surface);
        if (!frame) {
            menu::submitDrawCommands(kModule, {});
            return;
        }

        std::array<esp::BlockTarget, std::size(kSamples)> matches{};
        std::size_t count = 0;
        std::string_view status = "HUD trial - simulated blocks";
        if (identifiers_) {
            for (const auto& sample : kSamples) {
                if (const auto index = identifiers_->match(sample.id)) {
                    matches[count++] = {sample.position, esp::block_color(sample.id, *index)};
                }
            }
            if (!count) status = "HUD trial - no simulated blocks match";
        } else {
            status = "HUD trial - invalid Block IDs (use namespace:name)";
        }

        const auto batch = esp::project(kPreviewWorld, std::span(matches).first(count), *frame, options_);
        std::vector<menu::DrawCommand> commands;
        commands.reserve(1 + batch.fills.size() + batch.lines.size());
        for (const auto& fill : batch.fills) commands.push_back(fill_command(fill));
        const auto stroke = std::clamp(surface.height * 0.0035f, 2.0f, 4.0f);
        for (const auto& line : batch.lines) commands.push_back(line_command(line, stroke));
        commands.push_back(status_command(status, surface));
        menu::submitDrawCommands(kModule, commands);
    }

    std::mutex mutex_;
    std::uint64_t generation_ = 0;
    bool registered_ = false;
    bool active_ = false;
    esp::DrawOptions options_{false, false};
    std::optional<esp::IdentifierList> identifiers_;
};

BlockEspTrial& trial() {
    static BlockEspTrial instance;
    return instance;
}
} // namespace

PL_REGISTER_MOD(BlockEspTrial, trial())
