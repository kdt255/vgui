#pragma once

#include "menu_vgui.h"

namespace MenuVgui
{
// Coklu secim combo: *mask'in i. biti items[i]. Liste acik kalir, her satir VGUI onay kutusuyla isaretlenir.
// Warning: en fazla 32 oge (int bit maskesi).
inline bool MultiCombo(const char* label, int* mask, const char* const items[], int count, float s, bool horizontal, int max_rows = 8)
{
    ImGuiWindow* parent = ImGui::GetCurrentWindow();
    if (parent->SkipItems || count <= 0) return false;
    IM_ASSERT(count <= 32);
    ImGuiContext& g = *GImGui;
    ImGui::PushID(label);
    const ImGuiID id = parent->GetID("##multi");
    const ImGuiID popup_id = parent->GetID("##multi_list");
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float font = ImGui::GetFontSize();
    const float height = font + 12.0f*s;
    const float label_width = horizontal ? ImMax(112.0f*s, ImGui::CalcTextSize(label).x + 26.0f*s) : 0.0f;
    const ImVec2 field_pos(p.x + label_width, p.y + (horizontal ? 0.0f : font + 8.0f*s));
    const ImRect frame(field_pos, ImVec2(p.x + width, field_pos.y + height));
    const ImRect total(p, frame.Max);
    ImGui::ItemSize(total);
    if (!ImGui::ItemAdd(total, id, &frame)) { ImGui::PopID(); return false; }
    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(frame, id, &hovered, &held);
    bool open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !open) { ImGui::OpenPopupEx(popup_id); open = true; }
    const float active = Animate(id, open ? 1.0f : 0.0f, 0.0f);
    parent->DrawList->AddText(ImVec2(p.x, p.y + (horizontal ? 6.0f*s : 0.0f)), Anim::LerpColor(Color(Text), Color(Gold), active), label);
    Bevel(parent->DrawList, frame, Color(Field), false, s);

    // Onizleme: "None" ya da secilenlerin virgullu listesi, sigmazsa "... +N"
    char preview[256] = "";
    int selected = 0;
    for (int i = 0; i < count; ++i)
        if (*mask & (1 << i)) ++selected;
    const float preview_max = frame.GetWidth() - 23.0f*s - 6.0f*s;
    if (selected == 0)
        ImStrncpy(preview, "None", IM_ARRAYSIZE(preview));
    else
    {
        int shown = 0;
        for (int i = 0; i < count; ++i)
        {
            if (!(*mask & (1 << i))) continue;
            char candidate[256];
            ImFormatString(candidate, IM_ARRAYSIZE(candidate), "%s%s%s", preview, shown ? ", " : "", items[i]);
            char suffix[16] = "";
            if (shown + 1 < selected) ImFormatString(suffix, IM_ARRAYSIZE(suffix), ", +%d", selected - shown - 1);
            char measured[272];
            ImFormatString(measured, IM_ARRAYSIZE(measured), "%s%s", candidate, suffix);
            if (shown > 0 && ImGui::CalcTextSize(measured).x > preview_max)
            {
                char rest[16];
                ImFormatString(rest, IM_ARRAYSIZE(rest), ", +%d", selected - shown);
                ImStrncpy(candidate, preview, IM_ARRAYSIZE(candidate));
                ImFormatString(preview, IM_ARRAYSIZE(preview), "%s%s", candidate, rest);
                break;
            }
            ImStrncpy(preview, candidate, IM_ARRAYSIZE(preview));
            ++shown;
        }
    }
    parent->DrawList->PushClipRect(frame.Min, ImVec2(frame.Max.x - 23.0f*s, frame.Max.y), true);
    parent->DrawList->AddText(ImVec2(frame.Min.x + 3.0f*s, frame.Min.y + 6.0f*s), Color(selected ? Text : Muted), preview);
    parent->DrawList->PopClipRect();
    Triangle(parent->DrawList, ImVec2(frame.Max.x - 14.0f*s, frame.GetCenter().y), !open, s);
    ImGui::RenderNavCursor(frame, id);
    const ImGuiLastItemData combo_item = g.LastItemData;

    bool changed = false;
    if (open)
    {
        float& reveal = Motion().Get(ImHashStr("reveal", 0, id), 0.0f);
        if (pressed) reveal = 0.0f;
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
        const float scroll_width = scrolling ? 26.0f*s : 0.0f;
        const float popup_x = ImClamp(frame.Min.x, work.Min.x, ImMax(work.Min.x, work.Max.x - frame.GetWidth()));
        ImGui::SetNextWindowPos(ImVec2(popup_x, upwards ? frame.Min.y - popup_height + s : frame.Max.y - s));
        ImGui::SetNextWindowSize(ImVec2(frame.GetWidth(), popup_height));
        ImGui::SetNextWindowContentSize(ImVec2(0.0f, count*row_height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(s, s));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, Color(Background));
        if (ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGuiWindow* popup = ImGui::GetCurrentWindow();
            const ImRect outer(popup->Pos, ImVec2(popup->Pos.x + popup->Size.x, popup->Pos.y + popup->Size.y));
            const float item_width = outer.GetWidth() - 2.0f*s - scroll_width;
            const float square = IM_ROUND(16.0f*s);
            ImGui::PushClipRect(ImVec2(outer.Min.x + s, outer.Min.y + s), ImVec2(outer.Max.x - s - scroll_width, outer.Max.y - s), true);
            for (int i = 0; i < count; ++i)
            {
                ImGui::PushID(i);
                const ImVec2 row = ImGui::GetCursorScreenPos();
                const bool checked = (*mask & (1 << i)) != 0;
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
                if (ImGui::Selectable("##option", false, ImGuiSelectableFlags_NoAutoClosePopups, ImVec2(item_width, row_height)))
                {
                    *mask ^= (1 << i);
                    changed = true;
                }
                ImGui::PopStyleColor(3);
                const bool highlighted = ImGui::IsItemHovered() || ImGui::IsItemFocused();
                const float on = Animate(ImGui::GetItemID(), checked ? 1.0f : 0.0f, checked ? 1.0f : 0.0f, 26.0f);
                if (highlighted)
                    popup->DrawList->AddRectFilled(row, ImVec2(row.x + item_width, row.y + row_height), Anim::ScaleAlpha(Color(Selection), reveal));
                const ImRect box(ImVec2(row.x + 6.0f*s, IM_ROUND(row.y + (row_height - square)*0.5f)),
                                 ImVec2(row.x + 6.0f*s + square, IM_ROUND(row.y + (row_height - square)*0.5f) + square));
                Bevel(popup->DrawList, box, Color(Field), false, s);
                if (on > 0.001f)
                    ImGui::RenderCheckMark(popup->DrawList, ImVec2(box.Min.x + 3.0f*s, box.Min.y + 3.0f*s),
                                           Anim::ScaleAlpha(Color(Gold), on*reveal), square - 6.0f*s);
                const ImU32 ink = highlighted ? Color(White) : Anim::LerpColor(Color(Muted), Color(Text), on);
                popup->DrawList->AddText(ImVec2(box.Max.x + 8.0f*s, row.y + 5.0f*s), Anim::ScaleAlpha(ink, reveal), items[i]);
                ImGui::PopID();
            }
            ImGui::PopClipRect();
            popup->DrawList->PushClipRect(outer.Min, outer.Max, false);
            if (scrolling)
                Scrollbar(popup, ImRect(ImVec2(outer.Max.x - scroll_width - s, outer.Min.y + s), ImVec2(outer.Max.x - s, outer.Max.y - s)), row_height, s);
            popup->DrawList->AddRect(outer.Min, ImVec2(outer.Max.x - s, outer.Max.y - s), Color(DarkEdge));
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
