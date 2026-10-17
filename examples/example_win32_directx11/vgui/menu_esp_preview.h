#pragma once

#include "menu_vgui.h"

// Visuals sayfasindaki ESP onizlemesi. Disaridan model dokusu (orn. CS2 panorama karakter render'i) ve onun
// ekran-uzayi kemik/kutu bilgisi verilirse o kullanilir; yoksa karakter olmadan temiz kutu cizilir. Overlay'ler canli ayarlardan gelir.
// Warning: UI thread'e ozel; model dokusu ImGui frame'i render edilene kadar gecerli olmali.
namespace MenuEsp
{
enum BoxStyle { BoxStyle_Full, BoxStyle_Corners, BoxStyle_Filled };

struct PreviewState
{
    bool   Enabled = true;
    bool   Box = true;
    bool   Name = true;
    bool   Health = true;
    bool   Skeleton = false;
    bool   Distance = false;
    bool   Glow = false;
    int    BoxStyle = BoxStyle_Corners;
    ImVec4 BoxColor = ImVec4(1, 1, 1, 1);
    ImVec4 NameColor = ImVec4(1, 1, 1, 1);
    ImVec4 GlowColor = ImVec4(1, 1, 1, 1);
};

enum Joint { Head, Neck, Pelvis, ShoulderL, ShoulderR, ElbowL, ElbowR, HandL, HandR, HipL, HipR, KneeL, KneeR, FootL, FootR, JointCount };

// Model dokusuyla gelen overlay: koordinatlar model dokusunun pikselleri (0..w, 0..h), doku PreviewInner'a gerilir.
struct ModelOverlay
{
    bool   Valid = false;
    ImRect Box;
    ImVec2 Joints[JointCount] = {};
    bool   HasJoint[JointCount] = {};
};

// Bevel kenarinin icinde kalan cizim alani; model dokusu bu boyutta render edilir.
inline ImRect PreviewInner(const ImRect& area, float s)
{
    const float e = ImMax(1.0f, IM_ROUND(s));
    return ImRect(ImVec2(area.Min.x + e, area.Min.y + e), ImVec2(area.Max.x - e, area.Max.y - e));
}

// Preview target box geometry, centered in the panel.
inline ImRect ModelRect(const ImRect& area, float s)
{
    const float pad = 36.0f*s;
    const float max_h = area.GetHeight() - pad*2.6f;
    const float max_w = area.GetWidth() - pad*2.0f;
    const float h = IM_ROUND(ImMin(max_h, max_w*2.3f));
    const float w = IM_ROUND(h/2.3f);
    const ImVec2 c = area.GetCenter();
    return ImRect(ImVec2(IM_ROUND(c.x - w*0.5f), IM_ROUND(c.y - h*0.5f + 4.0f*s)),
                  ImVec2(IM_ROUND(c.x + w*0.5f), IM_ROUND(c.y + h*0.5f + 4.0f*s)));
}

struct Pose
{
    ImVec2 P[JointCount] = {};
    bool   Has[JointCount] = {};
};

inline void Bones(ImDrawList* dl, const Pose& pose, ImU32 col, float thickness)
{
    static constexpr int bones[][2] = {
        {Head,Neck}, {Neck,Pelvis}, {Neck,ShoulderL}, {Neck,ShoulderR}, {ShoulderL,ElbowL}, {ShoulderR,ElbowR},
        {ElbowL,HandL}, {ElbowR,HandR}, {Pelvis,HipL}, {Pelvis,HipR}, {HipL,KneeL}, {HipR,KneeR}, {KneeL,FootL}, {KneeR,FootR}
    };
    for (const auto& b : bones)
        if (pose.Has[b[0]] && pose.Has[b[1]])
            dl->AddLine(pose.P[b[0]], pose.P[b[1]], col, thickness);
}

inline void BoxOutline(ImDrawList* dl, const ImRect& b, int style, ImU32 col, float s)
{
    const float t = ImMax(1.0f, IM_ROUND(s));
    const ImU32 shadow = IM_COL32(0, 0, 0, (col >> IM_COL32_A_SHIFT) & 0xFF);
    if (style == BoxStyle_Corners)
    {
        const float lx = IM_ROUND(b.GetWidth()*0.25f), ly = IM_ROUND(b.GetHeight()*0.12f);
        const ImVec2 c[4] = { b.Min, ImVec2(b.Max.x, b.Min.y), b.Max, ImVec2(b.Min.x, b.Max.y) };
        const float dx[4] = { 1, -1, -1, 1 }, dy[4] = { 1, 1, -1, -1 };
        for (int pass = 0; pass < 2; ++pass)                        // once golge, sonra renk
            for (int i = 0; i < 4; ++i)
            {
                const ImU32 ink = pass == 0 ? shadow : col;
                const float th = pass == 0 ? t*3.0f : t;
                dl->AddLine(c[i], ImVec2(c[i].x + dx[i]*lx, c[i].y), ink, th);
                dl->AddLine(c[i], ImVec2(c[i].x, c[i].y + dy[i]*ly), ink, th);
            }
        return;
    }
    if (style == BoxStyle_Filled)
        dl->AddRectFilled(b.Min, b.Max, Anim::ScaleAlpha(col, 0.18f));
    dl->AddRect(ImVec2(b.Min.x - t, b.Min.y - t), ImVec2(b.Max.x + t, b.Max.y + t), shadow, 0.0f, 0, t);
    dl->AddRect(b.Min, b.Max, col, 0.0f, 0, t);
}

// Draws ESP overlays over the supplied model, or a clean target box when no model is supplied.
inline void DrawPreview(const ImRect& area, const PreviewState& st, float s,
                        ImTextureID model_texture = ImTextureID_Invalid, const ModelOverlay* overlay = nullptr)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    MenuVgui::Bevel(dl, area, MenuVgui::Color(MenuVgui::Field), false, s);
    const ImRect inner = PreviewInner(area, s);
    dl->PushClipRect(inner.Min, inner.Max, true);

    const bool has_model = model_texture != ImTextureID_Invalid && overlay != nullptr && overlay->Valid;
    Pose pose;
    ImRect box;
    float limb = 0.0f;                                              // glow kalinligi icin uzuv olcusu
    if (has_model)
    {
        dl->AddImage(ImTextureRef(model_texture), inner.Min, inner.Max);
        for (int j = 0; j < JointCount; ++j)
        {
            pose.Has[j] = overlay->HasJoint[j];
            pose.P[j] = ImVec2(IM_ROUND(inner.Min.x + overlay->Joints[j].x), IM_ROUND(inner.Min.y + overlay->Joints[j].y));
        }
        box = ImRect(ImVec2(IM_ROUND(inner.Min.x + overlay->Box.Min.x), IM_ROUND(inner.Min.y + overlay->Box.Min.y)),
                     ImVec2(IM_ROUND(inner.Min.x + overlay->Box.Max.x), IM_ROUND(inner.Min.y + overlay->Box.Max.y)));
        limb = box.GetWidth()*0.12f;
    }
    else
    {
        const ImRect target = ModelRect(area, s);
        dl->AddRectFilled(inner.Min, inner.Max, MenuVgui::Color(MenuVgui::Field));
        box = ImRect(ImVec2(target.Min.x - 6.0f*s, target.Min.y - 6.0f*s), ImVec2(target.Max.x + 6.0f*s, target.Max.y + 4.0f*s));
    }

    if (st.Enabled)
    {
        if (st.Glow)
        {
            const ImU32 glow = ImGui::GetColorU32(st.GlowColor);
            for (int i = 3; i >= 1; --i)
                Bones(dl, pose, Anim::ScaleAlpha(glow, 0.10f*(float)(4 - i)), IM_ROUND(limb) + (float)i*4.0f*s);
        }
        if (st.Skeleton)
            Bones(dl, pose, IM_COL32(255, 255, 255, 230), ImMax(1.0f, IM_ROUND(1.5f*s)));
        if (st.Box)
            BoxOutline(dl, box, st.BoxStyle, ImGui::GetColorU32(st.BoxColor), s);
        if (st.Health)
        {
            const float health = 0.72f;                             // onizleme icin sabit can orani
            const float t = ImMax(2.0f, IM_ROUND(3.0f*s));
            const ImRect bar(ImVec2(box.Min.x - 6.0f*s - t, box.Min.y), ImVec2(box.Min.x - 6.0f*s, box.Max.y));
            dl->AddRectFilled(ImVec2(bar.Min.x - s, bar.Min.y - s), ImVec2(bar.Max.x + s, bar.Max.y + s), IM_COL32(0, 0, 0, 200));
            const ImU32 fill = Anim::LerpColor(IM_COL32(220, 60, 50, 255), IM_COL32(90, 220, 90, 255), health);
            dl->AddRectFilled(ImVec2(bar.Min.x, ImLerp(bar.Max.y, bar.Min.y, health)), bar.Max, fill);
        }
        if (st.Name)
        {
            const char* name = "kdt255";
            const ImVec2 ts = ImGui::CalcTextSize(name);
            const ImVec2 pos(IM_ROUND(box.GetCenter().x - ts.x*0.5f), IM_ROUND(box.Min.y - ts.y - 4.0f*s));
            dl->AddText(ImVec2(pos.x + s, pos.y + s), IM_COL32(0, 0, 0, 200), name);
            dl->AddText(pos, ImGui::GetColorU32(st.NameColor), name);
        }
        if (st.Distance)
        {
            const char* dist = "24 m";
            const ImVec2 ts = ImGui::CalcTextSize(dist);
            const ImVec2 pos(IM_ROUND(box.GetCenter().x - ts.x*0.5f), IM_ROUND(box.Max.y + 4.0f*s));
            dl->AddText(ImVec2(pos.x + s, pos.y + s), IM_COL32(0, 0, 0, 200), dist);
            dl->AddText(pos, MenuVgui::Color(MenuVgui::Muted), dist);
        }
    }
    dl->PopClipRect();
}
}
