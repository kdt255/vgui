#pragma once

#include "menu_vgui_checkbox.h"
#include <cctype>

// Tus atamalari: VGUI keycap widget'i (Hotkey / CheckboxBind / BindRow) + calisma zamani durumu (UpdateBind).
// Keycap: sol tik -> tus dinle (her klavye tusu, MOUSE2-5 her yerde, MOUSE1 keycap'in ustunde; ESC iptal,
// BACKSPACE atamayi siler), sag tik -> mod menusu (Hold / Toggle / Always).
// Warning: UI thread'e ozel. KeyBind::Active'i oyun thread'i okuyacaksa frame sonunda kopyasini atomik yayinla.
namespace MenuVgui
{
enum class BindMode : int { Hold, Toggle, Always };

struct KeyBind
{
    ImGuiKey Key = ImGuiKey_None;
    BindMode Mode = BindMode::Hold;
    bool     Toggled = false;           // Toggle modunun kalici durumu
    bool     Active = true;             // UpdateBind yazar: ozellik bu frame calisabilir mi (atanmamis = hep acik)
};

inline const char* BindModeName(BindMode mode)
{
    switch (mode)
    {
    case BindMode::Toggle: return "toggle";
    case BindMode::Always: return "always";
    default:               return "hold";
    }
}

// CS 1.6 bind isimleri (MOUSE1, SHIFT, UPARROW, KP_5 ...); tabloda olmayanlar ImGui adinin buyuk harfi.
inline void KeyName(ImGuiKey key, char* out, size_t out_size)
{
    const char* name = nullptr;
    switch (key)
    {
    case ImGuiKey_None:        name = "NONE"; break;
    case ImGuiKey_MouseLeft:   name = "MOUSE1"; break;
    case ImGuiKey_MouseRight:  name = "MOUSE2"; break;
    case ImGuiKey_MouseMiddle: name = "MOUSE3"; break;
    case ImGuiKey_MouseX1:     name = "MOUSE4"; break;
    case ImGuiKey_MouseX2:     name = "MOUSE5"; break;
    case ImGuiKey_LeftShift:   case ImGuiKey_RightShift: name = "SHIFT"; break;
    case ImGuiKey_LeftCtrl:    case ImGuiKey_RightCtrl:  name = "CTRL"; break;
    case ImGuiKey_LeftAlt:     case ImGuiKey_RightAlt:   name = "ALT"; break;
    case ImGuiKey_LeftSuper:   case ImGuiKey_RightSuper: name = "WIN"; break;
    case ImGuiKey_Space:       name = "SPACE"; break;
    case ImGuiKey_Enter:       name = "ENTER"; break;
    case ImGuiKey_KeypadEnter: name = "KP_ENTER"; break;
    case ImGuiKey_Backspace:   name = "BACKSPACE"; break;
    case ImGuiKey_Insert:      name = "INS"; break;
    case ImGuiKey_Delete:      name = "DEL"; break;
    case ImGuiKey_PageUp:      name = "PGUP"; break;
    case ImGuiKey_PageDown:    name = "PGDN"; break;
    case ImGuiKey_UpArrow:     name = "UPARROW"; break;
    case ImGuiKey_DownArrow:   name = "DOWNARROW"; break;
    case ImGuiKey_LeftArrow:   name = "LEFTARROW"; break;
    case ImGuiKey_RightArrow:  name = "RIGHTARROW"; break;
    case ImGuiKey_CapsLock:    name = "CAPSLOCK"; break;
    case ImGuiKey_Escape:      name = "ESCAPE"; break;
    case ImGuiKey_GraveAccent: name = "`"; break;
    default: break;
    }
    if (name == nullptr && key >= ImGuiKey_Keypad0 && key <= ImGuiKey_Keypad9)
    {
        ImFormatString(out, out_size, "KP_%d", (int)(key - ImGuiKey_Keypad0));
        return;
    }
    ImStrncpy(out, name ? name : ImGui::GetKeyName(key), out_size);
    for (char* c = out; *c; ++c)
        *c = (char)toupper((unsigned char)*c);
}

// Gamepad (nav'a ait), tekerlek ve ImGuiMod_* yansimalari atanamaz.
inline bool IsBindableKey(ImGuiKey key)
{
    if (key >= ImGuiKey_GamepadStart && key <= ImGuiKey_GamepadRStickDown) return false;
    if (key == ImGuiKey_MouseWheelX || key == ImGuiKey_MouseWheelY) return false;
    if (key >= ImGuiKey_ReservedForModCtrl && key <= ImGuiKey_ReservedForModSuper) return false;
    return key != ImGuiKey_Escape && key != ImGuiKey_Backspace;     // dinleme kontrolleri
}

inline bool IsMouseKey(ImGuiKey key) { return key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseX2; }

// Dinleme modundaki keycap'in ID'si (ayni anda tek keycap dinler).
inline ImGuiID& BindListenId()
{
    static ImGuiID id = 0;
    return id;
}

inline bool IsListeningForBind() { return BindListenId() != 0 && GImGui->ActiveId == BindListenId(); }

// Her frame bir kez, UI'dan once. Tus dinlenirken ya da metin girilirken basilan tus ozelligi tetiklemez.
inline void UpdateBind(KeyBind& bind)
{
    if (bind.Mode == BindMode::Always || bind.Key == ImGuiKey_None)
    {
        bind.Active = true;
        return;
    }
    const bool blocked = IsListeningForBind() || ImGui::GetIO().WantTextInput;
    if (bind.Mode == BindMode::Hold)
    {
        bind.Active = !blocked && ImGui::IsKeyDown(bind.Key);
        return;
    }
    if (!blocked && ImGui::IsKeyPressed(bind.Key, false))
        bind.Toggled = !bind.Toggled;
    bind.Active = bind.Toggled;
}

inline float HotkeyFontSize() { return IM_ROUND(ImGui::GetFontSize()*0.8f); }

inline float HotkeyTargetWidth(const KeyBind& bind, bool listening, float s)
{
    char name[32];
    KeyName(bind.Key, name, sizeof(name));
    const float text = ImGui::GetFont()->CalcTextSizeA(HotkeyFontSize(), FLT_MAX, 0.0f, listening ? "..." : name).x;
    return IM_ROUND(ImMax(44.0f*s, text + 16.0f*s));
}

// Genislik metne gore yumusak degisir; frame basina bir kez cagrilmali (Hotkey width<=0 ise kendisi cagirir).
inline float HotkeyAnimatedWidth(ImGuiID id, const KeyBind& bind, float s)
{
    const float target = HotkeyTargetWidth(bind, BindListenId() == id, s);
    return IM_ROUND(Animate(ImHashStr("##width", 0, id), target, target, 26.0f));
}

inline bool HotkeyModePopup(ImGuiID popup_id, const ImRect& anchor, KeyBind* bind, float s)
{
    static const char* const modes[] = { "Hold", "Toggle", "Always" };
    const float font = ImGui::GetFontSize();
    const float row_height = IM_ROUND(font + 10.0f*s);
    const float width = ImMax(anchor.GetWidth(), IM_ROUND(124.0f*s));
    const float height = row_height*IM_ARRAYSIZE(modes) + 2.0f*s;
    const ImGuiViewport* viewport = ImGui::GetWindowViewport();
    const ImRect work(viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y));
    const float x = ImClamp(anchor.Max.x - width, work.Min.x, ImMax(work.Min.x, work.Max.x - width));
    const bool upwards = anchor.Max.y + height + 2.0f*s > work.Max.y && anchor.Min.y - height - 2.0f*s >= work.Min.y;
    ImGui::SetNextWindowPos(ImVec2(x, upwards ? anchor.Min.y - height - 2.0f*s : anchor.Max.y + 2.0f*s));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(s, s));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color(Background));
    bool changed = false;
    if (ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGuiWindow* popup = ImGui::GetCurrentWindow();
        float& reveal = Motion().Get(ImHashStr("##reveal", 0, popup_id), 0.0f);
        if (ImGui::IsWindowAppearing()) reveal = 0.0f;
        reveal = Anim::Smooth(reveal, 1.0f, 28.0f, Anim::Dt());
        if (reveal != 1.0f) Anim::Request();
        const float item_width = width - 2.0f*s;
        for (int i = 0; i < IM_ARRAYSIZE(modes); ++i)
        {
            ImGui::PushID(i);
            const ImVec2 row = ImGui::GetCursorScreenPos();
            const bool selected = (int)bind->Mode == i;
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
            if (ImGui::Selectable("##mode", selected, ImGuiSelectableFlags_None, ImVec2(item_width, row_height)))
            {
                if (!selected)
                {
                    bind->Mode = (BindMode)i;
                    bind->Toggled = false;
                    changed = true;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            const bool highlighted = selected || ImGui::IsItemHovered() || ImGui::IsItemFocused();
            if (highlighted)
                popup->DrawList->AddRectFilled(row, ImVec2(row.x + item_width, row.y + row_height), Anim::ScaleAlpha(Color(Selection), reveal));
            popup->DrawList->AddText(ImVec2(row.x + 6.0f*s, IM_ROUND(row.y + (row_height - font)*0.5f)),
                                     Anim::ScaleAlpha(Color(highlighted ? White : Muted), reveal), modes[i]);
            if (selected) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        const ImRect outer(popup->Pos, ImVec2(popup->Pos.x + popup->Size.x, popup->Pos.y + popup->Size.y));
        popup->DrawList->PushClipRect(outer.Min, outer.Max, false);
        popup->DrawList->AddRect(outer.Min, ImVec2(outer.Max.x - s, outer.Max.y - s), Color(DarkEdge));
        popup->DrawList->PopClipRect();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
    return changed;
}

// VGUI keycap. width <= 0: metne gore animasyonlu genislik. start_listening: tiklamadan dinlemeye gec (yeni bind).
inline bool Hotkey(const char* str_id, KeyBind* bind, float height, float s, float width = 0.0f, bool start_listening = false)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const ImGuiID id = window->GetID(str_id);
    ImGuiID& listen = BindListenId();
    if (listen == id && g.ActiveId != id) listen = 0;               // odak kaybi: tab degisti, popup kapandi, nav iptal
    bool listening = listen == id;
    if (width <= 0.0f) width = HotkeyAnimatedWidth(id, *bind, s);

    const ImVec2 p = window->DC.CursorPos;
    const ImRect bb(p, ImVec2(p.x + width, p.y + height));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    const ImGuiID popup_id = ImHashStr("##bind_mode", 0, id);
    bool changed = false, hovered = false, held = false;
    if (!listening)
    {
        const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if (pressed || (start_listening && g.ActiveId == 0))
        {
            listen = id;
            listening = true;
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
            ImGui::SetActiveIdUsingAllKeyboardKeys();               // nav/kisayollar tusu yemesin
        }
        else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopupEx(popup_id);
    }
    else
    {
        // Aktif ID bizde oldugu icin diger widget'lar hover/click almaz; hover'i kendimiz test ediyoruz.
        hovered = bb.Contains(g.IO.MousePos) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        ImGuiKey captured = ImGuiKey_None;
        bool done = false;
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            done = true;
        else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
        {
            changed = bind->Key != ImGuiKey_None;
            bind->Key = ImGuiKey_None;
            bind->Toggled = false;
            done = true;
        }
        else
        {
            for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k)
            {
                const ImGuiKey key = (ImGuiKey)k;
                if (!IsBindableKey(key) || !ImGui::IsKeyPressed(key, false)) continue;
                if (key == ImGuiKey_MouseLeft && !hovered)          // disari sol tik: iptal
                {
                    ImGui::SetKeyOwner(key, id, ImGuiInputFlags_LockUntilRelease);
                    done = true;
                    break;
                }
                captured = key;
                break;
            }
        }
        if (captured != ImGuiKey_None)
        {
            changed = bind->Key != captured;
            bind->Key = captured;
            bind->Toggled = false;
            done = true;
            // Warning: kilitlenmezse ayni tik bu frame'de sonraki widget'lara / pencere surukleme mantigina gecer.
            if (IsMouseKey(captured))
                ImGui::SetKeyOwner(captured, id, ImGuiInputFlags_LockUntilRelease);
        }
        if (done)
        {
            listen = 0;
            listening = false;
            ImGui::ClearActiveID();
        }
        else
            Anim::Request();                                        // "..." nabzi
    }
    if (changed) ImGui::MarkItemEdited(id);

    const bool bound = bind->Key != ImGuiKey_None;
    const float hover = Animate(id, hovered || listening ? 1.0f : 0.0f, 0.0f, 20.0f);
    const float on = Animate(ImHashStr("##on", 0, id), bound && bind->Active ? 1.0f : 0.0f, 0.0f, 24.0f);
    ImDrawList* dl = window->DrawList;
    const ImU32 face = listening ? Color(Field) : Anim::LerpColor(Color(Background), Color(ScrollTrack), hover);
    Bevel(dl, bb, face, !listening && !held, s);

    char name[32];
    KeyName(bind->Key, name, sizeof(name));
    const char* text = listening ? "..." : name;
    const float size = HotkeyFontSize();
    const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    ImU32 ink = bound ? Anim::LerpColor(Color(Text), Color(Gold), on) : Color(Muted);
    if (listening)
        ink = Anim::ScaleAlpha(Color(Gold), 0.35f + 0.65f*(0.5f + 0.5f*sinf((float)ImGui::GetTime()*8.0f)));
    const float e = ImMax(1.0f, IM_ROUND(s));
    dl->PushClipRect(ImVec2(bb.Min.x + e, bb.Min.y + e), ImVec2(bb.Max.x - e, bb.Max.y - e), true);
    dl->AddText(ImGui::GetFont(), size, ImVec2(IM_ROUND(bb.GetCenter().x - ts.x*0.5f), IM_ROUND(bb.GetCenter().y - size*0.5f)), ink, text);
    dl->PopClipRect();
    ImGui::RenderNavCursor(bb, id);

    const ImGuiLastItemData item = g.LastItemData;
    const bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (hovered && !listening && !popup_open)
        ImGui::SetTooltip("Click to bind, right-click for mode (%s)\nBackspace clears, Esc cancels", BindModeName(bind->Mode));
    if (popup_open)
        changed |= HotkeyModePopup(popup_id, bb, bind, s);
    g.LastItemData = item;
    return changed;
}

// Popup/ayar satiri: solda etiket, sagda (istege bagli mod +) keycap.
inline bool BindRow(const char* label, KeyBind* bind, float s, bool show_mode = true, bool start_listening = false)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGui::PushID(label);
    const ImGuiID key_id = window->GetID("##bind");
    const float width = HotkeyAnimatedWidth(key_id, *bind, s);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float avail = ImGui::GetContentRegionAvail().x;
    const float font = ImGui::GetFontSize();
    const float height = ImMax(IM_ROUND(22.0f*s), font);
    const float key_x = p.x + ImMax(0.0f, avail - width);

    const char* mode = BindModeName(bind->Mode);
    const float small = HotkeyFontSize();
    const float mode_w = show_mode ? ImGui::GetFont()->CalcTextSizeA(small, FLT_MAX, 0.0f, mode).x : 0.0f;
    const float mode_x = key_x - (show_mode ? 8.0f*s + mode_w : 0.0f);
    window->DrawList->PushClipRect(p, ImVec2(mode_x - 8.0f*s, p.y + height), true);
    window->DrawList->AddText(ImVec2(p.x, IM_ROUND(p.y + (height - font)*0.5f)), Color(Text), label, ImGui::FindRenderedTextEnd(label));
    window->DrawList->PopClipRect();
    if (show_mode)
        window->DrawList->AddText(ImGui::GetFont(), small, ImVec2(IM_ROUND(mode_x), IM_ROUND(p.y + (height - small)*0.5f)), Color(Muted), mode);

    ImGui::SetCursorScreenPos(ImVec2(key_x, p.y));
    const bool changed = Hotkey("##bind", bind, height, s, width, start_listening);
    ImGui::PopID();
    return changed;
}
}
