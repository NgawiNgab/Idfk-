#pragma once
// ============================================================================
//  menu.h  --  replacement menu shell for Apollon on Minecraft Bedrock 1.26.66.1
//
//  Drop-in replacement for the original menu.h. The widget bodies for every
//  page are carried over verbatim from the upstream Apollon sources, so no
//  toggle, slider or combo is lost; only the chrome around them changed:
//
//    * single violet accent instead of the old magenta/boxed look
//    * flat sidebar with an active-state pill instead of borderless buttons
//    * rounded window, no OS titlebar, custom header with drag + close
//    * every page is a scrollable child so nothing overflows off-screen on
//      phones (the old layout clipped long pages such as Combat/Visuals)
//    * buttons carry the modern "Apollon Reborn" branding and the actual
//      target version (1.26.66.1) rather than the stale 1.21.111 string
//
//  The one page that is NOT carried over is the old 1.21.111 "Info" page,
//  whose links/credits were replaced by a compatibility report that tells the
//  truth about which modules can work on this build.
// ============================================================================

#include "Images/YT.h"
#include "Images/Telegram.h"
#include "Images/Discord.h"

#include "Images/amethyst_shard.h"
#include "Images/coal.h"
#include "Images/copper_ingot.h"
#include "Images/diamond.h"
#include "Images/dye_powder_blue.h"
#include "Images/emerald.h"
#include "Images/gold_ingot.h"
#include "Images/iron_ingot.h"
#include "Images/netherite_scrap.h"
#include "Images/redstone_dust.h"
#include "Images/quartz.h"
#include "Images/barrel_side.h"
#include "Images/obsidian.h"

#include "Images/horse.h"
#include "Images/villager.h"
#include "Images/iron_golem.h"
#include "Images/snow_golem.h"
#include "Images/donkey.h"
#include "Images/allay.h"
#include "Images/armadillo.h"
#include "Images/axolotl.h"
#include "Images/bat.h"
#include "Images/bee.h"
#include "Images/camel.h"
#include "Images/cat.h"
#include "Images/chicken.h"
#include "Images/cod.h"
#include "Images/copper_golem.h"
#include "Images/cow.h"
#include "Images/dolphin.h"
#include "Images/fox.h"
#include "Images/frog.h"
#include "Images/glow_squid.h"
#include "Images/goat.h"
#include "Images/happy_ghast.h"
#include "Images/llama.h"
#include "Images/ocelot.h"
#include "Images/panda.h"
#include "Images/parrot.h"
#include "Images/pig.h"
#include "Images/piglin.h"
#include "Images/pufferfish.h"
#include "Images/rabbit.h"
#include "Images/salmon.h"
#include "Images/sheep.h"
#include "Images/sniffer.h"
#include "Images/squid.h"
#include "Images/strider.h"
#include "Images/tadpole.h"
#include "Images/tropical_fish.h"
#include "Images/turtle.h"
#include "Images/wandering_trader.h"
#include "Images/wolf.h"
#include "Images/zombie_piglin.h"
#include "Images/zombie_villager.h"

#include "aurora_theme.h"

// ---- new chrome ------------------------------------------------------------

static int  g_pageId        = 0;
static bool g_menuOpen      = true;
static bool g_dragWindow    = false;
static ImVec2 g_dragOffset = ImVec2(0, 0);

enum AURORA_PAGE {
    PAGE_MOVEMENT = 0,
    PAGE_COMBAT   = 1,
    PAGE_BUILDING = 2,
    PAGE_PLAYERS  = 3,
    PAGE_VISUALS  = 4,
    PAGE_HOTKEYS  = 5,
    PAGE_TWEAKS   = 6,
    PAGE_STATUS   = 7,
};

struct AuroraPageInfo { const char *icon; const char *label; const char *sub; };
static const AuroraPageInfo kPages[] = {
    { "M",  "Movement", "speed, gravity, flight" },
    { "C",  "Combat",   "aura, hitbox, autoclick" },
    { "B",  "Building", "auto mine, fast bridge" },
    { "P",  "Players",  "friends and bots" },
    { "V",  "Visuals",  "esp, xray, fov" },
    { "H",  "Hotkeys",  "keybind assignment" },
    { "T",  "Tweaks",   "market, lan, config" },
    { "S",  "Status",   "1.26.66.1 compatibility" },
};
static const int kPageCount = (int)(sizeof(kPages) / sizeof(kPages[0]));


// ---- icon textures + shared locals (hoisted from the original BeginDraw) ----
static GLint youtube_icon = 0;
static GLint telegram_icon = 0;
static GLint discord_icon = 0;
static GLint amethyst_shard_icon = 0;
static GLint coal_icon = 0;
static GLint copper_ingot_icon = 0;
static GLint diamond_icon = 0;
static GLint dye_powder_blue_icon = 0;
static GLint emerald_icon = 0;
static GLint gold_ingot_icon = 0;
static GLint iron_ingot_icon = 0;
static GLint netherite_scrap_icon = 0;
static GLint redstone_dust_icon = 0;
static GLint quartz_icon = 0;
static GLint barrel_side_icon = 0;
static GLint obsidian_icon = 0;
static GLint horse_icon = 0;
static GLint villager_icon = 0;
static GLint iron_golem_icon = 0;
static GLint snow_golem_icon = 0;
static GLint donkey_icon = 0;
static GLint allay_icon = 0;
static GLint armadillo_icon = 0;
static GLint axolotl_icon = 0;
static GLint bat_icon = 0;
static GLint bee_icon = 0;
static GLint camel_icon = 0;
static GLint cat_icon = 0;
static GLint chicken_icon = 0;
static GLint cod_icon = 0;
static GLint copper_golem_icon = 0;
static GLint cow_icon = 0;
static GLint dolphin_icon = 0;
static GLint fox_icon = 0;
static GLint frog_icon = 0;
static GLint glow_squid_icon = 0;
static GLint goat_icon = 0;
static GLint happy_ghast_icon = 0;
static GLint llama_icon = 0;
static GLint ocelot_icon = 0;
static GLint panda_icon = 0;
static GLint parrot_icon = 0;
static GLint pig_icon = 0;
static GLint piglin_icon = 0;
static GLint pufferfish_icon = 0;
static GLint rabbit_icon = 0;
static GLint salmon_icon = 0;
static GLint sheep_icon = 0;
static GLint sniffer_icon = 0;
static GLint squid_icon = 0;
static GLint strider_icon = 0;
static GLint tadpole_icon = 0;
static GLint tropical_fish_icon = 0;
static GLint turtle_icon = 0;
static GLint wandering_trader_icon = 0;
static GLint wolf_icon = 0;
static GLint zombie_piglin_icon = 0;
static GLint zombie_villager_icon = 0;

static float  imageSize   = 1.0f;
static ImVec2 endVertical = ImVec2(0, 0);

inline void AuroraInitIcons() {
    static bool once = false;
    if (once) return;
    once = true;
    youtube_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(YoutubeIcon), sizeof(YoutubeIcon));
    telegram_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(TelegramIcon), sizeof(TelegramIcon));
    discord_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(DiscordIcon), sizeof(DiscordIcon));
    amethyst_shard_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(AmethystShardIcon), sizeof(AmethystShardIcon));
    coal_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CoalIcon), sizeof(CoalIcon));
    copper_ingot_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CopperIngotIcon), sizeof(CopperIngotIcon));
    diamond_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(DiamondIcon), sizeof(DiamondIcon));
    dye_powder_blue_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(DyePowderBlueIcon), sizeof(DyePowderBlueIcon));
    emerald_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(EmeraldIcon), sizeof(EmeraldIcon));
    gold_ingot_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(GoldIngotIcon), sizeof(GoldIngotIcon));
    iron_ingot_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(IronIngotIcon), sizeof(IronIngotIcon));
    netherite_scrap_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(NetheriteScrapIcon), sizeof(NetheriteScrapIcon));
    redstone_dust_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(RedstoneDustIcon), sizeof(RedstoneDustIcon));
    quartz_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(QuartzIcon), sizeof(QuartzIcon));
    barrel_side_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(BarrelSideIcon), sizeof(BarrelSideIcon));
    obsidian_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(ObsidianIcon), sizeof(ObsidianIcon));
    horse_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(HorseIcon), sizeof(HorseIcon));
    villager_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(VillagerIcon), sizeof(VillagerIcon));
    iron_golem_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(IronGolemIcon), sizeof(IronGolemIcon));
    snow_golem_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(SnowGolemIcon), sizeof(SnowGolemIcon));
    donkey_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(DonkeyIcon), sizeof(DonkeyIcon));
    allay_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(AllayIcon), sizeof(AllayIcon));
    armadillo_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(ArmadilloIcon), sizeof(ArmadilloIcon));
    axolotl_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(AxolotlIcon), sizeof(AxolotlIcon));
    bat_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(BatIcon), sizeof(BatIcon));
    bee_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(BeeIcon), sizeof(BeeIcon));
    camel_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CamelIcon), sizeof(CamelIcon));
    cat_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CatIcon), sizeof(CatIcon));
    chicken_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(ChickenIcon), sizeof(ChickenIcon));
    cod_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CodIcon), sizeof(CodIcon));
    copper_golem_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CopperGolemIcon), sizeof(CopperGolemIcon));
    cow_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(CowIcon), sizeof(CowIcon));
    dolphin_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(DolphinIcon), sizeof(DolphinIcon));
    fox_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(FoxIcon), sizeof(FoxIcon));
    frog_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(FrogIcon), sizeof(FrogIcon));
    glow_squid_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(GlowSquidIcon), sizeof(GlowSquidIcon));
    goat_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(GoatIcon), sizeof(GoatIcon));
    happy_ghast_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(HappyGhastIcon), sizeof(HappyGhastIcon));
    llama_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(LlamaIcon), sizeof(LlamaIcon));
    ocelot_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(OcelotIcon), sizeof(OcelotIcon));
    panda_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(PandaIcon), sizeof(PandaIcon));
    parrot_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(ParrotIcon), sizeof(ParrotIcon));
    pig_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(PigIcon), sizeof(PigIcon));
    piglin_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(PiglinIcon), sizeof(PiglinIcon));
    pufferfish_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(PufferfishIcon), sizeof(PufferfishIcon));
    rabbit_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(RabbitIcon), sizeof(RabbitIcon));
    salmon_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(SalmonIcon), sizeof(SalmonIcon));
    sheep_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(SheepIcon), sizeof(SheepIcon));
    sniffer_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(SnifferIcon), sizeof(SnifferIcon));
    squid_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(SquidIcon), sizeof(SquidIcon));
    strider_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(StriderIcon), sizeof(StriderIcon));
    tadpole_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(TadpoleIcon), sizeof(TadpoleIcon));
    tropical_fish_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(TropicalfishIcon), sizeof(TropicalfishIcon));
    turtle_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(TurtleIcon), sizeof(TurtleIcon));
    wandering_trader_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(WanderingtraderIcon), sizeof(WanderingtraderIcon));
    wolf_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(WolfIcon), sizeof(WolfIcon));
    zombie_piglin_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(ZombiepiglinIcon), sizeof(ZombiepiglinIcon));
    zombie_villager_icon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(ZombievillagerIcon), sizeof(ZombievillagerIcon));
}

// ---- the shell -------------------------------------------------------------

inline void AuroraBeginDraw() {
    if (panic) return;

    AuroraInitIcons();
    imageSize   = ImGui::GetFontSize();
    endVertical = ImVec2(ImGui::GetWindowPos().x,
                         ImGui::GetWindowPos().y + ImGui::GetWindowHeight());

    ImGuiIO &io = ImGui::GetIO();
    float scale = calculateResolutionScale((int)io.DisplaySize.x, (int)io.DisplaySize.y, 2480, 1116);
    aurora::ApplyStyle(scale);

    const ImGuiViewport *vp = ImGui::GetMainViewport();
    ImVec2 winSize = ImVec2(vp->WorkSize.x / 2.5f, vp->WorkSize.y / 1.5f);
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(vp->WorkPos.x + vp->WorkSize.x * 0.5f,
               vp->WorkPos.y + vp->WorkSize.y * 0.5f),
        ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;

    if (!ImGui::Begin(OBFUSCATE("##aurora_root"), &g_menuOpen, flags)) {
        ImGui::End();
        return;
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetWindowPos();
    ImVec2 p1 = ImVec2(p0.x + ImGui::GetWindowWidth(), p0.y + ImGui::GetWindowHeight());
    float rounding = ImGui::GetStyle().WindowRounding;

    // window body
    dl->AddRectFilled(p0, p1, ImGui::GetColorU32(aurora::Bg0), rounding);
    dl->AddRect(p0, p1, ImGui::GetColorU32(aurora::Border), rounding);
    // accent hairline along the top edge
    dl->AddRectFilled(p0, ImVec2(p1.x, p0.y + 2.0f * scale),
                      ImGui::GetColorU32(aurora::Accent), rounding);

    const float sidebarW = 148.0f * scale;
    const float headerH  = 46.0f * scale;
    const float availH   = ImGui::GetWindowHeight() - headerH;

    // ---- sidebar background
    ImVec2 sb0 = ImVec2(p0.x, p0.y + headerH);
    ImVec2 sb1 = ImVec2(p0.x + sidebarW, p1.y);
    dl->AddRectFilled(sb0, sb1, ImGui::GetColorU32(aurora::Bg1));

    // ---- header (brand + close)
    ImGui::SetCursorScreenPos(ImVec2(p0.x + 16.0f * scale, p0.y + 12.0f * scale));
    ImGui::PushStyleColor(ImGuiCol_Text, aurora::Text);
    ImGui::TextUnformatted(OBFUSCATE("APOLLON"));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, aurora::Accent);
    ImGui::TextUnformatted(OBFUSCATE("REBORN"));
    ImGui::PopStyleColor();

    // drag handle: the whole header strip
    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y));
    ImGui::InvisibleButton(OBFUSCATE("##drag"), ImVec2(p1.x - p0.x - 46.0f * scale, headerH));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 d = io.MouseDelta;
        ImGui::SetWindowPos(ImVec2(p0.x + d.x, p0.y + d.y), ImGuiCond_Always);
    }

    // close button
    ImGui::SetCursorScreenPos(ImVec2(p1.x - 40.0f * scale, p0.y + 10.0f * scale));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, aurora::Danger);
    ImGui::PushStyleColor(ImGuiCol_Text, aurora::TextDim);
    if (ImGui::Button(OBFUSCATE("X"), ImVec2(28.0f * scale, 26.0f * scale))) {
        // mirror the legacy hotkey behaviour: closing the menu just hides it
        extern bool inMenu;
        inMenu = false;
    }
    ImGui::PopStyleColor(3);

    // ---- sidebar nav
    ImGui::SetCursorScreenPos(ImVec2(sb0.x + 10.0f * scale, sb0.y + 12.0f * scale));
    ImGui::BeginChild(OBFUSCATE("##nav"), ImVec2(sidebarW - 20.0f * scale, availH - 24.0f * scale), false);
    for (int i = 0; i < kPageCount; ++i) {
        ImGui::PushID(i);
        bool active = (g_pageId == i);
        ImVec2 cur = ImGui::GetCursorScreenPos();
        float rowW = ImGui::GetContentRegionAvail().x;
        float rowH = 34.0f * scale;

        if (active)
            dl->AddRectFilled(cur, ImVec2(cur.x + rowW, cur.y + rowH),
                              ImGui::GetColorU32(aurora::AccentDim),
                              ImGui::GetStyle().FrameRounding);

        if (ImGui::Selectable(OBFUSCATE("##sel"), active, ImGuiSelectableFlags_None,
                              ImVec2(rowW, rowH)))
            g_pageId = i;

        // label + subtitle drawn over the selectable
        ImGui::SetCursorScreenPos(ImVec2(cur.x + 10.0f * scale, cur.y + 4.0f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? aurora::Text : aurora::TextDim);
        ImGui::TextUnformatted(kPages[i].label);
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(cur.x + 10.0f * scale, cur.y + 19.0f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? aurora::Accent : ImVec4(1, 1, 1, 0.22f));
        ImGui::SetWindowFontScale(0.82f);
        ImGui::TextUnformatted(kPages[i].sub);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(cur.x, cur.y + rowH + 4.0f * scale));
        ImGui::PopID();
    }
    ImGui::EndChild();

    // ---- content area
    ImGui::SetCursorScreenPos(ImVec2(p0.x + sidebarW + 14.0f * scale, p0.y + headerH + 6.0f * scale));
    ImGui::BeginChild(OBFUSCATE("##content"),
                      ImVec2(ImGui::GetWindowWidth() - sidebarW - 28.0f * scale,
                             availH - 12.0f * scale),
                      false);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);

    switch (g_pageId) {
        case PAGE_MOVEMENT: {
            // ---------------- page 0 ----------------
				ImGui::PushItemWidth(calcResX * 100);
				if (ImGui::BeginTabBar(OBFUSCATE("Movements"))) {
					if (!isLANGame) {
					if (ImGui::BeginTabItem(OBFUSCATE("Speed"))) {
						ImGui::DragInt(OBFUSCATE("SpeedHack"), &speedValue, 0.05f, 1, 50);
						ImGui::SameLine();
                        ImGui::DragInt(OBFUSCATE("Water Speed"), &waterSpeedValue, 0.05f, 1, 50);
						ImGui::DragInt(OBFUSCATE("Lava Speed"), &lavaSpeedValue, 0.05f, 1, 50);
						ImGui::Checkbox(OBFUSCATE("Always Sprinting"), &alwaysSprinting);
			            ImGui::SameLine();
                        ImGui::Checkbox(OBFUSCATE("No SlowDown"), &noSlowDown);
						ImGui::Checkbox(OBFUSCATE("No Web"), &offSlowdownTrigger);
						ImGui::SameLine();
						ImGui::Checkbox(OBFUSCATE("No Jump Delay"), &noJumpDelay);
						ImGui::EndTabItem();
                    }
					}
					if (ImGui::BeginTabItem(OBFUSCATE("Other"))) {
						if (!isLANGame) {
						ImGui::DragFloat(OBFUSCATE("JumpHack"), &jumpValue, 0.05f, 1, 50, "%.0f");
						ImGui::PushItemWidth(calcResX * 300);
				        ImGui::SliderFloat(OBFUSCATE("Gravity"), &gravityValue, -0.08, 0.08, "%.2f");
                        ImGui::PushItemWidth(0);
						ImGui::Checkbox(OBFUSCATE("Fast Fall"), &fastFall);
				        ImGui::SameLine();
                        ImGui::Checkbox(OBFUSCATE("Step"), &step);
				        ImGui::Checkbox(OBFUSCATE("Ladder Fly"), &ladderFly);
				        ImGui::SameLine();
				        ImGui::Checkbox(OBFUSCATE("Noclip"), &noclip);
				        ImGui::Checkbox(OBFUSCATE("Creative Fly"), &creativeFly);
				        ImGui::SameLine();
				        ImGui::Checkbox(OBFUSCATE("Force Creative Fly"), &forceCreativeFly);
						ImGui::Checkbox(OBFUSCATE("No Water"), &offWaterTrigger);
						ImGui::SameLine();
						ImGui::Checkbox(OBFUSCATE("Strafe"), &strafe);
						DrawSeparatorToWindowEdge(endVertical, 16.0f);
						}
						ImGui::Text(OBFUSCATE("Bedrock Core Predicition Anti Cheat"));
						ImGui::Checkbox(OBFUSCATE("OFF Drowning"), &offDrown);
						ImGui::SameLine();
			        	ImGui::Checkbox(OBFUSCATE("Damage Fly\n(Sprint to control it)"), &damageFly);
						ImGui::EndTabItem();
                    }
					ImGui::EndTabBar();
                }
            break;
        }
        case PAGE_COMBAT: {
            // ---------------- page 1 ----------------
				if (ImGui::BeginTabBar(OBFUSCATE("Tab1"))) {
					if (ImGui::BeginTabItem(OBFUSCATE("Players"))) {
						ImGui::PushID(21);
				        ImGui::Checkbox(OBFUSCATE("KillAura"), &killAura);
				        if (killAura) {
                            ImGui::PushItemWidth(calcResX * 200);
                            if (ImGui::DragFloat(OBFUSCATE("Kill Range"), &killRange, 0.015f, 1.0f, 10.0f, "%.1f")) {
					            if (killRange > rotationsRange) rotationsRange = killRange;
					        }
					        if (rotations || targetLock) {
						        if(ImGui::DragFloat(OBFUSCATE("Rotations/Target Lock Range"), &rotationsRange, 0.015f, 1.0f, 100.0f, "%.1f")) {
							        if (killRange > rotationsRange) killRange = rotationsRange;
						        }
					        }
							if (!useGeyserCooldown) {
					            ImGui::DragInt(OBFUSCATE("Kill Delay"), &maxKillTick, 0.05f, 1, 150);
							    ImGui::SameLine();
							}
							ImGui::Checkbox(OBFUSCATE("Use Geyser Cooldown"), &useGeyserCooldown);
				        }
						ImGui::Checkbox(OBFUSCATE("HitBox"), &hitBox);
				        if (hitBox) {
					        ImGui::SameLine();
							ImGui::PushItemWidth(calcResX * 200);
					        ImGui::DragFloat(OBFUSCATE("##Size"), &hitBoxValue, 0.015f, 0.6f, 10.0f, "%.1f");
				        }
						ImGui::PushItemWidth(0);
				        DrawSeparatorToWindowEdge(endVertical, 16.0f);
				        ImGui::Text(OBFUSCATE("Aura Settings"));
				        ImGui::PushItemWidth(calcResX * 200);
						if (!useGeyserCooldown && !emulateNativeAttack) {
							ImGui::Combo(OBFUSCATE("Aura Type"), &auraType, AuraType, IM_ARRAYSIZE(AuraType));
						} else {
							ImGui::Combo(OBFUSCATE("Aura Type"), &auraType, AuraTypeGeyser, IM_ARRAYSIZE(AuraTypeGeyser));
							if (auraType == 2) auraType = 1;
						}
                        ImGui::PushItemWidth(0);
			            ImGui::Checkbox(OBFUSCATE("Swing"), &swing);
			            ImGui::SameLine();
						if (!emulateNativeAttack) {
							ImGui::Checkbox(OBFUSCATE("Rotations"), &rotations);
				            ImGui::Checkbox(OBFUSCATE("Target Lock"), &targetLock);
							ImGui::SameLine();
						}
						ImGui::Checkbox(OBFUSCATE("Hurt Check"), &hurtCheck);
						ImGui::Checkbox(OBFUSCATE("Simulate Native Attack\n(Split Touch Recommended)"), &emulateNativeAttack);
				        ImGui::PopID();
				        ImGui::EndTabItem();
                    }
					if (ImGui::BeginTabItem(OBFUSCATE("Mobs"))) {
						if (ImGui::BeginTabBar(OBFUSCATE("TabMobs"))) {
							ImGui::PushID(22);
					    	if (ImGui::BeginTabItem(OBFUSCATE("Main"))) {
				                ImGui::Checkbox(OBFUSCATE("MobAura"), &mobAura);
				                if (mobAura) {
					                ImGui::PushItemWidth(calcResX * 200);
                                    ImGui::DragFloat(OBFUSCATE("Kill Mob Range"), &mobAuraRange, 0.015f, 1.0f, 10.0f, "%.1f");
						    	    if (!useGeyserCooldown) {
								        ImGui::DragInt(OBFUSCATE("Kill Mob Delay"), &maxMobAuraTick, 0.05f, 1, 150);
								        ImGui::SameLine();
							        }
							        ImGui::Checkbox(OBFUSCATE("Use Geyser Cooldown"), &useGeyserCooldown);
                                    ImGui::PushItemWidth(0);
				                }
						
				                DrawSeparatorToWindowEdge(endVertical, 16.0f);
						
				                ImGui::Text(OBFUSCATE("Aura Settings"));
				                ImGui::PushItemWidth(calcResX * 200);
                                if (!useGeyserCooldown) {
							        ImGui::Combo(OBFUSCATE("Aura Type"), &auraType, AuraType, IM_ARRAYSIZE(AuraType));
						        } else {
							        ImGui::Combo(OBFUSCATE("Aura Type"), &auraType, AuraTypeGeyser, IM_ARRAYSIZE(AuraTypeGeyser));
							        if (auraType == 2) auraType = 1;
						        }
			                    ImGui::Checkbox(OBFUSCATE("Swing"), &swing);
			                    ImGui::SameLine();
			                    ImGui::Checkbox(OBFUSCATE("Rotations"), &rotations);
				                ImGui::Checkbox(OBFUSCATE("Target Lock"), &targetLock);
						        ImGui::SameLine();
						        ImGui::Checkbox(OBFUSCATE("Hurt Check"), &hurtCheck);
						        ImGui::PushItemWidth(0);
						        ImGui::EndTabItem();
                             }
						     if (ImGui::BeginTabItem(OBFUSCATE("Mob List"))) {
								ImGui::Text(OBFUSCATE("List of mobs that MobAura DOES NOT ATTACK"));
						        ImGui::BeginChild(OBFUSCATE("MobAuraPanel"), ImVec2(0, 0), true);
								ImGui::SetWindowFontScale(1.75);
				                ImGui::Checkbox(OBFUSCATE("##Horses"), &noHorses);
					            ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)horse_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Villagers"), &noVillagers);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)villager_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Snowmans"), &noSnowMan);
					            ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)snow_golem_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Iron Golems"), &noIronGolem);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)iron_golem_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Donkeys"), &noDonkeys);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)donkey_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Allays"), &noAllays);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)allay_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Armadillos"), &noArmadillos);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)armadillo_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Axolotls"), &noAxolotls);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)axolotl_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Bats"), &noBats);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)bat_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Bees"), &noBees);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)bee_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Camels"), &noCamels);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)camel_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Cats"), &noCats);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)cat_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Chickens"), &noChickens);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)chicken_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Cods"), &noCods);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)cod_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Copper Golems"), &noCopperGolems);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)copper_golem_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Cows"), &noCows);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)cow_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Dolphins"), &noDolphins);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)dolphin_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Foxes"), &noFoxes);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)fox_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Frogs"), &noFrogs);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)frog_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Glow Squids"), &noGlowSquids);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)glow_squid_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Goats"), &noGoats);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)goat_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Happy Ghasts"), &noHappyGhasts);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)happy_ghast_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Llamas"), &noLlamas);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)llama_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Ocelots"), &noOcelots);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)ocelot_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Pandas"), &noPandas);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)panda_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Parrots"), &noParrots);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)parrot_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Pigs"), &noPigs);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)pig_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Piglins"), &noPiglins);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)piglin_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Pufferfishes"), &noPufferfishes);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)pufferfish_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Rabbits"), &noRabbits);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)rabbit_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Salmons"), &noSalmons);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)salmon_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Sheeps"), &noSheeps);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)sheep_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Sniffers"), &noSniffers);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)sniffer_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Squids"), &noSquids);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)squid_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Striders"), &noStriders);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)strider_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Tadpoles"), &noTadpoles);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)tadpole_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Tropical fishes"), &noTropicalfishes);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)tropical_fish_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Turtles"), &noTurtles);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)turtle_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Wandering traders"), &noWanderingtraders);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)wandering_trader_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Wolfs"), &noWolfs);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)wolf_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::Checkbox(OBFUSCATE("##Zombie piglins"), &noZombiepiglins);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)zombie_piglin_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								ImGui::SameLine();
					            ImGui::Checkbox(OBFUSCATE("##Zombie villagers"), &noZombievillagers);
								ImGui::SameLine();
								ImGui::Image((ImTextureID)(intptr_t)zombie_villager_icon, ImVec2(imageSize * 2.25, imageSize * 2.25));
								
								ImGui::SetWindowFontScale(1.0);
						        ImGui::EndChild();
							    ImGui::EndTabItem();
                            }
							ImGui::EndTabBar();
                        }
						ImGui::PopID();
						ImGui::EndTabItem();
                    }
					if (ImGui::BeginTabItem(OBFUSCATE("Other"))) {
						ImGui::Checkbox(OBFUSCATE("Kill Near Items"), &killItems);
				        ImGui::SameLine();
				        ImGui::Checkbox(OBFUSCATE("Anti Fireball"), &antiFireball);
						ImGui::PushItemWidth(calcResX * 200);
						ImGui::Checkbox(OBFUSCATE("Auto Clicker"), &autoClicker);
						ImGui::SameLine();
						if (autoClicker) {
						    ImGui::DragInt(OBFUSCATE("Auto Click Delay"), &maxAutoClickTick, 0.05f, 1, 150);
						}
						if (!isLANGame) {
							ImGui::Checkbox(OBFUSCATE("Anti Knockback"), &antiKnockback);
						}
						ImGui::PushItemWidth(0);
						ImGui::EndTabItem();
                    }
					ImGui::EndTabBar();
                }
            break;
        }
        case PAGE_BUILDING: {
            // ---------------- page 2 ----------------
				ImGui::Checkbox(OBFUSCATE("1 Click Auto Mine"), &clickAutoMine);
                ImGui::SameLine();
                ImGui::Checkbox(OBFUSCATE("Auto Click Build (Hold Build Button)"), &fastBridge);
                ImGui::Checkbox(OBFUSCATE("Creative Block Reach"), &blockReach);
            break;
        }
        case PAGE_PLAYERS: {
            // ---------------- page 3 ----------------
				ImGui::Checkbox(OBFUSCATE("Anti Bot"), &antiBot);
				if (ImGui::BeginTabBar(OBFUSCATE("Tab3"))) {
					if (ImGui::BeginTabItem(OBFUSCATE("All"))) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
						ImGui::BeginChild(OBFUSCATE("PlayersPanel"), ImVec2(0, 0), true);
						if (!nearPlayerList.empty()) insertionSortPlayers(nearPlayerList);
			            for (PlayerStruct player : nearPlayerList) {
							if (!player.isBot) {
								if (player.isProtected) {
									ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
								} else if (player.isFriend) {
									ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 1, 1));
								} else {
									ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
								}
								ImGui::PushID(player.nickname.c_str());
								ImGuiColoredText(player.nameTag);
								if (!player.isFriend) {
                                    ImGui::SameLine();
								    if (ImGui::Button(OBFUSCATE("Add Friend"))) {
                                        friendList.push_back(player.nickname);
                                    }
						            ImGui::SameLine();
						            if (ImGui::Button(OBFUSCATE("Add Bot"))) {
                                        botList.push_back(player.nickname);
                                    }
								}
								ImGui::Separator();
				                ImGui::PopID();
							}
                        }
						ImGui::EndChild();
				        ImGui::EndTabItem();
				    }
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 1, 1));
					if (ImGui::BeginTabItem(OBFUSCATE("Friends"))) {
						ImGui::BeginChild(OBFUSCATE("FriendsPanel"), ImVec2(0, 0), true);
			            for (std::string player : friendList) {
							ImGui::PushID(player.c_str());
							ImGuiColoredText(player);
                            ImGui::SameLine();
						    if (ImGui::Button(OBFUSCATE("Remove Friend"))) {
                                friendList.erase(std::remove(friendList.begin(), friendList.end(), player), friendList.end());
                            }
						    ImGui::Separator();
				            ImGui::PopID();
                        }
						ImGui::EndChild();
				        ImGui::EndTabItem();
				    }
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
					if (ImGui::BeginTabItem(OBFUSCATE("Bots"))) {
						ImGui::BeginChild(OBFUSCATE("BotsPanel"), ImVec2(0, 0), true);
			            for (std::string bot : botList) {
							ImGui::PushID(bot.c_str());
							ImGuiColoredText(bot);
                            ImGui::SameLine();
						    if (ImGui::Button(OBFUSCATE("Remove Bot"))) {
                                botList.erase(std::remove(botList.begin(), botList.end(), bot), botList.end());
                            }
						    ImGui::Separator();
				            ImGui::PopID();
                        }
						ImGui::EndChild();
				        ImGui::EndTabItem();
				    }
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
				    ImGui::EndTabBar();
				}
            break;
        }
        case PAGE_VISUALS: {
            // ---------------- page 4 ----------------
				if (ImGui::BeginTabBar(OBFUSCATE("Tab4"))) {
					if (ImGui::BeginTabItem(OBFUSCATE("Xray"))) {
						ImGui::Checkbox(OBFUSCATE("Xray Caves"), &xrayCaves);
						ImGui::SameLine();
						ImGui::Checkbox(OBFUSCATE("Xray Camera (Third Person)"), &xrayCam);
						ImGui::BeginChild(OBFUSCATE("XrayPanel"), ImVec2(0, 0), xrayDefault);
						if (ImGui::Checkbox(OBFUSCATE("Xray"), &xrayDefault)) {
							rebuildChunkGeometry = true;
						}
						if (xrayDefault) {
							ImGui::Separator();
							if (ImGui::Checkbox(OBFUSCATE("##Diamond"), &xrayDiamond)) {
								if (xrayDiamond) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 80);
				            ImGui::Image((ImTextureID)(intptr_t)diamond_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Iron"), &xrayIron)) {
								if (xrayIron) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 110, calcResX * 110);
				            ImGui::Image((ImTextureID)(intptr_t)iron_ingot_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Gold"), &xrayGold)) {
								if (xrayGold) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 180, calcResX * 180);
				            ImGui::Image((ImTextureID)(intptr_t)gold_ingot_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));

							if (ImGui::Checkbox(OBFUSCATE("##Coal"), &xrayCoal)) {
								if (xrayCoal) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 80);
				            ImGui::Image((ImTextureID)(intptr_t)coal_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Copper"), &xrayCopper)) {
								if (xrayCopper) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 110, calcResX * 110);
				            ImGui::Image((ImTextureID)(intptr_t)copper_ingot_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Lapis"), &xrayLapis)) {
								if (xrayLapis) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 180, calcResX * 180);
				            ImGui::Image((ImTextureID)(intptr_t)dye_powder_blue_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							
							if (ImGui::Checkbox(OBFUSCATE("##Emerald"), &xrayEmerald)) {
								if (xrayEmerald) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 80);
				            ImGui::Image((ImTextureID)(intptr_t)emerald_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Redstone"), &xrayRedstone)) {
								if (xrayRedstone) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 110, calcResX * 110);
				            ImGui::Image((ImTextureID)(intptr_t)redstone_dust_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Amethyst"), &xrayAmethyst)) {
								if (xrayAmethyst) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 180, calcResX * 180);
				            ImGui::Image((ImTextureID)(intptr_t)amethyst_shard_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							
							if (ImGui::Checkbox(OBFUSCATE("##Netherite"), &xrayNetherite)) {
								if (xrayNetherite) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 80);
				            ImGui::Image((ImTextureID)(intptr_t)netherite_scrap_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Quartz"), &xrayQuartz)) {
								if (xrayQuartz) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 110, calcResX * 110);
				            ImGui::Image((ImTextureID)(intptr_t)quartz_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							ImGui::SameLine(0, calcResX * 40);
							if (ImGui::Checkbox(OBFUSCATE("##Obsidian"), &xrayObsidian)) {
								if (xrayObsidian) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 180, calcResX * 180);
				            ImGui::Image((ImTextureID)(intptr_t)obsidian_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
							
							if (ImGui::Checkbox(OBFUSCATE("##Barrel"), &xrayBarrel)) {
								if (xrayBarrel) rebuildChunkGeometry = true;
							}
							ImGui::SameLine(calcResX * 80);
				            ImGui::Image((ImTextureID)(intptr_t)barrel_side_icon, ImVec2(imageSize * 1.5, imageSize * 1.5));
						}
						ImGui::EndChild();
						ImGui::EndTabItem();
				    }
					if (ImGui::BeginTabItem(OBFUSCATE("Render"))) {
				        ImGui::PushItemWidth(calcResX * 250);
				        ImGui::Combo(OBFUSCATE("Fog Mode"), &fogState, FogStatement, IM_ARRAYSIZE(FogStatement));
						bool seeThickness;
				        if (chamsint == 1) {
							seeThickness = false;
						} else if (chamsint > 1) {
							seeThickness = true;
						}
						if (chamsint > 1) DrawSeparatorToWindowEdge(endVertical, 16.0f);
				        ImGui::PushItemWidth(calcResX * 450);
				        ImGui::Combo(OBFUSCATE("Current Chams"), &chamsint, Chams, IM_ARRAYSIZE(Chams));
				        if (chamsint > 1) {
                            ImGui::PushItemWidth(calcResX * 42);
                            ImGui::ColorEdit3(OBFUSCATE("##Visible Color"), (float *) &visibleColor);
                            ImGui::SameLine();
                            ImGui::Checkbox(OBFUSCATE("RGB"), &enableRainbow);
				        }
                        if (chamsint == 6 || chamsint == 7) {
							ImGui::SameLine();
                            ImGui::ColorEdit3(OBFUSCATE("##In Wall Color"), (float *) &inWallColor);
                            ImGui::SameLine();
                            ImGui::Checkbox(OBFUSCATE("RGB "), &enableRainbowWall);
                        }
                        ImGui::PushItemWidth(0);
				        if (chamsint > 1) DrawSeparatorToWindowEdge(endVertical, 16.0f);
						ImGui::Checkbox(OBFUSCATE("Enable FOV"), &enableFov);
                        if (enableFov) {
					        ImGui::SameLine();
                            ImGui::PushItemWidth(calcResX * 500);
                            ImGui::SliderFloat(OBFUSCATE("##FOV"), &fov, -90.0f, 90.0f, "%.0f");
                            ImGui::PushItemWidth(0);
                        }
						ImGui::Checkbox(OBFUSCATE("FullBright"), &fullBright);
				        ImGui::SameLine();
			        	ImGui::Checkbox(OBFUSCATE("OSMT FPS Boost"), &osmtBoost);
						ImGui::SameLine();
						ImGui::Checkbox(OBFUSCATE("Freelook"), &look360);
                        ImGui::Checkbox(OBFUSCATE("No Hurt Cam"), &noHurtCam);
			 	        ImGui::SameLine();
				        ImGui::Checkbox(OBFUSCATE("No Cam Distortion"), &noCamDist);
				        ImGui::Checkbox(OBFUSCATE("No Boat Rotation Limit"), &noBoatRotation);
						ImGui::SameLine();
				        ImGui::Checkbox(OBFUSCATE("No Sleep Vignette"), &noCamSleep);
                        ImGui::Checkbox(OBFUSCATE("Place Camera (Third Person)"), &placeCam);
						ImGui::SameLine();
			            ImGui::Checkbox(OBFUSCATE("Anti Invisible"), &antiInvis);
						if (!isLANGame) {
			            ImGui::Checkbox(OBFUSCATE("Derp"), &derp);
				        ImGui::SameLine();
						}
						ImGui::Checkbox(OBFUSCATE("No Blur"), &noBlur);
						ImGui::SameLine();
						ImGui::Checkbox(OBFUSCATE("Anti Blindness"), &antiBlind);
						if (antiBlind) {
							if (fogState == 0) fogState = 1;
						}
						ImGui::EndTabItem();
				}
		 	    if (ImGui::BeginTabItem(OBFUSCATE("UI"))) {
					ImGui::PushID(23);
					if (ImGui::BeginTabBar(OBFUSCATE("TabUI"))) {
						if (ImGui::BeginTabItem(OBFUSCATE("Main"))) {
					        ImGui::PushItemWidth(calcResX * 300);
			            	ImGui::SliderInt(OBFUSCATE("Font Size"), &fontSize, 0, 10);
					        ImGui::Checkbox(OBFUSCATE("Always Show Paperdoll"), &showPaperdoll);
					        ImGui::SameLine();
					        ImGui::Checkbox(OBFUSCATE("No Cave Vignette"), &noCaveVignette);
					        ImGui::Checkbox(OBFUSCATE("Death Position"), &deathPosition);
					        if (showPosition) DrawSeparatorToWindowEdge(endVertical, 16.0f);
				            ImGui::Checkbox(OBFUSCATE("Show Position"), &showPosition);
				            if (showPosition) {
                                ImGui::DragFloat(OBFUSCATE("Offset X"), &offsetTextPosX, 0.05f, -1500.0f, 1500.0f, "%.0f");
					            ImGui::DragFloat(OBFUSCATE("Offset Y"), &offsetTextPosY, 0.05f, -1000.0f, 1000.0f, "%.0f");
				            }
					        if (showPosition || playerNear) DrawSeparatorToWindowEdge(endVertical, 16.0f);
				            ImGui::Checkbox(OBFUSCATE("Near Players"), &playerNear);
				            if (playerNear) {
                                ImGui::DragFloat(OBFUSCATE("Offset X  "), &offsetTextPlayerNearPosX, 0.05f, -1500.0f, 1500.0f, "%.0f");
					            ImGui::DragFloat(OBFUSCATE("Offset Y  "), &offsetTextPlayerNearPosY, 0.05f, -1000.0f, 1000.0f, "%.0f");
					        }
					        ImGui::PushItemWidth(0);
						    ImGui::EndTabItem();
				        }
						if (ImGui::BeginTabItem(OBFUSCATE("ESP"))) {
							ImGui::Checkbox(OBFUSCATE("ESP"), &esp);
							if (esp) {
								ImGui::Checkbox(OBFUSCATE("Box"), &espBox);
								ImGui::SameLine();
								ImGui::Checkbox(OBFUSCATE("Line"), &espLine);
								ImGui::SameLine();
								ImGui::Checkbox(OBFUSCATE("Distance"), &espDistance);
								ImGui::Checkbox(OBFUSCATE("Nametag"), &espNametag);
								ImGui::SameLine();
								ImGui::Checkbox(OBFUSCATE("Health"), &espHealth);
								ImGui::SameLine();
								ImGui::Checkbox(OBFUSCATE("Platform"), &espPlatform);
							}
							ImGui::EndTabItem();
				        }
					    ImGui::EndTabBar();
                    }
					ImGui::PopID();
			        ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
            break;
        }
        case PAGE_HOTKEYS: {
            // ---------------- page 5 ----------------
				ImGui::BeginChild(OBFUSCATE("HotkeyPanel"), ImVec2(0, 0), true);
				ImGui::PushItemWidth(calcResX * 300);
				ImGui::Text(OBFUSCATE("Replace Android Buttons:"));
                ImGui::Combo(OBFUSCATE("KEYCODE_VOLUME_UP"), &hotkeyVolumeUp, Hotkeys, IM_ARRAYSIZE(Hotkeys));
				ImGui::Combo(OBFUSCATE("KEYCODE_VOLUME_DOWN"), &hotkeyVolumeDown, Hotkeys, IM_ARRAYSIZE(Hotkeys));
				ImGui::Combo(OBFUSCATE("KEYCODE_VOLUME_BACK"), &hotkeyBack, Hotkeys, IM_ARRAYSIZE(Hotkeys));
				ImGui::Separator();
				ImGui::Text(OBFUSCATE("Replace Physical Keyboard Buttons:"));
				ImGui::Combo(OBFUSCATE("KEYCODE_A"), &hotkey_A, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_B"), &hotkey_B, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_C"), &hotkey_C, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_D"), &hotkey_D, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_E"), &hotkey_E, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_F"), &hotkey_F, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_G"), &hotkey_G, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_H"), &hotkey_H, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_I"), &hotkey_I, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_J"), &hotkey_J, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_K"), &hotkey_K, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_L"), &hotkey_L, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_M"), &hotkey_M, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_N"), &hotkey_N, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_O"), &hotkey_O, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_P"), &hotkey_P, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_Q"), &hotkey_Q, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_R"), &hotkey_R, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_S"), &hotkey_S, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_T"), &hotkey_T, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_U"), &hotkey_U, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_V"), &hotkey_V, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_W"), &hotkey_W, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_X"), &hotkey_X, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_Y"), &hotkey_Y, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_Z"), &hotkey_Z, Hotkeys, IM_ARRAYSIZE(Hotkeys));
				ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_0"), &hotkey_numpad_0, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_1"), &hotkey_numpad_1, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_2"), &hotkey_numpad_2, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_3"), &hotkey_numpad_3, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_4"), &hotkey_numpad_4, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_5"), &hotkey_numpad_5, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_6"), &hotkey_numpad_6, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_7"), &hotkey_numpad_7, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_8"), &hotkey_numpad_8, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::Combo(OBFUSCATE("KEYCODE_NUMPAD_9"), &hotkey_numpad_9, Hotkeys, IM_ARRAYSIZE(Hotkeys));
                ImGui::PushItemWidth(0);
				ImGui::EndChild();
            break;
        }
        case PAGE_TWEAKS: {
            // ---------------- page 6 ----------------
				if (ImGui::BeginTabBar(OBFUSCATE("Tab2"))) {
					if (ImGui::BeginTabItem(OBFUSCATE("Exploits/Tweaks"))) {
				        ImGui::Checkbox(OBFUSCATE("Unlock Marketplace Items"), &unlockMarket);
                        ImGui::Checkbox(OBFUSCATE("Fast Item Drop"), &fastDrop);
						ImGui::SameLine();
						ImGui::Checkbox(OBFUSCATE("No Emote Use Cooldown"), &noEmoteCooldown);
						ImGui::EndTabItem();    
			    }
				if (ImGui::BeginTabItem(OBFUSCATE("LAN"))) {
				    ImGui::PushItemWidth(calcResX * 300);
				    ImGui::DragInt(OBFUSCATE("Host Max Players\n(Needs to re-enter the world)"), &myMaxPlayers, 0.05f, 1, 1000);
				    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(OBFUSCATE("Config"))) {
					ImGui::Text(OBFUSCATE("Config Load/Save Path: /sdcard/Download/Apollon.json")); 
					std::string loadPath = oxorany("/sdcard/Download/Apollon.json");
					if (ImGui::Button(OBFUSCATE("Load"))) {
					    std::ifstream file(loadPath);
                        if (file.good()){
                            loadConfig(loadPath);
                        }
						onFunctionSound = true;
		                AddNotification(oxorany("Config Loaded"), 6.0f, ImVec4(0.70f, 0.00f, 1.00f, 0.75f));
				    }
				    ImGui::SameLine();
				    if (ImGui::Button(OBFUSCATE("Save"))) {
					    saveConfig(loadPath);
						onFunctionSound = true;
		                AddNotification(oxorany("Config Saved"), 6.0f, ImVec4(0.70f, 0.00f, 1.00f, 0.75f));
				    }
					ImGui::EndTabItem();
                }
				ImGui::EndTabBar();
                }
            break;
        }
        case PAGE_STATUS: {
            // ---- the honest compatibility page -------------------------------
            aurora::SectionHeader(OBFUSCATE("BUILD"), scale);
            ImGui::Text(OBFUSCATE("Minecraft Bedrock 1.26.66.1"));
            ImGui::Text(OBFUSCATE("arm64-v8a / API 28+"));
            ImGui::Text(OBFUSCATE("Apollon Reborn v5.0"));

            aurora::SectionHeader(OBFUSCATE("VERIFIED ON THIS BUILD"), scale);
            ImGui::PushStyleColor(ImGuiCol_Text, aurora::Success);
            ImGui::TextUnformatted(OBFUSCATE("OK"));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextUnformatted(OBFUSCATE("Gravity / Water / Lava gravity"));

            ImGui::PushStyleColor(ImGuiCol_Text, aurora::Success);
            ImGui::TextUnformatted(OBFUSCATE("OK"));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextUnformatted(OBFUSCATE("Camera (hurt, distortion, blend)"));

            ImGui::PushStyleColor(ImGuiCol_Text, aurora::Success);
            ImGui::TextUnformatted(OBFUSCATE("OK"));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextUnformatted(OBFUSCATE("Movement (speed, water, lava, noclip)"));

            aurora::SectionHeader(OBFUSCATE("NOT YET AVAILABLE"), scale);
            ImGui::PushStyleColor(ImGuiCol_Text, aurora::Warning);
            ImGui::TextUnformatted(OBFUSCATE("--"));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped(OBFUSCATE(
                "Combat, ESP, Xray, Players and Hotkeys rely on game class methods "
                "(Actor, Player, GameMode) that are stripped from this build. "
                "They need a runtime vtable pass before they can work."));

            aurora::SectionHeader(OBFUSCATE("LINKS"), scale);
            static GLint ytIcon = 0, tgIcon = 0, dcIcon = 0;
            if (!ytIcon) ytIcon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(YoutubeIcon), sizeof(YoutubeIcon));
            if (!tgIcon) tgIcon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(TelegramIcon), sizeof(TelegramIcon));
            if (!dcIcon) dcIcon = CreateTextureFromMemoryPNG(const_cast<std::uint8_t*>(DiscordIcon), sizeof(DiscordIcon));
            float is = ImGui::GetFontSize();
            ImGui::Image((ImTextureID)(intptr_t)ytIcon, ImVec2(is, is));
            ImGui::SameLine(); ImGui::Text(OBFUSCATE(": @zeff_source"));
            ImGui::Image((ImTextureID)(intptr_t)tgIcon, ImVec2(is, is));
            ImGui::SameLine(); ImGui::Text(OBFUSCATE(": @zeff_cheats"));
            ImGui::Image((ImTextureID)(intptr_t)dcIcon, ImVec2(is, is));
            ImGui::SameLine(); ImGui::Text(OBFUSCATE(": discord.gg/RSC4ZrVjS8"));
            break;
        }
        default:
            break;
    }

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::End();
}

// ---- compatibility bridge --------------------------------------------------
// main.cpp calls BeginDraw(); the shell is AuroraBeginDraw(). Keeping this thin
// wrapper means the render loop, hotkeys and notification code stay untouched.
inline void BeginDraw() { AuroraBeginDraw(); }
