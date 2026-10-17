#pragma once

#include "menu_vgui.h"
#include "menu_vgui_glyph.h"
#include <cstring>
#include <vector>

// Ekran gostergeleri: ekranin en solunda, dikeyde tam ortada dizilen VGUI cipleri (kabarik govde + cokuk deger kutusu).
// Cip girerken soldan hafif yaylanarak kayar, cikarken sola cekilir; kalanlar yeni merkeze yumusakca kayar; deger
// degisince deger kutusu parlar. Background draw list'e cizilir: her pencerenin (menu dahil) arkasinda, girdi almaz.
// Boyut menu olceginden bagimsiz: tum olculer font_size'dan turetilir (cagiran ekran yuksekligine gore secer).
// Her frame: BeginFrame(); Add(...) ...; Draw(label_font, ScreenFontSize());
// Warning: UI thread'e ozel; oyun durumunu once kopyala, sonra Add() et.
namespace MenuVgui
{
class IndicatorStack
{
public:
    void BeginFrame()
    {
        for (Chip& chip : chips_) chip.Seen = false;
        cursor_ = 0;
    }

    // icon: soldaki Lucide glifi (nullptr = yok). value: sagdaki cokuk kutuda gosterilir (nullptr = yalniz etiket).
    // accent: ikon, etiket ve deger altin.
    void Add(ImGuiID id, const char* icon, const char* label, const char* value = nullptr, bool accent = true)
    {
        int index = -1;
        for (int i = 0; i < (int)chips_.size(); ++i)
            if (chips_[i].Id == id) { index = i; break; }
        if (index < 0)
        {
            Chip chip;
            chip.Id = id;
            chip.AccentT = accent ? 1.0f : 0.0f;
            index = ImMin(cursor_, (int)chips_.size());
            chips_.insert(chips_.begin() + index, chip);
        }
        Chip& chip = chips_[index];
        chip.Seen = true;
        chip.Accent = accent;
        ImStrncpy(chip.Label, label ? label : "", IM_ARRAYSIZE(chip.Label));
        ImStrncpy(chip.Icon, icon ? icon : "", IM_ARRAYSIZE(chip.Icon));
        const char* v = value ? value : "";
        if (strcmp(chip.Value, v) != 0)
        {
            if (chip.Value[0] && v[0]) chip.Flash = 1.0f;           // ilk gorunus degil, gercek degisim
            ImStrncpy(chip.Value, v, IM_ARRAYSIZE(chip.Value));
        }
        cursor_ = ImMax(cursor_, index + 1);
    }

    // Ekran yuksekligine oranli etiket boyutu: 1080p'de 21 (Tahoma ~17 ppem), 15..38 arasi.
    static float ScreenFontSize()
    {
        return IM_ROUND(ImClamp(ImGui::GetMainViewport()->WorkSize.y*(21.0f/1080.0f), 15.0f, 38.0f));
    }

    void Draw(ImFont* label_font, float font_size)
    {
        const float dt = Anim::Dt();
        ImFont* font = ImGui::GetFont();
        if (label_font == nullptr) label_font = font;
        const float fs = IM_ROUND(font_size);
        const float s = fs/24.0f;                                   // orantilar 24'luk referans boyutta tasarlandi
        const float small = IM_ROUND(fs*0.85f);
        const float chip_h = IM_ROUND(fs + 12.0f*s);
        const float spacing = IM_ROUND(ImMax(3.0f, 6.0f*s));
        const float pad = IM_ROUND(12.0f*s);
        const float margin = IM_ROUND(8.0f*s);
        const ImGuiViewport* vp = ImGui::GetMainViewport();

        bool moving = false;
        float stack_h = 0.0f;
        for (Chip& chip : chips_)
        {
            chip.Presence = Anim::Smooth(chip.Presence, chip.Seen ? 1.0f : 0.0f, 11.0f, dt);
            chip.AccentT = Anim::Smooth(chip.AccentT, chip.Accent ? 1.0f : 0.0f, 16.0f, dt);
            chip.Flash = Anim::Smooth(chip.Flash, 0.0f, 6.0f, dt);
            const float target_w = Width(label_font, font, fs, small, chip, pad, s);
            chip.W = chip.W <= 0.0f ? target_w : Anim::Smooth(chip.W, target_w, 20.0f, dt);
            moving |= chip.Presence != (chip.Seen ? 1.0f : 0.0f) || chip.AccentT != (chip.Accent ? 1.0f : 0.0f) || chip.Flash != 0.0f || chip.W != target_w;
            stack_h += (chip_h + spacing)*Anim::Ease::OutCubic(chip.Presence);
        }
        for (size_t i = 0; i < chips_.size();)
        {
            if (!chips_[i].Seen && chips_[i].Presence <= 0.0f) chips_.erase(chips_.begin() + (ptrdiff_t)i);
            else ++i;
        }
        stack_h = ImMax(0.0f, stack_h - spacing);

        // Hedef yuvalar: yigin dikeyde ekran merkezine oturur; her cip kendi yuvasina yumusak kayar.
        float slot = vp->WorkPos.y + vp->WorkSize.y*0.5f - stack_h*0.5f;
        top_ = slot;
        for (Chip& chip : chips_)
        {
            const float target_y = slot;
            chip.Y = chip.Y == FLT_MAX ? target_y : Anim::Smooth(chip.Y, target_y, 14.0f, dt);
            moving |= chip.Y != target_y;
            slot += (chip_h + spacing)*Anim::Ease::OutCubic(chip.Presence);
        }
        if (moving) Anim::Request();
        if (chips_.empty()) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->PushClipRectFullScreen();
        for (const Chip& chip : chips_)
        {
            if (chip.Presence <= 0.0f) continue;
            // Giris: hafif yaylanma (OutBack), cikis: ayni egri geri; alfa dogrusal
            const float slide = Anim::Ease::OutBack(chip.Presence, 1.25f);
            const float w = IM_ROUND(chip.W);
            const float x = IM_ROUND(vp->WorkPos.x + margin - (w + margin)*(1.0f - slide));
            const ImRect r(ImVec2(x, IM_ROUND(chip.Y)), ImVec2(x + w, IM_ROUND(chip.Y) + chip_h));
            Anim::AlphaScope alpha(ImSaturate(chip.Presence*1.4f));

            const float shadow = IM_ROUND(ImMax(1.0f, 2.0f*s));
            dl->AddRectFilled(ImVec2(r.Min.x + shadow, r.Min.y + shadow), ImVec2(r.Max.x + shadow, r.Max.y + shadow),
                              ImGui::GetColorU32(IM_COL32(0, 0, 0, 90)));
            Bevel(dl, r, Color(Background), true, s);

            const float cy = r.Min.y + IM_ROUND(chip_h*0.5f);
            const ImU32 ink = Anim::LerpColor(Color(Text), Color(Gold), chip.AccentT);
            float label_x = r.Min.x + pad;
            if (chip.Icon[0])
            {
                const float box = IconBox(fs);
                GlyphCentered(dl, font, IM_ROUND(fs*0.95f), ImVec2(label_x + box*0.5f, cy), Anim::LerpColor(Color(Muted), Color(Gold), chip.AccentT), chip.Icon);
                label_x += box + IconGap(s);
            }
            TextCapCentered(dl, label_font, fs, label_x, cy, ink, chip.Label);     // ikon ve deger kutusuyla ayni merkez cizgisi
            if (chip.Value[0])
            {
                const float value_w = ValueWidth(font, small, chip.Value, s);
                const float inset = IM_ROUND(ImMax(3.0f, 5.0f*s));
                const ImRect box(ImVec2(r.Max.x - inset - value_w, r.Min.y + inset), ImVec2(r.Max.x - inset, r.Max.y - inset));
                Bevel(dl, box, Anim::LerpColor(Color(Field), Color(Selection), chip.Flash*0.7f), false, s);
                const ImVec2 ts = font->CalcTextSizeA(small, FLT_MAX, 0.0f, chip.Value);
                TextCapCentered(dl, font, small, box.GetCenter().x - ts.x*0.5f, cy, Anim::LerpColor(ink, Color(White), chip.Flash), chip.Value);
            }
        }
        dl->PopClipRect();
    }

private:
    struct Chip
    {
        ImGuiID Id = 0;
        char    Icon[8] = {};
        char    Label[32] = {};
        char    Value[24] = {};
        bool    Seen = false;
        bool    Accent = true;
        float   Presence = 0.0f;
        float   AccentT = 0.0f;
        float   Flash = 0.0f;
        float   W = 0.0f;
        float   Y = FLT_MAX;
    };

    static float IconBox(float fs) { return IM_ROUND(fs); }
    static float IconGap(float s) { return IM_ROUND(ImMax(4.0f, 8.0f*s)); }

    static float ValueWidth(ImFont* font, float size, const char* value, float s)
    {
        return IM_ROUND(ImMax(40.0f*s, font->CalcTextSizeA(size, FLT_MAX, 0.0f, value).x + 16.0f*s));
    }

    static float Width(ImFont* label_font, ImFont* font, float fs, float small, const Chip& chip, float pad, float s)
    {
        float w = pad + label_font->CalcTextSizeA(fs, FLT_MAX, 0.0f, chip.Label).x + pad;
        if (chip.Icon[0]) w += IconBox(fs) + IconGap(s);
        if (chip.Value[0]) w += ValueWidth(font, small, chip.Value, s) - pad + IM_ROUND(ImMax(3.0f, 5.0f*s)) + IM_ROUND(14.0f*s);
        return IM_ROUND(w);
    }

    std::vector<Chip> chips_;
    int cursor_ = 0;
    float top_ = FLT_MAX;

public:
    // Yiginin (animasyonlu) ust kenari: sol ustteki HUD'lar (hit log) bu cizginin altina tasmaz. Cizilmeden once FLT_MAX.
    float Top() const { return top_; }
};
}
