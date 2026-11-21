#pragma once
// ============================================================================
//  Aurora theme -- a modern dark accent theme for the Apollon replacement menu.
//  Self-contained: no dependency on dark_theme.h or the old palette.
// ============================================================================
#include "ImGui/imgui.h"

namespace aurora {

// Accent palette (single hue family so the UI reads as one design).
inline const ImVec4 Accent      = ImVec4(0.47f, 0.36f, 1.00f, 1.00f); // violet
inline const ImVec4 AccentDim   = ImVec4(0.47f, 0.36f, 1.00f, 0.22f);
inline const ImVec4 AccentHover = ImVec4(0.58f, 0.48f, 1.00f, 1.00f);
inline const ImVec4 AccentAct   = ImVec4(0.38f, 0.27f, 0.90f, 1.00f);

inline const ImVec4 Bg0 = ImVec4(0.055f, 0.055f, 0.075f, 0.97f); // window
inline const ImVec4 Bg1 = ImVec4(0.078f, 0.078f, 0.106f, 1.00f); // sidebar
inline const ImVec4 Bg2 = ImVec4(0.106f, 0.106f, 0.141f, 1.00f); // panel
inline const ImVec4 Bg3 = ImVec4(0.145f, 0.145f, 0.188f, 1.00f); // widget

inline const ImVec4 Text     = ImVec4(0.92f, 0.92f, 0.96f, 1.00f);
inline const ImVec4 TextDim  = ImVec4(0.58f, 0.58f, 0.66f, 1.00f);
inline const ImVec4 Border   = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
inline const ImVec4 Success  = ImVec4(0.35f, 0.85f, 0.55f, 1.00f);
inline const ImVec4 Warning  = ImVec4(0.98f, 0.72f, 0.30f, 1.00f);
inline const ImVec4 Danger   = ImVec4(0.95f, 0.36f, 0.42f, 1.00f);

// Applies the full style. Call once per frame inside the menu window scope,
// or once at init (PushStyleColor/PopStyleColor is avoided by writing the
// style directly -- ImGui style is global and the rest of the overlay does not
// depend on it).
inline void ApplyStyle(float scale) {
    ImGuiStyle &s = ImGui::GetStyle();
    s.WindowPadding      = ImVec2(0, 0);
    s.FramePadding       = ImVec2(10 * scale, 6 * scale);
    s.ItemSpacing        = ImVec2(8 * scale, 9 * scale);
    s.ItemInnerSpacing   = ImVec2(6 * scale, 6 * scale);
    s.ScrollbarSize      = 10 * scale;
    s.GrabMinSize        = 14 * scale;
    s.WindowBorderSize   = 0.0f;
    s.ChildBorderSize    = 0.0f;
    s.PopupBorderSize    = 1.0f;
    s.FrameBorderSize    = 0.0f;
    s.WindowRounding     = 12.0f * scale;
    s.ChildRounding      = 8.0f * scale;
    s.FrameRounding      = 6.0f * scale;
    s.PopupRounding      = 8.0f * scale;
    s.ScrollbarRounding  = 8.0f * scale;
    s.GrabRounding       = 6.0f * scale;
    s.TabRounding        = 6.0f * scale;
    s.WindowTitleAlign   = ImVec2(0.0f, 0.5f);

    ImVec4 *c = s.Colors;
    c[ImGuiCol_Text]                  = Text;
    c[ImGuiCol_TextDisabled]          = TextDim;
    c[ImGuiCol_WindowBg]              = Bg0;
    c[ImGuiCol_ChildBg]               = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg]               = Bg2;
    c[ImGuiCol_Border]                = Border;
    c[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]               = Bg3;
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.19f, 0.19f, 0.25f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.22f, 0.22f, 0.29f, 1.00f);
    c[ImGuiCol_TitleBg]               = Bg1;
    c[ImGuiCol_TitleBgActive]         = Bg1;
    c[ImGuiCol_TitleBgCollapsed]      = Bg1;
    c[ImGuiCol_MenuBarBg]             = Bg1;
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(1, 1, 1, 0.14f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(1, 1, 1, 0.24f);
    c[ImGuiCol_ScrollbarGrabActive]   = Accent;
    c[ImGuiCol_CheckMark]             = Accent;
    c[ImGuiCol_SliderGrab]            = Accent;
    c[ImGuiCol_SliderGrabActive]      = AccentHover;
    c[ImGuiCol_Button]                = Bg3;
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.22f, 0.36f, 1.00f);
    c[ImGuiCol_ButtonActive]          = AccentAct;
    c[ImGuiCol_Header]                = AccentDim;
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.47f, 0.36f, 1.00f, 0.32f);
    c[ImGuiCol_HeaderActive]          = Accent;
    c[ImGuiCol_Separator]             = Border;
    c[ImGuiCol_SeparatorHovered]      = AccentDim;
    c[ImGuiCol_SeparatorActive]       = Accent;
    c[ImGuiCol_ResizeGrip]            = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ResizeGripHovered]     = AccentDim;
    c[ImGuiCol_ResizeGripActive]      = Accent;
    c[ImGuiCol_Tab]                   = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TabHovered]            = AccentDim;
    c[ImGuiCol_TabActive]             = AccentDim;
    c[ImGuiCol_TabUnfocused]          = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TabUnfocusedActive]    = AccentDim;
    c[ImGuiCol_PlotLines]             = Accent;
    c[ImGuiCol_PlotLinesHovered]      = AccentHover;
    c[ImGuiCol_PlotHistogram]         = Accent;
    c[ImGuiCol_PlotHistogramHovered]  = AccentHover;
    c[ImGuiCol_TextSelectedBg]        = AccentDim;
    c[ImGuiCol_NavHighlight]          = Accent;
}

// ---- small drawing helpers ------------------------------------------------

inline void SectionHeader(const char *label, float scale) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, TextDim);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(
        p, ImVec2(p.x + w, p.y),
        ImGui::GetColorU32(Border), 1.0f);
    ImGui::Spacing();
}

// A toggle row: label on the left, switch look on the right, full-width hitbox.
inline bool Toggle(const char *label, bool *v, float scale) {
    ImGui::PushID(label);
    ImVec2 start = ImGui::GetCursorScreenPos();
    float rowH = ImGui::GetFrameHeight();
    float avail = ImGui::GetContentRegionAvail().x;

    bool changed = ImGui::Checkbox("##chk", v);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    // Rounded track behind the checkbox, drawn as the row background.
    dl->AddRectFilled(start,
                      ImVec2(start.x + avail, start.y + rowH),
                      ImGui::GetColorU32(*v ? AccentDim : ImVec4(1, 1, 1, 0.03f)),
                      ImGui::GetStyle().FrameRounding);

    // Re-draw the label on top, after the background.
    ImGui::SetCursorScreenPos(ImVec2(start.x + rowH * 0.35f, start.y));
    ImGui::PushStyleColor(ImGuiCol_Text, *v ? Text : TextDim);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + rowH + ImGui::GetStyle().ItemSpacing.y));
    ImGui::PopID();
    return changed;
}

} // namespace aurora
