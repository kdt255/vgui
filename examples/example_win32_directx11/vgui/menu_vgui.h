#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "anim.h"
#include <cmath>

// Reference pixels, before DPI scaling. All state belongs to the ImGui UI thread.
namespace MenuVgui
{
inline bool ReferencePalette = true;

enum Ink { Background, Field, LightEdge, DarkEdge, Text, Muted, Gold, Selection, Track, Tick, ScrollTrack, Arrow, White };

inline ImU32 Color(Ink ink)
{
    static constexpr ImU32 reference[] = {
        IM_COL32(76,88,68,255), IM_COL32(62,70,55,255),
        IM_COL32(136,145,128,255), IM_COL32(40,46,34,255),
        IM_COL32(216,222,211,255), IM_COL32(160,170,149,255),
        IM_COL32(196,181,80,255), IM_COL32(149,136,49,255),
        IM_COL32(31,31,31,255), IM_COL32(127,140,127,255),
        IM_COL32(90,106,80,255), IM_COL32(176,174,174,255), IM_COL32(255,255,255,255)
    };
    static constexpr ImGuiCol theme[] = {
        ImGuiCol_WindowBg, ImGuiCol_FrameBg, ImGuiCol_Border, ImGuiCol_BorderShadow,
        ImGuiCol_Text, ImGuiCol_TextDisabled, ImGuiCol_TextLink, ImGuiCol_HeaderActive,
        ImGuiCol_BorderShadow, ImGuiCol_TextDisabled, ImGuiCol_ScrollbarBg, ImGuiCol_TextDisabled, ImGuiCol_Text
    };
    return ReferencePalette ? ImGui::GetColorU32(reference[ink]) : ImGui::GetColorU32(theme[ink]);
}

inline void Bevel(ImDrawList* dl, const ImRect& r, ImU32 fill, bool raised = false, float s = 1.0f)
{
    const float edge = ImMax(1.0f, IM_ROUND(s));
    const ImU32 top = Color(raised ? LightEdge : DarkEdge);
    const ImU32 bottom = Color(raised ? DarkEdge : LightEdge);
    dl->AddRectFilled(r.Min, r.Max, fill);
    dl->AddRectFilled(r.Min, ImVec2(r.Max.x, r.Min.y + edge), top);
    dl->AddRectFilled(r.Min, ImVec2(r.Min.x + edge, r.Max.y), top);
    dl->AddRectFilled(ImVec2(r.Min.x, r.Max.y - edge), r.Max, bottom);
    dl->AddRectFilled(ImVec2(r.Max.x - edge, r.Min.y), r.Max, bottom);
}

inline void Triangle(ImDrawList* dl, ImVec2 c, bool down, float s)
{
    c.x = IM_ROUND(c.x); c.y = IM_ROUND(c.y);
    const float direction = down ? 1.0f : -1.0f;
    dl->AddTriangleFilled(ImVec2(c.x - 5.0f*s, c.y - direction*2.5f*s),
                          ImVec2(c.x + 5.0f*s, c.y - direction*2.5f*s),
                          ImVec2(c.x, c.y + direction*3.5f*s), Color(Arrow));
}

inline Anim::StateMap<float>& Motion()
{
    static Anim::StateMap<float> states;
    return states;
}

inline float Animate(ImGuiID id, float target, float initial, float rate = 22.0f)
{
    float& value = Motion().Get(id, initial);
    value = Anim::Smooth(value, target, rate, Anim::Dt());
    if (value != target) Anim::Request();
    return value;
}

struct FieldStyle
{
    explicit FieldStyle(float s)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f*s, 6.0f*s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, Color(Text));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, Color(Selection));
        ImGui::PushStyleColor(ImGuiCol_InputTextCursor, Color(Text));
    }
    ~FieldStyle() { ImGui::PopStyleColor(6); ImGui::PopStyleVar(3); }
    FieldStyle(const FieldStyle&) = delete;
    FieldStyle& operator=(const FieldStyle&) = delete;
};

// centered: duzenleme disinda deger kutunun tam ortasinda; yazarken normal (sola dayali) giris.
inline bool NumberField(const char* id, float* value, float width, const char* format, float s, bool centered = false)
{
    FieldStyle style(s);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    Bevel(ImGui::GetWindowDrawList(), ImRect(p, ImVec2(p.x + width, p.y + ImGui::GetFrameHeight())), Color(Field), false, s);
    ImGui::SetNextItemWidth(width);

    // InputScalar ID'si label'dan uretilir; aktifken padding'i dondurmazsak her tus vurusunda metin kayar.
    const bool editing = ImGui::GetActiveID() == ImGui::GetID(id);
    if (!centered || editing)
        return ImGui::InputFloat(id, value, 0, 0, format, ImGuiInputTextFlags_CharsScientific);

    char text[64];
    ImFormatString(text, IM_ARRAYSIZE(text), format, *value);
    const float pad_x = ImMax(3.0f*s, IM_ROUND((width - ImGui::CalcTextSize(text).x) * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, 6.0f*s));
    const bool changed = ImGui::InputFloat(id, value, 0, 0, format, ImGuiInputTextFlags_CharsScientific);
    ImGui::PopStyleVar();
    return changed;
}

inline bool InputText(const char* label, char* buffer, size_t capacity, float s)
{
    ImGui::PushID(label);
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    bool changed;
    {
        FieldStyle style(s);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        Bevel(ImGui::GetWindowDrawList(), ImRect(p, ImVec2(p.x + width, p.y + ImGui::GetFrameHeight())), Color(Field), false, s);
        ImGui::SetNextItemWidth(width);
        changed = ImGui::InputText("##text", buffer, capacity);
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

inline bool InputFloat(const char* label, float* value, float label_width, float s)
{
    ImGui::PushID(label);
    ImGui::BeginGroup();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x, p.y + 6.0f*s), Color(Text), label);
    ImGui::SetCursorScreenPos(ImVec2(p.x + label_width, p.y));
    const bool changed = NumberField("##number", value, ImMax(40.0f*s, width - label_width), "%.6f", s);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

struct CheckboxRowLayout
{
    ImRect Bounds;
    ImRect Accessories;
    bool Visible = false;
};

inline bool Checkbox(const char* label, const char* hint, bool* value, float s,
                     float accessory_width = 0.0f, CheckboxRowLayout* row = nullptr)
{
    if (row) *row = CheckboxRowLayout{};
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const ImGuiID id = window->GetID(label);
    const ImVec2 p = window->DC.CursorPos;
    const float square = IM_ROUND(16.0f*s);                        // coklu combo satirlariyla ayni kutu
    const float row_height = ImMax(IM_ROUND(22.0f*s), ImGui::GetFontSize());
    const float label_x = p.x + square + 12.0f*s;
    const float label_width = ImGui::CalcTextSize(label,nullptr,true).x;
    const float width = ImGui::GetContentRegionAvail().x;
    const ImRect bb(p, ImVec2(p.x + width, p.y + row_height));
    const bool has_hint = hint && hint[0];
    const float hint_x = p.x + width*0.51f;
    const float controls_end = has_hint ? hint_x - 20.0f*s : bb.Max.x;
    const bool accessories_fit = accessory_width > 0.0f && controls_end - accessory_width >= p.x + square + 8.0f*s;
    const float accessories_start = controls_end - (accessories_fit ? accessory_width : 0.0f);
    const float toggle_end = ImMax(p.x + square, accessories_start - (accessories_fit ? 8.0f*s : 0.0f));
    const ImRect toggle(p, ImVec2(toggle_end, bb.Max.y));
    if (row)
    {
        row->Bounds = bb;
        row->Accessories = ImRect(ImVec2(accessories_start, p.y), ImVec2(controls_end, bb.Max.y));
    }
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id, &toggle)) return false;
    if (row) row->Visible = accessories_fit;
    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(toggle, id, &hovered, &held);
    if (pressed) { *value = !*value; ImGui::MarkItemEdited(id); }

    const float on = Animate(id, *value ? 1.0f : 0.0f, *value ? 1.0f : 0.0f);
    const float hot = Animate(ImHashStr("##hot", 0, id), hovered ? 1.0f : 0.0f, 0.0f);
    const float top = IM_ROUND(p.y + (row_height - square)*0.5f);
    const ImRect box(ImVec2(p.x, top), ImVec2(p.x + square, top + square));
    // Coklu combo kutusu: cokuk VGUI alan + altin tik (solarak gelir), hover'da yuz hafif acilir.
    Bevel(window->DrawList, box, Anim::LerpColor(Color(Field), Color(ScrollTrack), hot*0.45f), false, s);
    if (on > 0.001f)
        ImGui::RenderCheckMark(window->DrawList, ImVec2(box.Min.x + 3.0f*s, box.Min.y + 3.0f*s),
                               Anim::ScaleAlpha(Color(Gold), on), square - 6.0f*s);
    const ImVec2 label_size(label_width, ImGui::GetFontSize());
    ImGui::PushStyleColor(ImGuiCol_Text, Anim::LerpColor(Color(Text), Color(Gold), on));
    const float label_end = ImMax(label_x, toggle.Max.x);
    ImGui::RenderTextEllipsis(window->DrawList, ImVec2(label_x, p.y), ImVec2(label_end, bb.Max.y),
                             label_end, label, ImGui::FindRenderedTextEnd(label), &label_size);
    ImGui::PopStyleColor();
    if (label_width > label_end - label_x && hovered) ImGui::SetTooltip("%s", label);
    if (has_hint)
    {
        const float hint_width = ImMax(1.0f, bb.Max.x - hint_x);
        const ImVec2 hint_size = ImGui::CalcTextSize(hint);
        const ImVec2 hmin(hint_x, p.y), hmax(bb.Max.x, bb.Max.y);
        ImGui::PushStyleColor(ImGuiCol_Text, Color(Muted));
        ImGui::RenderTextEllipsis(window->DrawList, hmin, hmax, hmax.x, hint, nullptr, &hint_size);
        ImGui::PopStyleColor();
        if (hint_size.x > hint_width && ImGui::IsItemHovered() && ImGui::GetIO().MousePos.x >= hint_x)
            ImGui::SetTooltip("%s", hint);
    }
    ImGui::RenderNavCursor(toggle, id);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, GImGui->LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*value ? ImGuiItemStatusFlags_Checked : 0));
    return pressed;
}

inline bool Slider(const char* label, float* value, float low, float high, const char* format, float s, bool show_value)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    if (format == nullptr) format = "%.3f";
    ImGui::PushID(label);
    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float number_width = 72.0f*s;
    const float rail_width = ImMax(40.0f*s, width - (show_value ? number_width + 32.0f*s : 0.0f));
    ImGui::TextUnformatted(label);
    const ImVec2 row(origin.x, origin.y + ImGui::GetFontSize() + 2.0f*s);
    ImGui::SetCursorScreenPos(row);
    const ImGuiID id = window->GetID("##rail");
    const float knob_width = 12.0f*s;
    const float row_height = 30.0f*s;
    // SliderBehavior's two-pixel padding is offset so the thumb reaches both rail endpoints.
    const ImRect frame(ImVec2(row.x - 2.0f, row.y), ImVec2(row.x + rail_width + 2.0f, row.y + row_height));
    ImGui::ItemSize(ImVec2(rail_width, row_height));
    bool changed = false;
    if (ImGui::ItemAdd(frame, id, &frame, ImGuiItemFlags_Inputable))
    {
        const bool hovered = ImGui::ItemHoverable(frame, id, g.LastItemData.ItemFlags);
        bool text_input = ImGui::TempInputIsActive(id);
        if (!text_input)
        {
            const bool clicked = hovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, id);
            const bool activate = clicked || g.NavActivateId == id;
            if (clicked) ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
            if (activate)
            {
                memcpy(&g.ActiveIdValueOnActivation, value, sizeof(float));
                text_input = (clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput));
                if (!text_input)
                {
                    ImGui::SetActiveID(id, window);
                    ImGui::SetFocusID(id, window);
                    ImGui::FocusWindow(window);
                    g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
                }
            }
        }
        if (text_input)
        {
            Bevel(window->DrawList, frame, Color(Field), false, s);
            FieldStyle style(s);
            changed = ImGui::TempInputScalar(frame, id, "##rail", ImGuiDataType_Float, value, format, &low, &high);
        }
        else
        {
            ImRect grab;
            ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, knob_width);
            changed = ImGui::SliderBehavior(frame, id, ImGuiDataType_Float, value, &low, &high, format, ImGuiSliderFlags_AlwaysClamp, &grab);
            ImGui::PopStyleVar();
            if (changed) ImGui::MarkItemEdited(id);
            const float target = ImSaturate((grab.GetCenter().x - row.x) / rail_width);
            const float position = Animate(id, target, target, g.ActiveId == id ? 45.0f : 24.0f);
            const float center_x = IM_ROUND(row.x + rail_width*position);
            const float center_y = IM_ROUND(row.y + row_height*0.5f);
            const ImRect rail(ImVec2(row.x, center_y - 2.0f*s), ImVec2(row.x + rail_width, center_y + 3.0f*s));
            window->DrawList->AddRectFilled(rail.Min, rail.Max, Color(Track));
            window->DrawList->AddRectFilled(ImVec2(rail.Min.x, rail.Max.y - ImMax(1.0f, IM_ROUND(s))), rail.Max, Color(LightEdge));
            const ImRect knob(ImVec2(center_x - knob_width*0.5f, center_y - 12.0f*s),
                              ImVec2(center_x + knob_width*0.5f, center_y + 12.0f*s));
            Bevel(window->DrawList, knob, Color(Background), true, s);
            ImGui::RenderNavCursor(frame, id);
        }
        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);
    }
    const float tick_y = IM_ROUND(row.y + row_height*0.5f + 10.0f*s);
    for (int i = 0; i <= 10; ++i)
    {
        const float x = IM_ROUND(ImLerp(row.x + knob_width*0.5f, row.x + rail_width - knob_width*0.5f, i/10.0f));
        window->DrawList->AddRectFilled(ImVec2(x, tick_y), ImVec2(x + ImMax(1.0f, IM_ROUND(s)), tick_y + 7.0f*s), Color(Tick));
    }
    if (show_value)
    {
        ImGui::SetCursorScreenPos(ImVec2(origin.x + width - number_width, row.y));
        if (NumberField("##value", value, number_width, format, s, true))
        {
            if (!std::isfinite(*value)) *value = low;
            *value = ImClamp(*value, low, high);
            changed = true;
        }
        char min_text[48], max_text[48];
        ImFormatString(min_text, IM_ARRAYSIZE(min_text), "%.2f", low);
        ImFormatString(max_text, IM_ARRAYSIZE(max_text), "%.2f", high);
        const float font_size = ImGui::GetFontSize()*0.67f;
        const ImVec2 max_size = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, 0, max_text);
        const float y = tick_y + 8.0f*s;
        window->DrawList->AddText(ImGui::GetFont(), font_size, ImVec2(row.x, y), Color(Tick), min_text);
        window->DrawList->AddText(ImGui::GetFont(), font_size, ImVec2(row.x + rail_width - max_size.x, y), Color(Tick), max_text);
    }
    ImGui::SetCursorScreenPos(ImVec2(origin.x, tick_y + (show_value ? 28.0f : 15.0f)*s));
    ImGui::Dummy(ImVec2(width, 0));
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

// Draws in window coordinates, without disturbing content layout or keyboard focus.
// 'window' kaydirilan penceredir; cizim ve tiklama alanlari AKTIF pencereye aittir. Boylece cubuk kaydirilan alanin
// disina (orn. iki sutun arasindaki bosluga) konabilir: EndChild'dan sonra ust pencerede cagir.
inline void Scrollbar(ImGuiWindow* window, const ImRect& bb, float step, float s)
{
    if (window->ScrollMax.y <= 0) return;
    ImGuiContext& g = *GImGui;
    const ImGuiLastItemData last_item = g.LastItemData;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float button_size = ImMin(bb.GetWidth(), bb.GetHeight()*0.25f);
    const ImRect up(bb.Min, ImVec2(bb.Max.x, bb.Min.y + button_size));
    const ImRect down(ImVec2(bb.Min.x, bb.Max.y - button_size), bb.Max);
    const ImRect track(ImVec2(bb.Min.x, up.Max.y), ImVec2(bb.Max.x, down.Min.y));
    dl->AddRectFilled(bb.Min, bb.Max, Color(ScrollTrack));
    const auto arrow_button = [&](const ImRect& r, const char* name, bool is_down)
    {
        const ImGuiID id = window->GetID(name);
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        bool hovered = false, held = false;
        bool pressed = false;
        if (ImGui::ItemAdd(r, id, nullptr, ImGuiItemFlags_NoNav))
            pressed = ImGui::ButtonBehavior(r, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoNavFocus);
        ImGui::PopItemFlag();
        if (pressed) ImGui::SetScrollY(window, ImClamp(window->Scroll.y + (is_down ? step : -step), 0.0f, window->ScrollMax.y));
        Bevel(dl, r, Color(Background), !held, s);
        Triangle(dl, r.GetCenter(), is_down, s);
    };
    arrow_button(up, "##scroll_up", false);
    arrow_button(down, "##scroll_down", true);
    const float view_height = window->InnerRect.GetHeight();
    const float thumb_size = ImClamp(track.GetHeight()*view_height/(view_height + window->ScrollMax.y), 20.0f*s, track.GetHeight());
    const float travel = ImMax(1.0f, track.GetHeight() - thumb_size);
    const float thumb_y = track.Min.y + travel*ImSaturate(window->Scroll.y / window->ScrollMax.y);
    ImRect thumb(ImVec2(track.Min.x, thumb_y), ImVec2(track.Max.x, thumb_y + thumb_size));
    const ImGuiID id = window->GetID("##scroll_thumb");
    bool hovered = false, held = false;
    if (ImGui::ItemAdd(track, id, nullptr, ImGuiItemFlags_NoNav))
        ImGui::ButtonBehavior(track, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);
    if (held)
    {
        const ImGuiID offset_id = ImHashStr("offset", 0, id);
        if (g.ActiveIdIsJustActivated)
            window->StateStorage.SetFloat(offset_id, thumb.Contains(g.IO.MousePos) ? g.IO.MousePos.y - thumb.Min.y : thumb_size*0.5f);
        const float offset = window->StateStorage.GetFloat(offset_id, thumb_size*0.5f);
        ImGui::SetScrollY(window, ImSaturate((g.IO.MousePos.y - track.Min.y - offset)/travel)*window->ScrollMax.y);
    }
    Bevel(dl, thumb, Color(Background), true, s);
    g.LastItemData = last_item;
}

inline bool Combo(const char* label, int* current, const char* const items[], int count, float s, bool horizontal, int max_rows = 6)
{
    ImGuiWindow* parent = ImGui::GetCurrentWindow();
    if (parent->SkipItems || count <= 0) return false;
    ImGuiContext& g = *GImGui;
    ImGui::PushID(label);
    const ImGuiID id = parent->GetID("##combo");
    const ImGuiID popup_id = parent->GetID("##list");
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float font = ImGui::GetFontSize();
    const float height = font + 12.0f*s;
    const float label_width = horizontal ? ImMax(112.0f*s, ImGui::CalcTextSize(label).x + 26.0f*s) : 0;
    const ImVec2 field_pos(p.x + label_width, p.y + (horizontal ? 0.0f : font + 8.0f*s));
    const ImRect frame(field_pos, ImVec2(p.x + width, field_pos.y + height));
    const ImRect total(p, frame.Max);
    ImGui::ItemSize(total);
    if (!ImGui::ItemAdd(total, id, &frame)) { ImGui::PopID(); return false; }
    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(frame, id, &hovered, &held);
    bool open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !open) { ImGui::OpenPopupEx(popup_id); open = true; }
    const float active = Animate(id, open ? 1.0f : 0.0f, 0);
    parent->DrawList->AddText(ImVec2(p.x, p.y + (horizontal ? 6.0f*s : 0)),
                               Anim::LerpColor(Color(Text), Color(Gold), active), label);
    Bevel(parent->DrawList, frame, Color(Field), false, s);
    const char* preview = (*current >= 0 && *current < count) ? items[*current] : "";
    parent->DrawList->PushClipRect(frame.Min, ImVec2(frame.Max.x - 23.0f*s, frame.Max.y), true);
    parent->DrawList->AddText(ImVec2(frame.Min.x + 3.0f*s, frame.Min.y + 6.0f*s), Color(Text), preview);
    parent->DrawList->PopClipRect();
    Triangle(parent->DrawList, ImVec2(frame.Max.x - 14.0f*s, frame.GetCenter().y), true, s);
    ImGui::RenderNavCursor(frame, id);
    const ImGuiLastItemData combo_item = g.LastItemData;
    bool changed = false;
    if (open)
    {
        const ImGuiID reveal_id = ImHashStr("reveal", 0, id);
        float& reveal = Motion().Get(reveal_id, 0);
        if (pressed) reveal = 0;
        reveal = Anim::Smooth(reveal, 1.0f, 28.0f, Anim::Dt());
        if (reveal != 1.0f) Anim::Request();
        const float row_height = font + 10.0f*s;
        const ImGuiViewport* viewport = ImGui::GetWindowViewport();
        const ImRect work(viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y));
        const float below = work.Max.y - frame.Max.y - 4.0f*s;
        const float above = frame.Min.y - work.Min.y - 4.0f*s;
        const float desired_height = ImMin(count, max_rows)*row_height + 2.0f*s;
        const bool upwards = below < desired_height && above > below;
        const float popup_height = ImMin(desired_height, ImMax(row_height + 2.0f*s, upwards ? above : below));
        const bool scrolling = count*row_height + 2.0f*s > popup_height + 0.5f;
        const float scroll_width = scrolling ? 26.0f*s : 0;
        const float popup_x = ImClamp(frame.Min.x, work.Min.x, ImMax(work.Min.x, work.Max.x - frame.GetWidth()));
        ImGui::SetNextWindowPos(ImVec2(popup_x, upwards ? frame.Min.y - popup_height + s : frame.Max.y - s));
        ImGui::SetNextWindowSize(ImVec2(frame.GetWidth(), popup_height));
        ImGui::SetNextWindowContentSize(ImVec2(0, count*row_height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(s,s));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, Color(Background));
        if (ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGuiWindow* popup = ImGui::GetCurrentWindow();
            const ImRect outer(popup->Pos, ImVec2(popup->Pos.x + popup->Size.x, popup->Pos.y + popup->Size.y));
            const float item_width = outer.GetWidth() - 2.0f*s - scroll_width;
            if (ImGui::IsWindowAppearing() && scrolling)
                ImGui::SetScrollY(ImMax(0.0f, (*current - max_rows + 1)*row_height));
            const ImVec2 clip_min(outer.Min.x+s, outer.Min.y+s);
            const ImVec2 clip_max(outer.Max.x-s-scroll_width, outer.Max.y-s);
            ImGui::PushClipRect(clip_min, clip_max, true);
            for (int i = 0; i < count; ++i)
            {
                ImGui::PushID(i);
                const ImVec2 row = ImGui::GetCursorScreenPos();
                const bool selected = *current == i;
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));
                if (ImGui::Selectable("##option", selected, ImGuiSelectableFlags_None, ImVec2(item_width, row_height)))
                {
                    changed = *current != i;
                    *current = i;
                }
                ImGui::PopStyleColor(3);
                const bool highlighted = selected || ImGui::IsItemHovered() || ImGui::IsItemFocused();
                if (highlighted)
                    popup->DrawList->AddRectFilled(row, ImVec2(row.x + item_width, row.y + row_height), Anim::ScaleAlpha(Color(Selection), reveal));
                popup->DrawList->AddText(ImVec2(row.x + 3.0f*s, row.y + 5.0f*s),
                    Anim::ScaleAlpha(Color(highlighted ? White : Muted), reveal), items[i]);
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::PopClipRect();
            popup->DrawList->PushClipRect(outer.Min, outer.Max, false);
            if (scrolling)
                Scrollbar(popup, ImRect(ImVec2(outer.Max.x - scroll_width - s, outer.Min.y+s), ImVec2(outer.Max.x-s, outer.Max.y-s)), row_height, s);
            popup->DrawList->AddRect(outer.Min, ImVec2(outer.Max.x-s, outer.Max.y-s), Color(DarkEdge));
            popup->DrawList->PopClipRect();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
    }
    g.LastItemData = combo_item;
    if (changed) ImGui::MarkItemEdited(id);
    ImGui::PopID();
    return changed;
}
}
