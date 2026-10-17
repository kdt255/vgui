#pragma once

#include "imgui.h"
#include "imgui_internal.h"

namespace MenuVgui
{
// Tek glifi (Lucide ikonu vb.) gorunen piksel kutusuna gore tam ortalar. Satir kutusuna gore ortalamak, merge
// edilen ikonlarin GlyphOffset'i ve glifin kendi bosluklari yuzunden ikonu asagi/yana kaydirir.
inline void GlyphCentered(ImDrawList* dl, ImFont* font, float size, ImVec2 center, ImU32 col, const char* utf8)
{
    if (utf8 == nullptr || utf8[0] == 0) return;
    unsigned int codepoint = 0;
    ImTextCharFromUtf8(&codepoint, utf8, nullptr);
    ImFontBaked* baked = font->GetFontBaked(size);
    const ImFontGlyph* glyph = baked ? baked->FindGlyphNoFallback((ImWchar)codepoint) : nullptr;
    if (glyph == nullptr || glyph->X1 <= glyph->X0)
    {
        const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, utf8);
        dl->AddText(font, size, ImVec2(IM_ROUND(center.x - ts.x*0.5f), IM_ROUND(center.y - ts.y*0.5f)), col, utf8);
        return;
    }
    const float scale = size / baked->Size;
    const ImVec2 pos(IM_ROUND(center.x - (glyph->X0 + glyph->X1)*0.5f*scale), IM_ROUND(center.y - (glyph->Y0 + glyph->Y1)*0.5f*scale));
    dl->AddText(font, size, pos, col, utf8);
}

// Metni dikeyde ortalamak icin satir ustunden buyuk harf kutusunun ('H' glifi: taban cizgisi..buyuk harf ust siniri)
// ortasina olan mesafe. Satir kutusuna (ascent+descent) gore ortalamak, alt pay yuzunden metni ikonlara/kutulara gore
// optik olarak asagi-yukari kaydirir; rakamlar Tahoma'da buyuk harf yuksekliginde oldugu icin degerler de ayni cizgiye oturur.
inline float CapCenterOffset(ImFont* font, float size)
{
    ImFontBaked* baked = font->GetFontBaked(size);
    const ImFontGlyph* glyph = baked ? baked->FindGlyphNoFallback((ImWchar)'H') : nullptr;
    if (glyph == nullptr || glyph->Y1 <= glyph->Y0) return size*0.5f;
    return (glyph->Y0 + glyph->Y1)*0.5f*(size/baked->Size);
}

// Harf araligi acik metin (kucuk buyuk-harf basliklar icin): her glif ayri cizilir, aralarina 'tracking' eklenir.
// Donus: cizilen genislik. dl == nullptr ise yalniz olcer.
inline float TextTracked(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text, float tracking)
{
    float x = pos.x;
    bool any = false;
    for (const char* c = text; c && *c;)
    {
        unsigned int codepoint = 0;
        const int n = ImMax(1, ImTextCharFromUtf8(&codepoint, c, nullptr));
        if (dl) dl->AddText(font, size, ImVec2(IM_ROUND(x), pos.y), col, c, c + n);
        x += font->CalcTextSizeA(size, FLT_MAX, 0.0f, c, c + n).x + tracking;
        any = true;
        c += n;
    }
    return any ? x - pos.x - tracking : 0.0f;
}

// Metni 'center.y' cizgisine buyuk harf kutusuyla ortali, 'x'ten baslayarak cizer.
inline void TextCapCentered(ImDrawList* dl, ImFont* font, float size, float x, float center_y, ImU32 col, const char* text)
{
    dl->AddText(font, size, ImVec2(IM_ROUND(x), IM_ROUND(center_y - CapCenterOffset(font, size))), col, text);
}
}
