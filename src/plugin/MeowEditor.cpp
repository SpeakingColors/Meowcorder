#include "MeowEditor.h"
#include <catui/CatLookAndFeel.h>
#include <catui/RenderGate.h>
#include <catui/Logo.h>
#include "MeowIcons.h"
#include <cmath>
#include <functional>
#include <map>

using juce::Colour; using juce::Graphics; using juce::String;

namespace {
const Colour kBg(0xff101114), kPanel(0xff17181c), kEdge(0xff2c2e35), kText(0xffe8e6e1), kDim(0xff8c909a), kGold(0xffffd100), kPink(0xffff4fa0), kTeal(0xff12d6b4), kRed(0xffff3b4a), kBlue(0xff6ec6ff);
constexpr int kCheek = 30, kContentW = 1140, kBaseW = kContentW + 2 * kCheek, kBaseH = 800;
juce::Font mono(float h, bool bold = false) { return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain)); }
juce::Font sans(float h, bool bold = false) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }

class Pill : public juce::Button
{
public:
    explicit Pill(const String& t, Colour c = kPink) : juce::Button(t), col(c) {}
    Colour col; bool flash = false, raw = false;
    void paintButton(Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f); const bool on = getToggleState() || flash;
        g.setColour(on ? col.withAlpha(0.22f) : (over ? Colour(0xff25272e) : Colour(0xff1b1c21))); g.fillRoundedRectangle(r, 3.f);
        g.setColour(on ? col : Colour(0xff3a3d46)); g.drawRoundedRectangle(r, 3.f, on ? 1.5f : 1.f);
        g.setColour(on ? col : (down ? kGold : kText)); g.setFont(sans(13.f, true)); g.drawFittedText(raw ? getButtonText() : getButtonText().toUpperCase(), getLocalBounds().reduced(3, 0), juce::Justification::centred, 1);
    }
};

int gUnit = 0; double gUnitSr = 44100.0, gUnitBpm = 120.0;   // time line unit: 0 minutes:seconds, 1 seconds, 2 samples, 3 bars:beats
String timeText(double sec, bool fine = true)
{
    if (gUnit == 1) return String(sec, fine ? 3 : 1) + " s";
    if (gUnit == 2) return String((juce::int64) std::llround(sec * gUnitSr));
    if (gUnit == 3) { const double beats = sec * gUnitBpm / 60.0; const int bar = (int) std::floor(beats / 4.0) + 1; const double inBar = beats - std::floor(beats / 4.0) * 4.0; return String(bar) + "." + String(inBar + 1.0, fine ? 2 : 1); }
    const int m = (int) (sec / 60.0); const double s = sec - m * 60.0;
    return String(m) + ":" + String(s, fine ? 2 : 0).paddedLeft('0', fine ? 5 : 2);
}

// ---------------- icon button: the mockup's icons (tools/make_icons.py) drawn from their SVG paths ----------------
class IconButton : public juce::Button
{
public:
    explicit IconButton(const juce::String& iconName, Colour c = kText) : juce::Button(iconName), tint(c), icon(iconName.toStdString()) {}
    Colour tint; std::function<void()> onDouble; std::function<void(bool)> onHover;
    void paintButton(Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f); const bool on = getToggleState(), en = isEnabled();
        g.setColour(on ? tint.withAlpha(0.2f) : (over && en ? Colour(0xff2c2f38) : Colour(0xff23252c))); g.fillRoundedRectangle(r, 4.f);
        g.setColour(on ? tint : (over && en ? Colour(0xff50535f) : Colour(0xff343741))); g.drawRoundedRectangle(r, 4.f, 1.f);
        if (auto* d = get(!en ? tint.withAlpha(0.3f) : (on ? Colour(0xffffffff) : (down ? kGold : tint))))
            d->drawWithin(g, getLocalBounds().toFloat().withSizeKeepingCentre(20.f, 20.f), juce::RectanglePlacement::centred, 1.f);
    }
    void mouseDoubleClick(const juce::MouseEvent&) override { if (onDouble) onDouble(); }
    void mouseEnter(const juce::MouseEvent& e) override { juce::Button::mouseEnter(e); if (onHover) onHover(true); }
    void mouseExit(const juce::MouseEvent& e) override { juce::Button::mouseExit(e); if (onHover) onHover(false); }
private:
    std::string icon; std::map<juce::uint32, std::unique_ptr<juce::Drawable>> cache;
    juce::Drawable* get(Colour c)
    {
        auto it = cache.find(c.getARGB()); if (it != cache.end()) return it->second.get();
        const String hex = "#" + c.withAlpha(1.f).toDisplayString(false); String body = String(mcicons::find(icon)).replace("currentColor", hex), out;
        for (auto& part : juce::StringArray::fromTokens(body, "<", ""))
        {
            if (part.isEmpty()) continue; const String tag = part.upToFirstOccurrenceOf(" ", false, false);
            String attrs = String(" stroke=\"") + hex + "\" stroke-width=\"1.6\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-opacity=\"" + String(c.getFloatAlpha()) + "\"";
            if (!part.contains("fill=")) attrs = " fill=\"none\"" + attrs; else attrs += " fill-opacity=\"" + String(c.getFloatAlpha()) + "\"";
            out += "<" + tag + attrs + part.fromFirstOccurrenceOf(tag, false, false);
        }
        std::unique_ptr<juce::Drawable> d;
        if (auto xml = juce::XmlDocument::parse("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 20 20\" width=\"20\" height=\"20\">" + out + "</svg>")) d = juce::Drawable::createFromSVG(*xml);
        return (cache[c.getARGB()] = std::move(d)).get();
    }
};

// ---------------- level meter with dB marks ----------------
struct Meter : juce::Component
{
    String name; float level = 0.f, hold = 0.f;
    explicit Meter(String n) : name(std::move(n)) {}
    void set(float peak) { level = std::max(peak, level * 0.86f); hold = std::max(peak, hold * 0.985f); repaint(); }
    static float yFor(float db, int h) { return (float) h * (1.f - juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f)); }
    void paint(Graphics& g) override
    {
        const int top = 16, h = getHeight() - top; g.setColour(kDim); g.setFont(sans(11.f, true)); g.drawText(name, 0, 0, getWidth(), 14, juce::Justification::centred);
        auto bar = juce::Rectangle<float>(10.f, (float) top, 14.f, (float) h); g.setColour(Colour(0xff060608)); g.fillRect(bar); g.setColour(kEdge); g.drawRect(bar, 1.f);
        const float db = mc::linToDb(level); const float y = top + yFor(db, h);
        for (float yy = y; yy < top + h; yy += 3.f) { const float d = -60.f + 60.f * (1.f - (yy - top) / h); g.setColour(d > -3.f ? kRed : (d > -12.f ? kGold : kTeal)); g.fillRect(bar.getX() + 1.f, yy, bar.getWidth() - 2.f, 2.f); }
        g.setColour(juce::Colours::white); g.fillRect(bar.getX(), top + yFor(mc::linToDb(hold), h), bar.getWidth(), 1.5f);
        g.setFont(mono(9.f)); for (int d : { 0, -6, -12, -24, -48 }) { const float yy = top + yFor((float) d, h); g.setColour(kDim); g.drawHorizontalLine((int) yy, 24.f, 28.f); g.drawText(String(d), 28, (int) yy - 5, 22, 10, juce::Justification::left); }
    }
};

// ---------------- the waveform / spectral display ----------------
struct Display : juce::Component, juce::SettableTooltipClient
{
    MeowProcessor& p; bool spectral = false; double vStart = 0, vLen = 0; int seen = -1; juce::Image spec; String specKey;
    std::function<void()> onSelection;
    explicit Display(MeowProcessor& pr) : p(pr) { setTooltip("Click to place the cursor, drag to select. Mouse wheel zooms, Shift and wheel scrolls. Double click selects all."); }
    size_t frames() const { return p.doc.audio.frames(); }
    void fitAll() { vStart = 0; vLen = (double) std::max<size_t>(frames(), 1); repaint(); }
    void clampView() { const double n = (double) std::max<size_t>(frames(), 1); vLen = juce::jlimit(32.0, n, vLen); vStart = juce::jlimit(0.0, std::max(0.0, n - vLen), vStart); }
    double xToFrame(float x) const { return vStart + (double) x / (double) std::max(1, getWidth()) * vLen; }
    float frameToX(double f) const { return (float) ((f - vStart) / vLen * (double) getWidth()); }
    void zoom(double factor, float anchorX) { const double a = xToFrame(anchorX); vLen *= factor; clampView(); vStart = a - (double) anchorX / (double) getWidth() * vLen; clampView(); repaint(); }
    void zoomToSelection() { if (p.doc.hasSelection()) { vStart = (double) p.doc.selA; vLen = (double) (p.doc.selB - p.doc.selA); clampView(); repaint(); } }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (spectral && tool != Mouse) { specDown(e); return; }
        if (frames() == 0) return; dragFrom = (size_t) juce::jlimit(0.0, (double) frames(), xToFrame((float) e.x));
        if (e.mods.isShiftDown() && (p.doc.hasSelection() || p.doc.cursor > 0)) { const size_t a = p.doc.hasSelection() ? p.doc.selA : p.doc.cursor; p.doc.setSelection(a, dragFrom); }
        else p.doc.setCursor(dragFrom);
        if (onSelection) onSelection(); repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (sd.on) { specDrag(e); return; }
        if (frames() == 0) return; const size_t f = (size_t) juce::jlimit(0.0, (double) frames(), xToFrame((float) e.x));
        if (e.getDistanceFromDragStart() > 2) p.doc.setSelection(dragFrom, f); if (onSelection) onSelection(); repaint();
    }
    void mouseUp(const juce::MouseEvent& e) override { if (sd.on) specUp(e); }
    void mouseMove(const juce::MouseEvent& e) override { if (spectral && tool == Paste && p.doc.hasSpectralClip()) { ghost = true; ghostP = e.position; repaint(); } }
    void mouseExit(const juce::MouseEvent&) override { if (ghost) { ghost = false; repaint(); } }
    void mouseDoubleClick(const juce::MouseEvent&) override { if (spectral && tool != Mouse) return; if (frames() > 0) { p.doc.setSelection(0, frames()); if (onSelection) onSelection(); repaint(); } }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (frames() == 0) return;
        // trackpad: a sideways swipe pans, an up or down swipe zooms; mouse wheel zooms, Shift and wheel pans
        if (e.mods.isShiftDown() || std::abs(w.deltaX) > std::abs(w.deltaY)) { const float d = w.deltaX != 0 && !e.mods.isShiftDown() ? w.deltaX : (w.deltaY != 0 ? w.deltaY : w.deltaX); vStart -= d * vLen * 0.5; clampView(); repaint(); }
        else zoom(w.deltaY > 0 ? 0.8 : 1.25, (float) e.x);
    }
    // pinch on a Mac trackpad
    void mouseMagnify(const juce::MouseEvent& e, float scale) override { if (frames() > 0 && scale > 0.f) zoom(1.0 / (double) scale, (float) e.x); }

    void paint(Graphics& g) override
    {
        const int rulerH = 22; auto area = getLocalBounds().withTrimmedBottom(rulerH);
        g.fillAll(Colour(0xff060608));
        const auto& a = p.doc.audio; const size_t n = a.frames();
        if (n == 0) { g.setColour(kDim); g.setFont(sans(16.f)); g.drawFittedText("Empty. Press Record, or open a sound from the File menu.", getLocalBounds(), juce::Justification::centred, 1); return; }
        if (seen != p.version()) { seen = p.version(); if (vLen <= 0 || vStart + vLen > (double) n + 1) fitAll(); clampView(); specKey = {}; }
        if (vLen <= 0) fitAll();
        const int w = getWidth(); const int laneH = area.getHeight() / 2;
        if (spectral) paintSpectral(g, area); else
            for (int c = 0; c < 2; ++c)
            {
                const int top = c * laneH; g.setColour(kEdge); g.drawHorizontalLine(top + laneH, 0.f, (float) w); g.setColour(kDim); g.setFont(mono(10.f)); g.drawText(c == 0 ? "L" : "R", 4, top + 2, 14, 12, juce::Justification::left);
                g.setColour(Colour(0xff1a2a30)); g.drawHorizontalLine(top + laneH / 2, 0.f, (float) w);
                g.setColour(kTeal); const auto& d = a.ch[c];
                for (int x = 0; x < w; ++x)
                {
                    const size_t s0 = (size_t) juce::jlimit(0.0, (double) n, vStart + (double) x / w * vLen), s1 = std::min(n, std::max(s0 + 1, (size_t) juce::jlimit(0.0, (double) n, vStart + (double) (x + 1) / w * vLen)));
                    const size_t step = std::max<size_t>(1, (s1 - s0) / 48); float mn = 0, mx = 0; for (size_t i = s0; i < s1; i += step) { mn = std::min(mn, d[i]); mx = std::max(mx, d[i]); }
                    const float mid = top + laneH * 0.5f, amp = laneH * 0.46f; g.fillRect((float) x, mid - mx * amp, 1.f, std::max(1.f, (mx - mn) * amp));
                }
            }
        if (spectral) paintSpecOverlay(g);
        if (p.doc.hasSelection()) { const float x0 = frameToX((double) p.doc.selA), x1 = frameToX((double) p.doc.selB); g.setColour(kBlue.withAlpha(0.16f)); g.fillRect(x0, 0.f, x1 - x0, (float) area.getHeight()); g.setColour(kBlue); g.drawVerticalLine((int) x0, 0.f, (float) area.getHeight()); g.drawVerticalLine((int) x1, 0.f, (float) area.getHeight()); }
        else { const float x = frameToX((double) p.doc.cursor); g.setColour(kBlue); g.drawVerticalLine((int) x, 0.f, (float) area.getHeight()); }
        if (p.isPlaying()) { const float x = frameToX(p.playPosFrames()); g.setColour(kGold); g.drawVerticalLine((int) x, 0.f, (float) area.getHeight()); }
        // ruler
        g.setColour(kPanel); g.fillRect(0, getHeight() - rulerH, w, rulerH); g.setColour(kEdge); g.drawHorizontalLine(getHeight() - rulerH, 0.f, (float) w);
        const double secs = vLen / a.sr; double step = 0.001; for (double s : { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0, 120.0, 300.0 }) { step = s; if (secs / s < 9) break; }
        g.setFont(mono(10.f)); const double t0 = vStart / a.sr;
        for (double t = std::ceil(t0 / step) * step; t < t0 + secs; t += step) { const float x = (float) ((t - t0) / secs * w); g.setColour(kDim); g.drawVerticalLine((int) x, (float) (getHeight() - rulerH), (float) (getHeight() - rulerH + 6)); g.drawText(timeText(t, step < 1.0), (int) x + 3, getHeight() - rulerH + 6, 80, 12, juce::Justification::left); }
    }
    void paintSpectral(Graphics& g, juce::Rectangle<int> area)
    {
        const auto& a = p.doc.audio; const size_t n = a.frames(); const int w = area.getWidth(), h = area.getHeight();
        const String key = String(p.version()) + "," + String((juce::int64) vStart) + "," + String((juce::int64) vLen) + "," + String(w) + "," + String(h);
        if (key != specKey)
        {
            specKey = key; const size_t from = (size_t) vStart, to = std::min(n, (size_t) (vStart + vLen)); const int fftSize = 1024;
            const size_t hop = std::max<size_t>(64, (to - from) / (size_t) std::max(1, w)); const mc::Spectrogram sp = mc::spectrogram(a, from, to, fftSize, (int) hop);
            spec = juce::Image(juce::Image::ARGB, std::max(1, w), std::max(1, h), true);
            if (sp.frames > 0 && sp.bins > 0)
                for (int y = 0; y < h; ++y)
                {
                    const int bin = juce::jlimit(0, sp.bins - 1, (int) std::lround(hzAtY((float) y) / nyq() * (double) (sp.bins - 1)));
                    for (int x = 0; x < w; ++x)
                    {
                        const int fi = juce::jlimit(0, sp.frames - 1, (int) ((double) x / w * sp.frames)); const float v = juce::jlimit(0.f, 1.f, (sp.at(fi, bin) + 90.f) / 80.f);
                        spec.setPixelAt(x, y, Colour::fromHSV(0.72f - 0.72f * v, 0.85f, std::pow(v, 0.8f), 1.f));
                    }
                }
        }
        g.drawImageAt(spec, 0, 0); g.setColour(kDim); g.setFont(mono(10.f)); for (double hz : { 100.0, 1000.0, 10000.0 }) if (hz < nyq()) { const float y = yAtHz(hz); g.setColour(Colour(0x33ffffff)); g.drawHorizontalLine((int) y, 0.f, (float) w); g.setColour(kDim); g.drawText(hz >= 1000 ? String((int) (hz / 1000)) + "k" : String((int) hz), 4, (int) y - 12, 40, 12, juce::Justification::left); }
    }

    // ---- spectral tools (the mockup's nine) ----
    enum Tool { Mouse, Select, Scissors, Cutter, Eraser, Copy, Paste, Amp, Pencil };
    int tool = Mouse, brush = 12; float ampDb = 6.f, penDb = -18.f; bool hasReg = false; mc::SpecRegion sreg;
    std::function<void(const String&)> onHint; std::function<void()> onToolChanged;
    struct Drag { bool on = false, did = false, moved = false; juce::Point<float> p0, p, last; std::vector<mc::SpecRegion> rects, pending; double f0 = 0, pf = 0, phz = 0, pph = 0, pn = 0; juce::uint32 lastFlush = 0; } sd;
    bool ghost = false; juce::Point<float> ghostP;
    static const char* toolHint(int t)
    {
        static const char* h[] = { "Click for the cursor, drag to select a time range.", "Drag a box over time and frequency. Use Cutter, Amp or Copy on the box from the Spectral menu.", "Drag across time: that slice is cut out and the gap closes.",
            "Drag a box: the sound inside it is removed, the length stays.", "Paint over what should go. It is removed as you paint.", "Drag a box to copy that piece, then click with Paste where it should land.",
            "Click where the copied piece should go. Left edge is time, height is frequency.", "Drag a box: it gets the level change set next to the tools.", "Draw a line on the picture: a pure tone follows your pen. Drag forward in time. Click for a short beep." };
        return h[t];
    }
    void setTool(int t) { tool = t; ghost = false; if (onHint) onHint(toolHint(t)); if (onToolChanged) onToolChanged(); setMouseCursor(t == Mouse ? juce::MouseCursor::NormalCursor : juce::MouseCursor::CrosshairCursor); repaint(); }
    int specH() const { return std::max(2, getHeight() - 22); }
    double nyq() const { return std::max(100.0, p.doc.audio.sr * 0.5); }
    double hzAtY(float y) const { const float h = (float) specH(); return 20.0 * std::pow(nyq() / 20.0, 1.0 - (double) juce::jlimit(0.f, h - 1.f, y) / (double) (h - 1.f)); }
    float yAtHz(double hz) const { const double h = (double) specH(); return (float) ((1.0 - std::log(juce::jlimit(20.0, nyq(), hz) / 20.0) / std::log(nyq() / 20.0)) * (h - 1.0)); }
    double frAtX(float x) const { return juce::jlimit(0.0, (double) frames(), xToFrame(x)); }
    mc::SpecRegion boxOf(juce::Point<float> a, juce::Point<float> b) const { const double f0 = frAtX(a.x), f1 = frAtX(b.x), h0 = hzAtY(a.y), h1 = hzAtY(b.y); return { std::min(f0, f1), std::max(f0, f1), std::min(h0, h1), std::max(h0, h1) }; }
    mc::SpecRegion brushRect(juce::Point<float> q) const { const double fpp = vLen / (double) std::max(1, getWidth()), c = xToFrame(q.x), r = (double) brush; return { c - r * fpp, c + r * fpp, hzAtY(q.y + (float) brush), hzAtY(q.y - (float) brush) }; }
    double penAmp() const { return std::pow(10.0, (double) penDb / 20.0); }
    double penFade() const { return std::round(0.003 * p.doc.audio.sr); }
    void edited() { hasReg = false; p.changed(); if (onSelection) onSelection(); repaint(); }
    void flushEraser(bool force)
    {
        const auto now = juce::Time::getMillisecondCounter(); if (sd.pending.empty() || (!force && now - sd.lastFlush < 70)) return;
        mc::spectralEdit(p.doc.audio, sd.pending, true, 0.f); sd.pending.clear(); sd.did = true; sd.lastFlush = now; p.changed(); repaint();
    }
    void penBump(bool force) { const auto now = juce::Time::getMillisecondCounter(); if (force || now - sd.lastFlush > 70) { sd.lastFlush = now; p.changed(); repaint(); } }
    void penTail() { if (sd.pn > 0) { mc::pencilTone(p.doc.audio, sd.pf, sd.pf + penFade(), sd.phz, sd.phz, penAmp(), sd.pph, 0, penFade()); sd.did = true; } }
    void specDown(const juce::MouseEvent& e)
    {
        if (frames() == 0) return; sd = Drag(); sd.on = true; sd.p0 = sd.p = e.position; sd.f0 = frAtX(e.position.x);
        if (tool == Eraser) { p.stopPlay(); p.doc.beginEdit("Spectral eraser"); const auto r = brushRect(e.position); sd.rects = { r }; sd.pending = { r }; flushEraser(true); }
        if (tool == Pencil) { p.stopPlay(); p.doc.beginEdit("Pencil (spectral)"); sd.pf = frAtX(e.position.x); sd.phz = hzAtY(e.position.y); sd.pph = 0; sd.pn = 0; }
        repaint();
    }
    void specDrag(const juce::MouseEvent& e)
    {
        sd.p = e.position;
        if (tool == Eraser) { const auto l = sd.last == juce::Point<float>() ? sd.p0 : sd.last; if (l.getDistanceFrom(sd.p) >= (float) brush * 0.5f) { const auto r = brushRect(sd.p); sd.rects = { r }; sd.pending.push_back(r); sd.last = sd.p; flushEraser(false); } }
        else if (tool == Pencil)
        {
            const double f = frAtX(sd.p.x), hz = hzAtY(sd.p.y), fpp = vLen / (double) std::max(1, getWidth());
            if (f > sd.pf) { sd.pph = mc::pencilTone(p.doc.audio, sd.pf, f, sd.phz, hz, penAmp(), sd.pph, sd.pn > 0 ? 0 : std::min(penFade(), f - sd.pf), 0); sd.pn += f - sd.pf; sd.pf = f; sd.phz = hz; sd.did = true; penBump(false); }
            else if (f < sd.pf - 3 * fpp) { penTail(); sd.pf = f; sd.phz = hz; sd.pn = 0; sd.pph = 0; penBump(false); }
        }
        repaint();
    }
    void specUp(const juce::MouseEvent& e)
    {
        const auto d = sd; sd.on = false; const auto q = e.position; const bool tiny = std::abs(q.x - d.p0.x) < 4 && std::abs(q.y - d.p0.y) < 4; const auto box = boxOf(d.p0, q);
        switch (tool)
        {
            case Select: if (tiny) { hasReg = false; p.doc.setCursor((size_t) frAtX(q.x)); } else { sreg = box; hasReg = true; p.doc.setSelection((size_t) box.t0, (size_t) box.t1); } if (onSelection) onSelection(); break;
            case Scissors: if (!tiny) { p.stopPlay(); p.doc.setSelection((size_t) box.t0, (size_t) box.t1); p.doc.deleteSelection(); edited(); } break;
            case Cutter: if (!tiny) { p.stopPlay(); p.doc.spectralEdit(true, { box }, 0.f); edited(); } break;
            case Amp: if (!tiny) { p.stopPlay(); p.doc.spectralEdit(false, { box }, ampDb); edited(); } break;
            case Eraser: flushEraser(true); if (sd.did) edited(); else p.doc.cancelEdit(); break;
            case Pencil:
            {
                const double sr = p.doc.audio.sr; if (sd.pn <= 0) { const double n = std::round(0.06 * sr); sd.pph = mc::pencilTone(p.doc.audio, sd.pf, sd.pf + n, sd.phz, sd.phz, penAmp(), 0, penFade(), penFade()); sd.did = true; } else penTail();
                if (sd.did) edited(); else p.doc.cancelEdit(); break;
            }
            case Copy: if (!tiny && p.doc.spectralCopy(box)) { sreg = box; hasReg = true; setTool(Paste); if (onHint) onHint("Copied. Click where it should land."); } break;
            case Paste: p.stopPlay(); if (p.doc.spectralPaste(frAtX(q.x), hzAtY(q.y))) edited(); else if (onHint) onHint("Nothing copied yet. Use Copy first."); break;
            default: break;
        }
        repaint();
    }
    void drawBox(Graphics& g, const mc::SpecRegion& r, Colour col, Colour fill, const String& label)
    {
        const float x0 = frameToX(r.t0), x1 = frameToX(r.t1), y0 = yAtHz(r.f1), y1 = yAtHz(r.f0); g.setColour(fill); g.fillRect(x0, y0, x1 - x0, y1 - y0);
        g.setColour(col); g.drawRect(x0, y0, x1 - x0, y1 - y0, 1.f); if (label.isNotEmpty()) { g.setFont(mono(10.f)); g.drawText(label, (int) x0 + 3, (int) y0 + 2, 160, 12, juce::Justification::left); }
    }
    static String fmtHz(double f) { return f >= 1000 ? String(f / 1000.0, f >= 10000 ? 1 : 2) + "k" : String((int) std::lround(f)); }
    void paintSpecOverlay(Graphics& g)
    {
        if (hasReg) drawBox(g, sreg, kBlue, kBlue.withAlpha(0.13f), fmtHz(sreg.f0) + " to " + fmtHz(sreg.f1) + " Hz");
        if (sd.on)
        {
            if (tool == Eraser) for (auto& r : sd.rects) drawBox(g, r, kPink, kPink.withAlpha(0.33f), {});
            else if (tool == Select || tool == Scissors || tool == Cutter || tool == Copy || tool == Amp)
            { auto b = boxOf(sd.p0, sd.p); if (tool == Scissors) { b.f0 = 20; b.f1 = nyq(); } drawBox(g, b, tool == Cutter || tool == Scissors ? kPink : (tool == Amp ? kGold : kBlue), Colour(0x1cffffff), {}); }
        }
        else if (tool == Paste && ghost && p.doc.hasSpectralClip())
        {
            const auto& c = p.doc.spectralClip(); const double dur = (double) (c.nf - 1) * mc::kSH + mc::kSN / 2.0, half = ((double) c.nb * c.sr / mc::kSN) / 2.0, hz = hzAtY(ghostP.y), fr = frAtX(ghostP.x);
            drawBox(g, { fr, fr + dur, std::max(20.0, hz - half), hz + half }, kTeal, kTeal.withAlpha(0.13f), "paste here");
        }
    }
    size_t dragFrom = 0;
};


// ---------------- overview strip: the whole sound, with a box for what the display shows ----------------
struct Overview : juce::Component, juce::SettableTooltipClient
{
    MeowProcessor& p; Display& d; std::vector<float> peaks; int seen = -1, seenW = 0; enum Mode { None, Pan, Left, Right } mode = None; double grab = 0; double lastS = -1, lastL = -1;
    Overview(MeowProcessor& pr, Display& di) : p(pr), d(di) { setTooltip("The whole sound. The lighter box is what you see above. Drag the box to scroll, drag its edges to zoom, click to jump, wheel to zoom."); }
    size_t frames() const { return p.doc.audio.frames(); }
    void rebuild()
    {
        const int w = getWidth(); peaks.assign((size_t) std::max(w, 1), 0.f); const size_t n = frames(); if (n == 0) return; const auto& a = p.doc.audio;
        for (int x = 0; x < w; ++x)
        {
            const size_t s0 = (size_t) ((double) x / w * (double) n), s1 = std::min(n, std::max(s0 + 1, (size_t) ((double) (x + 1) / w * (double) n))); const size_t step = std::max<size_t>(1, (s1 - s0) / 32); float m = 0;
            for (size_t i = s0; i < s1; i += step) m = std::max(m, std::max(std::abs(a.ch[0][i]), std::abs(a.ch[1][i])));
            peaks[(size_t) x] = m;
        }
    }
    float boxX0() const { return (float) (d.vStart / (double) std::max<size_t>(frames(), 1) * getWidth()); }
    float boxX1() const { return (float) ((d.vStart + d.vLen) / (double) std::max<size_t>(frames(), 1) * getWidth()); }
    void tick() { if (d.vStart != lastS || d.vLen != lastL) { lastS = d.vStart; lastL = d.vLen; repaint(); } }
    void paint(Graphics& g) override
    {
        g.fillAll(Colour(0xff060608)); g.setColour(kEdge); g.drawRect(getLocalBounds(), 1);
        if (frames() == 0) return;
        if (seen != p.version() || seenW != getWidth()) { seen = p.version(); seenW = getWidth(); rebuild(); }
        const float mid = getHeight() * 0.5f; g.setColour(kDim.withAlpha(0.8f));
        for (int x = 0; x < (int) peaks.size(); ++x) { const float h = std::max(1.f, peaks[(size_t) x] * mid * 0.9f); g.fillRect((float) x, mid - h, 1.f, h * 2.f); }
        const float x0 = boxX0(), x1 = std::max(boxX1(), x0 + 3.f); auto box = juce::Rectangle<float>(x0, 1.f, x1 - x0, (float) getHeight() - 2.f);
        g.setColour(kBlue.withAlpha(0.18f)); g.fillRect(box); g.setColour(kBlue); g.drawRect(box, 1.5f);
        const float hw = 9.f; auto handle = [&](float x, bool left) { g.setColour(kBlue.withAlpha(0.9f)); g.fillRect(left ? x : x - hw, 1.f, hw, (float) getHeight() - 2.f);
            juce::Path t; const float cy = getHeight() * 0.5f, a = left ? x + 2.5f : x - 2.5f, b = left ? x + hw - 2.5f : x - hw + 2.5f; t.addTriangle(b, cy - 5.f, b, cy + 5.f, a, cy); g.setColour(kBg); g.fillPath(t); };
        if (x1 - x0 > 2 * hw + 4) { handle(x0, true); handle(x1, false); }
    }
    Mode hit(float x) const { const float x0 = boxX0(), x1 = std::max(boxX1(), x0 + 3.f); if (x1 - x0 > 24 && std::abs(x - x0) <= 9) return Left; if (x1 - x0 > 24 && std::abs(x - x1) <= 9) return Right; if (x >= x0 && x <= x1) return Pan; return None; }
    double frameAt(float x) const { return juce::jlimit(0.0, (double) frames(), (double) x / (double) std::max(1, getWidth()) * (double) frames()); }
    void mouseMove(const juce::MouseEvent& e) override { const auto m = hit((float) e.x); setMouseCursor(m == Left || m == Right ? juce::MouseCursor::LeftRightResizeCursor : (m == Pan ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::PointingHandCursor)); }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (frames() == 0) return; mode = hit((float) e.x);
        if (mode == None) { mode = Pan; grab = d.vLen * 0.5; } else grab = frameAt((float) e.x) - d.vStart;
        move((float) e.x);
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (frames() > 0) move((float) e.x); }
    void mouseDoubleClick(const juce::MouseEvent&) override { d.fitAll(); repaint(); }
    void move(float x)
    {
        const double f = frameAt(x), end = d.vStart + d.vLen;
        if (mode == Pan) d.vStart = f - grab;
        else if (mode == Left) { const double ns = juce::jlimit(0.0, end - 32.0, f); d.vStart = ns; d.vLen = end - ns; }
        else if (mode == Right) d.vLen = std::max(32.0, f - d.vStart);
        d.clampView(); d.repaint(); repaint();
    }
    void zoomAt(double factor, float x) { const double a = frameAt(x); const float ax = (float) ((a - d.vStart) / d.vLen * d.getWidth()); d.zoom(factor, ax); repaint(); }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (frames() == 0) return;
        if (std::abs(w.deltaX) > std::abs(w.deltaY) || e.mods.isShiftDown()) { d.vStart -= (w.deltaX != 0 ? w.deltaX : w.deltaY) * d.vLen * 0.5; d.clampView(); d.repaint(); repaint(); }
        else zoomAt(w.deltaY > 0 ? 0.8 : 1.25, (float) e.x);
    }
    void mouseMagnify(const juce::MouseEvent& e, float scale) override { if (frames() > 0 && scale > 0.f) zoomAt(1.0 / (double) scale, (float) e.x); }
};

// ---------------- a process panel: sliders for one operation ----------------
struct OpPanel : juce::Component
{
    OpPanel(MeowProcessor& pr, int opId, std::function<void()> done) : p(pr), op(opId), onDone(std::move(done))
    {
        const auto& info = mc::kOps[op]; mc::defaultParams(op, vals);
        if (p.doc.last.op == op) for (int i = 0; i < mc::kMaxOpParams; ++i) vals[i] = p.doc.last.p[i];
        for (int i = 0; i < info.nParams; ++i)
        {
            const auto& pp = info.params[i]; auto* row = rows.add(new Row());
            row->name.setText(pp.name, juce::dontSendNotification); row->name.setFont(sans(13.f)); row->name.setColour(juce::Label::textColourId, kText); addAndMakeVisible(row->name);
            if (pp.choice)
            {
                row->combo.addItemList(juce::StringArray::fromTokens(pp.choices, "|", ""), 1); row->combo.setSelectedItemIndex((int) vals[i], juce::dontSendNotification); row->combo.setTooltip(pp.tip);
                row->combo.onChange = [this, i, r = row] { vals[i] = (float) r->combo.getSelectedItemIndex(); }; addAndMakeVisible(row->combo);
            }
            else
            {
                row->slider.setSliderStyle(juce::Slider::LinearHorizontal); row->slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 78, 20); row->slider.setRange(pp.lo, pp.hi, 0);
                if (pp.lo > 0 && pp.hi / pp.lo >= 50) row->slider.setSkewFactorFromMidPoint(std::sqrt((double) pp.lo * (double) pp.hi));
                row->slider.setValue(vals[i], juce::dontSendNotification); row->slider.setTextValueSuffix(String(" ") + pp.unit); row->slider.setTooltip(pp.tip); row->slider.setDoubleClickReturnValue(true, pp.def);
                row->slider.setNumDecimalPlacesToDisplay((pp.hi - pp.lo) > 200 ? 0 : ((pp.hi - pp.lo) > 20 ? 1 : 2));
                row->slider.onValueChange = [this, i, r = row] { vals[i] = (float) r->slider.getValue(); }; addAndMakeVisible(row->slider);
            }
        }
        apply.setButtonText(p.doc.hasSelection() ? "Apply to selection" : "Apply to whole sound"); addAndMakeVisible(apply); addAndMakeVisible(close);
        apply.onClick = [this] { p.stopPlay(); if (p.doc.apply(op, vals)) p.changed(); if (onDone) onDone(); };
        close.onClick = [this] { if (onDone) onDone(); };
        setSize(440, 96 + 34 * std::max(1, info.nParams));
    }
    void paint(Graphics& g) override
    {
        g.fillAll(kPanel); const auto& info = mc::kOps[op]; g.setColour(kPink); g.setFont(sans(16.f, true)); g.drawText(String(info.name).toUpperCase(), 14, 8, 300, 22, juce::Justification::left);
        g.setColour(kDim); g.setFont(sans(12.f)); g.drawFittedText(info.tip, 14, 30, getWidth() - 28, 30, juce::Justification::topLeft, 2);
    }
    void resized() override
    {
        int y = 66; for (auto* r : rows) { r->name.setBounds(14, y, 100, 26); r->slider.setBounds(116, y, getWidth() - 130, 26); r->combo.setBounds(116, y, 160, 26); y += 34; }
        apply.setBounds(14, getHeight() - 40, 180, 30); close.setBounds(getWidth() - 94, getHeight() - 40, 80, 30);
    }
    struct Row { juce::Label name; juce::Slider slider; juce::ComboBox combo; };
    MeowProcessor& p; int op; float vals[mc::kMaxOpParams] {}; juce::OwnedArray<Row> rows; Pill apply { "Apply" }, close { "Close", kDim }; std::function<void()> onDone;
};

// a drag handle that hands the sound to the host as a file
struct DragHandle : juce::Component, juce::SettableTooltipClient
{
    MeowProcessor& p; juce::DragAndDropContainer* box;
    DragHandle(MeowProcessor& pr, juce::DragAndDropContainer* c) : p(pr), box(c) { setTooltip("Drag from here into the DAW or a folder to drop the sound as a WAV file."); }
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f); g.setColour(Colour(0xff1b1c21)); g.fillRoundedRectangle(r, 3.f); g.setColour(kTeal); g.drawRoundedRectangle(r, 3.f, 1.f);
        g.setColour(kTeal); g.setFont(sans(13.f, true)); g.drawFittedText("DRAG SOUND OUT", getLocalBounds(), juce::Justification::centred, 1);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 6 || p.doc.audio.frames() == 0 || dragging) return;
        const auto f = p.exportTempWav(); if (!f.existsAsFile()) return; dragging = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles({ f.getFullPathName() }, false, this, [this] { dragging = false; });
    }
    bool dragging = false;
};
}

struct MeowEditor::Impl
{
    Impl(MeowEditor& o, MeowProcessor& pr) : owner(o), p(pr), gate(o), display(pr), overview(pr, display), inM("IN"), outM("OUT"), drag(pr, &o)
    {
        o.setLookAndFeel(&laf); o.addAndMakeVisible(content); auto& c = content;
        for (auto* b : { &mFile, &mEdit, &mSelect, &mView, &mPlay, &mRecord, &mProcess, &mHelp, &guide, &about, &startNow, &startIn, &startCount, &takeB, &viewW, &viewS, &chCh, &chSr, &chBit, &chUnit, &mSpectral }) c.addAndMakeVisible(*b);
        buildSpecTools();
        for (auto* b : { &loop, &play, &stop, &rec, &monitor }) c.addAndMakeVisible(*b); c.addAndMakeVisible(recOpt);
        c.addAndMakeVisible(display); c.addAndMakeVisible(overview); c.addAndMakeVisible(inM); c.addAndMakeVisible(outM); c.addAndMakeVisible(drag);
        for (auto* l : { &readout, &timeL, &recHintL, &opHint }) c.addAndMakeVisible(*l);
        readout.setFont(mono(13.f)); readout.setColour(juce::Label::textColourId, kDim); recHintL.setFont(mono(12.f)); recHintL.setColour(juce::Label::textColourId, kPink);
        opHint.setFont(sans(12.f)); opHint.setColour(juce::Label::textColourId, kDim); opHint.setMinimumHorizontalScale(1.f); opHint.setJustificationType(juce::Justification::topLeft);
        timeL.setFont(mono(22.f, true)); timeL.setColour(juce::Label::textColourId, kText); timeL.setJustificationType(juce::Justification::centredLeft);
        auto knob = [&](juce::Slider& s, const char* id, const char* tip, const String& suffix) { s.setSliderStyle(juce::Slider::RotaryVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 14); s.setTooltip(tip); s.setTextValueSuffix(suffix);
            s.setColour(juce::Slider::textBoxTextColourId, kTeal); s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); c.addAndMakeVisible(s); atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, id, s)); };
        knob(thr, "thresh", "Level that starts a recording when the start is set to On input.", " dB"); knob(bpm, "bpm", "Tempo of the count-in click. The host tempo is used when the host gives one.", " bpm"); knob(pre, "preroll", "Sound from just before the start that is kept in the take.", " ms");
        loopA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "loop", loop); monA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "monitor", monitor);
        loop.setClickingTogglesState(true); monitor.setClickingTogglesState(true);
        viewW.setClickingTogglesState(false); viewS.setClickingTogglesState(false); viewW.setToggleState(true, juce::dontSendNotification);
        chCh.onClick = [this] { menu(chCh, channelMenu()); }; chSr.onClick = [this] { menu(chSr, rateMenu()); }; chBit.onClick = [this] { menu(chBit, bitMenu()); }; chUnit.onClick = [this] { menu(chUnit, unitMenu()); };
        chCh.setTooltip("Channels. Click to mix to mono, swap sides or use mid and side."); chSr.setTooltip("Sample rate. Click to convert the sound to another rate."); chBit.setTooltip("Export bit depth. Click to choose."); for (auto* c2 : { &chCh, &chSr, &chBit, &chUnit }) c2->raw = true;
        chUnit.setTooltip("Time line unit. Click to choose.");
        viewW.onClick = [this] { setSpectral(false); }; viewS.onClick = [this] { setSpectral(true); };
        loop.setTooltip("Repeat playback."); play.setTooltip("Play the selection, or from the cursor (Space)."); stop.setTooltip("Stop (Space or Esc)."); rec.setTooltip("Record (R). Press again to stop and keep the take."); recOpt.setTooltip("Record options: start mode and what a take does.");
        monitor.setTooltip("Hear the input while recording.");
        startNow.setTooltip("Recording starts the moment you press Record."); startIn.setTooltip("Press Record to arm. Recording starts when the incoming sound passes the threshold; the pre-roll is kept."); startCount.setTooltip("One bar of click (4/4) at the tempo, then recording starts.");
        takeB.setTooltip("What a new recording does: replace the sound, append at the end, punch over the selection or insert at the cursor. Click to change.");
        startNow.onClick = [this] { recMode = MeowProcessor::RecNow; }; startIn.onClick = [this] { recMode = MeowProcessor::RecOnInput; }; startCount.onClick = [this] { recMode = MeowProcessor::RecCountIn; };
        takeB.onClick = [this] { juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter("take")); const int cur = tp != nullptr ? tp->getIndex() : 0;
            if (tp != nullptr) for (int i = 0; i < tp->choices.size(); ++i) m.addItem(tp->choices[i], true, i == cur, [tp, i] { *tp = i; }); m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&takeB)); };
        guide.setTooltip("Open the manual."); about.setTooltip("Free and open source. Opens the Ko-fi page.");
        mFile.onClick = [this] { menu(mFile, fileMenu()); }; mEdit.onClick = [this] { menu(mEdit, editMenu()); }; mSelect.onClick = [this] { menu(mSelect, selectMenu()); }; mView.onClick = [this] { menu(mView, viewMenu()); };
        mPlay.onClick = [this] { menu(mPlay, playMenu()); }; mRecord.onClick = [this] { menu(mRecord, recordMenu()); }; mProcess.onClick = [this] { menu(mProcess, processMenu()); }; mSpectral.onClick = [this] { menu(mSpectral, spectralMenu()); }; mHelp.onClick = [this] { menu(mHelp, helpMenu()); };
        rec.onClick = [this] { toggleRecord(); }; stop.onClick = [this] { doStop(); }; play.onClick = [this] { doPlay(); }; recOpt.onClick = [this] { menu(recOpt, recordMenu()); };
        guide.onClick = [] { openManual(); }; about.onClick = [] { openAbout(); };
        rec.tint = kRed; play.tint = kGold; loop.tint = kGold; monitor.tint = kTeal; stop.tint = kText;
        buildTools(); buildOps();
        display.onSelection = [this] { refreshInfo(); };
        tip = std::make_unique<juce::TooltipWindow>(&o, 500);
        constrainer.setFixedAspectRatio((double) kBaseW / kBaseH); constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
        o.setConstrainer(&constrainer); o.setResizable(true, true); o.setWantsKeyboardFocus(true); o.setSize(kBaseW, kBaseH);
        gate.onFrame = [this] { tick(); }; gate.start(20);
        if (const char* f = std::getenv("MEOW_DEMO")) demo(String(f).getIntValue());   // documentation tool for screenshots, never set for users
    }
    ~Impl() { owner.setLookAndFeel(nullptr); }

    // ---- spectral tools bar ----
    void buildSpecTools()
    {
        auto& c = content; const char* ic[] = { "mouse", "select", "cut", "cutter", "eraser", "copy", "paste", "amp", "pencil" };
        const char* nm[] = { "Mouse", "Selector", "Scissors", "Cutter", "Eraser", "Copy", "Paste", "Amp", "Pencil" };
        for (int i = 0; i < 9; ++i)
        {
            auto* b = sTools.add(new IconButton(ic[i])); b->setTooltip(String(nm[i]) + " (Shift+" + String(i + 1) + "). " + display.toolHint(i)); b->onClick = [this, i] { display.setTool(i); }; c.addChildComponent(b);
        }
        int id = 1; for (int v : { -24, -12, -6, -3, 3, 6, 12 }) ampCb.addItem("Amp " + String(v > 0 ? "+" : "") + String(v) + " dB", id++);
        id = 1; for (int v : { -36, -24, -18, -12, -6 }) penCb.addItem("Pen " + String(v) + " dB", id++);
        brushCb.addItem("Brush small", 1); brushCb.addItem("Brush medium", 2); brushCb.addItem("Brush large", 3);
        ampCb.setSelectedId(6, juce::dontSendNotification); penCb.setSelectedId(3, juce::dontSendNotification); brushCb.setSelectedId(2, juce::dontSendNotification);
        ampCb.setTooltip("Level change the Amp tool applies to the box."); penCb.setTooltip("Level of the tone the Pencil draws."); brushCb.setTooltip("Size of the Eraser brush.");
        const int ampV[] = { -24, -12, -6, -3, 3, 6, 12 }, penV[] = { -36, -24, -18, -12, -6 }, brV[] = { 6, 12, 24 };
        ampCb.onChange = [this, ampV] { display.ampDb = (float) ampV[ampCb.getSelectedId() - 1]; }; penCb.onChange = [this, penV] { display.penDb = (float) penV[penCb.getSelectedId() - 1]; }; brushCb.onChange = [this, brV] { display.brush = brV[brushCb.getSelectedId() - 1]; };
        for (auto* cb : { &ampCb, &penCb, &brushCb }) c.addChildComponent(*cb);
        toolHintL.setFont(sans(12.f)); toolHintL.setColour(juce::Label::textColourId, kDim); toolHintL.setMinimumHorizontalScale(1.f); toolHintL.setJustificationType(juce::Justification::topLeft); c.addChildComponent(toolHintL);
        display.onHint = [this](const String& t) { toolHintL.setText(t, juce::dontSendNotification); };
        display.onToolChanged = [this] { for (int i = 0; i < sTools.size(); ++i) sTools[i]->setToggleState(i == display.tool, juce::dontSendNotification); };
        display.setTool(Display::Mouse);
    }
    void showSpecTools(bool on) { for (auto* b : sTools) b->setVisible(on); for (auto* cb : { &ampCb, &penCb, &brushCb }) cb->setVisible(on); toolHintL.setVisible(on); }
    juce::PopupMenu spectralMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0, reg = display.hasReg, clip = p.doc.hasSpectralClip();
        m.addItem("Show the spectral view", true, display.spectral, [this] { setSpectral(!display.spectral); }); m.addSeparator();
        const char* nm[] = { "Mouse", "Selector", "Scissors", "Cutter", "Eraser", "Copy", "Paste", "Amp", "Pencil" }; juce::PopupMenu t;
        for (int i = 0; i < 9; ++i) { juce::PopupMenu::Item it(nm[i]); it.setTicked(display.tool == i); it.shortcutKeyDescription = "Shift+" + String(i + 1); it.setAction([this, i] { setSpectral(true); display.setTool(i); }); t.addItem(std::move(it)); }
        m.addSubMenu("Tool", t);
        juce::PopupMenu am; { int k = 1; for (int v : { -24, -12, -6, -3, 3, 6, 12 }) { const int kk = k++; am.addItem(String(v > 0 ? "+" : "") + String(v) + " dB", true, (int) display.ampDb == v, [this, kk] { ampCb.setSelectedId(kk); }); } } m.addSubMenu("Amp amount", am);
        juce::PopupMenu br; { const char* bn[] = { "Small", "Medium", "Large" }; const int bv[] = { 6, 12, 24 }; for (int i = 0; i < 3; ++i) br.addItem(bn[i], true, display.brush == bv[i], [this, i] { brushCb.setSelectedId(i + 1); }); } m.addSubMenu("Eraser brush", br);
        m.addSeparator();
        auto done = [this] { display.edited(); };
        m.addItem("Cut the box", has && reg, false, [this, done] { p.stopPlay(); p.doc.spectralEdit(true, { display.sreg }, 0.f); done(); });
        m.addItem("Amp the box", has && reg, false, [this, done] { p.stopPlay(); p.doc.spectralEdit(false, { display.sreg }, display.ampDb); done(); });
        m.addItem("Copy the box", has && reg, false, [this] { p.doc.spectralCopy(display.sreg); });
        m.addItem("Paste at the cursor", has && clip, false, [this, done] { p.stopPlay(); p.doc.spectralPaste((double) p.doc.cursor, p.doc.spectralClip().fc); done(); });
        m.addItem("Clear the box", reg, false, [this] { display.hasReg = false; display.repaint(); });
        return m;
    }

    // ---- tool bar and process grid ----
    struct ToolSpec { IconButton* b; bool gapBefore; std::function<bool()> enabled; };
    void buildTools()
    {
        auto& c = content; auto& d = p.doc; const std::function<bool()> frames = [this] { return p.doc.audio.frames() > 0; }; const std::function<bool()> sel = [this] { return p.doc.hasSelection(); };
        auto add = [&](const char* ic, const String& tipText, bool gap, std::function<void()> f, std::function<bool()> en) { auto* b = tools.add(new IconButton(ic)); b->setTooltip(tipText); b->onClick = std::move(f); c.addAndMakeVisible(b); specs.push_back({ b, gap, std::move(en) }); return b; };
        auto edit = [this](std::function<void()> f) { return [this, f] { p.stopPlay(); f(); p.changed(); }; };
        add("new", "New, an empty sound.", false, [this] { p.newSound(); display.fitAll(); }, {});
        add("open", "Open a sound from a file.", false, [this] { openFile(); }, {});
        add("save", "Export the sound as a 24 bit WAV file.", false, [this] { saveFile(false); }, frames);
        add("undo", "Undo (Ctrl+Z).", true, [this] { doUndo(); }, [this] { return p.doc.canUndo(); });
        add("redo", "Redo (Ctrl+Y).", false, [this] { doRedo(); }, [this] { return p.doc.canRedo(); });
        { auto* hb = add("hist", "History: every step so far. Click one to go back to it, or forward again.", false, {}, frames); hb->onClick = [this, hb] { menu(*hb, historyMenu()); }; }
        add("cut", "Cut the selection (Ctrl+X).", true, edit([&d] { d.cut(); }), sel);
        add("copy", "Copy the selection (Ctrl+C).", false, [&d] { d.copy(); }, sel);
        add("paste", "Paste at the cursor, or over the selection (Ctrl+V).", false, edit([&d] { d.paste(); }), [this] { return p.doc.clipboard().frames() > 0; });
        add("del", "Delete the selection (Del).", false, edit([&d] { d.deleteSelection(); }), sel);
        add("crop", "Keep only the selection (Ctrl+T).", false, edit([&d] { d.cropToSelection(); }), sel);
        add("all", "Select everything (Ctrl+A).", true, [this] { selectAll(); }, frames);
        add("snap", "Move the selection edges, or the cursor, to the nearest zero crossing so cuts do not click.", false, [this] { snapSel(); }, frames);
        add("zin", "Zoom in.", true, [this] { display.zoom(0.5, display.getWidth() * 0.5f); }, frames);
        add("zout", "Zoom out.", false, [this] { display.zoom(2.0, display.getWidth() * 0.5f); }, frames);
        add("fit", "Show the whole sound.", false, [this] { display.fitAll(); }, frames);
        add("tosel", "Zoom to the selection.", false, [this] { display.zoomToSelection(); }, sel);
        add("repeat", "Repeat the last operation (Ctrl+Shift+Z).", true, edit([&d] { d.repeatLast(); }), [this] { return p.doc.last.op >= 0; });
    }
    static Colour groupColour(const String& g)
    {
        if (g == "Edit") return Colour(0xff6cc9f5); if (g == "Stereo") return Colour(0xffa43dff); if (g == "Time") return Colour(0xffff9000); if (g == "Colour") return Colour(0xffffd100); if (g == "Tone") return Colour(0xffff1383);
        return Colour(0xff02ebba);
    }
    static char keyFor(const String& id)
    {
        static const std::pair<const char*, char> k[] = { { "gain", 'G' }, { "normalize", 'N' }, { "fadeIn", 'I' }, { "fadeOut", 'O' }, { "reverse", 'R' }, { "invert", 'P' }, { "dcRemove", 'D' }, { "silence", 'S' }, { "mono", 'M' }, { "pan", 'B' }, { "rate", 'Y' }, { "bitcrush", 'K' },
            { "drive", 'V' }, { "eq", 'E' }, { "filter", 'F' }, { "delay", 'L' }, { "chorus", 'H' }, { "phaser", 'A' }, { "reverb", 'W' }, { "compressor", 'C' }, { "limiter", 'T' }, { "gate", 'U' }, { "trimSilence", 'X' }, { "loopXfade", 'J' } };
        for (auto& e : k) if (id == e.first) return e.second;
        return 0;
    }
    void buildOps()
    {
        std::vector<String> groups; for (int i = 0; i < mc::opCount; ++i) { const String g(mc::kOps[i].group); if (std::find(groups.begin(), groups.end(), g) == groups.end()) groups.push_back(g); }
        for (auto& g : groups) for (int i = 0; i < mc::opCount; ++i) if (String(mc::kOps[i].group) == g) opOrder.push_back(i);
        for (int i : opOrder)
        {
            const String id(mc::kOps[i].id); auto* b = opBtns.add(new IconButton(id == "mono" ? "monoOp" : id, groupColour(mc::kOps[i].group)));
            const char k = keyFor(id); const String full = String(mc::kOps[i].name) + ": " + mc::kOps[i].tip + (k != 0 ? String("  (Shift+") + String::charToString((juce::juce_wchar) k) + ")" : String());
            b->setTooltip(full); b->onHover = [this, full](bool on) { opHint.setText(on ? full : String(), juce::dontSendNotification); };
            b->onClick = [this, i, b] { openOp(i, b); }; b->onDouble = [this, i] { closeBox(); applyOpNow(i); }; content.addAndMakeVisible(b);
        }
    }

    // ---- actions ----
    void setSpectral(bool s) { display.spectral = s; showSpecTools(s); viewS.setToggleState(s, juce::dontSendNotification); viewW.setToggleState(!s, juce::dontSendNotification); display.repaint(); }
    static void openManual() { juce::URL("https://github.com/SpeakingColors/Meowcorder/blob/main/docs/Meowcorder_manual.pdf").launchInDefaultBrowser(); }
    static void openAbout() { juce::URL("https://ko-fi.com/studio87").launchInDefaultBrowser(); }
    void doStop() { if (p.isRecording()) p.stopRecord(); p.stopPlay(); }
    void doPlay() { if (p.isPlaying()) p.stopPlay(); else { if (p.isRecording()) p.stopRecord(); p.play(); } }
    void toggleRecord() { if (p.isRecording()) p.stopRecord(); else { p.startRecord(recMode); } }
    void doUndo() { p.stopPlay(); p.doc.undo(); p.changed(); } void doRedo() { p.stopPlay(); p.doc.redo(); p.changed(); }
    void selectAll() { p.doc.setSelection(0, p.doc.audio.frames()); display.repaint(); refreshInfo(); }
    void snapSel()
    {
        auto& d = p.doc; const size_t r = (size_t) std::max(8.0, d.audio.sr * 0.01);
        if (d.hasSelection()) { const size_t a = d.snapToZero(d.selA, r), b = d.snapToZero(d.selB, r); if (b > a) d.setSelection(a, b); } else d.setCursor(d.snapToZero(d.cursor, r));
        display.repaint(); refreshInfo();
    }
    void openFile()
    {
        chooser = std::make_unique<juce::FileChooser>("Open a sound", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) { if (p.loadFile(fc.getResult())) display.fitAll(); else status = "Could not read that file"; } });
    }
    void saveFile(bool flac)
    {
        chooser = std::make_unique<juce::FileChooser>(flac ? "Export FLAC" : "Export WAV", juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(flac ? "sound.flac" : "sound.wav"), flac ? "*.flac" : "*.wav");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this, flac](const juce::FileChooser& fc) { if (fc.getResult() != juce::File() && !p.saveFile(fc.getResult(), flac)) status = "Could not save"; });
    }
    void closeBox() { juce::Component::SafePointer<juce::Component> sp(box); box = nullptr; if (sp != nullptr) if (auto* cb = dynamic_cast<juce::CallOutBox*>(sp.getComponent())) cb->dismiss(); }
    void openOp(int op, juce::Component* at)
    {
        if (p.doc.audio.frames() == 0) return; closeBox();
        auto panel = std::make_unique<OpPanel>(p, op, [this] { closeBox(); });
        auto& cb = juce::CallOutBox::launchAsynchronously(std::move(panel), at->getScreenBounds(), nullptr); box = &cb;
    }
    void applyOpNow(int op)
    {
        if (p.doc.audio.frames() == 0) return; float v[mc::kMaxOpParams]; mc::defaultParams(op, v); if (p.doc.last.op == op) for (int i = 0; i < mc::kMaxOpParams; ++i) v[i] = p.doc.last.p[i];
        p.stopPlay(); if (p.doc.apply(op, v)) p.changed();
    }
    void menu(juce::Component& at, juce::PopupMenu m) { m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&at)); }
    juce::PopupMenu fileMenu()
    {
        juce::PopupMenu m; m.addItem("New (empty sound)", [this] { p.newSound(); display.fitAll(); }); m.addItem("Open sound...", [this] { openFile(); }); m.addSeparator();
        m.addItem("Export WAV (24 bit)...", p.doc.audio.frames() > 0, false, [this] { saveFile(false); }); m.addItem("Export FLAC (24 bit)...", p.doc.audio.frames() > 0, false, [this] { saveFile(true); }); return m;
    }
    juce::PopupMenu editMenu()
    {
        juce::PopupMenu m; auto item = [&](const String& t, const String& sc, bool en, std::function<void()> f) { juce::PopupMenu::Item it(t); it.setEnabled(en); it.setAction(std::move(f)); it.shortcutKeyDescription = sc; m.addItem(std::move(it)); };
        auto& d = p.doc; auto chg = [this](std::function<void()> f) { return [this, f] { p.stopPlay(); f(); p.changed(); }; };
        item("Undo", "Ctrl+Z", d.canUndo(), [this] { doUndo(); }); item("Redo", "Ctrl+Y", d.canRedo(), [this] { doRedo(); });
        item("Repeat last operation", "Ctrl+Shift+Z", d.last.op >= 0, chg([&d] { d.repeatLast(); })); m.addSeparator();
        item("Cut", "Ctrl+X", d.hasSelection(), chg([&d] { d.cut(); })); item("Copy", "Ctrl+C", d.hasSelection(), [&d] { d.copy(); }); item("Paste", "Ctrl+V", d.clipboard().frames() > 0, chg([&d] { d.paste(); }));
        item("Delete", "Del", d.hasSelection(), chg([&d] { d.deleteSelection(); })); item("Crop to selection", "Ctrl+T", d.hasSelection(), chg([&d] { d.cropToSelection(); })); return m;
    }
    // History: Original, then every step done; the undone ones stay below in grey until a new edit replaces them
    juce::PopupMenu historyMenu()
    {
        juce::PopupMenu m; const auto h = p.doc.history(); const int nd = (int) h.done.size();
        auto go = [this](int k) { p.stopPlay(); p.doc.jumpTo((size_t) k); p.changed(); };
        m.addItem("Original", true, nd == 0, [go] { go(0); });
        for (int i = 0; i < nd; ++i) m.addItem(String(i + 1) + "  " + String(h.done[(size_t) i]), true, i + 1 == nd, [go, i] { go(i + 1); });
        for (size_t j = 0; j < h.undone.size(); ++j) { const int k = nd + 1 + (int) j; m.addItem(String(k) + "  " + String(h.undone[j]) + "  (undone)", true, false, [go, k] { go(k); }); }
        return m;
    }
    juce::PopupMenu channelMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0;
        auto op = [this](const char* label, mc::ChannelKind k) { return [this, label, k] { p.stopPlay(); p.doc.transform(label, [k](mc::Audio& a) { mc::channelOp(a, k); }); p.changed(); }; };
        m.addItem("Mix down to mono", has, false, op("Mix to mono", mc::ChannelKind::Mix)); m.addItem("Left to both sides", has, false, op("Left to both", mc::ChannelKind::Left));
        m.addItem("Right to both sides", has, false, op("Right to both", mc::ChannelKind::Right)); m.addItem("Swap left and right", has, false, op("Swap channels", mc::ChannelKind::Swap)); m.addSeparator();
        m.addItem("Mid and side: encode", has, false, op("Mid/side encode", mc::ChannelKind::MidSideEnc)); m.addItem("Mid and side: decode", has, false, op("Mid/side decode", mc::ChannelKind::MidSideDec)); return m;
    }
    juce::PopupMenu rateMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0; const double cur = p.doc.audio.sr;
        for (double r : { 8000.0, 11025.0, 16000.0, 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
            m.addItem(String(r / 1000.0, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + " kHz", has, std::abs(cur - r) < 0.5, [this, r] { p.stopPlay(); p.doc.transform(("Convert to " + String(r, 0) + " Hz").toStdString(), [r](mc::Audio& a) { a = mc::resample(a, r); }); p.doc.setCursor(0); p.changed(); });
        m.addSeparator(); m.addItem("Converting changes the sound itself; it keeps its length in seconds.", false, false, [] {}); return m;
    }
    juce::PopupMenu bitMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0; const int cur = p.exportBits.load();
        for (int b : { 8, 16, 24, 32 }) m.addItem(String(b) + (b == 32 ? " bit float (WAV only)" : " bit") + " export", true, cur == b, [this, b] { p.exportBits = b; refreshInfo(); repaintChips(); });
        m.addSeparator(); m.addItem("Dither when exporting below 24 bit", true, p.exportDither.load(), [this] { p.exportDither = !p.exportDither.load(); });
        m.addItem("Round the sound itself to " + String(std::min(cur, 24)) + " bit now", has && cur < 24, false, [this, cur] { p.stopPlay(); const bool d = p.exportDither.load(); p.doc.transform(("Round to " + String(cur) + " bit").toStdString(), [cur, d](mc::Audio& a) { mc::quantize(a, cur, d); }); p.changed(); }); return m;
    }
    juce::PopupMenu unitMenu()
    {
        juce::PopupMenu m; const char* names[] = { "Minutes and seconds", "Seconds", "Samples", "Bars and beats (4/4, at the tempo knob)" };
        for (int i = 0; i < 4; ++i) m.addItem(names[i], true, gUnit == i, [this, i] { gUnit = i; refreshInfo(); display.repaint(); overview.repaint(); repaintChips(); });
        return m;
    }
    void repaintChips() { updateChips(); }
    juce::PopupMenu selectMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0;
        m.addItem("Select all", has, false, [this] { selectAll(); }); m.addItem("Select none", has, false, [this] { p.doc.setCursor(p.doc.hasSelection() ? p.doc.selA : p.doc.cursor); display.repaint(); refreshInfo(); });
        m.addItem("Snap to zero crossings", has, false, [this] { snapSel(); }); return m;
    }
    juce::PopupMenu processMenu()
    {
        juce::PopupMenu m; String last;
        for (int i : opOrder)
        {
            const String g(mc::kOps[i].group); if (g != last) { if (last.isNotEmpty()) m.addSeparator(); m.addSectionHeader(g); last = g; }
            juce::PopupMenu::Item it(mc::kOps[i].name); it.setEnabled(p.doc.audio.frames() > 0); const char k = keyFor(mc::kOps[i].id); if (k != 0) it.shortcutKeyDescription = String("Shift+") + String::charToString((juce::juce_wchar) k);
            it.setAction([this, i] { openOp(i, &mProcess); }); m.addItem(std::move(it));
        }
        return m;
    }
    juce::PopupMenu viewMenu()
    {
        juce::PopupMenu m; m.addItem("Waveform", true, !display.spectral, [this] { setSpectral(false); }); m.addItem("Spectral", true, display.spectral, [this] { setSpectral(true); }); m.addSeparator();
        m.addItem("Zoom in", [this] { display.zoom(0.5, display.getWidth() * 0.5f); }); m.addItem("Zoom out", [this] { display.zoom(2.0, display.getWidth() * 0.5f); });
        m.addItem("Zoom to selection", p.doc.hasSelection(), false, [this] { display.zoomToSelection(); }); m.addItem("Show all", [this] { display.fitAll(); }); return m;
    }
    juce::PopupMenu playMenu()
    {
        juce::PopupMenu m; juce::PopupMenu::Item a(p.isPlaying() ? "Pause" : "Play"); a.shortcutKeyDescription = "Space"; a.setAction([this] { doPlay(); }); m.addItem(std::move(a));
        m.addItem("Play from the start", p.doc.audio.frames() > 0, false, [this] { p.stopPlay(); p.doc.setCursor(0); p.play(); }); m.addItem("Stop", [this] { doStop(); }); return m;
    }
    juce::PopupMenu recordMenu()
    {
        juce::PopupMenu m; m.addSectionHeader("Start the recording");
        m.addItem("Right away", true, recMode == MeowProcessor::RecNow, [this] { recMode = MeowProcessor::RecNow; });
        m.addItem("When the input is heard", true, recMode == MeowProcessor::RecOnInput, [this] { recMode = MeowProcessor::RecOnInput; });
        m.addItem("After a 1 bar count-in (4/4)", true, recMode == MeowProcessor::RecCountIn, [this] { recMode = MeowProcessor::RecCountIn; }); m.addSeparator();
        m.addItem("Record now", [this] { if (!p.isRecording()) p.startRecord(recMode); }); return m;
    }
    juce::PopupMenu helpMenu()
    {
        juce::PopupMenu m; m.addItem("Manual", [] { openManual(); }); m.addItem("Keyboard shortcuts", [] {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Keyboard shortcuts",
                "Space: play or pause\nEsc: stop\nR: record\nCtrl+Z / Ctrl+Y: undo / redo\nCtrl+Shift+Z: repeat the last operation\nCtrl+X / C / V: cut, copy, paste\nDel: delete the selection\nCtrl+T: crop to the selection\nCtrl+A: select all\nShift+letter: apply a process at once (see the Process menu)\nMouse wheel: zoom; Shift+wheel: scroll\nDouble click a process icon: apply it with its last settings"); });
        m.addSeparator(); m.addItem("About and support (Ko-fi)", [] { openAbout(); }); return m;
    }
    bool keys(const juce::KeyPress& k)
    {
        const auto m = k.getModifiers(); const int c = k.getKeyCode(); const bool ctrl = m.isCommandDown() || m.isCtrlDown();
        if (!ctrl && m.isShiftDown() && c < 128 && std::isalpha(c)) { const int up = std::toupper(c); for (int i = 0; i < mc::opCount; ++i) if (keyFor(mc::kOps[i].id) == up) { applyOpNow(i); return true; } }
        if (!ctrl && m.isShiftDown() && display.spectral) { const juce::juce_wchar tc = k.getTextCharacter(); int d = -1; if (c >= '1' && c <= '9') d = c - '1'; else { const char* sh = "!@#$%^&*("; for (int i = 0; i < 9; ++i) if (tc == (juce::juce_wchar) sh[i]) d = i; } if (d >= 0) { display.setTool(d); return true; } }
        if (c == juce::KeyPress::spaceKey) { doPlay(); return true; } if (c == juce::KeyPress::escapeKey) { doStop(); return true; }
        if (!ctrl && !m.isShiftDown() && (c == 'R' || c == 'r')) { toggleRecord(); return true; }
        if (ctrl && (c == 'Z' || c == 'z')) { if (m.isShiftDown()) { p.doc.repeatLast(); p.changed(); } else doUndo(); return true; }
        if (ctrl && (c == 'Y' || c == 'y')) { doRedo(); return true; }
        if (ctrl && (c == 'X' || c == 'x')) { p.stopPlay(); p.doc.cut(); p.changed(); return true; }
        if (ctrl && (c == 'C' || c == 'c')) { p.doc.copy(); return true; }
        if (ctrl && (c == 'V' || c == 'v')) { p.stopPlay(); p.doc.paste(); p.changed(); return true; }
        if (ctrl && (c == 'T' || c == 't')) { p.stopPlay(); p.doc.cropToSelection(); p.changed(); return true; }
        if (ctrl && (c == 'A' || c == 'a')) { selectAll(); return true; }
        if (c == juce::KeyPress::deleteKey || c == juce::KeyPress::backspaceKey) { p.stopPlay(); p.doc.deleteSelection(); p.changed(); return true; }
        return false;
    }

    void updateChips()
    {
        const auto& a = p.doc.audio; const int bits = p.exportBits.load(); const char* un[] = { "min:sec", "seconds", "samples", "bars:beats" };
        chCh.setButtonText(mc::isMonoAudio(a) && a.frames() > 0 ? "Mono" : "Stereo"); chSr.setButtonText(String(a.sr / 1000.0, 1) + " kHz"); chBit.setButtonText(bits == 32 ? "32 bit float" : String(bits) + " bit"); chUnit.setButtonText(un[gUnit]);
        gUnitSr = a.sr > 0 ? a.sr : 44100.0; gUnitBpm = p.currentBpm();
    }
    void refreshInfo()
    {
        updateChips(); const auto& a = p.doc.audio;
        if (a.frames() == 0) { readout.setText("Length 0:00.000   Selection none   Cursor 0:00.000", juce::dontSendNotification); return; }
        String s = "Length " + timeText((double) a.frames() / a.sr) + "   Selection ";
        s += p.doc.hasSelection() ? timeText((double) p.doc.selA / a.sr) + " to " + timeText((double) p.doc.selB / a.sr) : String("none, operations use everything");
        s += "   Cursor " + timeText((double) p.doc.cursor / a.sr) + "   Peak " + String(mc::linToDb(a.peak(0, a.frames())), 1) + " dBFS";
        readout.setText(s, juce::dontSendNotification);
    }
    void tick()
    {
        overview.tick(); inM.set(p.inPeak.load()); outM.set(p.outPeak.load());
        if (seenVer != p.version()) { seenVer = p.version(); refreshInfo(); display.repaint(); }
        const bool r = p.isRecording(), arm = p.isArmedOnly(), has = p.doc.audio.frames() > 0;
        rec.setToggleState(r, juce::dontSendNotification); play.setToggleState(p.isPlaying(), juce::dontSendNotification);
        startNow.setToggleState(recMode == MeowProcessor::RecNow, juce::dontSendNotification); startIn.setToggleState(recMode == MeowProcessor::RecOnInput, juce::dontSendNotification); startCount.setToggleState(recMode == MeowProcessor::RecCountIn, juce::dontSendNotification);
        if (auto* tp = dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter("take"))) takeB.setButtonText(tp->getCurrentChoiceName());
        recHintL.setText(p.isCountingIn() ? "Count-in..." : (arm ? "Armed, waiting for sound" : (r ? "Recording" : String())), juce::dontSendNotification);
        timeL.setText(r ? timeText(p.recordedSeconds()) : (p.isPlaying() ? timeText(p.playPosFrames() / std::max(1.0, p.doc.audio.sr)) : timeText((double) p.doc.cursor / std::max(1.0, p.doc.audio.sr))), juce::dontSendNotification);
        for (auto& sp : specs) { const bool en = sp.enabled ? sp.enabled() : true; if (sp.b->isEnabled() != en) sp.b->setEnabled(en); }
        for (auto* b : opBtns) if (b->isEnabled() != has) b->setEnabled(has);
        if (p.isPlaying() || r) display.repaint();
        if (status.isNotEmpty() && ++statusAge > 60) { status = {}; statusAge = 0; } content.repaint(juce::Rectangle<int>(14, 764, 800, 20));
    }
    void demo(int what)
    {
        mc::Audio a; a.sr = 48000; const size_t n = (size_t) (a.sr * 3.2); a.resize(n); mc::Rng rng;
        for (size_t i = 0; i < n; ++i) { const double t = (double) i / a.sr; const double ph = std::fmod(t, 0.8); const double env = std::exp(-ph * 7.0); const double f = 55.0 + 180.0 * std::exp(-ph * 30.0);
            const float s = (float) (std::sin(2.0 * 3.14159265 * f * ph) * env * 0.8 + rng.bi() * std::exp(-ph * 40.0) * 0.3); a.ch[0][i] = s; a.ch[1][i] = s * 0.92f; }
        p.doc.load(a); p.doc.setSelection((size_t) (a.sr * 0.7), (size_t) (a.sr * 2.1)); p.changed(); display.fitAll();
        if (what == 1) { display.vStart = (double) n * 0.15; display.vLen = (double) n * 0.55; display.clampView(); }
        if (what == 2) { setSpectral(true); display.setTool(Display::Select); display.sreg = { n * 0.2, n * 0.5, 80.0, 900.0 }; display.hasReg = true; }
        if (what == 3 && !opBtns.isEmpty()) juce::Timer::callAfterDelay(800, [this] { openOp(opOrder[0], opBtns[0]); });
        if (const char* out = std::getenv("MEOW_SNAPSHOT")) juce::Timer::callAfterDelay(2500, [this, path = String(out)] {
            auto img = owner.createComponentSnapshot(owner.getLocalBounds(), true, 1.5f); juce::File f(path); f.deleteFile(); juce::FileOutputStream os(f); if (os.openedOk()) juce::PNGImageFormat().writeImageToStream(img, os);
            juce::MessageManager::getInstance()->stopDispatchLoop(); });
    }

    static float textW(const String& t) { juce::GlyphArrangement ga; ga.addLineOfText(sans(13.f, true), t, 0.f, 0.f); return ga.getBoundingBox(0, -1, true).getWidth(); }
    float scale() const { return (float) owner.getWidth() / (float) kBaseW; }
    void layout()
    {
        int x = 14; for (auto* b : { &mFile, &mEdit, &mSelect, &mView, &mPlay, &mRecord, &mProcess, &mSpectral, &mHelp }) { const int w = juce::roundToInt(textW(b->getButtonText().toUpperCase())) + 22; b->setBounds(x, 92, w, 28); x += w + 4; }
        guide.setBounds(900, 20, 90, 34); about.setBounds(1000, 20, 130, 34);
        loop.setBounds(14, 126, 36, 34); play.setBounds(54, 126, 36, 34); stop.setBounds(94, 126, 36, 34); rec.setBounds(134, 126, 36, 34); recOpt.setBounds(172, 126, 18, 34);
        startNow.setBounds(204, 129, 66, 28); startIn.setBounds(274, 129, 92, 28); startCount.setBounds(370, 129, 86, 28); takeB.setBounds(464, 129, 110, 28);
        thr.setBounds(14, 180, 66, 58); bpm.setBounds(88, 180, 66, 58); pre.setBounds(162, 180, 66, 58);
        timeL.setBounds(246, 176, 180, 30); recHintL.setBounds(246, 208, 380, 24);
        for (int k = 0; k < opBtns.size(); ++k) opBtns[k]->setBounds(640 + (k / 2) * 31, 92 + (k % 2) * 31, 28, 28);
        opHint.setBounds(640, 158, 384, 60);
        x = 14; for (auto& sp : specs) { if (sp.gapBefore) x += 10; sp.b->setBounds(x, 244, 30, 30); x += 33; }
        monitor.setBounds(x + 10, 244, 30, 30);
        readout.setBounds(14, 282, 1010, 20); chCh.setBounds(14, 308, 78, 20); chSr.setBounds(96, 308, 96, 20); chBit.setBounds(196, 308, 128, 20); chUnit.setBounds(328, 308, 110, 20); toolHintL.setBounds(450, 306, 574, 22);
        { int tx = 270; for (auto* b : sTools) { b->setBounds(tx, 330, 30, 30); tx += 33; } ampCb.setBounds(tx + 6, 332, 112, 26); penCb.setBounds(tx + 124, 332, 112, 26); brushCb.setBounds(tx + 242, 332, 130, 26); } viewW.setBounds(62, 332, 96, 26); viewS.setBounds(162, 332, 96, 26);
        display.setBounds(14, 364, 1010, 340); overview.setBounds(14, 708, 1010, 36);
        inM.setBounds(1036, 92, 52, 652); outM.setBounds(1088, 92, 52, 652); drag.setBounds(940, 756, 186, 30);
    }
    void paintContent(Graphics& g)
    {
        g.fillAll(kBg); catui::drawCatHead(g, { 22, 14, 60, 56 }, kText, kBg);
        g.setColour(kText); g.setFont(sans(34.f, true)); g.drawText("MEOWCORDER", 96, 8, 400, 46, juce::Justification::left);
        g.setColour(kGold); g.setFont(sans(14.f, true)); g.drawText("STUDIO 87", 98, 52, 200, 20, juce::Justification::left);
        g.setColour(kEdge); g.drawHorizontalLine(84, 14.f, 1126.f);
        g.setColour(kDim); g.setFont(sans(10.5f, true));
        g.drawText("THRESHOLD", 6, 166, 82, 12, juce::Justification::centred); g.drawText("TEMPO", 80, 166, 82, 12, juce::Justification::centred); g.drawText("PRE-ROLL", 154, 166, 82, 12, juce::Justification::centred);
        g.drawText("VIEW", 14, 332, 44, 26, juce::Justification::left);
        g.setColour(kDim); g.setFont(mono(11.f)); g.drawText(status.isNotEmpty() ? status : String("STUDIO 87 | MEOWCORDER | hover a control for help | collects nothing, no network"), 14, 764, 800, 16, juce::Justification::left);
    }

    struct Content : juce::Component { Impl& i; explicit Content(Impl& x) : i(x) {} void paint(Graphics& g) override { i.paintContent(g); } } content { *this };
    MeowEditor& owner; MeowProcessor& p; catui::CatLookAndFeel laf; juce::ComponentBoundsConstrainer constrainer; catui::RenderGate gate;
    Pill mFile { "File", kTeal }, mEdit { "Edit", kTeal }, mSelect { "Select", kTeal }, mView { "View", kTeal }, mPlay { "Play", kTeal }, mRecord { "Record", kTeal }, mProcess { "Process", kTeal }, mHelp { "Help", kTeal }, guide { "Guide" }, about { "About & Support" };
    juce::OwnedArray<IconButton> sTools; juce::ComboBox ampCb, penCb, brushCb; juce::Label toolHintL;
    Pill mSpectral { "Spectral", kTeal };
    Pill chCh { "Stereo", kTeal }, chSr { "44.1 kHz", kTeal }, chBit { "24 bit", kTeal }, chUnit { "min:sec", kTeal };
    Pill startNow { "Now", kPink }, startIn { "On input", kPink }, startCount { "Count-in", kPink }, takeB { "Replace", kGold }, viewW { "Waveform" }, viewS { "Spectral" };
    IconButton loop { "loop" }, play { "play" }, stop { "stop" }, rec { "rec" }, monitor { "spk" }; Pill recOpt { juce::String::fromUTF8("\xe2\x96\xbe"), kRed };
    Display display; Overview overview; Meter inM, outM; DragHandle drag; juce::Label readout, timeL, recHintL, opHint; juce::Slider thr, bpm, pre;
    juce::OwnedArray<IconButton> tools, opBtns; std::vector<ToolSpec> specs; std::vector<int> opOrder; 
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopA, monA; std::unique_ptr<juce::TooltipWindow> tip; std::unique_ptr<juce::FileChooser> chooser;
    juce::Component* box = nullptr; int recMode = MeowProcessor::RecNow, seenVer = -1, statusAge = 0; String status;
};

MeowEditor::MeowEditor(MeowProcessor& p) : AudioProcessorEditor(&p) { impl = std::make_unique<Impl>(*this, p); resized(); startTimerHz(10); }
MeowEditor::~MeowEditor() { stopTimer(); }
void MeowEditor::timerCallback() {}
// impl is still null while its constructor runs (setSize calls resized and paint), so those skip until it exists
void MeowEditor::paint(Graphics& g) { g.fillAll(kBg); if (!impl) return; catui::drawWoodCheeks(g, getLocalBounds(), juce::roundToInt(kCheek * impl->scale())); }
void MeowEditor::resized()
{
    if (!impl) return;
    const float s = impl->scale(); impl->content.setBounds(juce::roundToInt(kCheek * s), 0, kContentW, kBaseH); impl->content.setTransform(juce::AffineTransform::scale(s)); impl->layout();
}
void MeowEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!impl) return; const auto p = e.getEventRelativeTo(this).getPosition(); if (p.x < kCheek * impl->scale() || p.x > getWidth() - kCheek * impl->scale()) setSize(kBaseW, kBaseH);
}
bool MeowEditor::keyPressed(const juce::KeyPress& k) { return impl && impl->keys(k); }
