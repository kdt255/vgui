#pragma once

#include "menu_vgui_color.h"
#include "IconsLucide.h"

namespace MenuVgui
{
inline void CheckboxAccessoryAt(ImVec2 position)
{
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorScreenPos(position);
}

inline void SettingsGear(ImDrawList* dl, const ImRect& bounds, ImU32 ink, float s)
{
    ImFont* font = ImGui::GetFont();
    const float size = 20.0f*s;
    const ImVec2 center = bounds.GetCenter();
    if (font->IsGlyphInFont(0xe154))
    {
        const ImVec2 text = font->CalcTextSizeA(size, FLT_MAX, 0, ICON_LC_SETTINGS);
        dl->AddText(font, size, ImVec2(IM_ROUND(center.x - text.x*0.5f), IM_ROUND(center.y - size*0.5f)), ink, ICON_LC_SETTINGS);
    }
    else
    {
        // Preserve the affordance when the optional Lucide font is unavailable.
        for (int i = 0; i < 8; ++i)
        {
            const float a = (float)i*IM_PI/4.0f;
            const ImVec2 axis(cosf(a),sinf(a));
            dl->AddLine(ImVec2(center.x+axis.x*5*s,center.y+axis.y*5*s),
                        ImVec2(center.x+axis.x*8*s,center.y+axis.y*8*s),ink,2*s);
        }
        dl->AddCircle(center,5*s,ink,16,1.5f*s);
        dl->AddCircle(center,2*s,ink,12,s);
    }
}

inline bool BeginCheckboxSettings(const char* label, const ImRect& anchor, float s)
{
    if (!ImGui::IsPopupOpen("##settings")) return false;
    const ImGuiID popup_id = ImGui::GetID("##settings");
    char name[32];
    ImFormatString(name,IM_ARRAYSIZE(name),"##Popup_%08x",popup_id);
    const ImGuiWindow* previous = ImGui::FindWindowByName(name);
    const ImGuiViewport* viewport = ImGui::GetWindowViewport();
    const ImRect work(viewport->WorkPos,ImVec2(viewport->WorkPos.x+viewport->WorkSize.x,viewport->WorkPos.y+viewport->WorkSize.y));
    const float width = ImMin(340.0f*s,work.GetWidth()-8.0f*s);
    const float height = previous ? previous->Size.y : 220.0f*s;
    const float x = ImClamp(anchor.Max.x-width,work.Min.x,ImMax(work.Min.x,work.Max.x-width));
    float y = anchor.Max.y+6.0f*s;
    if (y+height > work.Max.y && anchor.Min.y-height-6.0f*s >= work.Min.y)
        y = anchor.Min.y-height-6.0f*s;
    y = ImClamp(y,work.Min.y,ImMax(work.Min.y,work.Max.y-height));
    ImGui::SetNextWindowPos(ImVec2(x,y));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width,0),ImVec2(width,ImMax(1.0f,work.GetHeight()-8*s)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12*s,12*s));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,0);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize,0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8*s,10*s));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,Color(Background));
    if (!ImGui::BeginPopup("##settings",ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
        return false;
    }

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    const ImVec2 header = ImGui::GetCursorScreenPos();
    const float content_width = ImGui::GetContentRegionAvail().x;
    const float line = ImMax(ImGui::GetFontSize(),22.0f*s);
    const ImRect close(ImVec2(header.x+content_width-22*s,header.y),ImVec2(header.x+content_width,header.y+22*s));
    ImGui::Dummy(ImVec2(content_width,line));
    dl->PushClipRect(header,ImVec2(close.Min.x-8*s,header.y+line),true);
    dl->AddText(header,Color(Text),label,ImGui::FindRenderedTextEnd(label));
    dl->PopClipRect();
    CheckboxAccessoryAt(close.Min);
    if (ImGui::InvisibleButton("##close",ImVec2(22*s,line),ImGuiButtonFlags_EnableNav)) ImGui::CloseCurrentPopup();
    Bevel(dl,close,Color(Background),!ImGui::IsItemActive(),s);
    dl->AddLine(ImVec2(close.Min.x+6*s,close.Min.y+6*s),ImVec2(close.Max.x-6*s,close.Max.y-6*s),Color(Text),2*s);
    dl->AddLine(ImVec2(close.Min.x+6*s,close.Max.y-6*s),ImVec2(close.Max.x-6*s,close.Min.y+6*s),Color(Text),2*s);
    ImGui::RenderNavCursor(close,ImGui::GetItemID());

    const ImRect body(ImVec2(window->Pos.x+6*s,ImGui::GetCursorScreenPos().y-4*s),
                      ImVec2(window->Pos.x+window->Size.x-6*s,window->Pos.y+window->Size.y-6*s));
    if (body.GetHeight() > 0) Bevel(dl,body,Color(Field),false,s);
    if (window->ScrollMax.y > 0)
    {
        window->WorkRect.Max.x -= 28*s;
        window->ContentRegionRect.Max.x -= 28*s;
    }
    return true;
}

inline void EndCheckboxSettings(float s)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImRect outer(window->Pos,ImVec2(window->Pos.x+window->Size.x,window->Pos.y+window->Size.y));
    ImGui::PushClipRect(outer.Min,outer.Max,false);
    if (window->ScrollMax.y > 0)
        Scrollbar(window,ImRect(ImVec2(outer.Max.x-28*s,outer.Min.y+2*s),ImVec2(outer.Max.x-2*s,outer.Max.y-2*s)),ImGui::GetTextLineHeightWithSpacing(),s);
    Edge(window->DrawList,outer,s,true);
    ImGui::PopClipRect();
    ImGui::EndPopup();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

inline bool CheckboxColor(const char* label, const char* hint, bool* value, float* rgba, float s, bool alpha = true)
{
    if (ImGui::GetCurrentWindow()->SkipItems) return false;
    if (!rgba) return Checkbox(label,hint,value,s);
    ImGui::BeginGroup();
    CheckboxRowLayout row;
    const float size = IM_ROUND(22*s);
    bool changed = Checkbox(label,hint,value,s,size,&row);
    if (row.Visible)
    {
        CheckboxAccessoryAt(row.Accessories.Min);
        changed |= ColorEdit(label,rgba,s,alpha,true);
    }
    ImGui::EndGroup();
    return changed;
}

// draw_settings(scale) returns whether a setting changed; the toggle, gear and swatch have separate IDs/hit boxes.
template <typename DrawSettings>
inline bool CheckboxPopup(const char* label, const char* hint, bool* value, float s,
                          DrawSettings&& draw_settings, float* rgba = nullptr, bool alpha = true)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGui::BeginGroup();
    CheckboxRowLayout row;
    const float size = IM_ROUND(22*s);
    const float width = size + (rgba ? 8*s+size : 0);
    bool changed = Checkbox(label,hint,value,s,width,&row);
    if (row.Visible)
    {
        ImGui::PushID(label);
        CheckboxAccessoryAt(row.Accessories.Min);
        const ImGuiID id = ImGui::GetID("##gear");
        const float y = IM_ROUND(row.Bounds.Min.y+(row.Bounds.GetHeight()-size)*0.5f);
        const ImRect gear(ImVec2(row.Accessories.Min.x,y),ImVec2(row.Accessories.Min.x+size,y+size));
        if (ImGui::InvisibleButton("##gear",ImVec2(size,row.Bounds.GetHeight()),ImGuiButtonFlags_EnableNav))
            ImGui::OpenPopup("##settings");
        const bool hovered = ImGui::IsItemHovered();
        const bool open = ImGui::IsPopupOpen("##settings");
        // Menu temasindaki VGUI butonu: kabarik cerceve (basiliyken cokuk), glif soluk -> hover/acik iken altin.
        const float active = Animate(id,hovered || open ? 1.0f : 0.0f,0.0f);
        Bevel(window->DrawList,gear,Color(Background),!ImGui::IsItemActive(),s);
        SettingsGear(window->DrawList,gear,Anim::LerpColor(Color(Muted),Color(Gold),active),s);
        ImGui::RenderNavCursor(gear,id);
        if (hovered && !open) ImGui::SetTooltip("%s settings",label);
        if (rgba)
        {
            CheckboxAccessoryAt(ImVec2(row.Accessories.Max.x-size,row.Bounds.Min.y));
            changed |= ColorEdit(label,rgba,s,alpha,true);
        }
        const ImGuiLastItemData last_item = GImGui->LastItemData;
        if (BeginCheckboxSettings(label,gear,s))
        {
            changed |= draw_settings(s);
            EndCheckboxSettings(s);
        }
        GImGui->LastItemData = last_item;
        ImGui::PopID();
    }
    ImGui::EndGroup();
    return changed;
}
}
