// ---------------------------------------------------------------------------
//  modmenu.cpp -- register Apollon's features with LeviLaunchroid's Mod Menu.
//
//  WHY THIS EXISTS
//  ---------------
//  The previous build drove its UI from
//  Java_com_mojang_minecraftpe_platforms_Platform2_step(), a JNI symbol the
//  GAME has to call by name. LeviLaunchpad dlopen()s this .so and never calls
//  JNI_OnLoad, so that function was never invoked and the ImGui frame was never
//  pumped -- the launcher reported "No GUI detected" and there was no way to
//  switch a feature on. The offsets were therefore untestable.
//
//  This file registers every feature as a first-class Mod Menu module instead.
//  Levi draws and persists the menu, toggles and sliders; we only supply
//  metadata and a callback that writes the same globals the ImGui menu used to
//  write (see variables.h). No rendering code of our own is required.
//
//  Feature state lives in the plain globals declared in variables.h, which the
//  already-running hack_thread polls every tick. So a toggle here takes effect
//  through exactly the same path the ImGui checkbox did -- including the
//  panic/`!panic` guards and the hexPatches.Modify()/Restore() pairs.
// ---------------------------------------------------------------------------

#include "modmenu.h"

#include <pl/ModMenu.hpp>

#include <android/log.h>
#include <cstdarg>
#include <string>
#include <string_view>

// The feature flags live in variables.h as plain file-scope globals. That header
// is include-order dependent (it assumes json.hpp is already included), so we
// declare only the globals we drive instead of pulling it in here.
//
// These MUST stay in sync with variables.h. If one is renamed there, this file
// fails to link, which is the loud failure we want.
extern bool noSlowDown;
extern bool noBoatRotation;
extern bool xrayCam;
extern bool placeCam;
extern bool offDrown;
extern bool offSlowdownTrigger;

extern int speedValue;
extern int waterSpeedValue;
extern int lavaSpeedValue;

extern float gravityValue;

namespace {

constexpr const char *kTag = "ApollonMenu";
constexpr const char *kModId = "apollon";

void logLine(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, kTag, fmt, ap);
    va_end(ap);
}

// --- bool toggles -----------------------------------------------------------
// Every entry here is a global from variables.h. The ImGui menu wrote these
// directly; a bool* is all the toggle needs.
struct ToggleSpec {
    const char *key;      // config key, also the module id suffix
    const char *label;    // shown in the menu
    bool *flag;           // the actual feature flag
    const char *hint;     // tooltip / description
    bool recovered;       // true when a 1.26.66.1 offset backs this feature
};

// Only features with a RECOVERED body offset are registered as working. The
// rest stay out of the menu rather than being offered as switches that do
// nothing -- a toggle that lies is worse than an absent toggle.
const ToggleSpec kToggles[] = {
    {"noSlowDown",   "No SlowDown",            &noSlowDown,   "Item-use slowdown does not slow you.",      true},
    {"noBoatRotation","No Boat Rotation Limit", &noBoatRotation,"Third-person camera ignores boat yaw limit.",true},
    {"xrayCam",      "Xray Camera (Third Person)", &xrayCam,   "Third-person camera sees through blocks.",   true},
    {"placeCam",     "Place Camera (Third Person)", &placeCam, "Free camera placement in third person.",     true},
    {"offDrown",     "Off Drowning",            &offDrown,     "No water/lava drowning damage.",             true},
    {"offSlowdownTrigger","No Web / Slowdown Trigger", &offSlowdownTrigger,
                                                               "Ignore web and soul-sand slowdown triggers.", true},
};

constexpr int kToggleCount = sizeof(kToggles) / sizeof(kToggles[0]);

// --- int sliders ------------------------------------------------------------
struct SliderSpec {
    const char *key;
    const char *label;
    int *value;
    int lo;
    int hi;
    const char *hint;
    bool recovered;
};

const SliderSpec kSliders[] = {
    {"speedValue",     "SpeedHack",  &speedValue,     1, 50, "Ground movement multiplier.",        true},
    {"waterSpeedValue","Water Speed",&waterSpeedValue,1, 50, "Horizontal speed in water.",         true},
    {"lavaSpeedValue", "Lava Speed", &lavaSpeedValue, 1, 50, "Horizontal speed in lava.",          true},
};

constexpr int kSliderCount = sizeof(kSliders) / sizeof(kSliders[0]);

// --- float sliders ----------------------------------------------------------
struct FloatSpec {
    const char *key;
    const char *label;
    float *value;
    const char *lo;
    const char *hi;
    const char *hint;
    bool recovered;
};

const FloatSpec kFloats[] = {
    {"gravityValue","Gravity",&gravityValue,"-0.08","0.08","Added to vertical motion. Negative = heavier.",true},
};

constexpr int kFloatCount = sizeof(kFloats) / sizeof(kFloats[0]);

// --- helpers ----------------------------------------------------------------

int parseInt(std::string_view s) {
    try {
        return std::stoi(std::string(s));
    } catch (...) {
        return 0;
    }
}

float parseFloat(std::string_view s) {
    try {
        return std::stof(std::string(s));
    } catch (...) {
        return 0.0f;
    }
}

const ToggleSpec *findToggle(std::string_view key) {
    for (int i = 0; i < kToggleCount; ++i)
        if (key == kToggles[i].key) return &kToggles[i];
    return nullptr;
}
const SliderSpec *findSlider(std::string_view key) {
    for (int i = 0; i < kSliderCount; ++i)
        if (key == kSliders[i].key) return &kSliders[i];
    return nullptr;
}
const FloatSpec *findFloat(std::string_view key) {
    for (int i = 0; i < kFloatCount; ++i)
        if (key == kFloats[i].key) return &kFloats[i];
    return nullptr;
}

// Every module id we register, so the toggle callback can look one up.
struct ModuleSpec {
    const char *id;
    const char *label;
    const char *hint;
    bool *flag;        // nullptr for slider-only modules
    int *ivalue;       // non-null for int sliders
    float *fvalue;     // non-null for float sliders
};

// A module is one "feature line" in the menu. A toggle and its tuning slider
// are grouped into a single module so the UI stays short and the relationship
// is obvious.
struct Group {
    const char *id;
    const char *label;
    const char *hint;
    bool *flag;         // optional
    const char *flagKey; // config key for the toggle
    int *ivalue;        // optional
    const char *ivalKey;
    int lo, hi;
    float *fvalue;      // optional
    const char *fvalKey;
    const char *flo, *fhi;
    bool defaultOn;
};

const Group kGroups[] = {
    // id                label                              hint                                                  flag               key              ival              ikey               lo hi   fval         fkey     flo    fhi   defOn
    {"movement_speed",  "SpeedHack",       "Ground movement multiplier.",           nullptr,            nullptr,          &speedValue,      "speedValue",       1, 50,  nullptr,     nullptr, nullptr,nullptr, false},
    {"movement_water",  "Water Speed",     "Horizontal speed while swimming.",      nullptr,            nullptr,          &waterSpeedValue, "waterSpeedValue",  1, 50,  nullptr,     nullptr, nullptr,nullptr, false},
    {"movement_lava",   "Lava Speed",      "Horizontal speed while in lava.",       nullptr,            nullptr,          &lavaSpeedValue,  "lavaSpeedValue",   1, 50,  nullptr,     nullptr, nullptr,nullptr, false},
    {"movement_gravity","Gravity",         "Added to vertical motion; negative is heavier.", nullptr,    nullptr,          nullptr,          nullptr,            0, 0,   &gravityValue,"gravityValue","-0.08","0.08", true},
    {"noslowdown",      "No SlowDown",     "Item use does not slow you down.",      &noSlowDown,        "noSlowDown",     nullptr,          nullptr,            0, 0,   nullptr,     nullptr, nullptr,nullptr, false},
    {"noweb",           "No Web / Slowdown Trigger","Ignore web and soul-sand slowdown.", &offSlowdownTrigger,"offSlowdownTrigger",nullptr, nullptr, 0, 0,   nullptr,     nullptr, nullptr,nullptr, false},
    {"noboatrotation",  "No Boat Rotation Limit","Third-person camera ignores boat yaw limit.", &noBoatRotation,"noBoatRotation",nullptr,nullptr,      0, 0,   nullptr,     nullptr, nullptr,nullptr, false},
    {"xraycam",         "Xray Camera (Third Person)","Camera sees through blocks in third person.", &xrayCam,"xrayCam",nullptr,nullptr,            0, 0,   nullptr,     nullptr, nullptr,nullptr, false},
    {"placecam",        "Place Camera (Third Person)","Free camera placement in third person.", &placeCam,"placeCam",nullptr,nullptr,            0, 0,   nullptr,     nullptr, nullptr,nullptr, false},
    {"offdrown",        "Off Drowning",    "No water or lava drowning damage.",     &offDrown,          "offDrown",       nullptr,          nullptr,            0, 0,   nullptr,     nullptr, nullptr,nullptr, false},
};

constexpr int kGroupCount = sizeof(kGroups) / sizeof(kGroups[0]);

void onToggle(std::string_view moduleId, bool enabled) {
    for (int i = 0; i < kGroupCount; ++i) {
        if (moduleId != kGroups[i].id) continue;
        if (kGroups[i].flag) *kGroups[i].flag = enabled;
        logLine("%s -> %s", kGroups[i].id, enabled ? "ON" : "OFF");
        return;
    }
    logLine("onToggle: unknown module '%.*s'", (int)moduleId.size(), moduleId.data());
}

void onConfigChanged(std::string_view moduleId, std::string_view key,
                     std::string_view value) {
    if (auto *s = findSlider(key)) {
        const int v = parseInt(value);
        *s->value = v < s->lo ? s->lo : (v > s->hi ? s->hi : v);
        logLine("%s = %d", s->key, *s->value);
        return;
    }
    if (auto *f = findFloat(key)) {
        *f->value = parseFloat(value);
        logLine("%s = %.3f", f->key, (double)*f->value);
        return;
    }
    if (auto *t = findToggle(key)) {
        *t->flag = (value == "true" || value == "1");
        logLine("%s = %s", t->key, *t->flag ? "true" : "false");
        return;
    }
    (void)moduleId;
}

bool registerOne(const Group &g) {
    pl::modmenu::ModuleBuilder b(g.id, g.label);
    b.modId(kModId).description(g.hint ? g.hint : "").defaultEnabled(g.defaultOn);

    if (g.flag)
        b.config(g.flagKey, "Enabled", pl::modmenu::ConfigType::Toggle,
                 *g.flag ? "true" : "false");

    if (g.ivalue) {
        b.config(g.ivalKey, g.label, pl::modmenu::ConfigType::SliderInt,
                 std::to_string(*g.ivalue), std::to_string(g.lo),
                 std::to_string(g.hi));
    }

    if (g.fvalue) {
        b.config(g.fvalKey, g.label, pl::modmenu::ConfigType::SliderFloat,
                 std::to_string(*g.fvalue), g.flo, g.fhi);
    }

    b.onToggle(&onToggle).onConfigChanged(&onConfigChanged);
    const bool ok = b.registerModule();
    logLine("register %-16s %s", g.id, ok ? "ok" : "REJECTED");
    return ok;
}

}  // namespace

int ApollonRegisterModMenu() {
    // Push current globals into the Levi config store so the menu opens showing
    // the values the mod is actually running with (they may have come from the
    // legacy /xal/config.json).
    int ok = 0;
    for (int i = 0; i < kGroupCount; ++i)
        if (registerOne(kGroups[i])) ++ok;

    logLine("mod menu ready: %d/%d modules registered", ok, kGroupCount);
    return ok;
}
