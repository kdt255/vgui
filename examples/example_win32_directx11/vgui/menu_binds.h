#pragma once

#include "menu_vgui_keybind.h"
#include "menu_vgui_multicombo.h"
#include "menu_vgui_glyph.h"
#include <climits>
#include <cstring>
#include <memory>
#include <vector>

// Sag tik bind sistemi: menudeki her deger widget'i (checkbox, slider, combo, coklu combo, sayi kutusu, renk) icin
// birden cok "tus -> deger" bindi. Bind aktifken widget'in degiskenine bindin degeri yazilir (ozellik kodu degiskeni
// dogrudan okur, ek API gerekmez); hicbir bind aktif degilken menude ayarlanan taban deger geri yuklenir.
// Ayni anda birden cok bind aktifse en son aktiflesen kazanir. Bind'i olan widget'in kontrol kutusunun sag ust kosesinde
// ucgen centik durur; tiklayinca bind listesi acilir (ekle / duzenle / sil), hover'da bir sey acilmaz.
// Override suresince centik altin yanar, widget kilitli gorunur (degeri gosterir, duzenlenmez).
//
// Kullanim: her widget cagrisini Bindable() ile sar (Declare* ile tipini/etiketini bildir), her frame UI'dan once
// Update() cagir. Kayit anahtari degiskenin adresidir: static/global degiskenler sayfa cizilmese de bind'e uyar.
// Warning: UI thread'e ozel. Ozellik kodu baska thread'deyse degerleri frame sonunda kopyalayip oradan okusun;
// Update() degiskenlere yazdigi icin ayni degiskeni oyun thread'inden yazmak yaris kosulu olusturur.
namespace MenuBinds
{
enum class ValueType { Bool, Float, Int, Mask, Color };

struct Value
{
    bool  B = false;
    float F = 0.0f;
    int   I = 0;
    float C[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

inline Value BoolValue(bool v)   { Value r; r.B = v; return r; }
inline Value FloatValue(float v) { Value r; r.F = v; return r; }
inline Value IntValue(int v)     { Value r; r.I = v; return r; }

struct Bind
{
    MenuVgui::KeyBind Key;
    Value   V;
    ImGuiID Serial = 0;             // satir kimligi (keybind list, ImGui ID)
    int     ActivatedFrame = -1;    // en son aktiflesen bind kazanir
    bool    On = false;             // tusu basili / toggle acik / Always
    bool    Applied = false;        // bu frame degeri yazan bind
};

struct Element
{
    void*              Ptr = nullptr;
    ValueType          Type = ValueType::Bool;
    char               Label[48] = {};
    float              Min = 0.0f, Max = 1.0f;
    const char*        Format = "%.0f";         // Warning: string literal olmali (adres saklanir)
    bool               FloatInput = false;      // true: slider degil sayi kutusu
    const char* const* Items = nullptr;         // Warning: static dizi olmali (adres saklanir)
    int                ItemCount = 0;
    bool               Alpha = true;
    std::vector<Bind>  Binds;
    bool               Overriding = false;
    Value              Base;                    // override oncesi menu degeri
    ImRect             NotchHit;                // son cizilen bind centiginin tiklama alani (bir frame gecikmeli)
    bool               NotchHovered = false;
};

inline std::vector<std::unique_ptr<Element>>& Elements()
{
    static std::vector<std::unique_ptr<Element>> elements;
    return elements;
}

inline Element* Find(const void* ptr)
{
    for (const std::unique_ptr<Element>& e : Elements())
        if (e->Ptr == ptr) return e.get();
    return nullptr;
}

inline bool IsOverriding(const void* ptr)
{
    const Element* e = Find(ptr);
    return e != nullptr && e->Overriding;
}

inline Element& Declare(void* ptr, ValueType type, const char* label)
{
    Element* e = Find(ptr);
    if (e == nullptr)
    {
        Elements().push_back(std::make_unique<Element>());
        e = Elements().back().get();
        e->Ptr = ptr;
    }
    e->Type = type;
    if (label) ImStrncpy(e->Label, label, IM_ARRAYSIZE(e->Label));
    return *e;
}

inline Element& DeclareBool(bool* v, const char* label) { return Declare(v, ValueType::Bool, label); }

inline Element& DeclareSlider(float* v, const char* label, float min, float max, const char* format)
{
    Element& e = Declare(v, ValueType::Float, label);
    e.Min = min; e.Max = max; e.Format = format ? format : "%.3f"; e.FloatInput = false;
    return e;
}

inline Element& DeclareInput(float* v, const char* label)
{
    Element& e = Declare(v, ValueType::Float, label);
    e.Format = "%g"; e.FloatInput = true;
    return e;
}

inline Element& DeclareCombo(int* v, const char* label, const char* const* items, int count)
{
    Element& e = Declare(v, ValueType::Int, label);
    e.Items = items; e.ItemCount = count;
    return e;
}

inline Element& DeclareMask(int* v, const char* label, const char* const* items, int count)
{
    Element& e = Declare(v, ValueType::Mask, label);
    e.Items = items; e.ItemCount = count;
    return e;
}

inline Element& DeclareColor(float* rgba, const char* label, bool alpha)
{
    Element& e = Declare(rgba, ValueType::Color, label);
    e.Alpha = alpha;
    return e;
}

inline Value Read(const Element& e)
{
    Value v;
    switch (e.Type)
    {
    case ValueType::Bool:  v.B = *static_cast<const bool*>(e.Ptr); break;
    case ValueType::Float: v.F = *static_cast<const float*>(e.Ptr); break;
    case ValueType::Int:
    case ValueType::Mask:  v.I = *static_cast<const int*>(e.Ptr); break;
    case ValueType::Color: memcpy(v.C, e.Ptr, sizeof(float)*(e.Alpha ? 4 : 3)); break;
    }
    return v;
}

inline void Write(const Element& e, const Value& v)
{
    switch (e.Type)
    {
    case ValueType::Bool:  *static_cast<bool*>(e.Ptr) = v.B; break;
    case ValueType::Float: *static_cast<float*>(e.Ptr) = v.F; break;
    case ValueType::Int:
    case ValueType::Mask:  *static_cast<int*>(e.Ptr) = v.I; break;
    case ValueType::Color: memcpy(e.Ptr, v.C, sizeof(float)*(e.Alpha ? 4 : 3)); break;
    }
}

inline ImGuiID NextSerial()
{
    static ImGuiID serial = 0x4b4c0000;
    return ++serial;
}

inline Bind& AddBind(Element& e, ImGuiKey key, MenuVgui::BindMode mode, const Value& v)
{
    Bind b;
    b.Key.Key = key;
    b.Key.Mode = mode;
    b.V = v;
    b.Serial = NextSerial();
    e.Binds.push_back(b);
    return e.Binds.back();
}

inline void FormatValue(const Element& e, const Value& v, char* out, size_t size)
{
    switch (e.Type)
    {
    case ValueType::Bool:
        ImStrncpy(out, v.B ? "on" : "off", size);
        break;
    case ValueType::Float:
        ImFormatString(out, size, e.Format, v.F);
        break;
    case ValueType::Int:
        ImStrncpy(out, (e.Items && v.I >= 0 && v.I < e.ItemCount) ? e.Items[v.I] : "?", size);
        break;
    case ValueType::Mask:
    {
        int count = 0, first = -1;
        for (int i = 0; i < e.ItemCount; ++i)
            if (v.I & (1 << i)) { if (first < 0) first = i; ++count; }
        if (count == 0)      ImStrncpy(out, "none", size);
        else if (count == 1) ImStrncpy(out, e.Items[first], size);
        else                 ImFormatString(out, size, "%d selected", count);
        break;
    }
    case ValueType::Color:
        ImFormatString(out, size, "#%02X%02X%02X", MenuVgui::ColorByte(v.C[0]), MenuVgui::ColorByte(v.C[1]), MenuVgui::ColorByte(v.C[2]));
        break;
    }
}

// Her frame bir kez, UI'dan once (ImGui::NewFrame sonrasi).
inline void Update()
{
    const int frame = ImGui::GetFrameCount();
    for (const std::unique_ptr<Element>& ep : Elements())
    {
        Element& e = *ep;
        int best = -1, best_frame = INT_MIN;
        for (int i = 0; i < (int)e.Binds.size(); ++i)
        {
            Bind& b = e.Binds[i];
            MenuVgui::UpdateBind(b.Key);
            const bool always = b.Key.Mode == MenuVgui::BindMode::Always;
            b.On = always || (b.Key.Key != ImGuiKey_None && b.Key.Active);
            if (!b.On) b.ActivatedFrame = -1;
            else if (b.ActivatedFrame < 0) b.ActivatedFrame = frame;
            b.Applied = false;
            if (b.On && b.ActivatedFrame >= best_frame) { best = i; best_frame = b.ActivatedFrame; }
        }
        if (best >= 0)
        {
            if (!e.Overriding) { e.Base = Read(e); e.Overriding = true; }
            e.Binds[best].Applied = true;
            Write(e, e.Binds[best].V);
        }
        else if (e.Overriding)
        {
            Write(e, e.Base);
            e.Overriding = false;
        }
    }
}

// Tusla tetiklenen (Always olmayan) bir bind su an acik mi: keybind list'in menu kapaliyken gorunme kosulu.
inline bool AnyKeyActive()
{
    for (const std::unique_ptr<Element>& e : Elements())
        for (const Bind& b : e->Binds)
            if (b.On && b.Key.Mode != MenuVgui::BindMode::Always) return true;
    return false;
}

// --- Popup ---
enum class Page { Menu, Editor, List };

struct PopupState
{
    void*  Ptr = nullptr;
    Page   View = Page::Menu;
    Page   Shown = Page::Menu;
    int    EditIndex = -1;          // -1: yeni bind
    Bind   Draft;
    ImVec2 Anchor;
    float  Reveal = 1.0f;
    bool   ListenNext = false;      // editor acilinca keycap dogrudan tus beklesin
};

inline PopupState& Popup()
{
    static PopupState state;
    return state;
}

inline void OpenEditor(Element& e, int index)
{
    PopupState& st = Popup();
    st.View = Page::Editor;
    st.EditIndex = index;
    if (index >= 0)
    {
        st.Draft = e.Binds[index];
        st.ListenNext = false;
    }
    else
    {
        st.Draft = Bind();
        st.Draft.V = e.Overriding ? e.Base : Read(e);
        if (e.Type == ValueType::Bool) st.Draft.V.B = !st.Draft.V.B;    // en sik kullanim: kapaliyi ac / aciyi kapat
        st.ListenNext = true;
    }
    st.Draft.Key.Toggled = false;
    st.Draft.Key.Active = false;                                    // taslak UpdateBind'e girmez: keycap aktif gorunmesin
}

// Ikon + metin + (istege bagli) sayac rozetli VGUI menu satiri.
inline bool MenuRow(const char* icon, const char* label, int badge, bool enabled, float s)
{
    using namespace MenuVgui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    const float small = IM_ROUND(fs*0.8f);
    const float h = IM_ROUND(fs + 12.0f*s);
    const float w = ImGui::GetContentRegionAvail().x;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::BeginDisabled(!enabled);
    const bool pressed = ImGui::InvisibleButton(label, ImVec2(w, h), ImGuiButtonFlags_EnableNav);
    const bool hovered = ImGui::IsItemHovered();
    const ImGuiID id = ImGui::GetItemID();
    ImGui::EndDisabled();
    const float hot = Animate(id, hovered && enabled ? 1.0f : 0.0f, 0.0f, 24.0f);
    ImDrawList* dl = window->DrawList;
    if (hot > 0.001f)
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), Anim::ScaleAlpha(Color(Selection), hot));
    const float cy = p.y + IM_ROUND(h*0.5f);
    const float icon_box = IM_ROUND(fs);
    GlyphCentered(dl, font, fs, ImVec2(p.x + 8.0f*s + icon_box*0.5f, cy),
                  enabled ? Anim::LerpColor(Color(Gold), Color(White), hot) : Color(Muted), icon);
    dl->AddText(ImVec2(p.x + 8.0f*s + icon_box + 10.0f*s, IM_ROUND(cy - fs*0.5f)),
                enabled ? Anim::LerpColor(Color(Text), Color(White), hot) : Color(Muted), label);
    if (badge > 0)
    {
        char text[16];
        ImFormatString(text, IM_ARRAYSIZE(text), "%d", badge);
        const ImVec2 ts = font->CalcTextSizeA(small, FLT_MAX, 0.0f, text);
        const float bw = IM_ROUND(ImMax(26.0f*s, ts.x + 12.0f*s)), bh = IM_ROUND(small + 6.0f*s);
        const ImRect b(ImVec2(p.x + w - 8.0f*s - bw, cy - IM_ROUND(bh*0.5f)), ImVec2(p.x + w - 8.0f*s, cy - IM_ROUND(bh*0.5f) + bh));
        Bevel(dl, b, Color(Field), false, s);
        dl->AddText(font, small, ImVec2(IM_ROUND(b.GetCenter().x - ts.x*0.5f), IM_ROUND(b.GetCenter().y - small*0.5f)), Color(Gold), text);
    }
    ImGui::RenderNavCursor(ImRect(p, ImVec2(p.x + w, p.y + h)), id);
    return pressed && enabled;
}

inline bool IconButton(const char* id, const char* icon, float size, float s)
{
    using namespace MenuVgui;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(id, ImVec2(size, size), ImGuiButtonFlags_EnableNav);
    const bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
    const float hot = Animate(ImGui::GetItemID(), hovered ? 1.0f : 0.0f, 0.0f, 24.0f);
    const ImRect r(p, ImVec2(p.x + size, p.y + size));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Bevel(dl, r, Anim::LerpColor(Color(Background), Color(ScrollTrack), hot), !held, s);
    GlyphCentered(dl, ImGui::GetFont(), IM_ROUND(ImGui::GetFontSize()*0.8f), r.GetCenter(), Anim::LerpColor(Color(Muted), Color(Gold), hot), icon);
    ImGui::RenderNavCursor(r, ImGui::GetItemID());
    return pressed;
}

inline void PopupPageMenu(Element& e, float s)
{
    PopupState& st = Popup();
    if (MenuRow(ICON_LC_PLUS, "New bind", 0, true, s))
        OpenEditor(e, -1);
    if (MenuRow(ICON_LC_LIST, "Show all binds", (int)e.Binds.size(), !e.Binds.empty(), s))
        st.View = Page::List;
}

inline void PopupPageEditor(Element& e, float s)
{
    using namespace MenuVgui;
    PopupState& st = Popup();
    Bind& d = st.Draft;
    const bool listen = st.ListenNext;
    st.ListenNext = false;
    BindRow("Key", &d.Key, s, false, listen);

    static const char* const modes[] = { "Hold", "Toggle", "Always" };
    int mode = (int)d.Key.Mode;
    if (Combo("Mode", &mode, modes, IM_ARRAYSIZE(modes), s, true))
    {
        d.Key.Mode = (BindMode)mode;
        d.Key.Toggled = false;
    }
    switch (e.Type)
    {
    case ValueType::Bool:
    {
        static const char* const states[] = { "Off", "On" };
        int on = d.V.B ? 1 : 0;
        if (Combo("Value", &on, states, IM_ARRAYSIZE(states), s, true)) d.V.B = on != 0;
        break;
    }
    case ValueType::Float:
        if (e.FloatInput) InputFloat("Value", &d.V.F, IM_ROUND(112.0f*s), s);
        else              Slider("Value", &d.V.F, e.Min, e.Max, e.Format, s, true);
        break;
    case ValueType::Int:   Combo("Value", &d.V.I, e.Items, e.ItemCount, s, true); break;
    case ValueType::Mask:  MultiCombo("Value", &d.V.I, e.Items, e.ItemCount, s, true); break;
    case ValueType::Color: ColorEdit("Value", d.V.C, s, e.Alpha); break;
    }

    ImGui::Dummy(ImVec2(0.0f, 2.0f*s));
    const bool valid = d.Key.Key != ImGuiKey_None || d.Key.Mode == BindMode::Always;
    const bool editing = st.EditIndex >= 0 && st.EditIndex < (int)e.Binds.size();
    const int buttons = editing ? 3 : 2;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float bw = IM_ROUND((ImGui::GetContentRegionAvail().x - spacing*(buttons - 1))/buttons);
    ImGui::BeginDisabled(!valid);
    if (ImGui::Button(editing ? "Save" : "Add bind", ImVec2(bw, 0.0f)))
    {
        d.ActivatedFrame = -1;
        d.On = d.Applied = false;
        if (editing)
        {
            d.Serial = e.Binds[st.EditIndex].Serial;
            e.Binds[st.EditIndex] = d;
        }
        else
        {
            d.Serial = NextSerial();
            e.Binds.push_back(d);
        }
        st.View = Page::List;
    }
    ImGui::EndDisabled();
    if (editing)
    {
        ImGui::SameLine();
        if (ImGui::Button("Delete", ImVec2(bw, 0.0f)))
        {
            e.Binds.erase(e.Binds.begin() + st.EditIndex);
            st.View = e.Binds.empty() ? Page::Menu : Page::List;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Back", ImVec2(bw, 0.0f)))
        st.View = editing ? Page::List : Page::Menu;
}

inline void PopupPageList(Element& e, float s)
{
    using namespace MenuVgui;
    PopupState& st = Popup();
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    const float small = HotkeyFontSize();
    const float h = IM_ROUND(fs + 12.0f*s);
    const float button = IM_ROUND(h - 6.0f*s);
    const float w = ImGui::GetContentRegionAvail().x;

    // Sutunlar tum satirlarda ayni genislikte: [tus][mod][deger] ........ [duzenle][sil]
    float key_w = 0.0f, mode_w = 0.0f;
    for (const Bind& b : e.Binds)
    {
        char key[32];
        KeyName(b.Key.Key, key, sizeof(key));
        key_w = ImMax(key_w, IM_ROUND(ImMax(44.0f*s, font->CalcTextSizeA(small, FLT_MAX, 0.0f, key).x + 16.0f*s)));
        mode_w = ImMax(mode_w, font->CalcTextSizeA(small, FLT_MAX, 0.0f, BindModeName(b.Key.Mode)).x);
    }

    int edit = -1, remove = -1;
    for (int i = 0; i < (int)e.Binds.size(); ++i)
    {
        const Bind& b = e.Binds[i];
        ImGui::PushID((int)b.Serial);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(w, h));
        const ImVec2 next = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float cy = p.y + IM_ROUND(h*0.5f);
        const float on = Animate(ImHashStr("##applied", 0, b.Serial), b.Applied ? 1.0f : 0.0f, 0.0f, 20.0f);
        const ImU32 ink = Anim::LerpColor(Color(Text), Color(Gold), on);

        char key[32], value[32];
        KeyName(b.Key.Key, key, sizeof(key));
        FormatValue(e, b.V, value, sizeof(value));
        const ImRect cap(ImVec2(p.x, cy - IM_ROUND((h - 10.0f*s)*0.5f)), ImVec2(p.x + key_w, cy - IM_ROUND((h - 10.0f*s)*0.5f) + IM_ROUND(h - 10.0f*s)));
        Bevel(dl, cap, Color(Background), true, s);
        const ImVec2 ks = font->CalcTextSizeA(small, FLT_MAX, 0.0f, key);
        dl->AddText(font, small, ImVec2(IM_ROUND(cap.GetCenter().x - ks.x*0.5f), IM_ROUND(cy - small*0.5f)),
                    b.Key.Key == ImGuiKey_None ? Color(Muted) : ink, key);
        const float mode_x = cap.Max.x + 10.0f*s;
        dl->AddText(font, small, ImVec2(IM_ROUND(mode_x), IM_ROUND(cy - small*0.5f)), Color(Muted), BindModeName(b.Key.Mode));
        const float value_x = mode_x + mode_w + 14.0f*s;
        const float buttons_x = p.x + w - button*2.0f - 6.0f*s;
        dl->PushClipRect(ImVec2(value_x, p.y), ImVec2(buttons_x - 8.0f*s, p.y + h), true);
        dl->AddText(ImVec2(IM_ROUND(value_x), IM_ROUND(cy - fs*0.5f)), ink, value);
        dl->PopClipRect();

        ImGui::SetCursorScreenPos(ImVec2(buttons_x, cy - IM_ROUND(button*0.5f)));
        if (IconButton("##edit", ICON_LC_PENCIL, button, s)) edit = i;
        ImGui::SameLine(0.0f, 6.0f*s);
        if (IconButton("##remove", ICON_LC_TRASH_2, button, s)) remove = i;
        ImGui::SetCursorScreenPos(next);
        ImGui::PopID();
    }
    if (edit >= 0)
        OpenEditor(e, edit);
    else if (remove >= 0)
    {
        e.Binds.erase(e.Binds.begin() + remove);
        if (e.Binds.empty()) st.View = Page::Menu;
    }

    ImGui::Dummy(ImVec2(0.0f, 2.0f*s));
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float bw = IM_ROUND((ImGui::GetContentRegionAvail().x - spacing)*0.5f);
    if (ImGui::Button("New bind", ImVec2(bw, 0.0f)))
        OpenEditor(e, -1);
    ImGui::SameLine();
    if (ImGui::Button("Back", ImVec2(bw, 0.0f)))
        st.View = Page::Menu;
}

inline void RenderPopup(Element& e, ImGuiID popup_id, float s)
{
    using namespace MenuVgui;
    PopupState& st = Popup();
    const ImGuiViewport* viewport = ImGui::GetWindowViewport();
    const ImRect work(viewport->WorkPos, ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y));
    const float width = IM_ROUND(ImMin(380.0f*s, work.GetWidth() - 8.0f*s));
    char window_name[32];
    ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Popup_%08x", popup_id);
    const ImGuiWindow* previous = ImGui::FindWindowByName(window_name);
    const float height = previous ? previous->Size.y : 180.0f*s;
    float x = ImClamp(st.Anchor.x, work.Min.x, ImMax(work.Min.x, work.Max.x - width));
    float y = st.Anchor.y + 4.0f*s;
    if (y + height > work.Max.y) y = st.Anchor.y - height - 4.0f*s;
    y = ImClamp(y, work.Min.y, ImMax(work.Min.y, work.Max.y - height));
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, ImMax(1.0f, work.GetHeight() - 8.0f*s)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f*s, 12.0f*s));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f*s, 10.0f*s));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color(Background));
    if (ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* dl = window->DrawList;
        ImFont* font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize();
        const Page page = st.View;                                  // sayfa degisimi bir sonraki frame'de cizilir
        if (st.Shown != page) { st.Shown = page; st.Reveal = 0.0f; }
        st.Reveal = Anim::Smooth(st.Reveal, 1.0f, 20.0f, Anim::Dt());
        if (st.Reveal != 1.0f) Anim::Request();

        // Arka plan (kanal 0) icerikten sonra, kesin boyutla cizilir; icerik kanal 1'de.
        dl->ChannelsSplit(2);
        dl->ChannelsSetCurrent(1);

        // Baslik: [element] [sayfa] .......... [X]
        const ImVec2 header = ImGui::GetCursorScreenPos();
        const float content_width = ImGui::GetContentRegionAvail().x;
        const float line = ImMax(fs, IM_ROUND(22.0f*s));
        const ImRect close(ImVec2(header.x + content_width - 22.0f*s, header.y), ImVec2(header.x + content_width, header.y + 22.0f*s));
        ImGui::Dummy(ImVec2(content_width, line));
        const char* page_name = page == Page::Editor ? (st.EditIndex >= 0 ? "Edit bind" : "New bind") : page == Page::List ? "Binds" : "Bind";
        const float small = IM_ROUND(fs*0.8f);
        const float page_w = font->CalcTextSizeA(small, FLT_MAX, 0.0f, page_name).x;
        const float title_max = close.Min.x - 16.0f*s - page_w;
        dl->PushClipRect(header, ImVec2(title_max, header.y + line), true);
        dl->AddText(header, Color(Text), e.Label);
        dl->PopClipRect();
        const float title_w = ImMin(ImGui::CalcTextSize(e.Label).x, title_max - header.x);
        dl->AddText(font, small, ImVec2(IM_ROUND(header.x + title_w + 10.0f*s), IM_ROUND(header.y + (fs - small)*0.5f + 1.0f*s)), Color(Gold), page_name);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetCursorScreenPos(close.Min);
        if (ImGui::InvisibleButton("##close", ImVec2(22.0f*s, line), ImGuiButtonFlags_EnableNav))
            ImGui::CloseCurrentPopup();
        Bevel(dl, close, Color(Background), !ImGui::IsItemActive(), s);
        GlyphCentered(dl, font, IM_ROUND(fs*0.75f), close.GetCenter(), Color(Text), ICON_LC_X);
        ImGui::RenderNavCursor(close, ImGui::GetItemID());
        const float body_top = ImGui::GetCursorScreenPos().y - 4.0f*s;

        {
            Anim::AlphaScope fade(0.2f + 0.8f*st.Reveal);
            switch (page)
            {
            case Page::Menu:   PopupPageMenu(e, s); break;
            case Page::Editor: PopupPageEditor(e, s); break;
            case Page::List:   PopupPageList(e, s); break;
            }
        }

        // Kanal 0: kabarik cerceve + cokuk govde (icerigin gercek alt sinirina gore)
        dl->ChannelsSetCurrent(0);
        const ImRect outer(window->Pos, ImVec2(window->Pos.x + window->Size.x, window->Pos.y + window->Size.y));
        const float content_bottom = ImMax(window->DC.CursorMaxPos.y + window->WindowPadding.y, outer.Max.y);
        dl->PushClipRect(outer.Min, ImVec2(outer.Max.x, content_bottom), false);
        const ImRect body(ImVec2(outer.Min.x + 6.0f*s, body_top), ImVec2(outer.Max.x - 6.0f*s, content_bottom - 6.0f*s));
        if (body.GetHeight() > 0.0f) Bevel(dl, body, Color(Field), false, s);
        Edge(dl, ImRect(outer.Min, ImVec2(outer.Max.x, content_bottom)), s, true);
        dl->PopClipRect();
        dl->ChannelsMerge();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

// Bind isareti: widget'in kontrol kutusunun (checkbox kutusu, combo / sayi alani, renk karesi) sag alt kosesinde kucuk ucgen
// centik; checkbox'ta tik sol-orta bolgede kaldigi icin cakismaz. Konum item rect'ine gore; FromRight: x item'in sag kenarindan
// olculur, Bottom: Corner kontrolun sag alt kosesidir (false: sag ust).
struct Marker
{
    ImVec2 Corner = ImVec2(0.0f, 0.0f);    // kontrol kutusunun sag ust (Bottom ise sag alt) kosesi
    bool   FromRight = false;
    bool   Bottom = false;
    float  Size = 7.0f;                     // ucgen kenari (referans px)
};

struct Notch
{
    ImRect Rect;
    bool   Bottom = false;
};

// Centigin cizim kutusu: kontrolun icinde, kabarik/cokuk kenar cizgisinin hemen icinde.
inline Notch NotchRect(const ImRect& rect, const Marker& m, float s)
{
    const float x = IM_ROUND((m.FromRight ? rect.Max.x : rect.Min.x) + m.Corner.x);
    const float y = IM_ROUND(rect.Min.y + m.Corner.y);
    const float e = ImMax(1.0f, IM_ROUND(s));
    const float t = IM_ROUND(ImMax(3.0f, m.Size*s));
    Notch n;
    n.Bottom = m.Bottom;
    n.Rect = m.Bottom ? ImRect(ImVec2(x - e - t, y - e - t), ImVec2(x - e, y - e))
                      : ImRect(ImVec2(x - e - t, y + e), ImVec2(x - e, y + e + t));
    return n;
}

// Tiklama alani: centik + kosenin disina 3px (kontrolun kendi tiklamasina mumkun oldugunca az biner).
inline ImRect NotchHitRect(const Notch& n, float s)
{
    const float grow = IM_ROUND(3.0f*s);
    return n.Bottom ? ImRect(n.Rect.Min, ImVec2(n.Rect.Max.x + grow, n.Rect.Max.y + grow))
                    : ImRect(ImVec2(n.Rect.Min.x, n.Rect.Min.y - grow), ImVec2(n.Rect.Max.x + grow, n.Rect.Max.y));
}

// Ucgen centik: pasifken soluk, bind deger yazarken altin; ilk gorunuste solarak gelir, ustundeyken hafif acilir.
inline void DrawNotch(ImDrawList* dl, const Notch& n, ImGuiID id, bool active, bool hovered)
{
    using namespace MenuVgui;
    const float presence = Animate(id, 1.0f, 0.0f, 16.0f);
    const float on = Animate(id + 1, active ? 1.0f : 0.0f, active ? 1.0f : 0.0f, 22.0f);
    const float hot = Animate(id + 2, hovered ? 1.0f : 0.0f, 0.0f, 24.0f);
    const ImU32 idle = Anim::LerpColor(Color(Muted), Color(Text), hot*0.6f);
    const ImU32 ink = Anim::ScaleAlpha(Anim::LerpColor(idle, Color(Gold), on), presence);
    const ImRect& r = n.Rect;
    if (n.Bottom) dl->AddTriangleFilled(ImVec2(r.Min.x, r.Max.y), r.Max, ImVec2(r.Max.x, r.Min.y), ink);   // sag alt
    else          dl->AddTriangleFilled(r.Min, ImVec2(r.Max.x, r.Min.y), r.Max, ink);                       // sag ust
}
inline ImGuiID PopupId(const void* ptr) { return ImHashData(&ptr, sizeof(ptr), ImHashStr("##bind_popup")); }
inline ImGuiID NotchId(const void* ptr)  { return ImHashStr("##bind_notch", 0, PopupId(ptr)); }

inline void OpenPopup(void* ptr, Page page, ImVec2 anchor)
{
    PopupState& st = Popup();
    st.Ptr = ptr;
    st.View = st.Shown = page;
    st.EditIndex = -1;
    st.Anchor = anchor;
    st.Reveal = 1.0f;
    ImGui::OpenPopupEx(PopupId(ptr));
}

// Centigin tiklama alani widget'tan ONCE kaydedilir (onceki frame'in rect'i): hover'i ilk alan item kazandigi
// icin centige tiklamak alttaki checkbox'i/slider'i tetiklemez. Tiklayinca bind listesi (ekle / duzenle / sil) acilir.
inline void NotchButton(void* ptr)
{
    Element* e = Find(ptr);
    if (e == nullptr || e->Binds.empty() || e->NotchHit.GetWidth() <= 0.0f) return;
    const ImGuiID id = NotchId(ptr);
    if (!ImGui::ItemAdd(e->NotchHit, id, nullptr, ImGuiItemFlags_NoNav)) return;
    bool hovered = false, held = false;
    if (ImGui::ButtonBehavior(e->NotchHit, id, &hovered, &held) && !MenuVgui::IsListeningForBind())
        OpenPopup(ptr, Page::List, ImVec2(e->NotchHit.Min.x, e->NotchHit.Max.y));
    e->NotchHovered = hovered;
}

// Widget'tan hemen sonra: sag tik -> bind popup'i, kontrolun kosesinde bind centigi.
inline void Attach(void* ptr, float s, const Marker& m)
{
    using namespace MenuVgui;
    Element* e = Find(ptr);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (e == nullptr || window->SkipItems) return;
    ImGuiContext& g = *GImGui;
    const ImGuiLastItemData item = g.LastItemData;
    const ImRect rect = item.Rect;
    const ImGuiID popup_id = PopupId(ptr);
    PopupState& st = Popup();

    const bool has_notch = !e->Binds.empty();
    const Notch notch = has_notch ? NotchRect(rect, m, s) : Notch();
    e->NotchHit = has_notch ? NotchHitRect(notch, s) : ImRect();    // bir sonraki frame'in tiklama alani
    const bool hovered = ImGui::IsWindowHovered() && (rect.Contains(g.IO.MousePos) || (has_notch && e->NotchHit.Contains(g.IO.MousePos)));
    if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !IsListeningForBind())
        OpenPopup(ptr, Page::Menu, g.IO.MousePos);
    if (st.Ptr == ptr && ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None))
        RenderPopup(*e, popup_id, s);

    if (has_notch)
        DrawNotch(window->DrawList, notch, NotchId(ptr), e->Overriding, e->NotchHovered);
    e->NotchHovered = false;
    g.LastItemData = item;
}

// Widget'i ciz; override aktifse kilitle (gorunum ayni, duzenleme kapali), ardindan Attach.
template <typename Draw>
inline bool Bindable(void* ptr, float s, const Marker& marker, Draw&& draw)
{
    NotchButton(ptr);
    const Element* e = Find(ptr);
    const bool locked = e != nullptr && e->Overriding;
    if (locked)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
        ImGui::BeginDisabled();
    }
    bool changed = draw();
    if (locked)
    {
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        changed = false;
    }
    Attach(ptr, s, marker);
    return changed;
}
}
