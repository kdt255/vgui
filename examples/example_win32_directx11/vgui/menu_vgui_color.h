#pragma once

#include "menu_vgui.h"
#include <cstring>

namespace MenuVgui
{
struct ColorPickerState
{
    float Hue = 0.0f, Saturation = 0.0f, Value = 0.0f;
    float LastRGB[3] = { -1.0f, -1.0f, -1.0f };
    float Original[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    void ReadRGB(const float* rgba)
    {
        if (memcmp(LastRGB, rgba, sizeof(LastRGB)) == 0) return;
        float h, saturation, value;
        ImGui::ColorConvertRGBtoHSV(rgba[0], rgba[1], rgba[2], h, saturation, value);
        // Keep hue on gray and saturation on black so dragging through either is reversible.
        if (saturation > 0.0f && !(h == 0.0f && Hue == 1.0f)) Hue = h;
        if (value > 0.0f) Saturation = saturation;
        Value = value;
        memcpy(LastRGB, rgba, sizeof(LastRGB));
    }

    void WriteRGB(float* rgba)
    {
        ImGui::ColorConvertHSVtoRGB(Hue, Saturation, Value, rgba[0], rgba[1], rgba[2]);
        memcpy(LastRGB, rgba, sizeof(LastRGB));
    }
};

inline Anim::StateMap<ColorPickerState>& ColorPickerStates()
{
    static Anim::StateMap<ColorPickerState> states;
    return states;
}

inline void Edge(ImDrawList* dl, const ImRect& rect, float s, bool raised = false)
{
    const float e = ImMax(1.0f, IM_ROUND(s));
    const ImU32 upper = Color(raised ? LightEdge : DarkEdge);
    const ImU32 lower = Color(raised ? DarkEdge : LightEdge);
    dl->AddRectFilled(rect.Min, ImVec2(rect.Max.x, rect.Min.y + e), upper);
    dl->AddRectFilled(rect.Min, ImVec2(rect.Min.x + e, rect.Max.y), upper);
    dl->AddRectFilled(ImVec2(rect.Min.x, rect.Max.y - e), rect.Max, lower);
    dl->AddRectFilled(ImVec2(rect.Max.x - e, rect.Min.y), rect.Max, lower);
}

inline void ColorSwatch(ImDrawList* dl, const ImRect& rect, const float* rgba, float s)
{
    const float e = ImMax(1.0f, IM_ROUND(s));
    const ImVec2 inner_min(rect.Min.x + e, rect.Min.y + e);
    const ImVec2 inner_max(rect.Max.x - e, rect.Max.y - e);
    ImGui::RenderColorRectWithAlphaCheckerboard(dl, inner_min, inner_max,
        ImGui::GetColorU32(ImVec4(rgba[0], rgba[1], rgba[2], rgba[3])), 6.0f*s, ImVec2(0,0));
    Edge(dl, rect, s);
}

inline bool ColorChannel(const char* label, int* value, int max_value, ImVec2 pos, float width, float s)
{
    ImGui::PushID(label);
    ImGui::SetCursorScreenPos(pos);
    ImGui::TextUnformatted(label);
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ImGui::GetFontSize() + 4.0f*s));
    bool changed;
    {
        FieldStyle style(s);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        Bevel(ImGui::GetWindowDrawList(), ImRect(p, ImVec2(p.x + width, p.y + ImGui::GetFrameHeight())), Color(Field), false, s);
        ImGui::SetNextItemWidth(width);
        changed = ImGui::InputInt("##value", value, 0, 0);
        if (changed) *value = ImClamp(*value, 0, max_value);
    }
    ImGui::PopID();
    return changed;
}

inline void PickerBarHandle(ImDrawList* dl, const ImRect& rect, float value, float s)
{
    const float y = IM_ROUND(ImLerp(rect.Min.y + 4.0f*s, rect.Max.y - 4.0f*s, ImSaturate(value)));
    Bevel(dl, ImRect(ImVec2(rect.Min.x - 2.0f*s, y - 3.0f*s), ImVec2(rect.Max.x + 2.0f*s, y + 4.0f*s)),
          Color(Background), true, s);
    dl->AddRectFilled(ImVec2(rect.Min.x + 2.0f*s, y), ImVec2(rect.Max.x - 2.0f*s, y + s), Color(DarkEdge));
}

inline bool PickerBarInput(const char* id, const ImRect& rect, float* value, float s)
{
    ImGui::SetCursorScreenPos(rect.Min);
    ImGui::InvisibleButton(id, rect.GetSize());
    if (!ImGui::IsItemActive()) return false;
    const float next = ImSaturate((ImGui::GetIO().MousePos.y - rect.Min.y - 4.0f*s) / ImMax(1.0f, rect.GetHeight() - 8.0f*s));
    if (next == *value) return false;
    *value = next;
    ImGui::MarkItemEdited(ImGui::GetItemID());
    return true;
}

inline int ColorByte(float value) { return (int)(ImSaturate(value)*255.0f + 0.5f); }

inline bool ColorEdit(const char* label, float* rgba, float s, bool alpha = true, bool compact = false)
{
    ImGuiWindow* parent = ImGui::GetCurrentWindow();
    if (parent->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const int components = alpha ? 4 : 3;
    float color[4] = { 0,0,0,1 };
    for (int i = 0; i < components; ++i)
        color[i] = std::isfinite(rgba[i]) ? ImSaturate(rgba[i]) : 0.0f;

    ImGui::PushID(label);
    const ImGuiID id = parent->GetID("##color_edit");
    ColorPickerStates().Collect();
    ColorPickerState& state = ColorPickerStates().Get(id);
    state.ReadRGB(color);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float square = compact ? IM_ROUND(22.0f*s) : ImGui::GetFontSize() + 10.0f*s;
    const float height = ImMax(square, ImGui::GetFontSize());
    const float top = IM_ROUND(p.y + (height - square)*0.5f);
    const ImRect swatch(ImVec2(p.x,top), ImVec2(p.x + square,top + square));
    const float button_width = square + (compact ? 0.0f : 12.0f*s + ImGui::CalcTextSize(label).x);
    const bool pressed = ImGui::InvisibleButton("##color_edit", ImVec2(button_width,height), ImGuiButtonFlags_EnableNav);
    const ImGuiLastItemData row_item = g.LastItemData;
    ColorSwatch(parent->DrawList, swatch, color, s);
    if (!compact)
        parent->DrawList->AddText(ImVec2(swatch.Max.x + 12.0f*s, p.y + 5.0f*s), Color(Text), label);
    else if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s color", label);
    ImGui::RenderNavCursor(row_item.Rect, id);
    if (pressed)
    {
        memcpy(state.Original, color, sizeof(color));
        ImGui::OpenPopup("##picker");
    }

    bool edited = false;
    ImGuiWindow* picker_window = nullptr;
    if (ImGui::IsPopupOpen("##picker"))
    {
        const ImGuiViewport* viewport = ImGui::GetWindowViewport();
        const ImRect work(viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y));
        const float padding = 12.0f*s, gap = 10.0f*s, bar_width = 18.0f*s;
        const float side_width = ImMax(84.0f*s, ImGui::CalcTextSize("Original").x + 4.0f*s);
        const float font = ImGui::GetFontSize();
        const float field_height = font + 12.0f*s;
        const float channel_height = font + 4.0f*s + field_height;
        const float fixed_height = padding*2.0f + font + 8.0f*s + 24.0f*s + channel_height*2.0f + field_height;
        const float sv_size = ImClamp(work.GetHeight() - fixed_height - 12.0f*s, 100.0f*s, 220.0f*s);
        const float picker_width = sv_size + (alpha ? 2 : 1)*(bar_width + gap);
        const float content_width = picker_width + 12.0f*s + side_width;
        const ImVec2 popup_size(content_width + padding*2, fixed_height + sv_size);
        float x = swatch.Min.x;
        float y = swatch.Max.y + 4.0f*s;
        if (y + popup_size.y > work.Max.y && swatch.Min.y - popup_size.y >= work.Min.y)
            y = swatch.Min.y - popup_size.y - 4.0f*s;
        x = ImClamp(x, work.Min.x, ImMax(work.Min.x, work.Max.x - popup_size.x));
        y = ImClamp(y, work.Min.y, ImMax(work.Min.y, work.Max.y - popup_size.y));
        ImGui::SetNextWindowPos(ImVec2(x,y));
        ImGui::SetNextWindowSize(popup_size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding,padding));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f*s,8.0f*s));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, Color(Background));
        if (ImGui::BeginPopup("##picker", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
        {
            ImGuiWindow* popup = ImGui::GetCurrentWindow();
            picker_window = popup;
            ImDrawList* dl = popup->DrawList;
            const ImVec2 start = ImGui::GetCursorScreenPos();
            ImGui::TextUnformatted(label);
            const ImRect close(ImVec2(start.x + content_width - 22.0f*s, start.y), ImVec2(start.x + content_width, start.y + 22.0f*s));
            ImGui::SetCursorScreenPos(close.Min);
            if (ImGui::InvisibleButton("##close", close.GetSize(), ImGuiButtonFlags_EnableNav)) ImGui::CloseCurrentPopup();
            Bevel(dl, close, Color(Background), !ImGui::IsItemActive(), s);
            const ImU32 cross = Color(Text);
            dl->AddLine(ImVec2(close.Min.x + 5*s,close.Min.y + 5*s), ImVec2(close.Max.x - 5*s,close.Max.y - 5*s), cross, 2*s);
            dl->AddLine(ImVec2(close.Min.x + 5*s,close.Max.y - 5*s), ImVec2(close.Max.x - 5*s,close.Min.y + 5*s), cross, 2*s);
            ImGui::RenderNavCursor(close, ImGui::GetItemID());

            const ImVec2 controls(start.x, start.y + font + 8.0f*s);
            const ImRect sv(controls, ImVec2(controls.x + sv_size, controls.y + sv_size));
            ImGui::SetCursorScreenPos(sv.Min);
            ImGui::InvisibleButton("##sv", sv.GetSize());
            if (ImGui::IsItemActive())
            {
                state.Saturation = ImSaturate((g.IO.MousePos.x - sv.Min.x - s)/ImMax(1.0f,sv_size - 2.0f*s));
                state.Value = 1.0f - ImSaturate((g.IO.MousePos.y - sv.Min.y - s)/ImMax(1.0f,sv_size - 2.0f*s));
                state.WriteRGB(color);
                edited = true;
                ImGui::MarkItemEdited(ImGui::GetItemID());
            }
            const ImRect hue(ImVec2(sv.Max.x + gap, sv.Min.y), ImVec2(sv.Max.x + gap + bar_width, sv.Max.y));
            if (PickerBarInput("##hue", hue, &state.Hue, s)) { state.WriteRGB(color); edited = true; }
            const ImRect opacity(ImVec2(hue.Max.x + gap, sv.Min.y), ImVec2(hue.Max.x + gap + bar_width, sv.Max.y));
            if (alpha)
            {
                float inverse_alpha = 1.0f - color[3];
                if (PickerBarInput("##alpha", opacity, &inverse_alpha, s)) { color[3] = 1.0f - inverse_alpha; edited = true; }
            }

            float hue_rgb[3];
            ImGui::ColorConvertHSVtoRGB(state.Hue, 1, 1, hue_rgb[0], hue_rgb[1], hue_rgb[2]);
            const ImVec2 sv_min(sv.Min.x + s, sv.Min.y + s), sv_max(sv.Max.x - s, sv.Max.y - s);
            const ImU32 hue_color = ImGui::GetColorU32(ImVec4(hue_rgb[0],hue_rgb[1],hue_rgb[2],1));
            const ImU32 black = ImGui::GetColorU32(IM_COL32(0,0,0,255));
            dl->AddRectFilledMultiColor(sv_min, sv_max, Color(White), hue_color, hue_color, Color(White));
            dl->AddRectFilledMultiColor(sv_min, sv_max, 0, 0, black, black);
            Edge(dl, sv, s);
            const ImVec2 cursor(IM_ROUND(ImLerp(sv_min.x, sv_max.x, state.Saturation)), IM_ROUND(ImLerp(sv_max.y, sv_min.y, state.Value)));
            dl->PushClipRect(sv.Min, sv.Max, true);
            dl->AddRect(ImVec2(cursor.x - 5*s, cursor.y - 5*s), ImVec2(cursor.x + 5*s, cursor.y + 5*s), black, 0, 0, 3*s);
            dl->AddRect(ImVec2(cursor.x - 4*s, cursor.y - 4*s), ImVec2(cursor.x + 4*s, cursor.y + 4*s), Color(White), 0, 0, s);
            dl->PopClipRect();
            static constexpr ImU32 rainbow[] = { IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255),
                IM_COL32(0,255,255,255), IM_COL32(0,0,255,255), IM_COL32(255,0,255,255), IM_COL32(255,0,0,255) };
            for (int i = 0; i < 6; ++i)
            {
                const float y0 = ImLerp(hue.Min.y + s, hue.Max.y - s, i/6.0f);
                const float y1 = ImLerp(hue.Min.y + s, hue.Max.y - s, (i+1)/6.0f);
                const ImU32 a = ImGui::GetColorU32(rainbow[i]), b = ImGui::GetColorU32(rainbow[i+1]);
                dl->AddRectFilledMultiColor(ImVec2(hue.Min.x+s,y0), ImVec2(hue.Max.x-s,y1), a,a,b,b);
            }
            Edge(dl, hue, s);
            PickerBarHandle(dl, hue, state.Hue, s);
            if (alpha)
            {
                const ImVec2 a_min(opacity.Min.x+s,opacity.Min.y+s), a_max(opacity.Max.x-s,opacity.Max.y-s);
                ImGui::RenderColorRectWithAlphaCheckerboard(dl, a_min, a_max, 0, 5.0f*s, ImVec2(0,0));
                const ImU32 opaque = ImGui::GetColorU32(ImVec4(color[0],color[1],color[2],1));
                const ImU32 transparent = opaque & ~IM_COL32_A_MASK;
                dl->AddRectFilledMultiColor(a_min,a_max,opaque,opaque,transparent,transparent);
                Edge(dl, opacity, s);
                PickerBarHandle(dl, opacity, 1.0f-color[3], s);
            }

            const float side_x = start.x + picker_width + 12.0f*s;
            dl->AddText(ImVec2(side_x,controls.y), Color(Muted), "Current");
            const float preview_y = controls.y + font + 6.0f*s;
            ColorSwatch(dl, ImRect(ImVec2(side_x,preview_y), ImVec2(side_x+side_width,preview_y+48.0f*s)), color, s);
            const float original_y = preview_y + 60.0f*s;
            dl->AddText(ImVec2(side_x,original_y), Color(Muted), "Original");
            const ImRect original(ImVec2(side_x,original_y+font+6*s), ImVec2(side_x+side_width,original_y+font+54*s));
            ImGui::SetCursorScreenPos(original.Min);
            if (ImGui::InvisibleButton("##original", original.GetSize(), ImGuiButtonFlags_EnableNav))
            {
                memcpy(color,state.Original,sizeof(color));
                state.ReadRGB(color);
                edited = true;
            }
            ColorSwatch(dl, original, state.Original, s);
            ImGui::RenderNavCursor(original, ImGui::GetItemID());
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restore original color");

            const float field_gap = 8.0f*s;
            const float channel_width = (content_width - (components-1)*field_gap)/components;
            const float channels_y = sv.Max.y + 8.0f*s;
            static const char* channel_labels[] = { "R", "G", "B", "A" };
            bool rgb_edited = false;
            for (int i = 0; i < components; ++i)
            {
                int channel = ColorByte(color[i]);
                if (ColorChannel(channel_labels[i], &channel, 255, ImVec2(start.x+i*(channel_width+field_gap),channels_y),channel_width,s))
                {
                    color[i] = channel/255.0f;
                    edited = true;
                    if (i < 3) rgb_edited = true;
                }
            }
            if (rgb_edited) state.ReadRGB(color);
            int hsv[] = { (int)(state.Hue*360+0.5f), (int)(state.Saturation*100+0.5f), (int)(state.Value*100+0.5f) };
            static const char* hsv_labels[] = { "H", "S %", "V %" };
            const float hsv_width = (content_width - field_gap*2)/3;
            const float hsv_y = channels_y + channel_height + 8.0f*s;
            bool hsv_edited = false;
            for (int i = 0; i < 3; ++i)
            {
                if (ColorChannel(hsv_labels[i], &hsv[i], i == 0 ? 360 : 100, ImVec2(start.x+i*(hsv_width+field_gap),hsv_y),hsv_width,s))
                {
                    if (i == 0) state.Hue = hsv[0]/360.0f;
                    if (i == 1) state.Saturation = hsv[1]/100.0f;
                    if (i == 2) state.Value = hsv[2]/100.0f;
                    hsv_edited = true;
                }
            }
            if (hsv_edited) { state.WriteRGB(color); edited = true; }

            const float hex_y = hsv_y + channel_height + 8.0f*s;
            dl->AddText(ImVec2(start.x,hex_y+6*s), Color(Muted), "HEX #");
            const float hex_label_width = ImGui::CalcTextSize("HEX #").x + 10.0f*s;
            ImGui::SetCursorScreenPos(ImVec2(start.x+hex_label_width,hex_y));
            char hex[9];
            if (alpha) ImFormatString(hex,IM_ARRAYSIZE(hex),"%02X%02X%02X%02X",ColorByte(color[0]),ColorByte(color[1]),ColorByte(color[2]),ColorByte(color[3]));
            else ImFormatString(hex,IM_ARRAYSIZE(hex),"%02X%02X%02X",ColorByte(color[0]),ColorByte(color[1]),ColorByte(color[2]));
            {
                FieldStyle style(s);
                const ImVec2 field = ImGui::GetCursorScreenPos();
                const float width = content_width - hex_label_width;
                Bevel(dl,ImRect(field,ImVec2(field.x+width,field.y+field_height)),Color(Field),false,s);
                ImGui::SetNextItemWidth(width);
                if (ImGui::InputText("##hex",hex,alpha ? 9 : 7,ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
                {
                    if (strlen(hex) == (size_t)components*2)
                    {
                        for (int i = 0; i < components; ++i)
                        {
                            unsigned int byte = 0;
                            for (int j = 0; j < 2; ++j)
                            {
                                const char c = hex[i*2+j];
                                byte = byte*16 + (c >= 'A' ? c-'A'+10 : c-'0');
                            }
                            color[i] = byte/255.0f;
                        }
                        state.ReadRGB(color);
                        edited = true;
                    }
                }
            }
            const ImRect outer(popup->Pos, ImVec2(popup->Pos.x+popup->Size.x,popup->Pos.y+popup->Size.y));
            dl->PushClipRect(outer.Min,outer.Max,false);
            Edge(dl,outer,s,true);
            dl->PopClipRect();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
    }
    const bool changed = edited && memcmp(rgba,color,components*sizeof(float)) != 0;
    if (changed) memcpy(rgba,color,components*sizeof(float));
    g.LastItemData = row_item;
    // Forward the active child ID, as ColorEdit4 does, for edit/deactivation tracking.
    if (picker_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_window)
        g.LastItemData.ID = g.ActiveId;
    if (changed) ImGui::MarkItemEdited(g.LastItemData.ID);
    ImGui::PopID();
    return changed;
}
}
