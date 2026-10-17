#pragma once

#include "menu_vgui.h"
#include "menu_vgui_glyph.h"
#include "IconsLucide.h"
#include <cstdarg>
#include <cstring>
#include <vector>

// Ekran HUD parcalari. Hepsi background draw list'e cizilir: menunun ve panellerin arkasinda kalir, girdi almaz.
//   Watermark          sag ust: marka + kullanici / fps / ping / tick / saat segmentleri, genislikler yumusak degisir
//   NotificationStack  sol ust: tur renkli ikonlu bildirim kartlari, kalan sure cubugu, ayni bildirim tekrarlaninca birlesir
//   HitLog             bildirimlerin altinda: renkli parcali vurus / iska satirlari, en yenisi altta
// Olculer verilen font boyutundan turetilir (IndicatorStack::ScreenFontSize ile ayni ekran olcegi).
// Warning: UI thread'e ozel. Oyun olaylarini (player_hurt, bullet_impact vb.) oyun thread'inde kuyruga al, UI thread'inde Push et.
namespace MenuVgui
{
inline ImU32 HudGreen()  { return ImGui::GetColorU32(IM_COL32(118, 205, 98, 255)); }
inline ImU32 HudOrange() { return ImGui::GetColorU32(IM_COL32(232, 160, 64, 255)); }
inline ImU32 HudRed()    { return ImGui::GetColorU32(IM_COL32(224, 86, 66, 255)); }

// Golgeli kabarik VGUI govde (gostergeler / paneller ile ayni).
inline void HudPanel(ImDrawList* dl, const ImRect& r, ImU32 face, float s)
{
    const float shadow = IM_ROUND(ImMax(1.0f, 2.0f*s));
    dl->AddRectFilled(ImVec2(r.Min.x + shadow, r.Min.y + shadow), ImVec2(r.Max.x + shadow, r.Max.y + shadow),
                      ImGui::GetColorU32(IM_COL32(0, 0, 0, 90)));
    Bevel(dl, r, face, true, s);
}

// Giris: soldan hafif yaylanarak kayar (OutBack), cikis ayni egri geri.
inline float HudSlideX(float left, float width, float margin, float presence)
{
    return IM_ROUND(left - (width + margin)*(1.0f - Anim::Ease::OutBack(presence, 1.2f)));
}

// ------------------------------------------------------------------------------------------------------------------
// Watermark
// ------------------------------------------------------------------------------------------------------------------
struct WatermarkSegment
{
    const char* Icon = nullptr;
    char        Text[32] = {};
};

class Watermark
{
public:
    static constexpr int MaxSegments = 8;

    // Sag ust koseye yaslanir; ustten kayarak gelir. Segment genislikleri metin degistikce yumusak degisir.
    void Draw(ImFont* bold, ImFont* regular, float font_size, const char* brand_icon, const char* brand,
              const WatermarkSegment* segments, int count, bool show)
    {
        const float dt = Anim::Dt();
        presence_ = Anim::Smooth(presence_, show ? 1.0f : 0.0f, 12.0f, dt);
        bool moving = presence_ != (show ? 1.0f : 0.0f);
        if (presence_ <= 0.0f) { if (moving) Anim::Request(); return; }
        count = ImClamp(count, 0, MaxSegments);

        const float fs = IM_ROUND(font_size), s = fs/24.0f;
        const float small = IM_ROUND(fs*0.85f);
        const float pad = IM_ROUND(10.0f*s), gap = IM_ROUND(6.0f*s), sep = IM_ROUND(9.0f*s), line = 2.0f;
        const float h = IM_ROUND(fs + 12.0f*s);
        const float icon_box = IM_ROUND(fs*0.9f);
        const float margin = IM_ROUND(8.0f*s);

        const float brand_w = (brand_icon ? icon_box + gap : 0.0f) + bold->CalcTextSizeA(fs, FLT_MAX, 0.0f, brand).x;
        float total = pad + brand_w + pad;
        for (int i = 0; i < count; ++i)
        {
            const float target = (segments[i].Icon ? icon_box + gap : 0.0f) + regular->CalcTextSizeA(small, FLT_MAX, 0.0f, segments[i].Text).x;
            widths_[i] = widths_[i] <= 0.0f ? target : Anim::Smooth(widths_[i], target, 18.0f, dt);
            moving |= widths_[i] != target;
            total += line + sep + IM_ROUND(widths_[i]) + pad;
        }
        if (moving) Anim::Request();

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float x1 = vp->WorkPos.x + vp->WorkSize.x - margin;
        const float y0 = IM_ROUND(vp->WorkPos.y + margin - (1.0f - Anim::Ease::OutCubic(presence_))*(h + margin));
        const ImRect r(ImVec2(IM_ROUND(x1 - total), y0), ImVec2(x1, y0 + h));
        const float cy = r.Min.y + IM_ROUND(h*0.5f);

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->PushClipRectFullScreen();
        Anim::AlphaScope alpha(presence_);
        HudPanel(dl, r, Color(Background), s);

        float x = r.Min.x + pad;
        if (brand_icon)
        {
            GlyphCentered(dl, regular, icon_box, ImVec2(x + icon_box*0.5f, cy), Color(Gold), brand_icon);
            x += icon_box + gap;
        }
        TextCapCentered(dl, bold, fs, x, cy, Color(Gold), brand);
        x = r.Min.x + pad + brand_w + pad;
        for (int i = 0; i < count; ++i)
        {
            // Cokuk ayirici: koyu + acik 1px cizgi
            const float sy0 = r.Min.y + IM_ROUND(7.0f*s), sy1 = r.Max.y - IM_ROUND(7.0f*s);
            dl->AddRectFilled(ImVec2(x, sy0), ImVec2(x + 1.0f, sy1), Color(DarkEdge));
            dl->AddRectFilled(ImVec2(x + 1.0f, sy0), ImVec2(x + 2.0f, sy1), Color(LightEdge));
            x += line + sep;
            const float w = IM_ROUND(widths_[i]);
            dl->PushClipRect(ImVec2(x, r.Min.y), ImVec2(x + w, r.Max.y), true);
            float sx = x;
            if (segments[i].Icon)
            {
                GlyphCentered(dl, regular, icon_box, ImVec2(sx + icon_box*0.5f, cy), Color(Muted), segments[i].Icon);
                sx += icon_box + gap;
            }
            TextCapCentered(dl, regular, small, sx, cy, Color(Text), segments[i].Text);
            dl->PopClipRect();
            x += w + pad;
        }
        dl->PopClipRect();
        bottom_ = r.Max.y;
    }

    float Bottom() const { return bottom_; }

private:
    float presence_ = 0.0f;
    float widths_[MaxSegments] = {};
    float bottom_ = 0.0f;
};

// ------------------------------------------------------------------------------------------------------------------
// Notifications
// ------------------------------------------------------------------------------------------------------------------
enum class NotifyKind { Info, Success, Warning, Error };

class NotificationStack
{
public:
    static constexpr int MaxVisible = 4;

    // Ayni tur + baslik + metin ekrandaysa yeni kart acilmaz: sure yenilenir, kart parlar, sagda "x2" sayaci artar.
    void Push(NotifyKind kind, const char* title, const char* text, float duration = 4.0f)
    {
        if (text == nullptr) text = "";
        for (Toast& t : toasts_)
            if (!t.Closing && t.Kind == kind && strcmp(t.Title, title) == 0 && strcmp(t.Text, text) == 0)
            {
                t.Age = 0.0f;
                t.Duration = duration;
                t.Flash = 1.0f;
                ++t.Repeat;
                return;
            }
        Toast t;
        t.Kind = kind;
        t.Duration = ImMax(0.5f, duration);
        ImStrncpy(t.Title, title, IM_ARRAYSIZE(t.Title));
        ImStrncpy(t.Text, text, IM_ARRAYSIZE(t.Text));
        toasts_.insert(toasts_.begin(), t);                         // en yenisi ustte
        int open = 0;
        for (Toast& old : toasts_)
            if (!old.Closing && ++open > MaxVisible) old.Closing = true;
    }

    // origin: yiginin sol ust kosesi. Donus: yiginin (animasyonlu) alt kenari, alttaki HUD'un baslangici.
    float Draw(ImFont* bold, ImFont* regular, float font_size, ImVec2 origin, bool show)
    {
        const float dt = Anim::Dt();
        const float fs = IM_ROUND(font_size), s = fs/24.0f;
        const float small = IM_ROUND(fs*0.82f);
        const float pad = IM_ROUND(10.0f*s);
        const float spacing = IM_ROUND(ImMax(3.0f, 6.0f*s));
        const float width = IM_ROUND(330.0f*s);
        const float icon_box = IM_ROUND(fs + 10.0f*s);
        const float bar_h = IM_ROUND(ImMax(2.0f, 3.0f*s));
        const float text_x = pad + icon_box + IM_ROUND(10.0f*s);
        const float wrap = width - text_x - pad;

        bool moving = false;
        for (Toast& t : toasts_)
        {
            if (!show) t.Closing = true;
            t.Age += dt;
            if (t.Age >= t.Duration) t.Closing = true;
            t.Presence = Anim::Smooth(t.Presence, t.Closing ? 0.0f : 1.0f, t.Closing ? 14.0f : 10.0f, dt);
            t.Flash = Anim::Smooth(t.Flash, 0.0f, 6.0f, dt);
            const float msg_h = t.Text[0] ? regular->CalcTextSizeA(small, FLT_MAX, wrap, t.Text).y : 0.0f;
            t.H = IM_ROUND(ImMax(icon_box, fs + (msg_h > 0.0f ? IM_ROUND(4.0f*s) + msg_h : 0.0f)) + pad*2.0f + bar_h);
            moving = true;                                          // kalan sure cubugu her frame ilerler
        }
        for (size_t i = 0; i < toasts_.size();)
        {
            if (toasts_[i].Closing && toasts_[i].Presence <= 0.0f) toasts_.erase(toasts_.begin() + (ptrdiff_t)i);
            else ++i;
        }
        float slot = origin.y;
        for (Toast& t : toasts_)
        {
            t.Y = t.Y == FLT_MAX ? slot : Anim::Smooth(t.Y, slot, 14.0f, dt);
            slot += (t.H + spacing)*Anim::Ease::OutCubic(t.Presence);
        }
        bottom_ = Anim::Smooth(bottom_ <= 0.0f ? slot : bottom_, slot, 14.0f, dt);
        if (moving || bottom_ != slot) Anim::Request();
        if (toasts_.empty()) return bottom_;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->PushClipRectFullScreen();
        for (const Toast& t : toasts_)
        {
            const float x = HudSlideX(origin.x, width, origin.x, t.Presence);
            const ImRect r(ImVec2(x, IM_ROUND(t.Y)), ImVec2(x + width, IM_ROUND(t.Y) + t.H));
            Anim::AlphaScope alpha(ImSaturate(t.Presence*1.4f));
            const ImU32 accent = KindColor(t.Kind);
            HudPanel(dl, r, Anim::LerpColor(Color(Background), Color(ScrollTrack), t.Flash*0.8f), s);

            // Ikon: cokuk kutuda tur renginde
            const ImRect icon(ImVec2(r.Min.x + pad, r.Min.y + pad), ImVec2(r.Min.x + pad + icon_box, r.Min.y + pad + icon_box));
            Bevel(dl, icon, Color(Field), false, s);
            GlyphCentered(dl, regular, IM_ROUND(fs*0.95f), icon.GetCenter(), accent, KindIcon(t.Kind));

            // Baslik (+ tekrar sayaci) ve sarili mesaj
            const float tx = r.Min.x + text_x;
            const float title_cy = r.Min.y + pad + IM_ROUND(fs*0.5f);
            TextCapCentered(dl, bold, fs, tx, title_cy, Color(Text), t.Title);
            if (t.Repeat > 0)
            {
                char repeat[16];
                ImFormatString(repeat, IM_ARRAYSIZE(repeat), "x%d", t.Repeat + 1);
                const float rw = regular->CalcTextSizeA(small, FLT_MAX, 0.0f, repeat).x;
                TextCapCentered(dl, regular, small, r.Max.x - pad - rw, title_cy, accent, repeat);
            }
            if (t.Text[0])
                dl->AddText(regular, small, ImVec2(tx, r.Min.y + pad + fs + IM_ROUND(4.0f*s)), Color(Muted), t.Text, nullptr, wrap);

            // Kalan sure: cokuk ray + tur renginde dolgu
            const float remaining = ImSaturate(1.0f - t.Age/t.Duration);
            const ImRect bar(ImVec2(r.Min.x + IM_ROUND(3.0f*s), r.Max.y - IM_ROUND(3.0f*s) - bar_h), ImVec2(r.Max.x - IM_ROUND(3.0f*s), r.Max.y - IM_ROUND(3.0f*s)));
            dl->AddRectFilled(bar.Min, bar.Max, Color(Track));
            dl->AddRectFilled(bar.Min, ImVec2(IM_ROUND(ImLerp(bar.Min.x, bar.Max.x, remaining)), bar.Max.y), accent);
        }
        dl->PopClipRect();
        return bottom_;
    }

    static ImU32 KindColor(NotifyKind kind)
    {
        switch (kind)
        {
        case NotifyKind::Success: return HudGreen();
        case NotifyKind::Warning: return HudOrange();
        case NotifyKind::Error:   return HudRed();
        default:                  return Color(Gold);
        }
    }

    static const char* KindIcon(NotifyKind kind)
    {
        switch (kind)
        {
        case NotifyKind::Success: return ICON_LC_CIRCLE_CHECK;
        case NotifyKind::Warning: return ICON_LC_TRIANGLE_ALERT;
        case NotifyKind::Error:   return ICON_LC_CIRCLE_X;
        default:                  return ICON_LC_INFO;
        }
    }

private:
    struct Toast
    {
        NotifyKind Kind = NotifyKind::Info;
        char  Title[48] = {};
        char  Text[160] = {};
        float Age = 0.0f;
        float Duration = 4.0f;
        float Presence = 0.0f;
        float Flash = 0.0f;
        float H = 0.0f;
        float Y = FLT_MAX;
        int   Repeat = 0;
        bool  Closing = false;
    };

    std::vector<Toast> toasts_;
    float bottom_ = 0.0f;
};

// ------------------------------------------------------------------------------------------------------------------
// Hit / miss log
// ------------------------------------------------------------------------------------------------------------------
enum class LogTone { Text, Muted, Gold, Good, Bad };

// Renkli parcalardan olusan tek satir: LogLine().Add(LogTone::Text, "Hit ").Add(LogTone::Gold, "%s", name) ...
struct LogLine
{
    static constexpr int MaxSpans = 14;
    struct Span { char Text[48]; LogTone Tone; };
    Span Spans[MaxSpans];
    int  Count = 0;

    LogLine& Add(LogTone tone, const char* fmt, ...)
    {
        if (Count >= MaxSpans) return *this;
        va_list args;
        va_start(args, fmt);
        ImFormatStringV(Spans[Count].Text, IM_ARRAYSIZE(Spans[Count].Text), fmt, args);
        va_end(args);
        Spans[Count].Tone = tone;
        ++Count;
        return *this;
    }
};

class HitLog
{
public:
    static constexpr int MaxLines = 8;

    void Push(bool hit, const LogLine& line, float duration = 6.0f)
    {
        Entry e;
        e.Hit = hit;
        e.Line = line;
        e.Duration = ImMax(0.5f, duration);
        lines_.push_back(e);                                        // en yenisi altta (konsol gibi)
        int open = 0;
        for (auto it = lines_.rbegin(); it != lines_.rend(); ++it)
            if (!it->Closing && ++open > MaxLines) it->Closing = true;
    }

    // origin: ilk satirin sol ustu. limit_y: bu cizginin altina tasan satirlar bekler (ekran ortasindaki gostergelere binmesin).
    void Draw(ImFont* regular, float font_size, ImVec2 origin, float limit_y, bool show)
    {
        const float dt = Anim::Dt();
        const float fs = IM_ROUND(font_size), s = fs/24.0f;
        const float size = IM_ROUND(fs*0.85f);
        const float pad = IM_ROUND(9.0f*s);
        const float h = IM_ROUND(size + 12.0f*s);
        const float spacing = IM_ROUND(ImMax(2.0f, 4.0f*s));
        const float icon_box = IM_ROUND(size*1.05f);

        bool moving = false;
        for (Entry& e : lines_)
        {
            if (!show) e.Closing = true;
            e.Age += dt;
            if (e.Age >= e.Duration) e.Closing = true;
            e.Presence = Anim::Smooth(e.Presence, e.Closing ? 0.0f : 1.0f, e.Closing ? 9.0f : 12.0f, dt);
            moving |= e.Presence != (e.Closing ? 0.0f : 1.0f);
        }
        for (size_t i = 0; i < lines_.size();)
        {
            if (lines_[i].Closing && lines_[i].Presence <= 0.0f) lines_.erase(lines_.begin() + (ptrdiff_t)i);
            else ++i;
        }
        float slot = origin.y;
        for (Entry& e : lines_)
        {
            e.Y = e.Y == FLT_MAX ? slot : Anim::Smooth(e.Y, slot, 14.0f, dt);
            moving |= e.Y != slot;
            slot += (h + spacing)*Anim::Ease::OutCubic(e.Presence);
        }
        if (moving) Anim::Request();
        if (lines_.empty()) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->PushClipRectFullScreen();
        for (const Entry& e : lines_)
        {
            if (e.Y + h > limit_y) continue;                        // yer acilinca gorunur
            float w = pad + icon_box + IM_ROUND(7.0f*s) + pad;
            for (int i = 0; i < e.Line.Count; ++i)
                w += regular->CalcTextSizeA(size, FLT_MAX, 0.0f, e.Line.Spans[i].Text).x;
            w = IM_ROUND(w);
            const float x = HudSlideX(origin.x, w, origin.x, e.Presence);
            const ImRect r(ImVec2(x, IM_ROUND(e.Y)), ImVec2(x + w, IM_ROUND(e.Y) + h));
            const float cy = r.Min.y + IM_ROUND(h*0.5f);
            Anim::AlphaScope alpha(ImSaturate(e.Presence*1.4f));
            HudPanel(dl, r, Color(Background), s);
            GlyphCentered(dl, regular, icon_box, ImVec2(r.Min.x + pad + icon_box*0.5f, cy), e.Hit ? Color(Gold) : HudRed(),
                          e.Hit ? ICON_LC_CROSSHAIR : ICON_LC_CIRCLE_X);
            float tx = r.Min.x + pad + icon_box + IM_ROUND(7.0f*s);
            for (int i = 0; i < e.Line.Count; ++i)
            {
                const LogLine::Span& span = e.Line.Spans[i];
                TextCapCentered(dl, regular, size, tx, cy, ToneColor(span.Tone), span.Text);
                tx += regular->CalcTextSizeA(size, FLT_MAX, 0.0f, span.Text).x;
            }
        }
        dl->PopClipRect();
    }

    static ImU32 ToneColor(LogTone tone)
    {
        switch (tone)
        {
        case LogTone::Muted: return Color(Muted);
        case LogTone::Gold:  return Color(Gold);
        case LogTone::Good:  return HudGreen();
        case LogTone::Bad:   return HudRed();
        default:             return Color(Text);
        }
    }

private:
    struct Entry
    {
        LogLine Line;
        bool    Hit = true;
        float   Age = 0.0f;
        float   Duration = 6.0f;
        float   Presence = 0.0f;
        float   Y = FLT_MAX;
        bool    Closing = false;
    };

    std::vector<Entry> lines_;
};
}
