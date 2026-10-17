// Dear ImGui: standalone example application for Windows API + DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_internal.h"      // ShadeVertsLinearColorGradientKeepAlpha (sidebar gradyani)
#include "imgui_anim_widgets.h"   // hover/durum animasyonu
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_freetype.h"
#include "vgui/IconsLucide.h"        // Lucide icon glyphs (licensed embedded font)
#include "vgui/menu_vgui.h"
#include "vgui/menu_vgui_color.h"
#include "vgui/menu_vgui_checkbox.h"
#include "vgui/menu_vgui_keybind.h"   // keycap widget'i + tus atamasi durumu
#include "vgui/menu_vgui_multicombo.h"
#include "vgui/menu_vgui_overlay.h"   // spectator list / keybind list panelleri
#include "vgui/menu_vgui_indicators.h" // ekran gostergeleri (sol, dikey orta)
#include "vgui/menu_binds.h"          // sag tik bind sistemi (her widget)
#include "vgui/menu_vgui_hud.h"       // watermark, bildirimler, hit/miss loglari
#include <unordered_map>
#include "vgui/menu_esp_preview.h"
#include "vgui/embedded_assets.h"     // licensed embedded icon font
#include <d3d11.h>
#include <tchar.h>

#define ALPHA    ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )
#define NO_ALPHA ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Paper & Ink light theme (ImGui::StyleColorsLight() yerine gecer)
static void SetupImGuiPaperAndInkStyle()
{
    ImGui::StyleColorsLight();      // taban: asagida set edilmeyen her rengi light-uyumlu birakir (dim overlay, nav, link vs.)

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // --- 1. Sizing & Spacing (Clean & Rigid) ---
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    // --- 2. Borders & Rounding (Technical/Drafting feel) ---
    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    // --- 3. Full Color Palette ---

    // Main Text & Background
    colors[ImGuiCol_Text] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // Deep Carbon Ink
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.94f, 1.00f); // Warm Paper
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.03f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // Clean White Popups

    // Borders & Separators
    colors[ImGuiCol_Border] = ImVec4(0.75f, 0.75f, 0.72f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.80f, 0.78f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.17f, 0.34f, 0.59f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);

    // Frames (Inputs, Checkboxes, etc)
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.92f, 0.95f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.88f, 0.92f, 1.00f);

    // Titles & Menus
    colors[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.92f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.88f, 0.88f, 0.86f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.92f, 0.92f, 0.90f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.92f, 0.92f, 0.90f, 1.00f);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.78f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.70f, 0.70f, 0.68f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.58f, 1.00f);

    // Interactables (Blueprint Blue)
    colors[ImGuiCol_CheckMark] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.17f, 0.34f, 0.59f, 0.70f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.34f, 0.59f, 0.08f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.17f, 0.34f, 0.59f, 0.20f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.34f, 0.59f, 0.35f);

    // Header (Selection in lists/trees)
    colors[ImGuiCol_Header] = ImVec4(0.17f, 0.34f, 0.59f, 0.12f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.34f, 0.59f, 0.25f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.17f, 0.34f, 0.59f, 0.40f);

    // Tables (Crucial for Light Mode)
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.90f, 0.90f, 0.88f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.75f, 0.75f, 0.72f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.85f, 0.85f, 0.82f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 0.00f, 0.00f, 0.03f);

    // Tabs (1.92 isimleri: TabActive/TabUnfocused/TabUnfocusedActive 1.90.9'da yeniden adlandirildi)
    colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.92f, 0.90f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.92f, 0.92f, 0.90f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);

    // Misc
    colors[ImGuiCol_PlotLines] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.17f, 0.34f, 0.59f, 0.70f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.17f, 0.34f, 0.59f, 0.70f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.17f, 0.34f, 0.59f, 0.25f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.17f, 0.34f, 0.59f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);

    // 1.92 ile gelen ve orijinal paletin kapsamadigi renkler
    colors[ImGuiCol_CheckboxSelectedBg] = ImVec4(0.17f, 0.34f, 0.59f, 0.12f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.17f, 0.34f, 0.59f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.17f, 0.34f, 0.59f, 0.50f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.17f, 0.34f, 0.59f, 0.80f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.17f, 0.34f, 0.59f, 0.35f);
    colors[ImGuiCol_TextLink] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.75f, 0.75f, 0.72f, 1.00f);
    colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.17f, 0.34f, 0.59f, 0.15f);
    colors[ImGuiCol_UnsavedMarker] = ImVec4(0.17f, 0.34f, 0.59f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.17f, 0.34f, 0.59f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_CustomButtonOne] = ImVec4(0.62f, 0.62f, 0.60f, 1.00f);   // VGUI buton kabartmasi (koyu kenar)
    colors[ImGuiCol_CustomButtonTwo] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);   // VGUI buton kabartmasi (acik kenar)
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

#ifdef IMGUI_HAS_DOCK
    colors[ImGuiCol_DockingPreview] = ImVec4(0.17f, 0.34f, 0.59f, 0.40f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
#endif
}

// GoldSource / VGUI (Half-Life) temasi
namespace ImGui { IMGUI_API void StyleColorsGoldSourceVGUI(ImGuiStyle* dst = nullptr); }


void ImGui::StyleColorsGoldSourceVGUI(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImGui::StyleColorsDark(style);      // taban: asagida set edilmeyen 1.92 renkleri koyu kalsin
    ImVec4* colors = style->Colors;

    //style->WindowRounding = 0.f;
    style->WindowBorderSize = 1.f;
    style->PopupBorderSize = 1.f;
    style->FrameBorderSize = .0f;
    style->SelectableTextAlign = ImVec2(0.f, 0.f);

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.298f, 0.345f, 0.266f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.24f, 0.27f, 0.215f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.13f, 0.17f, 0.11f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.298f, 0.345f, 0.266f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.44f, 0.34f, 1.00f);   // VGUI: hover acilir, koyulasmaz
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.46f, 0.53f, 0.41f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.54f, 0.42f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);  // altin tutamac
    colors[ImGuiCol_Button] = ImVec4(0.298f, 0.345f, 0.266f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.48f, 0.37f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
    colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.19f, 0.23f, 0.18f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);          // eski ad: ImGuiCol_TabActive
    colors[ImGuiCol_TabDimmed] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);            // eski ad: ImGuiCol_TabUnfocused
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);    // eski ad: ImGuiCol_TabUnfocusedActive
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);            // eski ad: ImGuiCol_NavHighlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // 1.92 ile gelen ve orijinal paletin kapsamadigi renkler (VGUI yesil/altin paletine uyarlandi)
    colors[ImGuiCol_CheckboxSelectedBg] = ImVec4(0.24f, 0.27f, 0.215f, 1.00f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.59f, 0.54f, 0.18f, 0.35f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.19f, 0.23f, 0.18f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextLink] = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
    colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.73f, 0.67f, 0.24f, 0.15f);
    colors[ImGuiCol_UnsavedMarker] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
    colors[ImGuiCol_CustomButtonOne] = ImVec4(0.15f, 0.18f, 0.133f, 1.00f);  // VGUI buton kabartmasi (koyu kenar)
    colors[ImGuiCol_CustomButtonTwo] = ImVec4(0.53f, 0.57f, 0.50f, 1.00f);   // VGUI buton kabartmasi (acik kenar)

    const auto rgb = [](int r, int g, int b) { return ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f); };
    colors[ImGuiCol_WindowBg] = colors[ImGuiCol_PopupBg] = rgb(76, 88, 68);
    colors[ImGuiCol_ChildBg] = rgb(62, 70, 55);
    colors[ImGuiCol_Text] = rgb(216, 222, 211);
    colors[ImGuiCol_TextDisabled] = rgb(160, 170, 149);
    colors[ImGuiCol_FrameBg] = rgb(62, 70, 55);
    colors[ImGuiCol_Header] = colors[ImGuiCol_HeaderHovered] = colors[ImGuiCol_HeaderActive] = rgb(149, 136, 49);
    colors[ImGuiCol_TextSelectedBg] = rgb(149, 136, 49);
    colors[ImGuiCol_Border] = rgb(136, 145, 128);
    colors[ImGuiCol_BorderShadow] = rgb(40, 46, 34);
    colors[ImGuiCol_CheckMark] = rgb(255, 255, 255);
    colors[ImGuiCol_TextLink] = rgb(196, 181, 80);
    style->WindowPadding = ImVec2(14, 12);
    style->FramePadding = ImVec2(3, 6);
    style->ItemSpacing = ImVec2(10, 10);
    style->ItemInnerSpacing = ImVec2(10, 6);
    style->WindowRounding = style->ChildRounding = style->FrameRounding = style->PopupRounding = 0;
    style->GrabRounding = style->ScrollbarRounding = 0;
    style->ScrollbarSize = 26;

#ifdef IMGUI_HAS_DOCK
    colors[ImGuiCol_DockingPreview] = ImVec4(0.59f, 0.54f, 0.18f, 0.40f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.24f, 0.27f, 0.215f, 1.00f);
#endif
}

// Tema secici
enum AppTheme { AppTheme_Dark, AppTheme_PaperAndInk, AppTheme_GoldSourceVGUI };
static const char* const g_ThemeNames[] = { "Dark", "Paper & Ink (light)", "GoldSource VGUI" };
static int g_Theme = AppTheme_GoldSourceVGUI;

static void ApplyTheme(int theme, float main_scale)
{
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();                       // temiz taban: onceki temanin padding/rounding kalintisi kalmasin
    switch (theme)
    {
    case AppTheme_PaperAndInk:      SetupImGuiPaperAndInkStyle(); break;
    case AppTheme_GoldSourceVGUI:   ImGui::StyleColorsGoldSourceVGUI(); break;
    default:                        ImGui::StyleColorsDark(); break;
    }
    MenuVgui::ReferencePalette = theme == AppTheme_GoldSourceVGUI;
    style.ScaleAllSizes(main_scale);            // ImGuiStyle() sifirladigi icin DPI olcegini her temada yeniden uygula
    style.FontScaleDpi = main_scale;
}

// --- Windows Segoe UI text font with the licensed Lucide icon font merged in ---
static ImFont* g_FontRegular = nullptr;
static ImFont* g_FontBold = nullptr;
static bool    g_HasIcons = false;

static ImFont* LoadMenuFont(ImGuiIO& io)
{
    static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };

    ImFontConfig base_cfg;
    base_cfg.SizePixels = 20.0f;
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", base_cfg.SizePixels, &base_cfg);
    if (font == nullptr)
        font = io.Fonts->AddFontDefault(&base_cfg);
    if (font == nullptr)
        return nullptr;

    ImFontConfig icon_cfg;
    icon_cfg.SizePixels = font->LegacySize;
    icon_cfg.MergeMode = true;
    icon_cfg.PixelSnapH = true;
    icon_cfg.GlyphRanges = icon_ranges;
    icon_cfg.GlyphOffset = ImVec2(0.0f, 3.0f);
    icon_cfg.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoAutoHint;
    ImFormatString(icon_cfg.Name, IM_ARRAYSIZE(icon_cfg.Name), "Lucide icons");
    const EmbeddedAssets::Blob icons = EmbeddedAssets::LucideFont();
    g_HasIcons = io.Fonts->AddFontFromMemoryCompressedTTF(icons.Data, (int)icons.Size, 0.0f, &icon_cfg) != nullptr;
    return font;
}

static void LoadMenuFonts(ImGuiIO& io)
{
    g_FontRegular = LoadMenuFont(io);
    if (g_FontRegular == nullptr)
        g_FontRegular = io.Fonts->AddFontDefault();
    g_FontBold = g_FontRegular;
    io.FontDefault = g_FontRegular;
}

// --- Menu: ust barda marka + tablar, altta icerik ---

namespace MenuStyle
{
    static const ImU32 Online = IM_COL32(74, 222, 128, 255);        // durum noktasi
}

enum MenuPage { MenuPage_Rage, MenuPage_Legit, MenuPage_Visuals, MenuPage_Inventory, MenuPage_Misc, MenuPage_Config, MenuPage_COUNT };
struct MenuTabDesc { const char* Label; const char* Icon; int Page; };
static const MenuTabDesc g_MenuTabs[MenuPage_COUNT] =
{
    { "Rage",          ICON_LC_CROSSHAIR, MenuPage_Rage },
    { "Legit",         ICON_LC_MOUSE,     MenuPage_Legit },
    { "Visuals",       ICON_LC_EYE,       MenuPage_Visuals },
    { "Inventory",     ICON_LC_IMAGE,   MenuPage_Inventory },
    { "Miscellaneous", ICON_LC_SLIDERS,   MenuPage_Misc },
    { "Config",        ICON_LC_SAVE,   MenuPage_Config },
};
static int  g_MenuPage = MenuPage_Rage;
static float g_MenuTabReveal[MenuPage_COUNT] = { 0.0f };
static bool g_CenterMenuRequest = false;      // "Menuyu ortala" butonu
static bool g_MenuOpen = true;                // INSERT: menu ac/kapa (kapaliyken paneller fare girdisi almaz)

// --- Ozellik durumlari: menu, bind sistemi, keybind list ve ekran gostergeleri ayni degiskenleri okur ---
// Bindler (sag tik) aktifken bu degiskenlere bindin degeri yazilir; ozellik kodu dogrudan bunlari okur.
static bool  g_RageEnabled = true;
static float g_HitChance = 60.0f;
static float g_MinDamage = 70.0f;
static bool  g_ForceShoot = false;
static bool  g_ForceBodyAim = false;
static bool  g_NoSpread = false;
static bool  g_SelfTrack = false;
static bool  g_HideShots = false;
static int   g_ForceSide = 0;
static bool  g_QuickPeek = false;
static bool  g_DuckPeek = false;
static bool  g_SlowWalk = false;
static bool  g_LegitEnabled = true;
static bool  g_Triggerbot = false;
static bool  g_Thirdperson = false;
static bool  g_BunnyHop = false;
static const char* const g_ForceSides[] = { "None", "Left", "Right" };

enum ScreenIndicator
{
    Indicator_MinDamage, Indicator_HitChance, Indicator_ForceShoot, Indicator_ForceBodyAim, Indicator_NoSpread, Indicator_SelfTrack,
    Indicator_HideShots, Indicator_ForceSide, Indicator_QuickPeek, Indicator_DuckPeek, Indicator_SlowWalk, Indicator_COUNT
};
static const char* const g_IndicatorNames[Indicator_COUNT] =
{
    "Min damage", "Hit chance", "Force shoot", "Force body aim", "No spread", "Self track",
    "Hide shots", "Force left / right", "Quick peek", "Duck peek", "Slow walk"
};
static int g_IndicatorMask = (1 << Indicator_MinDamage) | (1 << Indicator_HitChance) | (1 << Indicator_ForceBodyAim) |
                             (1 << Indicator_HideShots) | (1 << Indicator_ForceSide) | (1 << Indicator_QuickPeek) |
                             (1 << Indicator_DuckPeek) | (1 << Indicator_SlowWalk);

// Ornek bindler (ilk acilis); config sistemi gelince oradan yuklenir. Etiketler widget'lardakiyle ayni.
static void RegisterDefaultBinds()
{
    using MenuVgui::BindMode;
    MenuBinds::AddBind(MenuBinds::DeclareSlider(&g_MinDamage, "Minimum Damage", 0.0f, 100.0f, "%.0f"), ImGuiKey_MouseX1, BindMode::Hold, MenuBinds::FloatValue(40.0f));
    MenuBinds::AddBind(MenuBinds::DeclareBool(&g_ForceBodyAim, "Force body aim"), ImGuiKey_Z, BindMode::Toggle, MenuBinds::BoolValue(true));
    MenuBinds::AddBind(MenuBinds::DeclareBool(&g_QuickPeek, "Quick peek"), ImGuiKey_MouseX2, BindMode::Hold, MenuBinds::BoolValue(true));
    MenuBinds::AddBind(MenuBinds::DeclareBool(&g_SlowWalk, "Slow walk"), ImGuiKey_LeftShift, BindMode::Hold, MenuBinds::BoolValue(true));
    MenuBinds::AddBind(MenuBinds::DeclareBool(&g_Triggerbot, "Triggerbot"), ImGuiKey_LeftAlt, BindMode::Hold, MenuBinds::BoolValue(true));
    MenuBinds::AddBind(MenuBinds::DeclareBool(&g_Thirdperson, "Thirdperson"), ImGuiKey_V, BindMode::Toggle, MenuBinds::BoolValue(true));
    MenuBinds::AddBind(MenuBinds::DeclareBool(&g_BunnyHop, "Bunny hop"), ImGuiKey_Space, BindMode::Hold, MenuBinds::BoolValue(true));
}

// --- Oyun ici paneller ---
static bool g_ShowSpectatorList = true;
static bool g_ShowKeybindList = true;
static bool g_KeybindListInactive = true;     // false: sadece o an deger yazan bind'ler
static MenuVgui::OverlayList g_SpectatorList;
static MenuVgui::OverlayList g_KeybindList;
static MenuVgui::IndicatorStack g_Indicators;

// --- HUD: watermark (sag ust), bildirimler + hit/miss loglari (sol ust) ---
enum WatermarkItem { Watermark_User, Watermark_Fps, Watermark_Ping, Watermark_Tick, Watermark_Time, Watermark_COUNT };
static const char* const g_WatermarkItemNames[Watermark_COUNT] = { "User", "FPS", "Ping", "Tickrate", "Time" };
static bool  g_ShowWatermark = true;
static int   g_WatermarkMask = (1 << Watermark_COUNT) - 1;
static bool  g_ShowNotifications = true;
static float g_NotifyDuration = 4.0f;
static bool  g_ShowHitLogs = true;
static bool  g_LogMisses = true;
static float g_HitLogDuration = 6.0f;
static const char g_UserName[] = "kdt";
static MenuVgui::Watermark         g_Watermark;
static MenuVgui::NotificationStack g_Notifications;
static MenuVgui::HitLog            g_HitLog;

static void Notify(MenuVgui::NotifyKind kind, const char* title, const char* text)
{
    if (g_ShowNotifications)
        g_Notifications.Push(kind, title, text, g_NotifyDuration);
}

// Toggle modundaki bindlerin durum degisimi bildirim olarak duser (Hold her basista spam yapacagi icin haric).
static void NotifyBindToggles()
{
    static std::unordered_map<ImGuiID, bool> previous;
    for (const std::unique_ptr<MenuBinds::Element>& e : MenuBinds::Elements())
        for (const MenuBinds::Bind& b : e->Binds)
        {
            if (b.Key.Mode != MenuVgui::BindMode::Toggle) continue;
            auto it = previous.find(b.Serial);
            if (it == previous.end()) { previous.emplace(b.Serial, b.On); continue; }
            if (it->second == b.On) continue;
            it->second = b.On;
            char key[32], value[32], text[96];
            MenuVgui::KeyName(b.Key.Key, key, sizeof(key));
            if (e->Type == MenuBinds::ValueType::Bool && b.V.B)
                ImFormatString(text, IM_ARRAYSIZE(text), "%s via %s", b.On ? "Enabled" : "Disabled", key);
            else
            {
                MenuBinds::FormatValue(*e, b.V, value, sizeof(value));
                ImFormatString(text, IM_ARRAYSIZE(text), b.On ? "Set to %s via %s" : "Restored (%s, %s)", value, key);
            }
            Notify(b.On ? MenuVgui::NotifyKind::Success : MenuVgui::NotifyKind::Info, e->Label, text);
        }
}

// Demo olaylari: CS2'de player_hurt (vurus) ve weapon_fire + bullet_impact eslesmeyen atis (iska) olaylarindan uretilir.
static unsigned DemoRandom()
{
    static unsigned state = 0x9E3779B9u;
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}

static void LogDemoHit()
{
    static const char* const targets[] = { "kdt255", "nightowl", "BOT Albert", "vertigo_main" };
    static const char* const hitboxes[] = { "head", "chest", "stomach", "left arm", "right leg" };
    const char* target = targets[DemoRandom() % IM_ARRAYSIZE(targets)];
    const int hitbox = (int)(DemoRandom() % IM_ARRAYSIZE(hitboxes));
    const int damage = hitbox == 0 ? 85 + (int)(DemoRandom() % 30) : 18 + (int)(DemoRandom() % 40);
    const int remaining = ImMax(0, 100 - damage);
    MenuVgui::LogLine line;
    line.Add(MenuVgui::LogTone::Text, "Hit ").Add(MenuVgui::LogTone::Gold, "%s", target)
        .Add(MenuVgui::LogTone::Muted, " in the ").Add(MenuVgui::LogTone::Text, "%s", hitboxes[hitbox])
        .Add(MenuVgui::LogTone::Muted, " for ").Add(MenuVgui::LogTone::Gold, "%d", damage);
    if (remaining > 0)
        line.Add(MenuVgui::LogTone::Muted, " (%d hp left)", remaining);
    else
        line.Add(MenuVgui::LogTone::Muted, " (").Add(MenuVgui::LogTone::Good, "dead").Add(MenuVgui::LogTone::Muted, ")");
    line.Add(MenuVgui::LogTone::Muted, "  hc %d%%  bt %dt", 55 + (int)(DemoRandom() % 40), (int)(DemoRandom() % 6));
    g_HitLog.Push(true, line, g_HitLogDuration);
}

static void LogDemoMiss()
{
    if (!g_LogMisses) return;
    static const char* const targets[] = { "kdt255", "nightowl", "BOT Albert", "vertigo_main" };
    static const char* const reasons[] = { "spread", "resolver", "prediction error", "occlusion", "death" };
    MenuVgui::LogLine line;
    line.Add(MenuVgui::LogTone::Text, "Missed ").Add(MenuVgui::LogTone::Gold, "%s", targets[DemoRandom() % IM_ARRAYSIZE(targets)])
        .Add(MenuVgui::LogTone::Muted, " due to ").Add(MenuVgui::LogTone::Bad, "%s", reasons[DemoRandom() % IM_ARRAYSIZE(reasons)])
        .Add(MenuVgui::LogTone::Muted, "  hc %d%%", 40 + (int)(DemoRandom() % 50));
    g_HitLog.Push(false, line, g_HitLogDuration);
}

static void RenderHud()
{
    ImGuiIO& io = ImGui::GetIO();
    const float size = MenuVgui::IndicatorStack::ScreenFontSize();
    const float u = size / 24.0f;

    // FPS: yarim saniyede bir guncellenir (her frame degisen sayi okunmaz)
    static float fps_shown = 0.0f, fps_timer = 0.0f;
    fps_timer -= io.DeltaTime;
    if (fps_timer <= 0.0f) { fps_shown = io.Framerate; fps_timer = 0.5f; }

    MenuVgui::WatermarkSegment segments[Watermark_COUNT];
    int count = 0;
    const auto add = [&](int item, const char* icon, const char* fmt, auto... args)
    {
        if (!(g_WatermarkMask & (1 << item))) return;
        segments[count].Icon = icon;
        ImFormatString(segments[count].Text, IM_ARRAYSIZE(segments[count].Text), fmt, args...);
        ++count;
    };
    SYSTEMTIME now = {};
    ::GetLocalTime(&now);
    add(Watermark_User, ICON_LC_USER, "%s", g_UserName);
    add(Watermark_Fps, ICON_LC_GAUGE, "%d fps", (int)(fps_shown + 0.5f));
    add(Watermark_Ping, ICON_LC_WIFI, "%d ms", 12);                 // CS2: CNetworkGameClient latency
    add(Watermark_Tick, ICON_LC_SERVER, "%d tick", 64);             // CS2: 1 / interval_per_tick
    add(Watermark_Time, ICON_LC_CLOCK, "%02d:%02d:%02d", now.wHour, now.wMinute, now.wSecond);
    g_Watermark.Draw(g_FontBold, g_FontRegular, size, ICON_LC_CROSSHAIR, "killark.cc", segments, count, g_ShowWatermark);

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float margin = IM_ROUND(8.0f * u);
    const ImVec2 origin(vp->WorkPos.x + margin, vp->WorkPos.y + margin);
    const float notes_bottom = g_Notifications.Draw(g_FontBold, g_FontRegular, size, origin, g_ShowNotifications);
    const float logs_top = notes_bottom > origin.y + 0.5f ? notes_bottom + IM_ROUND(4.0f * u) : origin.y;
    g_HitLog.Draw(g_FontRegular, size, ImVec2(origin.x, logs_top), g_Indicators.Top() - IM_ROUND(10.0f * u), g_ShowHitLogs);
}

// Demo verisi: CS2'de bunun yerine entity dongusu doldurur; yalniz bizi izleyenler
// (m_pObserverServices->m_hObserverTarget == local pawn) ve modlari (m_iObserverMode: in-eye / chase).
static void FeedDemoSpectators(MenuVgui::OverlayList& list)
{
    struct DemoSpectator { const char* Name; const char* Mode; const char* Icon; bool FirstPerson; float Phase; };
    static const DemoSpectator spectators[] =
    {
        { "kdt255",     "first person", ICON_LC_EYE,   true,  0.0f },
        { "nightowl",   "third person", ICON_LC_VIDEO, false, 3.0f },
        { "BOT Albert", "first person", ICON_LC_EYE,   true,  6.0f },
    };
    const float t = (float)ImGui::GetTime();
    for (const DemoSpectator& sp : spectators)
        if (ImFmod(t + 12.0f - sp.Phase, 12.0f) < 4.5f)            // 12 sn'lik dongu: sirayla izler, sonda 1.5 sn kimse yok
            list.Add(ImHashStr(sp.Name), sp.Name, sp.Mode, nullptr, sp.Icon,
                     sp.FirstPerson ? MenuVgui::RowTone::Active : MenuVgui::RowTone::Normal);
}

static void FeedKeybinds(MenuVgui::OverlayList& list)
{
    for (const std::unique_ptr<MenuBinds::Element>& e : MenuBinds::Elements())
        for (const MenuBinds::Bind& b : e->Binds)
        {
            if (!g_KeybindListInactive && !b.Applied) continue;
            char key[32], value[32] = "";
            if (b.Key.Key == ImGuiKey_None) ImStrncpy(key, "-", IM_ARRAYSIZE(key));
            else MenuVgui::KeyName(b.Key.Key, key, sizeof(key));
            if (e->Type != MenuBinds::ValueType::Bool || !b.V.B)    // "acar" bindinde deger yazmaya gerek yok
                MenuBinds::FormatValue(*e, b.V, value, sizeof(value));
            const MenuVgui::RowTone tone = b.Applied ? MenuVgui::RowTone::Active : b.On ? MenuVgui::RowTone::Normal : MenuVgui::RowTone::Dim;
            list.Add(b.Serial, e->Label, MenuVgui::BindModeName(b.Key.Mode), key, nullptr, tone, value);
        }
}

// Ekranin solunda, dikey ortada. Background draw list: menunun ve panellerin arkasinda.
static void RenderIndicators(float main_scale)
{
    const auto enabled = [](int indicator) { return (g_IndicatorMask & (1 << indicator)) != 0; };
    char value[16];
    g_Indicators.BeginFrame();
    if (enabled(Indicator_MinDamage))
    {
        ImFormatString(value, IM_ARRAYSIZE(value), "%.0f", g_MinDamage);
        g_Indicators.Add(ImHashStr("ind_min_damage"), ICON_LC_HEART_CRACK, "Min Damage", value, MenuBinds::IsOverriding(&g_MinDamage));
    }
    if (enabled(Indicator_HitChance))
    {
        ImFormatString(value, IM_ARRAYSIZE(value), "%.0f%%", g_HitChance);
        g_Indicators.Add(ImHashStr("ind_hit_chance"), ICON_LC_CROSSHAIR, "Hit Chance", value, MenuBinds::IsOverriding(&g_HitChance));
    }
    struct Flag { int Indicator; const bool* State; const char* Icon; const char* Label; };
    static const Flag flags[] =
    {
        { Indicator_ForceShoot,   &g_ForceShoot,   ICON_LC_ZAP,             "Force Shoot" },
        { Indicator_ForceBodyAim, &g_ForceBodyAim, ICON_LC_PERSON_STANDING, "Body Aim" },
        { Indicator_NoSpread,     &g_NoSpread,     ICON_LC_FOCUS,           "No Spread" },
        { Indicator_SelfTrack,    &g_SelfTrack,    ICON_LC_ROUTE,           "Self Track" },
        { Indicator_HideShots,    &g_HideShots,    ICON_LC_EYE_OFF,         "Hide Shots" },
    };
    for (const Flag& f : flags)
        if (enabled(f.Indicator) && *f.State)
            g_Indicators.Add(ImHashStr(f.Label), f.Icon, f.Label);
    if (enabled(Indicator_ForceSide) && g_ForceSide != 0)
        g_Indicators.Add(ImHashStr("ind_force_side"), ICON_LC_ARROW_LEFT_RIGHT, "Force Side", g_ForceSide == 1 ? "Left" : "Right");
    static const Flag movement[] =
    {
        { Indicator_QuickPeek, &g_QuickPeek, ICON_LC_UNDO_2,        "Quick Peek" },
        { Indicator_DuckPeek,  &g_DuckPeek,  ICON_LC_CHEVRONS_DOWN, "Duck Peek" },
        { Indicator_SlowWalk,  &g_SlowWalk,  ICON_LC_FOOTPRINTS,    "Slow Walk" },
    };
    for (const Flag& f : movement)
        if (enabled(f.Indicator) && *f.State)
            g_Indicators.Add(ImHashStr(f.Label), f.Icon, f.Label);
    IM_UNUSED(main_scale);                          // gosterge boyutu menu olcegi degil ekran yuksekligi izler
    g_Indicators.Draw(g_FontBold, MenuVgui::IndicatorStack::ScreenFontSize());
}

// Menuden once cizilir: NoBringToFrontOnFocus ile menunun arkasinda kalir (HUD katmani).
static void RenderOverlays(float main_scale)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float right = vp->WorkPos.x + vp->WorkSize.x - 16.0f * main_scale;

    // Ilk acilista watermark'in altina (watermark: ust bosluk 8u + yukseklik font + 12u, u = gosterge olcegi)
    const float hud = MenuVgui::IndicatorStack::ScreenFontSize();
    const float watermark_bottom = vp->WorkPos.y + IM_ROUND(8.0f * hud / 24.0f) + IM_ROUND(hud + 12.0f * hud / 24.0f);

    MenuVgui::OverlayStyle spectators;
    spectators.WindowName = "##spectators";                     // eski kayitli konum (watermark'in yeri) devralinmasin
    spectators.Title = "Spectators";
    spectators.Icon = ICON_LC_EYE;
    spectators.EmptyText = "No spectators";
    spectators.TitleFont = g_FontBold;
    spectators.DefaultPos = ImVec2(right, watermark_bottom + 10.0f * main_scale);
    spectators.DefaultPivot = ImVec2(1.0f, 0.0f);
    spectators.MinWidth = 260.0f;
    g_SpectatorList.BeginFrame();
    FeedDemoSpectators(g_SpectatorList);
    g_SpectatorList.Draw(spectators, g_ShowSpectatorList, g_MenuOpen, main_scale);   // menu acik ya da biri bizi izliyorsa

    MenuVgui::OverlayStyle keybinds;
    keybinds.WindowName = "##keybind_list";
    keybinds.Title = "Keybinds";
    keybinds.Icon = ICON_LC_KEYBOARD;
    keybinds.EmptyText = "No binds";
    keybinds.TitleFont = g_FontBold;
    keybinds.DefaultPos = ImVec2(right, vp->WorkPos.y + 280.0f * main_scale);
    keybinds.DefaultPivot = ImVec2(1.0f, 0.0f);
    keybinds.MinWidth = 260.0f;
    g_KeybindList.BeginFrame();
    FeedKeybinds(g_KeybindList);
    // Menu acikken ya da tusla tetiklenen bir bind basili/acikken gorunur
    g_KeybindList.Draw(keybinds, g_ShowKeybindList && (g_MenuOpen || MenuBinds::AnyKeyActive()), g_MenuOpen, main_scale);
}

// VGUI/konsol tarzi cokme (sunken) cerceve: koyu zemin, ust+sol koyu kenar, alt+sag acik kenar.
static void DrawSunkenFrame(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1, const ImVec4& base)
{
    if (MenuVgui::ReferencePalette)
    {
        MenuVgui::Bevel(dl, ImRect(p0, p1), MenuVgui::Color(MenuVgui::Field));
        return;
    }
    const float lum = base.x * 0.299f + base.y * 0.587f + base.z * 0.114f;
    const float k = (lum > 0.5f) ? 0.92f : 0.70f;               // acik temada daha az koyulastir
    const ImU32 fill  = ImGui::GetColorU32(ImVec4(base.x * k, base.y * k, base.z * k, 1.00f));
    const ImU32 dark  = ImGui::GetColorU32(ImVec4(base.x * k * 0.62f, base.y * k * 0.62f, base.z * k * 0.62f, 1.00f));
    const ImU32 light = ImGui::GetColorU32(ImVec4(ImMin(base.x * 1.40f, 1.00f), ImMin(base.y * 1.40f, 1.00f), ImMin(base.z * 1.40f, 1.00f), 1.00f));

    dl->AddRectFilled(p0, p1, fill);
    dl->AddLine(ImVec2(p0.x, p0.y + 0.5f), ImVec2(p1.x - 1.0f, p0.y + 0.5f), dark);          // ust
    dl->AddLine(ImVec2(p0.x + 0.5f, p0.y), ImVec2(p0.x + 0.5f, p1.y - 1.0f), dark);          // sol
    dl->AddLine(ImVec2(p0.x, p1.y - 0.5f), ImVec2(p1.x, p1.y - 0.5f), light);                // alt
    dl->AddLine(ImVec2(p1.x - 0.5f, p0.y), ImVec2(p1.x - 0.5f, p1.y), light);                // sag
}

// Gruplar arasi bosluk - cerceve olmadigi icin ayrimi tek basina bu tasiyor
static float MenuColumnGap(float main_scale) { return 32.0f * main_scale; }

// Icerik alaninda n sutunluk kutu genisligi
static float MenuColumnWidth(int columns, float main_scale)
{
    const float gap = MenuColumnGap(main_scale);
    return (ImGui::GetContentRegionAvail().x - gap * (float)(columns - 1)) / (float)columns;
}

// --- Grup: oyma VGUI GroupBox ---
//   ┌ AIMBOT ─────────────────────┐   baslik ust kenarin uzerinde (soluk, kucuk, harf araligi acik buyuk harf),
//   │  [ ] Enabled   Master switch │   cerceve oyma (koyu dista + acik 1px iceride), zemin yok, icerik 12px iceride.
//   └──────────────────────────────┘   Basliksiz grup: cercevesiz, icerik sutun kenarina hizali.
struct MenuGroupState
{
    bool  Framed = false;
    float CaptionStart = 0.0f;      // ust kenardaki baslik boslugu
    float CaptionEnd = 0.0f;
    float CaptionY = 0.0f;          // baslik satirinin dikey ortasi = ust kenarin y'si
    float Scale = 1.0f;
};
static ImVector<MenuGroupState> g_GroupStack;

// Oyma dikdortgen: koyu cizgi dista, acik cizgi 1px iceride; ust kenarda [gap0, gap1] baslik boslugu.
static void EtchedFrame(ImDrawList* dl, const ImRect& r, float gap0, float gap1, float s)
{
    const float e = ImMax(1.0f, IM_ROUND(s));
    const ImU32 dark = MenuVgui::Color(MenuVgui::DarkEdge), light = MenuVgui::Color(MenuVgui::LightEdge);
    const auto h_line = [&](float x0, float x1, float y, ImU32 c) { if (x1 > x0) dl->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + e), c); };
    h_line(r.Min.x, gap0, r.Min.y, dark);                       h_line(gap1, r.Max.x - e, r.Min.y, dark);
    h_line(r.Min.x + e, gap0, r.Min.y + e, light);              h_line(gap1, r.Max.x - e, r.Min.y + e, light);
    dl->AddRectFilled(ImVec2(r.Min.x, r.Min.y), ImVec2(r.Min.x + e, r.Max.y - e), dark);               // sol
    dl->AddRectFilled(ImVec2(r.Min.x + e, r.Min.y + e), ImVec2(r.Min.x + 2.0f * e, r.Max.y - 2.0f * e), light);
    dl->AddRectFilled(ImVec2(r.Max.x - 2.0f * e, r.Min.y), ImVec2(r.Max.x - e, r.Max.y - e), dark);    // sag
    dl->AddRectFilled(ImVec2(r.Max.x - e, r.Min.y), ImVec2(r.Max.x, r.Max.y), light);
    dl->AddRectFilled(ImVec2(r.Min.x, r.Max.y - 2.0f * e), ImVec2(r.Max.x - e, r.Max.y - e), dark);    // alt
    dl->AddRectFilled(ImVec2(r.Min.x, r.Max.y - e), ImVec2(r.Max.x, r.Max.y), light);
}

static bool BeginMenuGroup(const char* id, float width, float main_scale, const char* caption = nullptr)
{
    const float s = main_scale;
    MenuGroupState state;
    state.Framed = caption != nullptr;
    state.Scale = s;
    const float padding = state.Framed ? IM_ROUND(12.0f * s) : 0.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    // Warning: cercevesiz child'larda ImGui WindowPadding'i yok sayar; AlwaysUseWindowPadding olmadan icerik kenara yapisir.
    const bool open = ImGui::BeginChild(id, ImVec2(width, 0.0f),
                                        ImGuiChildFlags_AutoResizeY | (state.Framed ? ImGuiChildFlags_AlwaysUseWindowPadding : 0));
    if (open && state.Framed)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImFont* font = g_FontBold ? g_FontBold : ImGui::GetFont();
        const float size = IM_ROUND(ImGui::GetFontSize() * 0.68f);
        const float row = IM_ROUND(size + 4.0f * s);
        const float x = window->Pos.x + padding;
        state.CaptionY = window->Pos.y + IM_ROUND(row * 0.5f);
        const float text_w = MenuVgui::TextTracked(window->DrawList, font, size,
                                                   ImVec2(IM_ROUND(x), IM_ROUND(state.CaptionY - MenuVgui::CapCenterOffset(font, size))),
                                                   MenuVgui::Color(MenuVgui::Muted), caption, IM_ROUND(1.5f * s));
        state.CaptionStart = IM_ROUND(x - 6.0f * s);
        state.CaptionEnd = IM_ROUND(x + text_w + 8.0f * s);
        ImGui::SetCursorScreenPos(ImVec2(x, state.CaptionY + IM_ROUND(row * 0.5f) + IM_ROUND(8.0f * s)));
    }
    else
        state.Framed = false;
    g_GroupStack.push_back(state);
    return open;
}

static void EndMenuGroup()
{
    const MenuGroupState state = g_GroupStack.back();
    g_GroupStack.pop_back();
    if (state.Framed)
    {
        // Cerceve kenarlarda (icerik 12px iceride, ustune binmez); alt kenar bu frame'in gercek icerik sonundan.
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* dl = window->DrawList;
        const float bottom = window->DC.CursorMaxPos.y + window->WindowPadding.y;
        // Child'in kendi clip'i padding'in yarisi kadar iceride (kenar cizgisini keser); onun yerine pencere rect'i,
        // ust pencerenin (kaydirma alani vb.) siniriyla kesisik: kaydirilinca cerceve alanin disina tasmaz.
        ImRect clip(window->Pos, ImVec2(window->Pos.x + window->Size.x, ImMax(bottom, window->Pos.y + window->Size.y)));
        if (window->ParentWindow)
            clip.ClipWith(window->ParentWindow->InnerClipRect);
        dl->PushClipRect(clip.Min, clip.Max, false);
        EtchedFrame(dl, ImRect(ImVec2(window->Pos.x, state.CaptionY), ImVec2(window->Pos.x + window->Size.x, bottom)),
                    state.CaptionStart, state.CaptionEnd, state.Scale);
        dl->PopClipRect();
    }
    ImGui::EndChild();                                          // BeginChild her durumda EndChild ister
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}
// Ic ice kaydirilabilir alan: ImGui scrollbar'i kapali; icerik alanin tam genisligini kullanir, VGUI scrollbar alanin
// DISINDA, sagdaki sutun boslugunun ortasinda durur (gruplarin cercevesine yapismaz).
static ImGuiWindow* BeginMenuScrollRegion(const char* id, const ImVec2& size, float main_scale)
{
    IM_UNUSED(main_scale);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::BeginChild(id, size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    return ImGui::GetCurrentWindow();
}

// gap: alanin sagindaki sutun boslugu; cubuk (16px) bu boslugun ortasina, ust pencerede cizilir.
static void EndMenuScrollRegion(ImGuiWindow* window, float main_scale, float gap)
{
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    if (window->ScrollMax.y > 0.0f)
    {
        const float width = IM_ROUND(16.0f * main_scale);
        const float x = IM_ROUND(window->Pos.x + window->Size.x + (gap - width) * 0.5f);
        const ImRect bar(ImVec2(x, window->Pos.y), ImVec2(x + width, window->Pos.y + window->Size.y));
        MenuVgui::Scrollbar(window, bar, ImGui::GetTextLineHeightWithSpacing(), main_scale);
    }
}

// Ust bar tabi: ikon + etiket, secilide accent, hover animasyonlu.
static bool MenuTabButton(const MenuTabDesc& tab, bool selected, float main_scale, float* out_width)
{
    const float icon_width = ImGui::CalcTextSize(tab.Icon).x;
    const float label_width = ImGui::CalcTextSize(tab.Label).x;
    const float pad_x = 8.0f * main_scale;
    const float reveal = g_MenuTabReveal[tab.Page];
    const float w = icon_width + pad_x * 2.0f + (label_width + 8.0f * main_scale) * reveal;
    const float h = ImGui::GetFrameHeight();
    if (out_width != nullptr)                                   // sadece genislik hesabi
    {
        *out_width = w;
        return false;
    }

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(tab.Label, ImVec2(w, h), ImGuiButtonFlags_EnableNav);
    const ImAnimWidgets::ItemState& an = ImAnimWidgets::Update(ImGui::GetItemID(), ImGui::IsItemHovered(), ImGui::IsItemActive(), selected ? 1.0f : 0.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p1 = ImVec2(p0.x + w, p0.y + h);
    const ImU32 text_col = Anim::LerpColor(ImGui::GetColorU32(ImGuiCol_TextDisabled),
                                           ImGui::GetColorU32(ImGuiCol_Text), ImMax(reveal, an.Hot * 0.65f));
    const ImVec2 icon_pos(p0.x + pad_x, p0.y + (h - ImGui::GetTextLineHeight()) * 0.5f);
    dl->AddText(icon_pos, text_col, tab.Icon);
    if (reveal > 0.01f)
    {
        const ImVec2 label_pos(icon_pos.x + icon_width + 8.0f * main_scale, icon_pos.y);
        dl->PushClipRect(ImVec2(label_pos.x, p0.y), ImVec2(ImMin(p1.x - pad_x + 1.0f, label_pos.x + (label_width + 1.0f)*reveal), p1.y), true);
        dl->AddText(label_pos, Anim::ScaleAlpha(text_col, reveal), tab.Label);
        dl->PopClipRect();
    }
    return pressed;
}

// --- Menu widget'lari: hepsi sag tikla bindlenebilir (MenuBinds). bind_name: keybind list'te gorunen ad
// (bos = etiket); "Enabled" gibi sayfadan sayfaya tekrarlanan etiketlerde verilir.
// Marker: bind centiginin (ucgen) kondugu kontrol kutusunun sag alt kosesi; olculer MenuVgui widget'larinin cizimiyle ayni.

// Checkbox kutusu: 16x16, satirin dikey ortasinda, satirin solunda; centik kutunun sag alt kosesinde.
static MenuBinds::Marker CheckboxMarker(float s)
{
    MenuBinds::Marker m;
    const float square = IM_ROUND(16.0f * s);
    const float row = ImMax(IM_ROUND(22.0f * s), ImGui::GetFontSize());
    m.Corner = ImVec2(square, IM_ROUND((row - square) * 0.5f) + square);   // kutunun sag alt kosesi
    m.Bottom = true;
    m.Size = 5.0f;                                  // tik sol-orta bolgede (x+y <= ~19), ucgen x+y >= 23: cakismaz
    return m;
}

// Item'in sag kenarina dayali alan (combo alani, slider sayi kutusu, sayi alani); top: alanin ust kenari.
// Alan yuksekligi VGUI FieldStyle ile font + 2x6 dolgu; centik alanin sag alt kosesinde.
static MenuBinds::Marker FieldMarker(float top, float s)
{
    MenuBinds::Marker m;
    m.Corner = ImVec2(0.0f, top + ImGui::GetFontSize() + 12.0f * s);
    m.FromRight = true;
    m.Bottom = true;
    return m;
}

static bool MenuCheckbox(const char* label, const char* hint, bool* value, float main_scale, const char* bind_name = nullptr)
{
    MenuBinds::DeclareBool(value, bind_name ? bind_name : label);
    return MenuBinds::Bindable(value, main_scale, CheckboxMarker(main_scale),
                               [&] { return MenuVgui::Checkbox(label, hint, value, main_scale); });
}

template <typename DrawSettings>
static bool MenuCheckboxPopup(const char* label, const char* hint, bool* value, float main_scale, DrawSettings&& settings,
                              float* rgba = nullptr, const char* bind_name = nullptr)
{
    MenuBinds::DeclareBool(value, bind_name ? bind_name : label);
    return MenuBinds::Bindable(value, main_scale, CheckboxMarker(main_scale),
                               [&] { return MenuVgui::CheckboxPopup(label, hint, value, main_scale, settings, rgba); });
}

static bool MenuCheckboxColor(const char* label, const char* hint, bool* value, float* rgba, float main_scale)
{
    MenuBinds::DeclareBool(value, label);
    return MenuBinds::Bindable(value, main_scale, CheckboxMarker(main_scale),
                               [&] { return MenuVgui::CheckboxColor(label, hint, value, rgba, main_scale); });
}

static bool AnimatedSliderFloat(const char* label, float* value, float low, float high,
                                const char* format, float main_scale, bool show_value = true, bool show_ticks = true,
                                const char* bind_name = nullptr)
{
    IM_UNUSED(show_ticks);
    MenuBinds::DeclareSlider(value, bind_name ? bind_name : label, low, high, format);
    // Centik sagdaki sayi kutusunun sag alt kosesinde (kutu etiket satirinin hemen altinda baslar)
    return MenuBinds::Bindable(value, main_scale, FieldMarker(ImGui::GetFontSize() + 2.0f * main_scale, main_scale),
                               [&] { return MenuVgui::Slider(label, value, low, high, format, main_scale, show_value); });
}

static bool AnimatedCombo(const char* label, int* current, const char* const items[], int count,
                          float main_scale, bool horizontal = false,
                          ImGuiComboFlags flags = ImGuiComboFlags_HeightLarge, bool bindable = true)
{
    const int rows = (flags & ImGuiComboFlags_HeightSmall) ? 6 : 8;
    if (!bindable)
        return MenuVgui::Combo(label, current, items, count, main_scale, horizontal, rows);
    MenuBinds::DeclareCombo(current, label, items, count);
    return MenuBinds::Bindable(current, main_scale, FieldMarker(horizontal ? 0.0f : ImGui::GetFontSize() + 8.0f * main_scale, main_scale),
                               [&] { return MenuVgui::Combo(label, current, items, count, main_scale, horizontal, rows); });
}

static bool MenuMultiCombo(const char* label, int* mask, const char* const items[], int count, float main_scale, bool horizontal = false)
{
    MenuBinds::DeclareMask(mask, label, items, count);
    return MenuBinds::Bindable(mask, main_scale, FieldMarker(horizontal ? 0.0f : ImGui::GetFontSize() + 8.0f * main_scale, main_scale),
                               [&] { return MenuVgui::MultiCombo(label, mask, items, count, main_scale, horizontal); });
}

static bool MenuInputFloat(const char* label, float* value, float label_width, float main_scale)
{
    MenuBinds::DeclareInput(value, label);
    return MenuBinds::Bindable(value, main_scale, FieldMarker(0.0f, main_scale),
                               [&] { return MenuVgui::InputFloat(label, value, label_width, main_scale); });
}

static bool MenuColorEdit(const char* label, float* rgba, float main_scale, bool alpha)
{
    MenuBinds::DeclareColor(rgba, label, alpha);
    MenuBinds::Marker marker;                                                        // renk karesinin sag alt kosesi
    marker.Corner = ImVec2(ImGui::GetFontSize() + 10.0f * main_scale, ImGui::GetFontSize() + 10.0f * main_scale);
    marker.Bottom = true;
    return MenuBinds::Bindable(rgba, main_scale, marker, [&] { return MenuVgui::ColorEdit(label, rgba, main_scale, alpha); });
}
static void RenderMenuTopBar(float main_scale)
{
    // Sol: marka
    ImGui::PushFont(g_FontBold, ImGui::GetFontSize() * 1.25f);
    ImGui::TextUnformatted("killark.cc");
    ImGui::PopFont();

    // Sag: tablar
    for (int i = 0; i < MenuPage_COUNT; ++i)
    {
        g_MenuTabReveal[i] = Anim::Smooth(g_MenuTabReveal[i], g_MenuPage == i ? 1.0f : 0.0f, 16.0f, Anim::Dt());
        if (g_MenuTabReveal[i] > 0.001f && g_MenuTabReveal[i] < 0.999f) Anim::Request();
    }
    float total = 0.0f;
    for (int i = 0; i < MenuPage_COUNT; i++)
    {
        float w = 0.0f;
        MenuTabButton(g_MenuTabs[i], false, main_scale, &w);
        total += w + ImGui::GetStyle().ItemSpacing.x;
    }
    total -= ImGui::GetStyle().ItemSpacing.x;

    ImGui::SameLine(ImGui::GetContentRegionMax().x - total);
    for (int i = 0; i < MenuPage_COUNT; i++)
    {
        if (i > 0)
            ImGui::SameLine();
        if (MenuTabButton(g_MenuTabs[i], g_MenuPage == i, main_scale, nullptr))
            g_MenuPage = i;
    }
}

static void RenderMenu(float main_scale, bool& show_demo_window, bool& show_another_window, ImVec4& clear_color)
{
    const ImVec2 menu_size = ImVec2(1100.0f * main_scale, 700.0f * main_scale);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(menu_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - menu_size.x) * 0.5f,
                                   vp->WorkPos.y + (vp->WorkSize.y - menu_size.y) * 0.5f),
                            g_CenterMenuRequest ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    g_CenterMenuRequest = false;

    if (!ImGui::Begin("killark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::End();
        return;
    }

    bool apply_theme = false;
    MenuVgui::Motion().Collect();
    RenderMenuTopBar(main_scale);

    // Widget alani: konsol tarzi cokme cerceveli koyu zemin (zemini kendimiz ciziyoruz)
    const float footer_h = ImGui::GetTextLineHeightWithSpacing() - 26;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * main_scale, 16.0f * main_scale));
    ImGui::BeginChild("##console", ImVec2(0.0f, -footer_h), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    ImGuiWindow* content_window = ImGui::GetCurrentWindow();
    if (content_window->ScrollMax.y > 0.0f)
    {
        content_window->WorkRect.Max.x -= 28.0f * main_scale;
        content_window->ContentRegionRect.Max.x -= 28.0f * main_scale;
    }
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 c0 = ImGui::GetWindowPos();
        const ImVec2 c1 = ImVec2(c0.x + ImGui::GetWindowWidth(), c0.y + ImGui::GetWindowHeight());
        dl->PushClipRect(c0, c1, false);                            // govde clip'i ic alana daralir
        DrawSunkenFrame(dl, c0, c1, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        dl->PopClipRect();
    }

    switch (g_MenuPage)
    {
    case MenuPage_Rage:
    {
        bool& enabled = g_RageEnabled;
        static bool silent = true, auto_fire = true, through_walls = true;
        static bool auto_scope = false, auto_stop = true, quick_stop = false, resolver = true;
        static float fov = 90.0f;
        static int hitbox = 0, point_scale = 1;
        static const char* const hitboxes[] = { "Head", "Neck", "Chest", "Pelvis", "Nearest" };
        static const char* const scales[] = { "Low", "Medium", "High" };

        const float col_w = MenuColumnWidth(2, main_scale);
        const float gap = MenuColumnGap(main_scale);
        ImGui::BeginGroup();                                        // sol sutun
        if (BeginMenuGroup("rage_main", col_w, main_scale, "AIMBOT"))
        {
            MenuCheckbox("Enabled", "Master switch", &enabled, main_scale, "Rage aimbot");
            ImGui::BeginDisabled(!enabled);
            MenuCheckbox("Silent Aim", "Hide aim corrections", &silent, main_scale);
            MenuCheckbox("Automatic Fire", "Auto fire on target", &auto_fire, main_scale);
            MenuCheckbox("Aim Through Walls", "Ignore obstacles", &through_walls, main_scale);
            AnimatedSliderFloat("Field Of View", &fov, 0.0f, 180.0f, "%.0f", main_scale, true, true, "Rage FOV");
            AnimatedSliderFloat("Hit Chance", &g_HitChance, 0.0f, 100.0f, "%.0f", main_scale);
            AnimatedCombo("Hitbox", &hitbox, hitboxes, IM_ARRAYSIZE(hitboxes), main_scale);
            ImGui::EndDisabled();
        }
        EndMenuGroup();
        ImGui::Dummy(ImVec2(0.0f, gap * 0.5f));
        if (BeginMenuGroup("rage_exploits", col_w, main_scale, "EXPLOITS"))
        {
            ImGui::BeginDisabled(!enabled);
            MenuCheckbox("Force shoot", "Fire without hit chance", &g_ForceShoot, main_scale);
            MenuCheckbox("Force body aim", "Ignore head hitboxes", &g_ForceBodyAim, main_scale);
            MenuCheckbox("No spread", "Compensate weapon spread", &g_NoSpread, main_scale);
            ImGui::EndDisabled();
        }
        EndMenuGroup();
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, gap);
        ImGui::BeginGroup();                                        // sag sutun
        if (BeginMenuGroup("rage_other", col_w, main_scale, "ACCURACY"))
        {
            ImGui::BeginDisabled(!enabled);
            MenuCheckbox("Auto scope", "Scope before firing", &auto_scope, main_scale);
            MenuCheckbox("Auto stop", "Brake while firing", &auto_stop, main_scale);
            MenuCheckbox("Quick stop", "Stop on target", &quick_stop, main_scale);
            MenuCheckbox("Resolver", "Resolve animations", &resolver, main_scale);
            AnimatedSliderFloat("Minimum Damage", &g_MinDamage, 0.0f, 100.0f, "%.0f", main_scale);
            AnimatedCombo("Point scale", &point_scale, scales, IM_ARRAYSIZE(scales), main_scale, true);
            ImGui::EndDisabled();
        }
        EndMenuGroup();
        ImGui::Dummy(ImVec2(0.0f, gap * 0.5f));
        if (BeginMenuGroup("rage_movement", col_w, main_scale, "MOVEMENT"))
        {
            MenuCheckbox("Self track", "Track own movement", &g_SelfTrack, main_scale);
            MenuCheckbox("Hide shots", "Hide shot animation", &g_HideShots, main_scale);
            AnimatedCombo("Force side", &g_ForceSide, g_ForceSides, IM_ARRAYSIZE(g_ForceSides), main_scale, true);
            MenuCheckbox("Quick peek", "Return after shot", &g_QuickPeek, main_scale);
            MenuCheckbox("Duck peek", "Peek while ducked", &g_DuckPeek, main_scale);
            MenuCheckbox("Slow walk", "Walk silently", &g_SlowWalk, main_scale);
        }
        EndMenuGroup();
        ImGui::EndGroup();
        break;
    }
    case MenuPage_Legit:
    {
        bool& enabled = g_LegitEnabled;
        bool& trigger = g_Triggerbot;
        static bool visible_only = true, team_check = false, trigger_hs = true;
        static float fov = 3.5f, smooth = 6.0f, delay = 80.0f;

        const float col_w = MenuColumnWidth(2, main_scale);
        if (BeginMenuGroup("legit_aim", col_w, main_scale, "AIM ASSIST"))
        {
            MenuCheckbox("Enabled", "Master switch", &enabled, main_scale, "Legit aimbot");
            ImGui::BeginDisabled(!enabled);
            MenuCheckbox("Visible only", "Require line of sight", &visible_only, main_scale);
            MenuCheckbox("Team check", "Skip teammates", &team_check, main_scale);
            AnimatedSliderFloat("Smoothing", &smooth, 1.0f, 20.0f, "%.1f", main_scale);
            AnimatedSliderFloat("Field Of View", &fov, 0.0f, 30.0f, "%.1f", main_scale, true, true, "Legit FOV");
            ImGui::EndDisabled();
        }
        EndMenuGroup();

        ImGui::SameLine(0.0f, MenuColumnGap(main_scale));
        if (BeginMenuGroup("legit_trigger", col_w, main_scale, "TRIGGERBOT"))
        {
            MenuCheckboxPopup("Triggerbot", "Fire on target", &trigger, main_scale, [&](float scale)
            {
                ImGui::BeginDisabled(!trigger);
                bool changed = MenuCheckbox("Head only", nullptr, &trigger_hs, scale, "Trigger head only");
                changed |= AnimatedSliderFloat("Delay", &delay, 0.0f, 300.0f, "%.0f ms", scale, true, false, "Trigger delay");
                ImGui::EndDisabled();
                return changed;
            });
        }
        EndMenuGroup();
        break;
    }
    case MenuPage_Visuals:
    {
        static bool enabled = true, box = true, name = true, health = true, skeleton = false, distance = false;
        static bool glow = false, dropped = false, grenades = true;
        static ImVec4 box_color = ImVec4(0.29f, 0.53f, 0.97f, 1.00f);
        static ImVec4 name_color = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        static ImVec4 glow_color = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        static float max_distance = 1400.0f;
        static int box_style = 1;
        static const char* const box_styles[] = { "Full", "Corners", "Filled" };

        // Sagda sabit ESP onizlemesi (sayfa kaysa da yerinde durur), solda kaydirilabilir ayarlar.
        const float gap = MenuColumnGap(main_scale);
        const float preview_w = IM_ROUND(340.0f * main_scale);
        const float settings_w = ImMax(200.0f * main_scale, ImGui::GetContentRegionAvail().x - preview_w - gap);

        ImGuiWindow* settings = BeginMenuScrollRegion("vis_settings", ImVec2(settings_w, 0.0f), main_scale);
        if (BeginMenuGroup("vis_esp", ImGui::GetContentRegionAvail().x, main_scale, "PLAYER ESP"))
        {
            MenuCheckbox("Enabled", "Master switch", &enabled, main_scale, "ESP");
            ImGui::BeginDisabled(!enabled);
            MenuCheckboxPopup("Box", "Draw player outline", &box, main_scale, [&](float scale)
            {
                return AnimatedCombo("Box style", &box_style, box_styles, IM_ARRAYSIZE(box_styles), scale);
            }, &box_color.x);
            MenuCheckboxColor("Name", "Show player name", &name, &name_color.x, main_scale);
            MenuCheckbox("Health bar", "Show remaining health", &health, main_scale);
            MenuCheckbox("Skeleton", "Draw bone overlay", &skeleton, main_scale);
            MenuCheckbox("Distance", "Show range to player", &distance, main_scale);
            AnimatedSliderFloat("Max Distance", &max_distance, 100.0f, 4000.0f, "%.0f", main_scale, true, true);
            ImGui::EndDisabled();
        }
        EndMenuGroup();

        ImGui::Dummy(ImVec2(0.0f, gap * 0.5f));                     // gruplar arasi bosluk (cerceve yok)
        if (BeginMenuGroup("vis_colors", ImGui::GetContentRegionAvail().x, main_scale, "WORLD"))
        {
            ImGui::BeginDisabled(!enabled);
            MenuCheckboxColor("Glow", "Highlight players", &glow, &glow_color.x, main_scale);
            MenuCheckbox("Dropped weapons", "Show dropped items", &dropped, main_scale);
            MenuCheckbox("Grenades", "Show thrown objects", &grenades, main_scale);
            ImGui::EndDisabled();
        }
        EndMenuGroup();

        ImGui::Dummy(ImVec2(0.0f, gap * 0.5f));
        if (BeginMenuGroup("vis_view", ImGui::GetContentRegionAvail().x, main_scale, "VIEW"))
        {
            MenuCheckbox("Thirdperson", "Camera behind player", &g_Thirdperson, main_scale);
            MenuMultiCombo("Screen indicators", &g_IndicatorMask, g_IndicatorNames, Indicator_COUNT, main_scale);
        }
        EndMenuGroup();
        EndMenuScrollRegion(settings, main_scale, gap);

        ImGui::SameLine(0.0f, gap);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::BeginChild("vis_preview", ImVec2(preview_w, 0.0f), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            MenuEsp::PreviewState preview;
            preview.Enabled = enabled;
            preview.Box = box;
            preview.Name = name;
            preview.Health = health;
            preview.Skeleton = skeleton;
            preview.Distance = distance;
            preview.Glow = glow;
            preview.BoxStyle = box_style;
            preview.BoxColor = box_color;
            preview.NameColor = name_color;
            preview.GlowColor = glow_color;
            const ImVec2 p = ImGui::GetWindowPos();
            const ImRect area(p, ImVec2(p.x + ImGui::GetWindowWidth(), p.y + ImGui::GetWindowHeight()));
            // CS2: panorama karakter render'inin dokusu (MenuEsp::PreviewInner(area) boyutunda) ve ekran-uzayi
            // Kemik/kutu overlay verilmediginde onizleme karakter olmadan temiz kutu cizer.
            MenuEsp::DrawPreview(area, preview, main_scale);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        break;
    }
    case MenuPage_Misc:
    {
        static bool auto_strafe = false, fast_stop = false;
        static char player_name[64] = "kdt";
        static float body_lifetime = 600.0f, decal_limit = 300.0f, max_shells = 120.0f, max_smoke_puffs = 120.0f;
        static int resolution = 7;
        static const char* const resolutions[] = {
            "1280 x 720", "1366 x 768", "1440 x 810", "1440 x 900", "1600 x 900", "1600 x 1024",
            "1680 x 1050", "1920 x 1080", "1920 x 1200", "2560 x 1440", "2560 x 1600", "3840 x 2160"
        };

        const float col_w = MenuColumnWidth(2, main_scale);
        if (BeginMenuGroup("misc_move", col_w, main_scale, "MOVEMENT"))
        {
            MenuCheckbox("Bunny hop", "Jump while moving", &g_BunnyHop, main_scale);
            MenuCheckbox("Auto strafe", "Assist air movement", &auto_strafe, main_scale);
            MenuCheckbox("Fast stop", "Brake on release", &fast_stop, main_scale);
            ImGui::Spacing();
            MenuVgui::InputText("Player name", player_name, IM_ARRAYSIZE(player_name), main_scale);   // metin: bindlenmez
            ImGui::Spacing();
            const float label_column = 210.0f * main_scale;
            MenuInputFloat("Body lifetime", &body_lifetime, label_column, main_scale);
            MenuInputFloat("Decal limit", &decal_limit, label_column, main_scale);
            MenuInputFloat("Max shells", &max_shells, label_column, main_scale);
            MenuInputFloat("Max smoke puffs", &max_smoke_puffs, label_column, main_scale);
        }
        EndMenuGroup();

        ImGui::SameLine(0.0f, MenuColumnGap(main_scale));
        ImGui::BeginGroup();                                        // sag sutun: HUD + arayuz
        if (BeginMenuGroup("misc_hud", col_w, main_scale, "HUD"))
        {
            MenuCheckboxPopup("Watermark", "Brand, fps and clock", &g_ShowWatermark, main_scale, [&](float scale)
            {
                return MenuMultiCombo("Watermark items", &g_WatermarkMask, g_WatermarkItemNames, Watermark_COUNT, scale);
            });
            MenuCheckboxPopup("Notifications", "Toasts on the top left", &g_ShowNotifications, main_scale, [&](float scale)
            {
                bool changed = AnimatedSliderFloat("Duration", &g_NotifyDuration, 1.0f, 10.0f, "%.1f s", scale, true, false, "Notify duration");
                if (ImGui::Button("Send test", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                {
                    static int kind = 0;
                    static const char* const titles[] = { "Info", "Config saved", "Low fps", "Injection failed" };
                    static const char* const texts[] = { "This is how a notification looks.", "Settings written to default.cfg",
                                                         "Frame rate dropped below 60.", "Could not resolve schema offset." };
                    Notify((MenuVgui::NotifyKind)kind, titles[kind], texts[kind]);
                    kind = (kind + 1) % 4;
                }
                return changed;
            });
            MenuCheckboxPopup("Hit logs", "Log hits and misses", &g_ShowHitLogs, main_scale, [&](float scale)
            {
                bool changed = MenuCheckbox("Log misses", nullptr, &g_LogMisses, scale);
                changed |= AnimatedSliderFloat("Duration", &g_HitLogDuration, 2.0f, 15.0f, "%.1f s", scale, true, false, "Hit log duration");
                const float half = IM_ROUND((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f);
                if (ImGui::Button("Simulate hit", ImVec2(half, 0.0f))) LogDemoHit();
                ImGui::SameLine();
                if (ImGui::Button("Simulate miss", ImVec2(half, 0.0f))) LogDemoMiss();
                return changed;
            });
            MenuCheckboxPopup("Keybind list", "Show bound features", &g_ShowKeybindList, main_scale, [&](float scale)
            {
                return MenuCheckbox("Show inactive binds", nullptr, &g_KeybindListInactive, scale);
            });
            MenuCheckbox("Spectator list", "Show who is watching", &g_ShowSpectatorList, main_scale);
        }
        EndMenuGroup();
        ImGui::Dummy(ImVec2(0.0f, MenuColumnGap(main_scale) * 0.5f));
        if (BeginMenuGroup("misc_ui", col_w, main_scale, "INTERFACE"))
        {
            // Tema bindlenmez: degisimi ApplyTheme ister, bind'in yazdigi degeri stil takip etmez
            if (AnimatedCombo("Theme", &g_Theme, g_ThemeNames, IM_ARRAYSIZE(g_ThemeNames), main_scale, true,
                              ImGuiComboFlags_HeightLarge, false))
                apply_theme = true;
            AnimatedCombo("Resolution", &resolution, resolutions, IM_ARRAYSIZE(resolutions), main_scale,
                          true, ImGuiComboFlags_HeightSmall);
            if (ImGui::Button("Center The Menu"))
                g_CenterMenuRequest = true;
            ImGui::SameLine();
            if (ImGui::Button("Re-Apply Style"))
                apply_theme = true;
            MenuColorEdit("Clear color", &clear_color.x, main_scale, false);
            MenuCheckbox("Demo window", nullptr, &show_demo_window, main_scale);
            MenuCheckbox("Another window", nullptr, &show_another_window, main_scale);
        }
        EndMenuGroup();
        ImGui::EndGroup();
        break;
    }
    default:
        break;
    }
    if (content_window->ScrollMax.y > 0.0f)
    {
        const ImVec2 pos = content_window->Pos;
        const ImVec2 size = content_window->Size;
        const ImRect scrollbar(ImVec2(pos.x + size.x - 28.0f*main_scale, pos.y + main_scale),
                               ImVec2(pos.x + size.x - main_scale, pos.y + size.y - main_scale));
        ImGui::PushClipRect(scrollbar.Min, scrollbar.Max, false);
        MenuVgui::Scrollbar(content_window, scrollbar, ImGui::GetTextLineHeightWithSpacing(), main_scale);
        ImGui::PopClipRect();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::End();
    if (apply_theme) ApplyTheme(g_Theme, main_scale);
}

// Main code
int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    // 1880x900: 1100x700 menunun yaninda spectator/keybind panellerine yer kalsin; calisma alanina sigdirilir, ortalanir
    RECT work = {};
    ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    const int window_w = ImMin((int)(1880 * main_scale), (int)(work.right - work.left) - 40);
    const int window_h = ImMin((int)(900 * main_scale), (int)(work.bottom - work.top) - 40);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW,
                                work.left + ((work.right - work.left) - window_w) / 2, work.top + ((work.bottom - work.top) - window_h) / 2,
                                window_w, window_h, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls


    // Setup Dear ImGui style + scaling
    ApplyTheme(g_Theme, main_scale);        // tema + DPI olcegi (ScaleAllSizes / FontScaleDpi icerde)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap(&cfg);

    LoadMenuFonts(io);                              // Segoe UI text font with licensed Lucide icons
    RegisterDefaultBinds();
    Notify(MenuVgui::NotifyKind::Success, "killark.cc", "Loaded successfully. Press INSERT to toggle the menu.");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImAnimWidgets::NewFrame();              // [ANIM] kullanilmayan ID'lerin state'ini topla

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Tus atamalari -> oyun ici paneller (HUD katmani) -> menu
        if (ImGui::IsKeyPressed(ImGuiKey_Insert, false) && !MenuVgui::IsListeningForBind() && !io.WantTextInput)
            g_MenuOpen = !g_MenuOpen;
        MenuBinds::Update();
        NotifyBindToggles();
        RenderIndicators(main_scale);
        RenderHud();                                // indicator yiginindan sonra: hit log onun ustune tasmasin
        RenderOverlays(main_scale);
        if (g_MenuOpen)
            RenderMenu(main_scale, show_demo_window, show_another_window, clear_color);

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
