#pragma once

#include "menu_vgui.h"
#include "menu_vgui_glyph.h"
#include <cstring>
#include <vector>

// Oyun ici bilgi panelleri (spectator list, keybind list): GoldSrc VGUI cercevesi (kabarik govde, cokuk liste
// alani), basliktaki sayac rozeti, satirlar kayarak/solarak girer-cikar, panel icerige gore boyut animasyonu yapar.
// Sag taraftaki deger / detay / tus sutunlari tum satirlarda ayni genislikte: kolonlar alt alta hizali durur.
// Her frame: list.BeginFrame(); list.Add(...) ...; list.Draw(style, show, interactive, s);
// Warning: UI thread'e ozel. Oyun thread'inden gelen veriyi (entity listesi vb.) once kopyala, sonra Add() et.
namespace MenuVgui
{
enum class RowTone { Normal, Active, Dim };     // Active: altin yazi, Dim: soluk

struct OverlayStyle
{
    const char* WindowName = "##overlay";       // ImGui pencere adi; konum imgui.ini'de saklanir
    const char* Title = "";
    const char* Icon = nullptr;                 // baslik ikonu (Lucide UTF-8), nullptr = yok
    const char* EmptyText = "None";             // menu acikken bos listede gosterilir
    ImFont*     TitleFont = nullptr;            // nullptr = aktif font
    ImVec2      DefaultPos = ImVec2(0, 0);      // ilk acilis konumu (ekran pikseli)
    ImVec2      DefaultPivot = ImVec2(0, 0);    // (1,0): DefaultPos sag ust kose, genislik ne olursa olsun kenara yaslanir
    float       MinWidth = 240.0f;              // referans piksel, s ile carpilir
};

// Metin max_x'e sigmazsa UTF-8 guvenli "..." ile kisaltir.
inline void TextEllipsis(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, float max_x, ImU32 col, const char* text)
{
    const float avail = max_x - pos.x;
    if (avail <= 0.0f || text == nullptr || text[0] == 0) return;
    const char* end = text + strlen(text);
    if (font->CalcTextSizeA(size, FLT_MAX, 0.0f, text, end).x <= avail)
    {
        dl->AddText(font, size, pos, col, text, end);
        return;
    }
    const float dots = font->CalcTextSizeA(size, FLT_MAX, 0.0f, "...").x;
    const char* cut = text;
    for (const char* c = text; c < end;)
    {
        unsigned int codepoint = 0;
        const char* next = c + ImMax(1, ImTextCharFromUtf8(&codepoint, c, end));
        if (font->CalcTextSizeA(size, FLT_MAX, 0.0f, text, next).x + dots > avail) break;
        cut = c = next;
    }
    dl->AddText(font, size, pos, col, text, cut);
    const float w = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text, cut).x;
    dl->AddText(font, size, ImVec2(pos.x + w, pos.y), col, "...");
}

class OverlayList
{
public:
    void BeginFrame()
    {
        for (Row& row : rows_) row.Seen = false;
        cursor_ = 0;
        seen_ = 0;
    }

    // id: satirin kimligi (oyuncu: userid/steamid hash'i, bind: seri no). Ayni id ayni satiri gunceller.
    // value: sagda deger (altin vurgulanir), detail: soluk kucuk metin, key: keycap, icon: solda Lucide glifi.
    void Add(ImGuiID id, const char* label, const char* detail = nullptr, const char* key = nullptr,
             const char* icon = nullptr, RowTone tone = RowTone::Normal, const char* value = nullptr)
    {
        int index = -1;
        for (int i = 0; i < (int)rows_.size(); ++i)
            if (rows_[i].Id == id) { index = i; break; }
        if (index < 0)                                              // yeni satir: gonderim sirasindaki yerine
        {
            Row row;
            row.Id = id;
            row.ActiveT = tone == RowTone::Active ? 1.0f : 0.0f;
            row.DimT = tone == RowTone::Dim ? 1.0f : 0.0f;
            index = ImMin(cursor_, (int)rows_.size());
            rows_.insert(rows_.begin() + index, row);
        }
        Row& row = rows_[index];
        if (!row.Seen) ++seen_;
        row.Seen = true;
        row.Tone = tone;
        ImStrncpy(row.Label, label ? label : "", IM_ARRAYSIZE(row.Label));
        ImStrncpy(row.Value, value ? value : "", IM_ARRAYSIZE(row.Value));
        ImStrncpy(row.Detail, detail ? detail : "", IM_ARRAYSIZE(row.Detail));
        ImStrncpy(row.Key, key ? key : "", IM_ARRAYSIZE(row.Key));
        ImStrncpy(row.Icon, icon ? icon : "", IM_ARRAYSIZE(row.Icon));
        cursor_ = ImMax(cursor_, index + 1);
    }

    int Count() const { return seen_; }

    // show: gorunurluk kurali (ayar + baglam). interactive: menu acik -> surukle, bos listeyi yer tutucuyla goster;
    // kapali -> fare girdisi almaz, bos liste kaybolur.
    void Draw(const OverlayStyle& st, bool show, bool interactive, float s)
    {
        const float dt = Anim::Dt();
        bool moving = false;
        for (Row& row : rows_)
        {
            row.Presence = Anim::Smooth(row.Presence, row.Seen ? 1.0f : 0.0f, 14.0f, dt);
            row.ActiveT = Anim::Smooth(row.ActiveT, row.Tone == RowTone::Active ? 1.0f : 0.0f, 18.0f, dt);
            row.DimT = Anim::Smooth(row.DimT, row.Tone == RowTone::Dim ? 1.0f : 0.0f, 18.0f, dt);
            moving |= row.Presence != (row.Seen ? 1.0f : 0.0f);
            moving |= row.ActiveT != (row.Tone == RowTone::Active ? 1.0f : 0.0f) || row.DimT != (row.Tone == RowTone::Dim ? 1.0f : 0.0f);
        }
        for (size_t i = 0; i < rows_.size();)
        {
            if (!rows_[i].Seen && rows_[i].Presence <= 0.0f) rows_.erase(rows_.begin() + (ptrdiff_t)i);
            else ++i;
        }

        const bool want = show && (seen_ > 0 || interactive);
        panel_ = Anim::Smooth(panel_, want ? 1.0f : 0.0f, 16.0f, dt);
        empty_ = Anim::Smooth(empty_, seen_ == 0 ? 1.0f : 0.0f, 14.0f, dt);
        if (seen_ != last_count_) { flash_ = last_count_ < 0 ? 0.0f : 1.0f; last_count_ = seen_; }
        flash_ = Anim::Smooth(flash_, 0.0f, 5.0f, dt);
        moving |= panel_ != (want ? 1.0f : 0.0f) || empty_ != (seen_ == 0 ? 1.0f : 0.0f) || flash_ != 0.0f;
        if (moving) Anim::Request();
        if (panel_ <= 0.0f) return;

        // --- Olculer ---
        ImFont* font = ImGui::GetFont();
        ImFont* title_font = st.TitleFont ? st.TitleFont : font;
        const float fs = ImGui::GetFontSize();
        const float small = IM_ROUND(fs*0.8f);
        const float e = ImMax(1.0f, IM_ROUND(s));
        const float pad = IM_ROUND(10.0f*s);
        const float inset = IM_ROUND(6.0f*s);
        const float gap = IM_ROUND(10.0f*s);
        const float header_h = IM_ROUND(fs + 14.0f*s);
        const float row_h = IM_ROUND(fs + 10.0f*s);
        const float list_pad = IM_ROUND(4.0f*s);
        const float icon_box = IM_ROUND(fs);                        // ikonlar fs x fs kutuda ortalanir

        bool any_icon = false;
        float label_w = 0.0f, value_w = 0.0f, detail_w = 0.0f, key_w = 0.0f;
        for (const Row& row : rows_)
        {
            any_icon |= row.Icon[0] != 0;
            label_w = ImMax(label_w, font->CalcTextSizeA(fs, FLT_MAX, 0.0f, row.Label).x);
            if (row.Value[0])  value_w = ImMax(value_w, font->CalcTextSizeA(small, FLT_MAX, 0.0f, row.Value).x);
            if (row.Detail[0]) detail_w = ImMax(detail_w, font->CalcTextSizeA(small, FLT_MAX, 0.0f, row.Detail).x);
            if (row.Key[0])    key_w = ImMax(key_w, KeycapWidth(font, small, row.Key, s));
        }
        const float icon_col = any_icon ? icon_box + IM_ROUND(8.0f*s) : 0.0f;
        float columns_w = 0.0f;
        for (float w : { value_w, detail_w, key_w })
            if (w > 0.0f) columns_w += w + gap;
        const float content_w = pad + icon_col + label_w + (columns_w > 0.0f ? IM_ROUND(14.0f*s) + columns_w - gap : 0.0f) + pad;

        char count[16];
        ImFormatString(count, IM_ARRAYSIZE(count), "%d", seen_);
        const float badge_w = IM_ROUND(ImMax(28.0f*s, font->CalcTextSizeA(small, FLT_MAX, 0.0f, count).x + 14.0f*s));
        const float title_icon_w = st.Icon ? icon_box + IM_ROUND(8.0f*s) : 0.0f;
        const float header_w = pad + title_icon_w + title_font->CalcTextSizeA(fs, FLT_MAX, 0.0f, st.Title).x + IM_ROUND(24.0f*s) + badge_w + pad;
        const float empty_w = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, st.EmptyText).x + pad*2.0f + inset*2.0f;
        const float target_w = IM_ROUND(ImMax(ImMax(st.MinWidth*s, header_w), ImMax(content_w + inset*2.0f + e*2.0f, empty_w)));
        width_ = width_ <= 0.0f ? target_w : Anim::Smooth(width_, target_w, 18.0f, dt);
        if (width_ != target_w) Anim::Request();

        float rows_h = 0.0f;
        for (const Row& row : rows_) rows_h += row_h*Anim::Ease::OutCubic(row.Presence);
        const float body_h = list_pad*2.0f + rows_h + row_h*empty_;
        const ImVec2 size(IM_ROUND(width_), IM_ROUND(header_h + body_h + inset));

        // --- Pencere ---
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse;
        if (!interactive) flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
        Anim::AlphaScope alpha(panel_);
        ImGui::SetNextWindowPos(st.DefaultPos, ImGuiCond_FirstUseEver, st.DefaultPivot);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
        const bool open = ImGui::Begin(st.WindowName, nullptr, flags);
        ImGui::PopStyleVar(3);
        if (!open) { ImGui::End(); return; }

        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        if (g.MovingWindow != window)
        {
            // Ekranin sag yarisindaki panel sag kenarina yaslanir: genislik degisince sola dogru buyur/kuculur.
            ImVec2 pos = window->Pos;
            if (last_width_ > 0.0f && size.x != last_width_ && pos.x + last_width_*0.5f > vp->WorkPos.x + vp->WorkSize.x*0.5f)
                pos.x -= size.x - last_width_;
            // Cozunurluk degisince / ekran disina surukleyince geri al
            const ImVec2 max(ImMax(vp->WorkPos.x, vp->WorkPos.x + vp->WorkSize.x - size.x),
                             ImMax(vp->WorkPos.y, vp->WorkPos.y + vp->WorkSize.y - size.y));
            pos = ImVec2(ImClamp(pos.x, vp->WorkPos.x, max.x), ImClamp(pos.y, vp->WorkPos.y, max.y));
            if (pos.x != window->Pos.x || pos.y != window->Pos.y)
                ImGui::SetWindowPos(window, pos);
        }
        last_width_ = size.x;

        ImDrawList* dl = window->DrawList;
        dl->PushClipRectFullScreen();                               // giris kaymasi pencere rect'inin disina tasabilir
        const float rise = IM_ROUND((1.0f - Anim::Ease::OutCubic(panel_))*8.0f*s);
        const ImRect outer(ImVec2(window->Pos.x, window->Pos.y + rise), ImVec2(window->Pos.x + size.x, window->Pos.y + rise + size.y));

        // Govde: oyun sahnesinde okunurluk icin hafif golge + kabarik VGUI cerceve
        dl->AddRectFilled(ImVec2(outer.Min.x + 2.0f*s, outer.Min.y + 2.0f*s), ImVec2(outer.Max.x + 2.0f*s, outer.Max.y + 2.0f*s),
                          ImGui::GetColorU32(IM_COL32(0, 0, 0, 90)));
        Bevel(dl, outer, Color(Background), true, s);

        // Baslik: [ikon kutusu][baslik] ... [sayac]; sol ve sag ic bosluk ayni, hepsi ayni dikey merkezde
        const float header_cy = outer.Min.y + IM_ROUND(header_h*0.5f);
        float x = outer.Min.x + pad;
        if (st.Icon)
        {
            GlyphCentered(dl, font, fs, ImVec2(x + icon_box*0.5f, header_cy), Color(Gold), st.Icon);
            x += title_icon_w;
        }
        const float badge_h = IM_ROUND(small + 8.0f*s);
        const ImRect badge(ImVec2(outer.Max.x - pad - badge_w, header_cy - IM_ROUND(badge_h*0.5f)),
                           ImVec2(outer.Max.x - pad, header_cy - IM_ROUND(badge_h*0.5f) + badge_h));
        TextEllipsis(dl, title_font, fs, ImVec2(x, IM_ROUND(header_cy - fs*0.5f)), badge.Min.x - IM_ROUND(12.0f*s), Color(Text), st.Title);
        Bevel(dl, badge, Anim::LerpColor(Color(Field), Color(Selection), flash_*0.65f), false, s);
        const ImVec2 count_size = font->CalcTextSizeA(small, FLT_MAX, 0.0f, count);
        dl->AddText(font, small, ImVec2(IM_ROUND(badge.GetCenter().x - count_size.x*0.5f), IM_ROUND(badge.GetCenter().y - small*0.5f)),
                    Anim::LerpColor(Color(seen_ > 0 ? Gold : Muted), Color(White), flash_), count);
        if (interactive && ImGui::IsWindowHovered() && ImGui::GetIO().MousePos.y < outer.Min.y + header_h)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

        // Liste alani: cokuk VGUI alan, satirlar kirpilir
        const ImRect field(ImVec2(outer.Min.x + inset, outer.Min.y + header_h), ImVec2(outer.Max.x - inset, outer.Max.y - inset));
        Bevel(dl, field, Color(Field), false, s);
        const ImRect clip(ImVec2(field.Min.x + e, field.Min.y + e), ImVec2(field.Max.x - e, field.Max.y - e));
        dl->PushClipRect(clip.Min, clip.Max, true);
        float y = field.Min.y + list_pad;
        for (const Row& row : rows_)
        {
            const float t = Anim::Ease::OutCubic(row.Presence);
            const float h = row_h*t;
            if (h >= 0.5f)
            {
                Anim::AlphaScope row_alpha(row.Presence);
                dl->PushClipRect(ImVec2(clip.Min.x, y), ImVec2(clip.Max.x, ImMin(y + h, clip.Max.y)), true);
                const ImRect r(ImVec2(clip.Min.x, y), ImVec2(clip.Max.x, y + row_h));
                const float cy = r.Min.y + IM_ROUND(row_h*0.5f);
                const float slide = IM_ROUND((1.0f - t)*14.0f*s);
                const ImU32 base = Anim::LerpColor(Color(Text), Color(Muted), row.DimT);
                const ImU32 ink = Anim::LerpColor(base, Color(Gold), row.ActiveT);
                float left = r.Min.x + pad + slide;
                if (any_icon)
                {
                    if (row.Icon[0])
                        GlyphCentered(dl, font, fs, ImVec2(left + icon_box*0.5f, cy), Anim::LerpColor(Color(Muted), Color(Gold), row.ActiveT), row.Icon);
                    left += icon_col;
                }
                // Sagdan sola sabit genislikli sutunlar: [deger][detay][tus]
                float right = r.Max.x - pad + slide;
                if (key_w > 0.0f)
                {
                    if (row.Key[0])
                    {
                        const ImRect key(ImVec2(right - key_w, IM_ROUND(r.Min.y + 5.0f*s)), ImVec2(right, IM_ROUND(r.Max.y - 5.0f*s)));
                        Bevel(dl, key, Color(Background), true, s);
                        const ImVec2 ks = font->CalcTextSizeA(small, FLT_MAX, 0.0f, row.Key);
                        dl->AddText(font, small, ImVec2(IM_ROUND(key.GetCenter().x - ks.x*0.5f), IM_ROUND(key.GetCenter().y - small*0.5f)), ink, row.Key);
                    }
                    right -= key_w + gap;
                }
                const float small_y = IM_ROUND(cy - small*0.5f);
                if (detail_w > 0.0f)
                {
                    if (row.Detail[0])
                    {
                        const float w = font->CalcTextSizeA(small, FLT_MAX, 0.0f, row.Detail).x;
                        dl->AddText(font, small, ImVec2(IM_ROUND(right - w), small_y), Color(Muted), row.Detail);
                    }
                    right -= detail_w + gap;
                }
                if (value_w > 0.0f)
                {
                    if (row.Value[0])
                    {
                        const float w = font->CalcTextSizeA(small, FLT_MAX, 0.0f, row.Value).x;
                        dl->AddText(font, small, ImVec2(IM_ROUND(right - w), small_y), ink, row.Value);
                    }
                    right -= value_w + gap;
                }
                TextEllipsis(dl, font, fs, ImVec2(left, IM_ROUND(cy - fs*0.5f)), right + gap - IM_ROUND(14.0f*s), ink, row.Label);
                dl->PopClipRect();
            }
            y += h;
        }
        if (empty_ > 0.001f)
        {
            Anim::AlphaScope empty_alpha(empty_);
            const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, st.EmptyText);
            dl->AddText(font, fs, ImVec2(IM_ROUND(clip.GetCenter().x - ts.x*0.5f), IM_ROUND(y + (row_h - fs)*0.5f)), Color(Muted), st.EmptyText);
        }
        dl->PopClipRect();
        dl->PopClipRect();
        ImGui::End();
    }

private:
    struct Row
    {
        ImGuiID Id = 0;
        char    Label[64] = {};
        char    Value[32] = {};
        char    Detail[32] = {};
        char    Key[32] = {};
        char    Icon[8] = {};
        RowTone Tone = RowTone::Normal;
        bool    Seen = false;
        float   Presence = 0.0f;        // 0..1 giris/cikis
        float   ActiveT = 0.0f;         // 0..1 altin vurgu
        float   DimT = 0.0f;            // 0..1 soluk
    };

    static float KeycapWidth(ImFont* font, float size, const char* key, float s)
    {
        return IM_ROUND(ImMax(34.0f*s, font->CalcTextSizeA(size, FLT_MAX, 0.0f, key).x + 14.0f*s));
    }

    std::vector<Row> rows_;
    int   cursor_ = 0;
    int   seen_ = 0;
    int   last_count_ = -1;
    float panel_ = 0.0f;
    float empty_ = 1.0f;
    float width_ = 0.0f;
    float last_width_ = 0.0f;       // sag kenara yaslama icin onceki frame genisligi
    float flash_ = 0.0f;
};
}
