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
    enum Layer { LWave, LSpec, LPitch };
    static constexpr int kRuler = 22;
    MeowProcessor& p; bool spectral = false; double vStart = 0, vLen = 0; int seen = -1; juce::Image spec; String specKey;
    std::function<void()> onSelection, onLayers;
    bool lay[3] = { true, false, false }, combine = false, split = false, showHits = false, wpen = false; int front = LWave, active = LWave, snapNote = 0;   // snapNote: 0 drag free, 1 drag lands on a semitone
    std::vector<int> vis() const { std::vector<int> v; if (combine) for (int l = 0; l < 3; ++l) if (lay[l]) v.push_back(l); if (v.empty()) v.push_back(front); return v; }
    int frontLayer() const { const auto v = vis(); return std::find(v.begin(), v.end(), front) == v.end() ? v[0] : front; }
    void layersChanged() { front = frontLayer(); spectral = front == LSpec; if (onLayers) onLayers(); repaint(); }
    void showOnly(int l) { combine = false; lay[0] = lay[1] = lay[2] = false; lay[l] = true; front = l; layersChanged(); }
    void toggleLayer(int l) { if (combine) { lay[l] = !lay[l]; if (!lay[0] && !lay[1] && !lay[2]) lay[l] = true; if (lay[l]) front = l; else if (front == l) front = vis()[0]; layersChanged(); } else showOnly(l); }
    void setCombine(bool c) { if (!c) showOnly(frontLayer()); else { const int f = frontLayer(); combine = true; lay[f] = true; layersChanged(); } }
    bool splitNow() const { return combine && split && vis().size() > 1; }
    juce::Rectangle<int> areaR() const { return getLocalBounds().withTrimmedBottom(kRuler); }
    juce::Rectangle<int> bandOf(int l) const
    {
        const auto a = areaR(); const auto v = vis(); if (!splitNow()) return a; int i = -1; for (int k = 0; k < (int) v.size(); ++k) if (v[(size_t) k] == l) i = k; if (i < 0) return a;
        const int h = a.getHeight() / (int) v.size(); return { a.getX(), a.getY() + i * h, a.getWidth(), i == (int) v.size() - 1 ? a.getHeight() - i * h : h };
    }
    int layerAt(float y) const { if (!splitNow()) return frontLayer(); for (int l : vis()) if (bandOf(l).contains(0, (int) y)) return l; return frontLayer(); }
    juce::Point<float> inBand(int l, juce::Point<float> q) const { return q - bandOf(l).getPosition().toFloat(); }
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
        if (frames() == 0) return; active = layerAt((float) e.y);
        if (active == LSpec && tool != Mouse) { specDown(e); return; }
        if (active == LPitch) { const int ni = noteAt(inBand(LPitch, e.position)); if (ni >= 0) { pd = PDrag(); pd.on = true; pd.note = ni; pd.y0 = e.position.y; repaint(); return; } }
        if (active == LWave && wpen && vLen / (double) std::max(1, getWidth()) <= 4.0) { wpStart(e); return; }
        dragFrom = (size_t) juce::jlimit(0.0, (double) frames(), xToFrame((float) e.x));
        if (e.mods.isShiftDown() && (p.doc.hasSelection() || p.doc.cursor > 0)) { const size_t a = p.doc.hasSelection() ? p.doc.selA : p.doc.cursor; p.doc.setSelection(a, dragFrom); }
        else p.doc.setCursor(dragFrom);
        if (onSelection) onSelection(); repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (sd.on) { specDrag(e); return; } if (pd.on) { pitchDrag(e); return; } if (wp.on) { wpPut(e); return; }
        if (frames() == 0) return; const size_t f = (size_t) juce::jlimit(0.0, (double) frames(), xToFrame((float) e.x));
        if (e.getDistanceFromDragStart() > 2) p.doc.setSelection(dragFrom, f); if (onSelection) onSelection(); repaint();
    }
    void mouseUp(const juce::MouseEvent& e) override { if (sd.on) specUp(e); else if (pd.on) pitchUp(); else if (wp.on) wpEnd(); }
    void mouseMove(const juce::MouseEvent& e) override
    {
        if (spectral && tool == Paste && p.doc.hasSpectralClip()) { ghost = true; ghostP = inBand(LSpec, e.position); repaint(); }
        if (layerAt((float) e.y) == LPitch) { const int ni = noteAt(inBand(LPitch, e.position)); if (ni != hoverNote) { hoverNote = ni; repaint(); } setMouseCursor(ni >= 0 ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::IBeamCursor); }
        else setMouseCursor(layerAt((float) e.y) == LSpec && tool != Mouse ? juce::MouseCursor::CrosshairCursor : (wpen ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor));
    }
    void mouseExit(const juce::MouseEvent&) override { if (ghost || hoverNote >= 0) { ghost = false; hoverNote = -1; repaint(); } }
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (frames() == 0) return; const int l = layerAt((float) e.y);
        if (l == LSpec && tool != Mouse) return;
        if (l == LPitch) { const int ni = noteAt(inBand(LPitch, e.position)); if (ni >= 0) { const auto& n = pitchInfo().notes[(size_t) ni]; const double dm = std::round((double) n.m) - (double) n.m; if (std::abs(dm) > 0.02) shiftNote(ni, dm); return; } }
        p.doc.setSelection(0, frames()); if (onSelection) onSelection(); repaint();
    }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (frames() == 0) return;
        // trackpad: a sideways swipe pans, an up or down swipe zooms; mouse wheel zooms, Shift and wheel pans
        if (e.mods.isShiftDown() || std::abs(w.deltaX) > std::abs(w.deltaY)) { const float d = w.deltaX != 0 && !e.mods.isShiftDown() ? w.deltaX : (w.deltaY != 0 ? w.deltaY : w.deltaX); vStart -= d * vLen * 0.5; clampView(); repaint(); }
        else zoom(w.deltaY > 0 ? 0.8 : 1.25, (float) e.x);
    }
    // pinch on a Mac trackpad
    void mouseMagnify(const juce::MouseEvent& e, float scale) override { if (frames() > 0 && scale > 0.f) zoom(1.0 / (double) scale, (float) e.x); }

    bool isMono() { if (monoVer != p.version() || monoN != frames()) { monoVer = p.version(); monoN = frames(); monoV = mc::isMonoAudio(p.doc.audio); } return monoV; }
    void paint(Graphics& g) override
    {
        const int rulerH = kRuler; const auto area = areaR(); g.fillAll(Colour(0xff060608));
        const auto& a = p.doc.audio; const size_t n = a.frames();
        if (n == 0) { g.setColour(kDim); g.setFont(sans(16.f)); g.drawFittedText("Empty. Press Record, or open a sound from the File menu.", getLocalBounds(), juce::Justification::centred, 1); return; }
        if (seen != p.version()) { seen = p.version(); if (vLen <= 0 || vStart + vLen > (double) n + 1) fitAll(); clampView(); specKey = {}; }
        if (vLen <= 0) fitAll();
        const int w = getWidth(); const auto v = vis(); const int f = frontLayer(); std::vector<int> order; for (int l : v) if (l != f) order.push_back(l); order.push_back(f);
        for (size_t k = 0; k < order.size(); ++k)
        {
            const int l = order[k]; const auto b = bandOf(l); juce::Graphics::ScopedSaveState ss(g); g.reduceClipRegion(b); g.setOrigin(b.getPosition()); const auto r = b.withZeroOrigin();
            const bool opaque = splitNow() || k == 0, behind = !splitNow() && l != f;
            if (splitNow() && k > 0) { g.setColour(kEdge); g.drawHorizontalLine(0, 0.f, (float) w); }
            if (l == LWave) paintWave(g, r, opaque); else if (l == LSpec) { paintSpectral(g, r, behind ? 0.6f : 1.f); paintSpecOverlay(g); } else paintPitch(g, r, opaque);
        }
        if (showHits) { g.setColour(kGold.withAlpha(0.45f)); for (size_t h : p.hits()) { const float x = frameToX((double) h); if (x >= 0 && x < (float) w) g.drawVerticalLine((int) x, 0.f, (float) area.getHeight()); } }
        if (p.doc.hasSelection()) { const float x0 = frameToX((double) p.doc.selA), x1 = frameToX((double) p.doc.selB); g.setColour(kBlue.withAlpha(0.16f)); g.fillRect(x0, 0.f, x1 - x0, (float) area.getHeight()); g.setColour(kBlue); g.drawVerticalLine((int) x0, 0.f, (float) area.getHeight()); g.drawVerticalLine((int) x1, 0.f, (float) area.getHeight()); }
        else { const float x = frameToX((double) p.doc.cursor); g.setColour(kBlue); g.drawVerticalLine((int) x, 0.f, (float) area.getHeight()); }
        if (p.isPlaying()) { const float x = frameToX(p.playPosFrames()); g.setColour(kGold); g.drawVerticalLine((int) x, 0.f, (float) area.getHeight()); }
        // ruler
        g.setColour(kPanel); g.fillRect(0, getHeight() - rulerH, w, rulerH); g.setColour(kEdge); g.drawHorizontalLine(getHeight() - rulerH, 0.f, (float) w);
        const double secs = vLen / a.sr; double step = 0.001; for (double s : { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0, 120.0, 300.0 }) { step = s; if (secs / s < 9) break; }
        g.setFont(mono(10.f)); const double t0 = vStart / a.sr;
        for (double t = std::ceil(t0 / step) * step; t < t0 + secs; t += step) { const float x = (float) ((t - t0) / secs * w); g.setColour(kDim); g.drawVerticalLine((int) x, (float) (getHeight() - rulerH), (float) (getHeight() - rulerH + 6)); g.drawText(timeText(t, step < 1.0), (int) x + 3, getHeight() - rulerH + 6, 80, 12, juce::Justification::left); }
        if (wpen && vLen / (double) std::max(1, w) > 4.0) { g.setColour(kPink); g.setFont(mono(11.f)); g.drawText("Pencil is on: zoom in until you can see single samples, then draw.", 8, 6, 520, 14, juce::Justification::left); }
    }
    void paintWave(Graphics& g, juce::Rectangle<int> r, bool opaque)
    {
        const auto& a = p.doc.audio; const size_t n = a.frames(); const int w = r.getWidth(); const bool mono1 = isMono(); const int lanes = mono1 ? 1 : 2, laneH = r.getHeight() / lanes;
        if (opaque) g.fillAll(Colour(0xff060608));
        for (int c = 0; c < lanes; ++c)
        {
            const int top = c * laneH; g.setColour(kEdge); if (c > 0) g.drawHorizontalLine(top, 0.f, (float) w); g.setColour(kDim); g.setFont(mono(10.f)); g.drawText(mono1 ? "L R" : (c == 0 ? "L" : "R"), 4, top + 2, 24, 12, juce::Justification::left);
            g.setColour(Colour(0xff1a2a30)); g.drawHorizontalLine(top + laneH / 2, 0.f, (float) w);
            g.setColour(kTeal); const auto& d = a.ch[c];
            for (int x = 0; x < w; ++x)
            {
                const size_t s0 = (size_t) juce::jlimit(0.0, (double) n, vStart + (double) x / w * vLen), s1 = std::min(n, std::max(s0 + 1, (size_t) juce::jlimit(0.0, (double) n, vStart + (double) (x + 1) / w * vLen)));
                const size_t step = std::max<size_t>(1, (s1 - s0) / 48); float mn = 0, mx = 0; for (size_t i = s0; i < s1; i += step) { mn = std::min(mn, d[i]); mx = std::max(mx, d[i]); }
                const float mid = top + laneH * 0.5f, amp = laneH * 0.46f; g.fillRect((float) x, mid - mx * amp, 1.f, std::max(1.f, (mx - mn) * amp));
            }
        }
    }
    // ---- waveform pencil: sets single samples under the pen ----
    struct WP { bool on = false, did = false; bool has = false; double f = 0, v = 0; int lane = -1; juce::uint32 lastFlush = 0; } wp;
    void wpValue(const juce::MouseEvent& e, double& f, double& v, int& lane) const
    {
        const auto b = bandOf(LWave); const bool m = monoV; const int lanes = m ? 1 : 2; const float lh = (float) b.getHeight() / (float) lanes, y = e.position.y - (float) b.getY();
        lane = m ? -1 : juce::jlimit(0, 1, (int) (y / lh)); const float mid = lh * (m ? 0.5f : ((float) lane + 0.5f)); f = xToFrame((float) e.x); v = juce::jlimit(-1.0, 1.0, (double) (-(y - mid) / (lh * 0.46f)));
    }
    void wpStart(const juce::MouseEvent& e) { p.stopPlay(); p.doc.beginEdit("Pencil (waveform)"); wp = WP(); wp.on = true; wpPut(e); }
    void wpPut(const juce::MouseEvent& e)
    {
        double f, v; int lane; wpValue(e, f, v, lane);
        if (wp.has && wp.lane == lane) mc::pencilWave(p.doc.audio, wp.f, wp.v, f, v, lane); else mc::pencilWave(p.doc.audio, f, v, f, v, lane);
        wp.has = true; wp.f = f; wp.v = v; wp.lane = lane; wp.did = true; const auto now = juce::Time::getMillisecondCounter(); if (now - wp.lastFlush > 50) { wp.lastFlush = now; p.changed(); } repaint();
    }
    void wpEnd() { const bool did = wp.did; wp.on = false; if (did) { p.changed(); if (onSelection) onSelection(); } else p.doc.cancelEdit(); repaint(); }
    // ---- pitch view: notes found in the sound, drag one up or down to retune it ----
    mc::NoteResult pitchData; int pitchVer = -1, pLo = 55, pHi = 67; bool pitchTooLong = false, monoV = true; int monoVer = -1; size_t monoN = 0; int hoverNote = -1;
    struct PDrag { bool on = false, moved = false; int note = -1; float y0 = 0; double dm = 0; } pd;
    static String noteName(double m) { static const char* NN[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; const int r = (int) std::lround(m); return String(NN[((r % 12) + 12) % 12]) + String((int) std::floor(r / 12.0) - 1); }
    const mc::NoteResult& pitchInfo()
    {
        if (pitchVer != p.version()) {
            pitchVer = p.version(); pitchData = {}; pitchTooLong = false;
            if (frames() > 4096) { if ((double) frames() > p.doc.audio.sr * 120.0) pitchTooLong = true; else pitchData = mc::detectNotes(p.doc.audio); }
            double lo = 1e9, hi = -1e9; for (auto& nn : pitchData.notes) { lo = std::min(lo, (double) nn.m); hi = std::max(hi, (double) nn.m); } if (lo > hi) { lo = 55; hi = 67; }
            int l = (int) std::floor(lo) - 3, h = (int) std::ceil(hi) + 3; while (h - l < 23) { l--; h++; } pLo = juce::jlimit(12, 100, l); pHi = juce::jlimit(pLo + 12, 120, h);
        }
        return pitchData;
    }
    float pRow(int H) const { return (float) H / (float) (pHi - pLo + 1); }
    float pY(double m, int H) const { return (float) H - (float) (m - pLo + 0.5) * pRow(H); }
    int noteAt(juce::Point<float> q)
    {
        const auto& P = pitchInfo(); const int H = bandOf(LPitch).getHeight(); const double f = xToFrame(q.x); const float rh = pRow(H);
        for (size_t i = 0; i < P.notes.size(); ++i) { const auto& n = P.notes[i]; if (f >= (double) n.t0 && f <= (double) n.t1 && std::abs(q.y - pY(n.m, H)) <= rh * 0.6f + 2.f) return (int) i; }
        return -1;
    }
    void pitchDrag(const juce::MouseEvent& e)
    {
        const int H = bandOf(LPitch).getHeight(); const auto& n = pitchInfo().notes[(size_t) pd.note]; double dm = -(double) (e.position.y - pd.y0) / (double) pRow(H);
        if (std::abs(e.position.y - pd.y0) > 3.f) pd.moved = true; if (snapNote == 1) dm = std::round((double) n.m + dm) - (double) n.m; pd.dm = dm; repaint();
    }
    void pitchUp() { const bool go = pd.moved && std::abs(pd.dm) > 0.02; const int ni = pd.note; const double dm = pd.dm; pd.on = false; if (go) shiftNote(ni, dm); repaint(); }
    void shiftNote(int ni, double dm)
    {
        const auto n = pitchInfo().notes[(size_t) ni]; p.stopPlay(); const bool keep = p.warp.keepFormants; const double fs = p.warp.fshift;
        p.doc.transform(("Pitch " + noteName(n.m) + (dm > 0 ? " +" : " ") + String(dm, 2) + " st").toStdString(), [&](mc::Audio& a) { mc::pitchShiftRange(a, (size_t) n.t0, (size_t) n.t1, dm, keep, fs); }); p.changed(); if (onSelection) onSelection();
    }
    // Moves every note (of the selection, if there is one) to the nearest semitone. Returns how many were moved, or -1 when there are no notes.
    int snapAllNotes()
    {
        auto notes = pitchInfo().notes; if (notes.empty()) return -1; if (p.doc.hasSelection()) notes.erase(std::remove_if(notes.begin(), notes.end(), [this](const mc::Note& n) { return !((size_t) n.t1 > p.doc.selA && (size_t) n.t0 < p.doc.selB); }), notes.end());
        std::vector<std::pair<mc::Note, double>> todo; for (auto& n : notes) { const double dm = std::round((double) n.m) - (double) n.m; if (std::abs(dm) > 0.02) todo.push_back({ n, dm }); } if (todo.empty()) return 0;
        p.stopPlay(); const bool keep = p.warp.keepFormants; const double fs = p.warp.fshift;
        p.doc.transform(("Snap " + String((int) todo.size()) + " notes to semitones").toStdString(), [&](mc::Audio& a) { for (auto& t : todo) mc::pitchShiftRange(a, (size_t) t.first.t0, (size_t) t.first.t1, t.second, keep, fs); }); p.changed(); return (int) todo.size();
    }
    void paintPitch(Graphics& g, juce::Rectangle<int> r, bool opaque)
    {
        const int w = r.getWidth(), H = r.getHeight(); const auto& P = pitchInfo(); const float rh = pRow(H); if (opaque) g.fillAll(Colour(0xff0a0a0c));
        for (int m = pLo; m <= pHi; ++m)
        {
            const int pc = m % 12; const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; const float y = pY(m, H) - rh / 2;
            g.setColour(black ? (opaque ? Colour(0xff0d0e12) : Colour(0x47000000)) : (opaque ? Colour(0xff13151a) : Colour(0x09ffffff))); g.fillRect(0.f, y, (float) w, rh); if (pc == 0) { g.setColour(Colour(0x2affffff)); g.fillRect(0.f, y + rh - 1.f, (float) w, 1.f); }
        }
        g.setFont(mono(10.f)); for (int m = pLo; m <= pHi; ++m) if (m % 12 == 0 || rh >= 13.f) { g.setColour(m % 12 == 0 ? Colour(0xffc8ccd8) : Colour(0xff6b6f7c)); g.drawText(noteName(m), 4, (int) pY(m, H) - 6, 40, 12, juce::Justification::left); }
        if (pitchTooLong) { g.setColour(kDim); g.setFont(sans(14.f)); g.drawFittedText("The pitch view looks at up to 2 minutes. Crop to a part of the sound to see its notes.", r, juce::Justification::centred, 2); return; }
        if (P.notes.empty() && frames() > 4096) { g.setColour(kDim); g.setFont(sans(13.f)); g.drawText("No notes found. The pitch view needs a clear single voice or instrument.", 50, 6, w - 60, 16, juce::Justification::left); }
        const auto& tr = P.track; { juce::Path path; bool pen = false; double pt = -1e18; g.setColour(kGold.withAlpha(0.8f));
            for (size_t i = 0; i < tr.midi.size(); ++i) { const float m = tr.midi[i]; if (std::isnan(m) || m < pLo - 2 || m > pHi + 2) { pen = false; continue; } const float x = frameToX((double) tr.t[i]), y = pY(m, H); if (!pen || (double) tr.t[i] - pt > tr.hop * 2.5) path.startNewSubPath(x, y); else path.lineTo(x, y); pen = true; pt = (double) tr.t[i]; }
            g.strokePath(path, juce::PathStrokeType(1.2f)); }
        for (size_t i = 0; i < P.notes.size(); ++i)
        {
            const auto& n = P.notes[i]; const float x0 = frameToX((double) n.t0), x1 = frameToX((double) n.t1); if (x1 < 0 || x0 > (float) w) continue; const float y = pY(n.m, H), bw = std::max(3.f, x1 - x0), bh = rh * 0.78f; const bool hot = (int) i == hoverNote || (pd.on && pd.note == (int) i);
            g.setColour(hot ? Colour(0xd9ff5aa8) : Colour(0xa8ff1383)); g.fillRect(x0, y - bh / 2, bw, bh); g.setColour(Colour(0xffff7ab8)); g.drawRect(x0, y - bh / 2, bw, bh, 1.f);
            if (bw > 38 && bh > 9) { const int ct = (int) std::lround((n.m - std::round(n.m)) * 100.0); g.setColour(juce::Colours::white); g.setFont(mono(10.f)); g.drawText(noteName(n.m) + (ct ? String(" ") + (ct > 0 ? "+" : "") + String(ct) : String()), (int) x0 + 4, (int) y - 6, 120, 12, juce::Justification::left); }
        }
        if (pd.on && pd.moved) { const auto& n = P.notes[(size_t) pd.note]; const float x0 = frameToX((double) n.t0), x1 = frameToX((double) n.t1), y = pY(n.m + pd.dm, H), bh = rh * 0.78f; g.setColour(kTeal); g.drawRect(x0, y - bh / 2, std::max(3.f, x1 - x0), bh, 1.5f);
            g.setFont(mono(10.f)); g.drawText(noteName(n.m) + " to " + noteName(n.m + pd.dm) + "  (" + (pd.dm > 0 ? "+" : "") + String(pd.dm, 2) + " st)", (int) x0 + 4, (int) (y - bh / 2) - 14, 260, 12, juce::Justification::left); }
    }
    void paintSpectral(Graphics& g, juce::Rectangle<int> area, float alpha)
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
    int specH() const { return std::max(2, bandOf(LSpec).getHeight()); }
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
    juce::Point<float> sp(const juce::MouseEvent& e) const { return inBand(LSpec, e.position); }
    void specDown(const juce::MouseEvent& e)
    {
        if (frames() == 0) return; sd = Drag(); sd.on = true; sd.p0 = sd.p = sp(e); sd.f0 = frAtX(sp(e).x);
        if (tool == Eraser) { p.stopPlay(); p.doc.beginEdit("Spectral eraser"); const auto r = brushRect(sp(e)); sd.rects = { r }; sd.pending = { r }; flushEraser(true); }
        if (tool == Pencil) { p.stopPlay(); p.doc.beginEdit("Pencil (spectral)"); sd.pf = frAtX(sp(e).x); sd.phz = hzAtY(sp(e).y); sd.pph = 0; sd.pn = 0; }
        repaint();
    }
    void specDrag(const juce::MouseEvent& e)
    {
        sd.p = sp(e);
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
        const auto d = sd; sd.on = false; const auto q = sp(e); const bool tiny = std::abs(q.x - d.p0.x) < 4 && std::abs(q.y - d.p0.y) < 4; const auto box = boxOf(d.p0, q);
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
        hasScope = p.doc.hasSelection(); hasMix = !info.changesLength;
        if (hasScope) { scopeCb.addItem("Selection", 1); scopeCb.addItem("Everything", 2); scopeCb.setSelectedId(1, juce::dontSendNotification); scopeCb.setTooltip("Where it works: only the selection, or the whole sound."); addAndMakeVisible(scopeCb); scopeL.setText("Scope", juce::dontSendNotification); scopeL.setFont(sans(13.f)); scopeL.setColour(juce::Label::textColourId, kText); addAndMakeVisible(scopeL); }
        if (hasMix) { mixS.setSliderStyle(juce::Slider::LinearHorizontal); mixS.setTextBoxStyle(juce::Slider::TextBoxRight, false, 78, 20); mixS.setRange(0, 100, 1); mixS.setValue(100, juce::dontSendNotification); mixS.setTextValueSuffix(" %"); mixS.setDoubleClickReturnValue(true, 100); mixS.setTooltip("How much of the effect is mixed with the original. 100 percent is all effect."); addAndMakeVisible(mixS); mixL.setText("Mix", juce::dontSendNotification); mixL.setFont(sans(13.f)); mixL.setColour(juce::Label::textColourId, kText); addAndMakeVisible(mixL); }
        apply.setButtonText(p.doc.hasSelection() ? "Apply" : "Apply to whole sound"); addAndMakeVisible(apply); addAndMakeVisible(close);
        apply.onClick = [this] { p.stopPlay(); const bool all = hasScope && scopeCb.getSelectedId() == 2; if (p.doc.apply(op, vals, all, hasMix ? (float) (mixS.getValue() / 100.0) : 1.f)) p.changed(); if (onDone) onDone(); };
        close.onClick = [this] { if (onDone) onDone(); };
        setSize(440, 96 + 34 * (std::max(1, info.nParams) + (hasScope ? 1 : 0) + (hasMix ? 1 : 0)));
    }
    void paint(Graphics& g) override
    {
        g.fillAll(kPanel); const auto& info = mc::kOps[op]; g.setColour(kPink); g.setFont(sans(16.f, true)); g.drawText(String(info.name).toUpperCase(), 14, 8, 300, 22, juce::Justification::left);
        g.setColour(kDim); g.setFont(sans(12.f)); g.drawFittedText(info.tip, 14, 30, getWidth() - 28, 30, juce::Justification::topLeft, 2);
    }
    void resized() override
    {
        int y = 66; for (auto* r : rows) { r->name.setBounds(14, y, 100, 26); r->slider.setBounds(116, y, getWidth() - 130, 26); r->combo.setBounds(116, y, 160, 26); y += 34; }
        if (hasScope) { scopeL.setBounds(14, y, 100, 26); scopeCb.setBounds(116, y, 160, 26); y += 34; } if (hasMix) { mixL.setBounds(14, y, 100, 26); mixS.setBounds(116, y, getWidth() - 130, 26); y += 34; }
        apply.setBounds(14, getHeight() - 40, 180, 30); close.setBounds(getWidth() - 94, getHeight() - 40, 80, 30);
    }
    struct Row { juce::Label name; juce::Slider slider; juce::ComboBox combo; };
    bool hasScope = false, hasMix = false; juce::ComboBox scopeCb; juce::Slider mixS; juce::Label scopeL, mixL;
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


// ---------------- live input strip: the last 3 seconds, always listening ----------------
struct LiveStrip : juce::Component, juce::SettableTooltipClient
{
    MeowProcessor& p; std::function<int()> startMode; std::vector<float> mn, mx; std::vector<char> ok;
    explicit LiveStrip(MeowProcessor& pr) : p(pr) { setTooltip("Input, always listening: the last 3 seconds. The pink band is the pre-roll that a recording would include. The gold lines are the threshold of On input."); }
    void paint(Graphics& g) override
    {
        const int w = getWidth(), h = getHeight(); g.fillAll(Colour(0xff0a0a0c)); g.setColour(Colour(0xff1d2026)); g.drawHorizontalLine(h / 2, 0.f, (float) w);
        const float pre = juce::jlimit(0.f, 1.f, p.apvts.getRawParameterValue("preroll")->load() / 1000.f / 3.f) * (float) w; g.setColour(kPink.withAlpha(0.13f)); g.fillRect((float) w - pre, 0.f, pre, (float) h);
        mn.assign((size_t) w, 0.f); mx.assign((size_t) w, 0.f); std::unique_ptr<bool[]> okb(new bool[(size_t) w]); p.livePeaks(3.0, w, mn.data(), mx.data(), okb.get());
        g.setColour(p.isRecording() && !p.isArmedOnly() ? kPink : Colour(0xff6cc9f5)); const float mid = (float) h * 0.5f, amp = (float) h * 0.5f * 0.95f;
        for (int x = 0; x < w; ++x) if (okb[(size_t) x]) g.fillRect((float) x, mid - mx[(size_t) x] * amp, 1.f, std::max(1.f, (mx[(size_t) x] - mn[(size_t) x]) * amp));
        if (startMode && startMode() == MeowProcessor::RecOnInput) { const float t = std::pow(10.f, p.apvts.getRawParameterValue("thresh")->load() / 20.f) * amp; g.setColour(kGold.withAlpha(0.65f)); g.fillRect(0.f, mid - t, (float) w, 1.f); g.fillRect(0.f, mid + t, (float) w, 1.f); }
    }
};

// ---------------- warp panel: tempo and pitch of what plays; Bake in writes it into the sound ----------------
struct WarpPanel : juce::Component, private juce::Timer
{
    WarpPanel(MeowProcessor& pr, std::function<void()> changedCb, std::function<void()> closeCb) : p(pr), onChanged(std::move(changedCb)), onClose(std::move(closeCb))
    {
        auto knob = [&](juce::Slider& k, juce::Label& l, const char* nm, double lo, double hi, double step, const char* suffix, const char* tip) {
            k.setSliderStyle(juce::Slider::RotaryVerticalDrag); k.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16); k.setRange(lo, hi, step); k.setTextValueSuffix(suffix); k.setTooltip(tip);
            k.setColour(juce::Slider::textBoxTextColourId, kTeal); k.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); addAndMakeVisible(k);
            l.setText(nm, juce::dontSendNotification); l.setFont(sans(11.f, true)); l.setColour(juce::Label::textColourId, kDim); l.setJustificationType(juce::Justification::centred); addAndMakeVisible(l); };
        knob(sBpm, lBpm, "SAMPLE TEMPO", 40, 240, 0.1, " bpm", "The tempo of this sound. Meowcorder listens for it. Type a value if it guessed wrong.");
        knob(hBpm, lH, "HOST TEMPO", 40, 240, 0.1, " bpm", "The tempo of your project. In the plugin this follows your DAW. It is the same as the Tempo knob.");
        knob(sSemi, lSemi, "TRANSPOSE", -24, 24, 1, " st", "Changes the pitch in semitones. 12 is an octave.");
        knob(sCent, lCent, "FINE TUNE", -100, 100, 1, " ct", "Changes the pitch in hundredths of a semitone.");
        knob(sForm, lForm, "FORMANT", -12, 12, 0.1, " st", "Moves the voice character (the formants) up or down by itself. Works when Formants is on Keep.");
        hAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "bpm", hBpm);
        sBpm.onValueChange = [this] { if (!busy) { p.warp.bpm = std::round(sBpm.getValue() * 10.0) / 10.0; touched(); } };
        sSemi.onValueChange = [this] { if (!busy) { p.warp.semis = std::round(sSemi.getValue()); touched(); } }; sCent.onValueChange = [this] { if (!busy) { p.warp.cents = std::round(sCent.getValue()); touched(); } };
        sForm.onValueChange = [this] { if (!busy) { p.warp.fshift = sForm.getValue(); touched(); } }; hBpm.onValueChange = [this] { if (!busy) touched(); };
        auto seg = [&](juce::OwnedArray<Pill>& arr, std::initializer_list<const char*> names, std::function<void(int)> f) { int i = 0; for (auto* n : names) { auto* b = arr.add(new Pill(n, kPink)); const int k = i++; b->onClick = [this, f, k] { f(k); touched(); sync(); }; addAndMakeVisible(b); } };
        seg(syncB, { "Free", "Sync" }, [this](int k) { p.warp.sync = k == 1; });
        seg(modeB, { "Stretch", "Repitch", "Beats", "Texture" }, [this](int k) { p.warp.mode = k; });
        seg(formB, { "Follow", "Keep" }, [this](int k) { p.warp.keepFormants = k == 1; });
        seg(scopeB, { "Selection", "Everything" }, [this](int k) { p.warp.all = k == 1; });
        syncB[0]->setTooltip("Free: the sound plays at its own speed."); syncB[1]->setTooltip("Sync: the sound is stretched to the tempo of your project, using the sample tempo.");
        modeB[0]->setTooltip("Keeps the pitch, changes the length. Best for most things."); modeB[1]->setTooltip("Plays faster or slower, so the pitch moves with the speed, like a record."); modeB[2]->setTooltip("Keeps the hits sharp and moves the gaps. Best for drums."); modeB[3]->setTooltip("Smooth, washed out stretch. Best for pads and noise.");
        formB[0]->setTooltip("The voice character moves along with the pitch."); formB[1]->setTooltip("The voice character stays where it was while the pitch changes."); scopeB[0]->setTooltip("Bake in works on the selection, or on everything when nothing is selected."); scopeB[1]->setTooltip("Bake in works on the whole sound.");
        for (auto* b : { &prev, &bake, &reset, &detect, &close }) addAndMakeVisible(*b);
        prev.setTooltip("Hear the sound with the warp, from the selection or the whole sound."); bake.setTooltip("Writes the warp into the sound. One undo step."); reset.setTooltip("Back to free, no transposing."); detect.setTooltip("Listen for the tempo again and forget a typed one."); close.setTooltip("Close this panel.");
        prev.onClick = [this] { p.stopPlay(); const bool all = p.warp.all || !p.doc.hasSelection(); const size_t a = all ? 0 : p.doc.selA, b = all ? p.doc.audio.frames() : p.doc.selB; p.playRange(a, b, true); };
        bake.onClick = [this] { if (!p.bakeWarp(false)) info = "Nothing to bake in yet. Turn on Sync, or change the pitch."; if (onChanged) onChanged(); sync(); };
        reset.onClick = [this] { p.resetWarp(); touched(); sync(); }; detect.onClick = [this] { p.warp.bpm = 0; touched(); sync(); }; close.onClick = [this] { if (onClose) onClose(); };
        setSize(560, 330); sync(); startTimerHz(4);
    }
    ~WarpPanel() override { stopTimer(); }
    void touched() { p.stopPlay(); if (onChanged) onChanged(); updateInfo(); repaint(); }
    void timerCallback() override { if (!sBpm.isMouseButtonDown() && !sBpm.hasKeyboardFocus(true)) { busy = true; sBpm.setValue(p.sampleBpm() > 0 ? p.sampleBpm() : 120.0, juce::dontSendNotification); busy = false; } updateInfo(); }
    void sync()
    {
        busy = true; sBpm.setValue(p.sampleBpm() > 0 ? p.sampleBpm() : 120.0, juce::dontSendNotification); sSemi.setValue(p.warp.semis, juce::dontSendNotification); sCent.setValue(p.warp.cents, juce::dontSendNotification); sForm.setValue(p.warp.fshift, juce::dontSendNotification); busy = false;
        for (int i = 0; i < 2; ++i) { syncB[i]->setToggleState((i == 1) == p.warp.sync, juce::dontSendNotification); formB[i]->setToggleState((i == 1) == p.warp.keepFormants, juce::dontSendNotification); scopeB[i]->setToggleState((i == 1) == p.warp.all, juce::dontSendNotification); }
        for (int i = 0; i < 4; ++i) modeB[i]->setToggleState(i == p.warp.mode, juce::dontSendNotification);
        sForm.setAlpha(p.warp.keepFormants ? 1.f : 0.35f); updateInfo(); repaint();
    }
    void updateInfo()
    {
        const double sb = p.sampleBpm(), host = p.currentBpm(), r = p.warp.sync && sb > 0 ? host / sb : 1.0, st = p.warp.semis + p.warp.cents / 100.0; const char* mn[] = { "Stretch", "Repitch", "Beats", "Texture" };
        String t = p.warp.sync ? (sb > 0 ? "Sync: the sample (" + String(sb, 1) + " bpm) plays at " + String(host, 1) + " bpm, speed x" + String(r, 3) + ", " + mn[p.warp.mode] + ". " : String("No tempo found in this sound: type the sample tempo. ")) : String("Free: the sound plays at its own speed. ");
        t += st != 0 ? "Pitch " + String(st > 0 ? "+" : "") + String(st, 2) + " semitones." : String("Pitch unchanged."); if (info.isNotEmpty()) { t = info + "  " + t; info = {}; }
        if (t != shown) { shown = t; repaint(); }
    }
    void paint(Graphics& g) override
    {
        g.fillAll(kPanel); g.setColour(kPink); g.setFont(sans(16.f, true)); g.drawText("WARP, TEMPO AND PITCH", 14, 8, 360, 22, juce::Justification::left);
        g.setColour(kDim); g.setFont(sans(12.f)); g.drawFittedText("Plays the sound at the tempo of your project and with another pitch, without touching the sound until you press Bake in.", 14, 30, getWidth() - 28, 30, juce::Justification::topLeft, 2);
        g.setFont(sans(10.5f, true)); g.drawText("TEMPO", 14, 166, 60, 12, juce::Justification::left); g.drawText("MODE", 14, 196, 60, 12, juce::Justification::left); g.drawText("FORMANTS", 262, 166, 80, 12, juce::Justification::left); g.drawText("BAKE IN ON", 262, 196, 90, 12, juce::Justification::left);
        g.setColour(kTeal); g.setFont(sans(12.f)); g.drawFittedText(shown, 14, 232, getWidth() - 28, 34, juce::Justification::topLeft, 2);
    }
    void resized() override
    {
        Slider* ks[] = { &sBpm, &hBpm, &sSemi, &sCent, &sForm }; juce::Label* ls[] = { &lBpm, &lH, &lSemi, &lCent, &lForm };
        for (int i = 0; i < 5; ++i) { ks[i]->setBounds(14 + i * 106, 80, 100, 74); ls[i]->setBounds(14 + i * 106, 64, 100, 14); }
        syncB[0]->setBounds(74, 162, 60, 24); syncB[1]->setBounds(138, 162, 60, 24);
        { int x = 74; for (auto* b : modeB) { b->setBounds(x, 192, 70, 24); x += 74; } }
        formB[0]->setBounds(344, 162, 70, 24); formB[1]->setBounds(418, 162, 70, 24); scopeB[0]->setBounds(354, 192, 84, 24); scopeB[1]->setBounds(442, 192, 90, 24);
        prev.setBounds(14, getHeight() - 42, 90, 30); bake.setBounds(110, getHeight() - 42, 90, 30); reset.setBounds(206, getHeight() - 42, 80, 30); detect.setBounds(292, getHeight() - 42, 130, 30); close.setBounds(getWidth() - 94, getHeight() - 42, 80, 30);
    }
    using Slider = juce::Slider;
    MeowProcessor& p; std::function<void()> onChanged, onClose; bool busy = false; String info, shown;
    juce::Slider sBpm, hBpm, sSemi, sCent, sForm; juce::Label lBpm, lH, lSemi, lCent, lForm; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hAtt;
    juce::OwnedArray<Pill> syncB, modeB, formB, scopeB; Pill prev { "Preview", kTeal }, bake { "Bake in", kGold }, reset { "Reset", kDim }, detect { "Detect tempo again", kDim }, close { "Close", kDim };
};

struct MeowEditor::Impl
{
    Impl(MeowEditor& o, MeowProcessor& pr) : owner(o), p(pr), gate(o), display(pr), overview(pr, display), inM("IN"), outM("OUT"), drag(pr, &o)
    {
        o.setLookAndFeel(&laf); o.addAndMakeVisible(content); auto& c = content;
        for (auto* b : { &mFile, &mEdit, &mSelect, &mView, &mPlay, &mRecord, &mProcess, &mHelp, &guide, &about, &startNow, &startIn, &startCount, &takeB, &viewW, &viewS, &viewP, &layoutB, &frontB, &snapAllB, &fFollow, &fKeep, &chCh, &chSr, &chBit, &chUnit, &mSpectral, &mWarp, &chTempo, &chSync }) c.addAndMakeVisible(*b);
        c.addAndMakeVisible(vComb); c.addAndMakeVisible(vPen); c.addAndMakeVisible(dragCb);
        buildSpecTools();
        for (auto* b : { &loop, &play, &stop, &rec, &monitor }) c.addAndMakeVisible(*b); c.addAndMakeVisible(recOpt);
        c.addAndMakeVisible(display); c.addAndMakeVisible(overview); c.addAndMakeVisible(live); live.startMode = [this] { return recMode; }; c.addAndMakeVisible(inM); c.addAndMakeVisible(outM); c.addAndMakeVisible(drag);
        for (auto* l : { &readout, &timeL, &recHintL, &opHint }) c.addAndMakeVisible(*l);
        readout.setFont(mono(13.f)); readout.setColour(juce::Label::textColourId, kDim); recHintL.setFont(mono(12.f)); recHintL.setColour(juce::Label::textColourId, kPink);
        opHint.setFont(sans(12.f)); opHint.setColour(juce::Label::textColourId, kDim); opHint.setMinimumHorizontalScale(1.f); opHint.setJustificationType(juce::Justification::topLeft);
        timeL.setFont(mono(22.f, true)); timeL.setColour(juce::Label::textColourId, kText); timeL.setJustificationType(juce::Justification::centredLeft);
        auto knob = [&](juce::Slider& s, const char* id, const char* tip, const String& suffix) { s.setSliderStyle(juce::Slider::RotaryVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 14); s.setTooltip(tip); s.setTextValueSuffix(suffix);
            s.setColour(juce::Slider::textBoxTextColourId, kTeal); s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); c.addAndMakeVisible(s); atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, id, s)); };
        knob(thr, "thresh", "Level that starts a recording when the start is set to On input.", " dB"); knob(bpm, "bpm", "Tempo of the count-in click. The host tempo is used when the host gives one.", " bpm"); knob(pre, "preroll", "Sound from just before the start that is kept in the take.", " ms");
        loopA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "loop", loop); monA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "monitor", monitor);
        loop.setClickingTogglesState(true); monitor.setClickingTogglesState(true);
        viewW.setClickingTogglesState(false); viewS.setClickingTogglesState(false); viewP.setClickingTogglesState(false); viewW.setToggleState(true, juce::dontSendNotification);
        chCh.onClick = [this] { menu(chCh, channelMenu()); }; chSr.onClick = [this] { menu(chSr, rateMenu()); }; chBit.onClick = [this] { menu(chBit, bitMenu()); }; chUnit.onClick = [this] { menu(chUnit, unitMenu()); };
        chCh.setTooltip("Channels. Click to mix to mono, swap sides or use mid and side."); chSr.setTooltip("Sample rate. Click to convert the sound to another rate."); chBit.setTooltip("Export bit depth. Click to choose."); for (auto* c2 : { &chCh, &chSr, &chBit, &chUnit }) c2->raw = true;
        chUnit.setTooltip("Time line unit. Click to choose.");
        viewW.onClick = [this] { display.toggleLayer(Display::LWave); }; viewS.onClick = [this] { display.toggleLayer(Display::LSpec); }; viewP.onClick = [this] { display.toggleLayer(Display::LPitch); };
        vComb.onClick = [this] { display.setCombine(!display.combine); }; vPen.onClick = [this] { toggleWPen(); };
        layoutB.onClick = [this] { display.split = !display.split; display.layersChanged(); };
        frontB.onClick = [this] { const auto v = display.vis(); const int f = display.frontLayer(); size_t i = 0; for (size_t k = 0; k < v.size(); ++k) if (v[k] == f) i = k; display.front = v[(i + 1) % v.size()]; display.layersChanged(); };
        snapAllB.onClick = [this] { const int n = display.snapAllNotes(); status = n < 0 ? "No notes found. The pitch view needs a clear single voice or instrument." : (n == 0 ? "All notes are already on a semitone." : "Moved " + String(n) + " notes to the nearest semitone."); statusAge = 0; };
        fFollow.onClick = [this] { p.warp.keepFormants = false; syncLayerUi(); }; fKeep.onClick = [this] { p.warp.keepFormants = true; syncLayerUi(); };
        dragCb.addItem("Drag: free", 1); dragCb.addItem("Drag: to a semitone", 2); dragCb.setSelectedId(1, juce::dontSendNotification); dragCb.onChange = [this] { display.snapNote = dragCb.getSelectedId() - 1; };
        snapAllB.setTooltip("Moves every note to the nearest exact semitone. Like a tuner for the whole sound, or for the selection."); fFollow.setTooltip("The voice character moves with the pitch, like speeding a tape."); fKeep.setTooltip("Keeps the voice character (the formants) while the pitch changes. Best for voices and instruments."); dragCb.setTooltip("How a dragged note lands: anywhere, or on a whole semitone.");
        layoutB.setTooltip("Overlay: the views sit on top of each other. Split: one above the other."); frontB.setTooltip("Which view is in front and gets the mouse (click to switch)."); vComb.setTooltip("Show the views together (Alt+B)."); vPen.setTooltip("Waveform pencil: draw the wave by hand. Zoom in until you can see single samples, then drag over the wave.");
        viewP.setTooltip("The pitch view: the notes in the sound as a piano roll. Drag a note to retune it.");
        display.onLayers = [this] { syncLayerUi(); }; for (auto* b : { &snapAllB, &fFollow, &fKeep, &layoutB, &frontB }) b->raw = true; snapAllB.raw = false;
        loop.setTooltip("Repeat playback."); play.setTooltip("Play the selection, or from the cursor (Space)."); stop.setTooltip("Stop (Space or Esc)."); rec.setTooltip("Record (R). Press again to stop and keep the take."); recOpt.setTooltip("Record options: start mode and what a take does.");
        monitor.setTooltip("Hear the input while recording.");
        startNow.setTooltip("Recording starts the moment you press Record."); startIn.setTooltip("Press Record to arm. Recording starts when the incoming sound passes the threshold; the pre-roll is kept."); startCount.setTooltip("One bar of click (4/4) at the tempo, then recording starts.");
        takeB.setTooltip("What a new recording does: replace the sound, append at the end, punch over the selection or insert at the cursor. Click to change.");
        startNow.onClick = [this] { recMode = MeowProcessor::RecNow; }; startIn.onClick = [this] { recMode = MeowProcessor::RecOnInput; }; startCount.onClick = [this] { recMode = MeowProcessor::RecCountIn; };
        takeB.onClick = [this] { juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter("take")); const int cur = tp != nullptr ? tp->getIndex() : 0;
            if (tp != nullptr) for (int i = 0; i < tp->choices.size(); ++i) m.addItem(tp->choices[i], true, i == cur, [tp, i] { *tp = i; }); m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&takeB)); };
        guide.setTooltip("Open the manual."); about.setTooltip("Free and open source. Opens the Ko-fi page.");
        mFile.onClick = [this] { menu(mFile, fileMenu()); }; mEdit.onClick = [this] { menu(mEdit, editMenu()); }; mSelect.onClick = [this] { menu(mSelect, selectMenu()); }; mView.onClick = [this] { menu(mView, viewMenu()); };
        mPlay.onClick = [this] { menu(mPlay, playMenu()); }; mRecord.onClick = [this] { menu(mRecord, recordMenu()); }; mProcess.onClick = [this] { menu(mProcess, processMenu()); }; mSpectral.onClick = [this] { menu(mSpectral, spectralMenu()); }; mWarp.onClick = [this] { menu(mWarp, warpMenu()); };
        chTempo.onClick = [this] { menu(chTempo, tempoMenu()); }; chSync.onClick = [this] { p.stopPlay(); p.warp.sync = !p.warp.sync; updateChips(); refreshInfo(); };
        chTempo.setTooltip("Tempo of this sound. Click to use the detected one, double or halve it."); chSync.setTooltip("Free: plays at its own speed. Sync: plays at the host tempo using the sample tempo (click to switch)."); chTempo.raw = chSync.raw = true; mHelp.onClick = [this] { menu(mHelp, helpMenu()); };
        rec.onClick = [this] { toggleRecord(); }; stop.onClick = [this] { doStop(); }; play.onClick = [this] { doPlay(); }; recOpt.onClick = [this] { menu(recOpt, recordMenu()); };
        guide.onClick = [] { openManual(); }; about.onClick = [] { openAbout(); };
        rec.tint = kRed; play.tint = kGold; loop.tint = kGold; monitor.tint = kTeal; stop.tint = kText;
        buildTools(); buildOps();
        display.onSelection = [this] { refreshInfo(); };
        tip = std::make_unique<juce::TooltipWindow>(&o, 500);
        constrainer.setFixedAspectRatio((double) kBaseW / kBaseH); constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
        o.setConstrainer(&constrainer); o.setResizable(true, true); o.setWantsKeyboardFocus(true); o.setSize(kBaseW, kBaseH);
        syncLayerUi();
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
        toolHintL.setFont(sans(11.f)); toolHintL.setColour(juce::Label::textColourId, kDim); toolHintL.setMinimumHorizontalScale(1.f); toolHintL.setJustificationType(juce::Justification::topLeft); c.addChildComponent(toolHintL);
        display.onHint = [this](const String& t) { toolHintL.setText(t, juce::dontSendNotification); };
        display.onToolChanged = [this] { for (int i = 0; i < sTools.size(); ++i) sTools[i]->setToggleState(i == display.tool, juce::dontSendNotification); };
        display.setTool(Display::Mouse);
    }
    bool toolRow = false;
    void showSpecTools(bool on) { for (auto* b : sTools) b->setVisible(on); for (auto* cb : { &ampCb, &penCb, &brushCb }) cb->setVisible(on); toolHintL.setVisible(on); }
    juce::PopupMenu spectralMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0, reg = display.hasReg, clip = p.doc.hasSpectralClip();
        m.addItem("Show the spectral view", true, specShown(), [this] { display.toggleLayer(Display::LSpec); }); m.addSeparator();
        const char* nm[] = { "Mouse", "Selector", "Scissors", "Cutter", "Eraser", "Copy", "Paste", "Amp", "Pencil" }; juce::PopupMenu t;
        for (int i = 0; i < 9; ++i) { juce::PopupMenu::Item it(nm[i]); it.setTicked(display.tool == i); it.shortcutKeyDescription = "Shift+" + String(i + 1); it.setAction([this, i] { if (!specShown()) display.toggleLayer(Display::LSpec); display.setTool(i); }); t.addItem(std::move(it)); }
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
        add("grab", "Grab the last 3 seconds of input into the sound, even if you were not recording. What Take does is set next to the transport.", false, [this] { p.stopPlay(); if (p.grabLast(3.0)) display.fitAll(); else { status = "Nothing heard yet. Turn on the input and try again."; statusAge = 0; } }, {});
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
    void setSpectral(bool s) { display.showOnly(s ? Display::LSpec : Display::LWave); }
    void toggleWPen() { display.wpen = !display.wpen; if (display.wpen && !display.lay[Display::LWave]) display.toggleLayer(Display::LWave); vPen.setToggleState(display.wpen, juce::dontSendNotification); display.repaint(); if (display.wpen) { status = "Waveform pencil on. Zoom in until you can see single samples, then drag over the wave."; statusAge = 0; } }
    bool specShown() const { const auto v = display.vis(); return std::find(v.begin(), v.end(), (int) Display::LSpec) != v.end(); }
    bool pitchShown() const { const auto v = display.vis(); return std::find(v.begin(), v.end(), (int) Display::LPitch) != v.end(); }
    void syncLayerUi()
    {
        viewW.setToggleState(display.lay[0] && (display.combine || display.front == 0), juce::dontSendNotification); viewS.setToggleState(display.lay[1] && (display.combine || display.front == 1), juce::dontSendNotification); viewP.setToggleState(display.lay[2] && (display.combine || display.front == 2), juce::dontSendNotification);
        vComb.setToggleState(display.combine, juce::dontSendNotification); const bool multi = display.combine && display.vis().size() > 1; layoutB.setVisible(display.combine); frontB.setVisible(multi && !display.split);
        layoutB.setButtonText(display.split ? "Split" : "Overlay"); const char* fn[] = { "Waveform", "Spectral", "Pitch" }; frontB.setButtonText(String("Front: ") + fn[display.frontLayer()]);
        showSpecTools(specShown()); const bool pt = pitchShown(); for (auto* c2 : std::initializer_list<juce::Component*>{ &snapAllB, &fFollow, &fKeep, &dragCb }) c2->setVisible(pt);
        fFollow.setToggleState(!p.warp.keepFormants, juce::dontSendNotification); fKeep.setToggleState(p.warp.keepFormants, juce::dontSendNotification); toolRow = specShown() || pt; layout(); display.repaint();
    }
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
    void openWarp()
    {
        if (p.doc.audio.frames() == 0) return; closeBox();
        auto panel = std::make_unique<WarpPanel>(p, [this] { updateChips(); refreshInfo(); display.repaint(); }, [this] { closeBox(); });
        auto& cb = juce::CallOutBox::launchAsynchronously(std::move(panel), mWarp.getScreenBounds(), nullptr); box = &cb;
    }
    juce::PopupMenu warpMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0; auto chg = [this](std::function<void()> f) { return [this, f] { p.stopPlay(); f(); updateChips(); refreshInfo(); }; };
        m.addItem("Warp, tempo and pitch...", has, false, [this] { openWarp(); }); m.addSeparator();
        m.addItem("Sync to the host tempo", has, p.warp.sync, chg([this] { p.warp.sync = !p.warp.sync; }));
        const char* mn[] = { "Stretch", "Repitch", "Beats", "Texture" }; juce::PopupMenu wm; for (int i = 0; i < 4; ++i) wm.addItem(mn[i], true, p.warp.mode == i, chg([this, i] { p.warp.mode = i; })); m.addSubMenu("Warp mode", wm);
        juce::PopupMenu fm; fm.addItem("Follow the pitch", true, !p.warp.keepFormants, chg([this] { p.warp.keepFormants = false; })); fm.addItem("Keep", true, p.warp.keepFormants, chg([this] { p.warp.keepFormants = true; })); m.addSubMenu("Formants", fm);
        juce::PopupMenu tm; for (int v : { -12, -7, -5, -2, -1, 1, 2, 5, 7, 12 }) { const int vv = v; tm.addItem(String(v > 0 ? "+" : "") + String(v) + " semitones", true, false, chg([this, vv] { p.warp.semis = (double) vv; })); } m.addSubMenu("Transpose", tm);
        m.addSeparator();
        m.addItem("Preview with the warp", has, false, [this] { p.stopPlay(); const bool all = p.warp.all || !p.doc.hasSelection(); p.playRange(all ? 0 : p.doc.selA, all ? p.doc.audio.frames() : p.doc.selB, true); });
        m.addItem("Bake in", has && (p.warpActive() || (p.warp.keepFormants && p.warp.fshift != 0)), false, [this] { p.bakeWarp(false); display.fitAll(); updateChips(); refreshInfo(); });
        m.addItem("Reset the warp", has, false, chg([this] { p.resetWarp(); })); m.addItem("Detect the tempo again", has, false, chg([this] { p.warp.bpm = 0; }));
        return m;
    }
    juce::PopupMenu tempoMenu()
    {
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0; const double det = p.detectedBpm(), cur = p.sampleBpm(); auto set = [this](double v) { return [this, v] { p.stopPlay(); p.warp.bpm = std::round(juce::jlimit(40.0, 240.0, v) * 10.0) / 10.0; updateChips(); refreshInfo(); }; };
        m.addItem(det > 0 ? "Use the detected tempo (" + String(det, 1) + " bpm)" : String("No tempo detected in this sound"), has && det > 0, false, set(det));
        m.addItem("Double the tempo", has && cur > 0, false, set(cur * 2.0)); m.addItem("Half the tempo", has && cur > 0, false, set(cur * 0.5));
        m.addItem("Use the host tempo (" + String(p.currentBpm(), 1) + " bpm)", has, false, set(p.currentBpm())); m.addSeparator();
        m.addItem("Warp, tempo and pitch...", has, false, [this] { openWarp(); }); return m;
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
        m.addItem("From the cursor to the end", has, false, [this] { const size_t a = p.doc.hasSelection() ? p.doc.selA : p.doc.cursor; p.doc.setSelection(a, p.doc.audio.frames()); display.repaint(); refreshInfo(); });
        m.addItem("From the start to the cursor", has, false, [this] { const size_t b = p.doc.hasSelection() ? p.doc.selB : p.doc.cursor; p.doc.setSelection(0, b); display.repaint(); refreshInfo(); });
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
        juce::PopupMenu m; const bool has = p.doc.audio.frames() > 0;
        auto vis = [this](int l) { const auto v = display.vis(); return std::find(v.begin(), v.end(), l) != v.end(); };
        m.addItem("Waveform view", true, vis(Display::LWave), [this] { display.toggleLayer(Display::LWave); }); m.addItem("Spectral view", true, vis(Display::LSpec), [this] { display.toggleLayer(Display::LSpec); });
        m.addItem("Pitch view (piano roll)", true, vis(Display::LPitch), [this] { display.toggleLayer(Display::LPitch); }); m.addItem("Waveform pencil (draw samples)", true, display.wpen, [this] { toggleWPen(); });
        m.addItem("Show views together", true, display.combine, [this] { display.setCombine(!display.combine); });
        juce::PopupMenu lm; lm.addItem("Overlay (on top of each other)", true, !display.split, [this] { display.split = false; display.layersChanged(); }); lm.addItem("Split (one above the other)", true, display.split, [this] { display.split = true; display.layersChanged(); }); m.addSubMenu("Together as", lm);
        m.addItem("Bring the next view to the front", display.combine && display.vis().size() > 1, false, [this] { frontB.onClick(); }); m.addSeparator();
        m.addItem("Zoom in", has, false, [this] { display.zoom(0.5, display.getWidth() * 0.5f); }); m.addItem("Zoom out", has, false, [this] { display.zoom(2.0, display.getWidth() * 0.5f); });
        m.addItem("Show the whole sound", has, false, [this] { display.fitAll(); }); m.addItem("Zoom to the selection", p.doc.hasSelection(), false, [this] { display.zoomToSelection(); }); m.addSeparator();
        m.addItem("Show hit marks", true, display.showHits, [this] { display.showHits = !display.showHits; display.repaint(); });
        juce::PopupMenu um; const char* names[] = { "Minutes and seconds", "Seconds", "Samples", "Bars and beats (4/4)" }; for (int i = 0; i < 4; ++i) um.addItem(names[i], true, gUnit == i, [this, i] { gUnit = i; refreshInfo(); display.repaint(); overview.repaint(); updateChips(); }); m.addSubMenu("Time line unit", um);
        m.addItem("Snap notes to semitones", has, false, [this] { snapAllB.onClick(); });
        return m;
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
        if (!ctrl && m.isShiftDown() && specShown()) { const juce::juce_wchar tc = k.getTextCharacter(); int d = -1; if (c >= '1' && c <= '9') d = c - '1'; else { const char* sh = "!@#$%^&*("; for (int i = 0; i < 9; ++i) if (tc == (juce::juce_wchar) sh[i]) d = i; } if (d >= 0) { display.setTool(d); return true; } }
        if (!ctrl && !m.isShiftDown() && p.doc.audio.frames() > 0)
        {
            if (c == juce::KeyPress::homeKey) { p.doc.setCursor(0); display.vStart = 0; display.clampView(); display.repaint(); refreshInfo(); return true; }
            if (c == juce::KeyPress::endKey) { p.doc.setCursor(p.doc.audio.frames()); display.vStart = (double) p.doc.audio.frames(); display.clampView(); display.repaint(); refreshInfo(); return true; }
            if (c == '+' || c == '=') { display.zoom(0.5, display.getWidth() * 0.5f); return true; } if (c == '-') { display.zoom(2.0, display.getWidth() * 0.5f); return true; }
            if (c == 'F' || c == 'f') { display.fitAll(); return true; } if ((c == 'S' || c == 's') && p.doc.hasSelection()) { display.zoomToSelection(); return true; }
        }
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
        gUnitSr = a.sr > 0 ? a.sr : 44100.0; const double sb = a.frames() > 0 ? p.sampleBpm() : 0.0; gUnitBpm = sb > 0 ? sb : p.currentBpm();
        chTempo.setButtonText(sb > 0 ? String(sb, 1) + " bpm" : String("no tempo")); chSync.setButtonText(p.warp.sync ? "Sync" : "Free"); chSync.setToggleState(p.warp.sync, juce::dontSendNotification);
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
        overview.tick(); inM.set(p.inPeak.load()); outM.set(p.outPeak.load()); if (++liveN % 3 == 0) live.repaint();
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
        if (what == 4) { display.setCombine(true); display.toggleLayer(Display::LSpec); display.toggleLayer(Display::LPitch); display.split = true; display.layersChanged(); }
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
        int x = 14; for (auto* b : { &mFile, &mEdit, &mSelect, &mView, &mPlay, &mRecord, &mProcess, &mSpectral, &mWarp, &mHelp }) { const int w = juce::roundToInt(textW(b->getButtonText().toUpperCase())) + 22; b->setBounds(x, 92, w, 28); x += w + 4; }
        guide.setBounds(900, 20, 90, 34); about.setBounds(1000, 20, 130, 34);
        loop.setBounds(14, 126, 36, 34); play.setBounds(54, 126, 36, 34); stop.setBounds(94, 126, 36, 34); rec.setBounds(134, 126, 36, 34); recOpt.setBounds(172, 126, 18, 34);
        startNow.setBounds(204, 129, 66, 28); startIn.setBounds(274, 129, 92, 28); startCount.setBounds(370, 129, 86, 28); takeB.setBounds(464, 129, 110, 28);
        thr.setBounds(14, 180, 66, 58); bpm.setBounds(88, 180, 66, 58); pre.setBounds(162, 180, 66, 58);
        timeL.setBounds(246, 176, 180, 30); recHintL.setBounds(246, 208, 380, 24);
        for (int k = 0; k < opBtns.size(); ++k) opBtns[k]->setBounds(640 + (k / 2) * 31, 92 + (k % 2) * 31, 28, 28);
        opHint.setBounds(640, 158, 384, 60);
        x = 14; for (auto& sp : specs) { if (sp.gapBefore) x += 10; sp.b->setBounds(x, 244, 30, 30); x += 33; }
        monitor.setBounds(x + 10, 244, 30, 30);
        readout.setBounds(14, 282, 1010, 20); chCh.setBounds(14, 308, 78, 20); chSr.setBounds(96, 308, 96, 20); chBit.setBounds(196, 308, 128, 20); chUnit.setBounds(328, 308, 110, 20); chTempo.setBounds(442, 308, 108, 20); chSync.setBounds(554, 308, 70, 20); toolHintL.setBounds(664, 300, 360, 30);
        { int tx = 14; for (auto* b : sTools) { b->setBounds(tx, 366, 30, 30); tx += 33; } ampCb.setBounds(tx + 6, 368, 112, 26); penCb.setBounds(tx + 124, 368, 112, 26); brushCb.setBounds(tx + 242, 368, 130, 26); }
        snapAllB.setBounds(14, 368, 190, 26); fFollow.setBounds(212, 368, 130, 26); fKeep.setBounds(346, 368, 130, 26); dragCb.setBounds(484, 368, 170, 26);
        { const int dy = toolRow ? 402 : 364; display.setBounds(14, dy, 1010, 704 - dy); } overview.setBounds(14, 708, 1010, 36); live.setBounds(640, 222, 384, 20);
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
    Pill mSpectral { "Spectral", kTeal }, mWarp { "Warp", kTeal }, chTempo { "120.0 bpm", kTeal }, chSync { "Free", kTeal };
    Pill chCh { "Stereo", kTeal }, chSr { "44.1 kHz", kTeal }, chBit { "24 bit", kTeal }, chUnit { "min:sec", kTeal };
    Pill startNow { "Now", kPink }, startIn { "On input", kPink }, startCount { "Count-in", kPink }, takeB { "Replace", kGold }, viewW { "Waveform" }, viewS { "Spectral" }, viewP { "Pitch" }, layoutB { "Overlay", kTeal }, frontB { "Front: Waveform", kTeal }, snapAllB { "Snap notes to semitones", kPink }, fFollow { "Formants follow" }, fKeep { "Keep formants" };
    IconButton vComb { "layers" }, vPen { "pencil" }; juce::ComboBox dragCb;
    IconButton loop { "loop" }, play { "play" }, stop { "stop" }, rec { "rec" }, monitor { "spk" }; Pill recOpt { juce::String::fromUTF8("\xe2\x96\xbe"), kRed };
    Display display; Overview overview; LiveStrip live { p }; Meter inM, outM; DragHandle drag; juce::Label readout, timeL, recHintL, opHint; juce::Slider thr, bpm, pre;
    juce::OwnedArray<IconButton> tools, opBtns; std::vector<ToolSpec> specs; std::vector<int> opOrder; 
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopA, monA; std::unique_ptr<juce::TooltipWindow> tip; std::unique_ptr<juce::FileChooser> chooser;
    juce::Component* box = nullptr; int liveN = 0; int recMode = MeowProcessor::RecNow, seenVer = -1, statusAge = 0; String status;
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
