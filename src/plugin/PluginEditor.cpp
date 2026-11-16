#include "PluginEditor.h"
#include "Layout.h"
#include "Studio87Link.h"
#include "Icons.h"
#include <BinaryData.h>
#include <functional>
#include <map>

namespace
{
const juce::Colour kGround { 0xff0b0b0d }, kPanel { 0xff15161a }, kPanel2 { 0xff1c1d22 }, kLine { 0xff2b2d34 }, kInk { 0xfff4f5f7 }, kDim { 0xffa9adb8 },
                   kKnob { 0xff23252c }, kBtnLine { 0xff343741 }, kGreen { 0xff02EBBA }, kYellow { 0xffFFD100 }, kOrange { 0xffFF9000 }, kPink { 0xffFF1383 }, kBlue { 0xff6CC9F5 };
const char* const kSoon = "Not in this build yet. See the ROADMAP.";

juce::Rectangle<int> rc (const layout::R& r) { return { r.x, r.y, r.w, r.h }; }

juce::Font labelFont (float h, bool bold = false)
{
    static auto med = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedMedium_ttf, (size_t) BinaryData::BarlowCondensedMedium_ttfSize);
    static auto semi = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, (size_t) BinaryData::BarlowCondensedSemiBold_ttfSize);
    return juce::Font (juce::FontOptions (bold ? semi : med).withHeight (h));
}
juce::Font titleFont (float h)
{
    static auto bold = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedBold_ttf, (size_t) BinaryData::BarlowCondensedBold_ttfSize);
    return juce::Font (juce::FontOptions (bold).withHeight (h)).withExtraKerningFactor (0.12f);
}
juce::Font monoFont (float h)
{
    static auto mono = juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMonoRegular_ttf, (size_t) BinaryData::JetBrainsMonoRegular_ttfSize);
    return juce::Font (juce::FontOptions (mono).withHeight (h));
}

juce::String fmtClock (double sec, int decimals)
{
    if (sec < 0) sec = 0;
    const int m = (int) (sec / 60.0); const double s = sec - 60.0 * m;
    return juce::String (m) + ":" + juce::String (s, decimals).paddedLeft ('0', decimals + 3);
}
juce::String fmtPos (double frames, double sr, int unit, bool ruler = false)
{
    if (unit == 2) return juce::String ((juce::int64) std::llround (frames)) + (ruler ? "" : " smp");
    if (unit == 1) return juce::String (frames / sr, ruler ? 2 : 3) + (ruler ? "" : " s");
    return fmtClock (frames / sr, ruler ? 2 : 3);
}
juce::String fmtDb (float lin) { return lin <= 1e-5f ? juce::String ("-inf") : juce::String (20.f * std::log10 (lin), 1); }

// ---------------------------------------------------------------- small widgets
class IconButton : public juce::Button
{
public:
    explicit IconButton (const juce::String& key, juce::Colour accentColour = kPink) : juce::Button (key), accent (accentColour)
    {
        for (auto& d : icons::kIcons)
            if (key == d.key) if (auto xml = juce::parseXML (d.svg)) icon = juce::Drawable::createFromSVG (*xml);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void setOn (bool b) { if (on != b) { on = b; repaint(); } }
    void setActive (bool a) { if (isEnabled() != a) { setEnabled (a); setAlpha (a ? 1.f : 0.3f); } }
    juce::Colour accent;
private:
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        juce::Colour bg = on ? accent.withAlpha (0.2f) : (over ? juce::Colour (0xff2c2f38) : kKnob);
        if (down) bg = bg.brighter (0.1f);
        g.setColour (bg); g.fillRoundedRectangle (b, 4.f);
        g.setColour (on ? accent : (over ? juce::Colour (0xff50535f) : kBtnLine)); g.drawRoundedRectangle (b, 4.f, 1.f);
        if (icon) icon->drawWithin (g, juce::Rectangle<float> (0, 0, 20, 20).withCentre (b.getCentre()), juce::RectanglePlacement::centred, 1.f);
    }
    std::unique_ptr<juce::Drawable> icon; bool on = false;
};

class TextBtn : public juce::Button
{
public:
    enum Style { menu, chip, segment, tab, take };
    TextBtn (const juce::String& text, Style s) : juce::Button (text), style (s) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void setOn (bool b) { if (on != b) { on = b; repaint(); } }
    void setActive (bool a) { if (isEnabled() != a) { setEnabled (a); setAlpha (a ? 1.f : 0.45f); } }
    void setText (const juce::String& t) { label = t; repaint(); }
    juce::String label;
private:
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f); const juce::String t = label.isEmpty() ? getName() : label;
        switch (style)
        {
            case menu:
                if (over || down) { g.setColour (kPanel2); g.fillRoundedRectangle (b, 3.f); }
                g.setColour (kInk); g.setFont (labelFont (16.f, true)); g.drawText (t, getLocalBounds(), juce::Justification::centred); break;
            case chip:
                g.setColour (over ? kPanel2 : kPanel); g.fillRoundedRectangle (b, 3.f); g.setColour (kLine); g.drawRoundedRectangle (b, 3.f, 1.f);
                g.setColour (kInk); g.setFont (monoFont (12.5f)); g.drawText (t + juce::String (juce::CharPointer_UTF8 ("  \xe2\x96\xbe")), getLocalBounds(), juce::Justification::centred); break;
            case segment:
                g.setColour (on ? kPink.withAlpha (0.18f) : kPanel); g.fillRect (b); g.setColour (on ? kPink : kLine); g.drawRect (b, 1.f);
                g.setColour (on ? kInk : kInk.withAlpha (0.9f)); g.setFont (labelFont (13.f, true)); g.drawText (t.toUpperCase(), getLocalBounds(), juce::Justification::centred); break;
            case tab:
                g.setColour (on ? kPink.withAlpha (0.18f) : kPanel); g.fillRect (b); g.setColour (on ? kPink : kLine); g.drawRect (b, 1.f);
                g.setColour (kInk); g.setFont (labelFont (14.f, true)); g.drawText (t, getLocalBounds(), juce::Justification::centred); break;
            case take:
                g.setColour (kPink); g.fillEllipse (6.f, (float) getHeight() * 0.5f - 2.5f, 5.f, 5.f);
                g.setColour (over ? kInk : kPink); g.setFont (labelFont (13.f, true)); g.drawText (t.toUpperCase(), getLocalBounds().withTrimmedLeft (16), juce::Justification::centredLeft); break;
        }
    }
    Style style; bool on = false;
};

class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (juce::RangedAudioParameter& prm, std::function<juce::String (float)> fmt, std::function<void (const juce::String&)> setTip)
        : param (prm), format (std::move (fmt)), att (prm, [this] (float) { repaint(); }, nullptr)
    {
        att.sendInitialUpdate(); (void) setTip; setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }
    juce::String valueText() const { return format (param.convertFrom0to1 (param.getValue())); }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.f); const float r = b.getWidth() * 0.5f, cx = b.getCentreX(), cy = b.getCentreY();
        const float a0 = -2.35f, a1 = 2.35f, a = a0 + (a1 - a0) * param.getValue();
        g.setColour (kKnob); g.fillEllipse (b); g.setColour (kBtnLine); g.drawEllipse (b, 1.f);
        juce::Path track; track.addCentredArc (cx, cy, r + 1.5f, r + 1.5f, 0.f, a0, a1, true); g.setColour (kLine); g.strokePath (track, juce::PathStrokeType (2.f));
        juce::Path val; val.addCentredArc (cx, cy, r + 1.5f, r + 1.5f, 0.f, a0, a, true); g.setColour (kInk.withAlpha (0.55f)); g.strokePath (val, juce::PathStrokeType (2.f));
        juce::Path p; p.addRoundedRectangle (-1.2f, -r + 3.f, 2.4f, r * 0.5f, 1.f); g.setColour (kInk); g.fillPath (p, juce::AffineTransform::rotation (a).translated (cx, cy));
    }
    void mouseDown (const juce::MouseEvent&) override { att.beginGesture(); startNorm = param.getValue(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float n = juce::jlimit (0.f, 1.f, startNorm - (float) e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.f : 200.f));
        att.setValueAsPartOfGesture (param.convertFrom0to1 (n));
    }
    void mouseUp (const juce::MouseEvent&) override { att.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { att.beginGesture(); att.setValueAsPartOfGesture (param.convertFrom0to1 (param.getDefaultValue())); att.endGesture(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        att.beginGesture(); att.setValueAsPartOfGesture (param.convertFrom0to1 (juce::jlimit (0.f, 1.f, param.getValue() + w.deltaY * 0.1f))); att.endGesture();
    }
private:
    juce::RangedAudioParameter& param; std::function<juce::String (float)> format; juce::ParameterAttachment att; float startNorm = 0.f;
};

class Meters : public juce::Component
{
public:
    explicit Meters (MeowcorderProcessor& p) : proc (p) {}
    float thresholdDb = -40.f;
    void paint (juce::Graphics& g) override
    {
        const float top = 24.f, bot = (float) getHeight() - 24.f, h = bot - top;
        auto yOf = [&] (float db) { return bot - h * juce::jlimit (0.f, 1.f, (db + 72.f) / 72.f); };
        g.setColour (kDim); g.setFont (monoFont (10.5f)); g.drawText ("IN", 4, 4, 30, 14, juce::Justification::centred); g.drawText ("OUT", 66, 4, 30, 14, juce::Justification::centred);
        const float xs[2] = { 4.f, 66.f };
        for (int m = 0; m < 2; ++m)
        {
            for (int c = 0; c < 2; ++c)
            {
                const float x = xs[m] + c * 16.f; auto lane = juce::Rectangle<float> (x, top, 14.f, h);
                g.setColour (juce::Colour (0xff0a0a0c)); g.fillRect (lane);
                const float pk = (m == 0 ? proc.inPeak[c] : proc.outPeak[c]).load(); const float db = pk <= 1e-4f ? -80.f : 20.f * std::log10 (pk);
                const float y = yOf (db);
                juce::ColourGradient gr (kPink, 0, top, kGreen, 0, bot, false); gr.addColour (0.25, kYellow); gr.addColour (0.5, kGreen);
                g.setGradientFill (gr); g.fillRect (juce::Rectangle<float> (x, y, 14.f, bot - y));
                g.setColour (kInk); g.fillRect (x, y - 1.f, 14.f, 2.f);
            }
        }
        g.setColour (kDim); g.setFont (monoFont (10.f));
        for (int db = 0; db >= -72; db -= 6) g.drawText (juce::String (db), 36, (int) yOf ((float) db) - 6, 28, 12, juce::Justification::centred);
        g.drawText ("dBFS", 30, getHeight() - 16, 40, 12, juce::Justification::centred);
        const float ty = yOf (thresholdDb); juce::Path tri; tri.addTriangle (0.f, ty - 5.f, 0.f, ty + 5.f, 9.f, ty); g.setColour (kYellow); g.fillPath (tri);
    }
private:
    MeowcorderProcessor& proc;
};

class LiveStrip : public juce::Component
{
public:
    explicit LiveStrip (MeowcorderProcessor& p) : proc (p) {}
    float thresholdDb = -40.f;
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff08080a)); const int w = getWidth(), h = getHeight(); const int head = proc.liveHead.load();
        const bool rec = proc.recState() == MeowcorderProcessor::recRecording;
        g.setColour (rec ? kPink : kGreen.withAlpha (0.8f));
        for (int x = 0; x < w; ++x)
        {
            const int idx = (head + (x * MeowcorderProcessor::kLive) / std::max (1, w)) % MeowcorderProcessor::kLive; const float v = proc.livePeaks[(size_t) idx].load();
            const float amp = juce::jlimit (0.f, 1.f, v) * (float) h * 0.5f; g.drawVerticalLine (x, (float) h * 0.5f - amp, (float) h * 0.5f + amp + 1.f);
        }
        const float ty = (float) h * 0.5f - juce::Decibels::decibelsToGain (thresholdDb) * (float) h * 0.5f;
        g.setColour (kYellow.withAlpha (0.55f)); g.drawHorizontalLine ((int) ty, 0.f, (float) w); g.drawHorizontalLine ((int) ((float) h - ty), 0.f, (float) w);
        g.setColour (kLine); g.drawRect (getLocalBounds(), 1);
        g.setColour (kDim); g.setFont (labelFont (12.f));
        g.drawText (rec ? "Recording " + fmtClock (proc.recordedSeconds(), 1) : proc.recState() == MeowcorderProcessor::recCountIn ? "Count-in" : proc.recState() == MeowcorderProcessor::recArmed ? "Waiting for sound above the threshold" : "Live input", 8, 0, w - 16, h, juce::Justification::centredLeft);
    }
private:
    MeowcorderProcessor& proc;
};

// The waveform: two lanes, selection, cursor, playhead, zoom and scroll.
class WaveView : public juce::Component
{
public:
    WaveView (MeowcorderProcessor& p, std::function<void()> changed) : proc (p), onChange (std::move (changed)) {}
    int unit = 0; bool snapZero = false; std::function<void()> onZoom;

    double viewStart = 0, viewLen = 0;
    double frames() const { return (double) proc.ed.audio.frames(); }
    void fit() { viewStart = 0; viewLen = std::max (1.0, frames()); dirtyPeaks(); onZoom ? onZoom() : void(); }
    void clampView()
    {
        const double n = std::max (1.0, frames()); viewLen = juce::jlimit (std::min (n, 64.0), n, viewLen <= 0 ? n : viewLen);
        viewStart = juce::jlimit (0.0, std::max (0.0, n - viewLen), viewStart); dirtyPeaks(); if (onZoom) onZoom();
    }
    void zoom (double factor, double centre) { const double f = (centre - viewStart) / std::max (1.0, viewLen); viewLen *= factor; viewStart = centre - f * viewLen; clampView(); }
    void zoomToSelection() { if (proc.ed.hasSelection()) { viewStart = (double) proc.ed.selA; viewLen = (double) (proc.ed.selB - proc.ed.selA); clampView(); } }
    void dirtyPeaks() { cacheKey = ~0ull; repaint(); }
    double frameAt (int x) const { return viewStart + (double) x / std::max (1, getWidth()) * viewLen; }
    int xAt (double f) const { return (int) std::lround ((f - viewStart) / std::max (1.0, viewLen) * getWidth()); }

    void audioReplaced() { if (viewLen <= 0 || viewStart + viewLen > frames() + 1) { fit(); } else clampView(); transientsKey = ~0ull; }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (kGround); const auto& a = proc.ed.audio; const int w = getWidth(), h = getHeight();
        if (a.frames() == 0)
        {
            g.setColour (kDim); g.setFont (labelFont (18.f)); g.drawText ("Press record, or drop an audio file here.", getLocalBounds(), juce::Justification::centred); return;
        }
        buildPeaks (w);
        const int laneH = h / 2;
        for (int c = 0; c < 2; ++c)
        {
            const int top = c * laneH, mid = top + laneH / 2;
            g.setColour (kLine); g.drawHorizontalLine (mid, 0.f, (float) w);
            if (c == 1) g.drawHorizontalLine (top, 0.f, (float) w);
            g.setColour (kDim); g.setFont (monoFont (10.f)); g.drawText (c ? "R" : "L", 3, top + 3, 14, 12, juce::Justification::centredLeft);
            g.setColour (kGreen);
            for (int x = 0; x < w && x < (int) pkMax[c].size(); ++x)
            {
                const float hi = juce::jlimit (-1.f, 1.f, pkMax[(size_t) c][(size_t) x]), lo = juce::jlimit (-1.f, 1.f, pkMin[(size_t) c][(size_t) x]);
                const float y0 = (float) mid - hi * (float) laneH * 0.48f, y1 = (float) mid - lo * (float) laneH * 0.48f; g.drawVerticalLine (x, y0, std::max (y1, y0 + 1.f));
            }
        }
        if (transientsKey != proc.version) { trans = mc::detectTransients (a, 0.5f); transientsKey = proc.version; }
        g.setColour (kOrange);
        for (auto t : trans) { const int x = xAt ((double) t); if (x >= 0 && x < w) g.fillRect (x, 0, 1, 8); }
        if (proc.ed.hasSelection())
        {
            const int x0 = xAt ((double) proc.ed.selA), x1 = xAt ((double) proc.ed.selB);
            g.setColour (kPink.withAlpha (0.16f)); g.fillRect (x0, 0, std::max (1, x1 - x0), h); g.setColour (kPink); g.drawVerticalLine (x0, 0.f, (float) h); g.drawVerticalLine (x1, 0.f, (float) h);
        }
        else { const int x = xAt ((double) proc.ed.cursor); g.setColour (kInk.withAlpha (0.8f)); g.drawVerticalLine (x, 0.f, (float) h); }
        if (proc.isPlaying() || proc.playheadFrames() > 0) { const int x = xAt (proc.playheadFrames()); if (x >= 0 && x < w && proc.isPlaying()) { g.setColour (kYellow); g.drawVerticalLine (x, 0.f, (float) h); } }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (frames() == 0) return; if (auto* top = getParentComponent()) top->grabKeyboardFocus();
        const size_t f = (size_t) juce::jlimit (0.0, frames(), frameAt (e.x));
        if (e.mods.isShiftDown() && (proc.ed.hasSelection() || proc.ed.cursor != 0)) { anchor = proc.ed.hasSelection() ? (f < (proc.ed.selA + proc.ed.selB) / 2 ? proc.ed.selB : proc.ed.selA) : proc.ed.cursor; proc.ed.setSelection (anchor, f); }
        else { anchor = f; proc.ed.setCursor (f); if (proc.isPlaying()) proc.setPlayhead ((double) f); }
        dragged = false; repaint(); if (onChange) onChange();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (frames() == 0) return;
        if (e.getDistanceFromDragStart() > 3) dragged = true;
        if (! dragged && ! e.mods.isShiftDown()) return;
        const size_t f = (size_t) juce::jlimit (0.0, frames(), frameAt (e.x)); proc.ed.setSelection (anchor, f);
        if (e.x < 0) { viewStart -= viewLen * 0.02; clampView(); } else if (e.x > getWidth()) { viewStart += viewLen * 0.02; clampView(); }
        repaint(); if (onChange) onChange();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragged && snapZero && proc.ed.hasSelection())
        {
            const size_t r = (size_t) std::max (32.0, proc.ed.audio.sr * 0.01); proc.ed.setSelection (proc.ed.snapToZero (proc.ed.selA, r), proc.ed.snapToZero (proc.ed.selB, r));
        }
        repaint(); if (onChange) onChange();
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { proc.ed.setSelection (0, proc.ed.audio.frames()); repaint(); if (onChange) onChange(); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (frames() == 0) return;
        if (e.mods.isCtrlDown() || e.mods.isCommandDown()) zoom (std::exp (-w.deltaY * 1.2), frameAt (e.x));
        else { viewStart -= (w.deltaX != 0 ? w.deltaX : w.deltaY) * viewLen * 0.5; clampView(); }
    }
private:
    void buildPeaks (int w)
    {
        const uint64_t key = proc.version * 1000003ull + (uint64_t) viewStart * 31ull + (uint64_t) viewLen * 17ull + (uint64_t) w;
        if (key == cacheKey) return; cacheKey = key; const auto& a = proc.ed.audio;
        for (int c = 0; c < 2; ++c)
        {
            pkMin[c].assign ((size_t) w, 0.f); pkMax[c].assign ((size_t) w, 0.f);
            const double per = viewLen / std::max (1, w);
            for (int x = 0; x < w; ++x)
            {
                size_t x0 = (size_t) (viewStart + x * per), x1 = (size_t) (viewStart + (x + 1) * per); if (x1 <= x0) x1 = x0 + 1; x1 = std::min (x1, a.frames()); if (x0 >= x1) continue;
                float lo = 0.f, hi = 0.f; for (size_t i = x0; i < x1; ++i) { lo = std::min (lo, a.ch[c][i]); hi = std::max (hi, a.ch[c][i]); }
                pkMin[c][(size_t) x] = lo; pkMax[c][(size_t) x] = hi;
            }
        }
    }
    MeowcorderProcessor& proc; std::function<void()> onChange; size_t anchor = 0; bool dragged = false;
    std::vector<float> pkMin[2], pkMax[2]; uint64_t cacheKey = ~0ull, transientsKey = ~0ull; std::vector<size_t> trans;
};

class Ruler : public juce::Component
{
public:
    Ruler (MeowcorderProcessor& p, WaveView& w) : proc (p), wave (w) {}
    void paint (juce::Graphics& g) override
    {
        g.fillAll (kGround); if (proc.ed.audio.frames() == 0) return;
        const double sr = proc.ed.audio.sr, secs = wave.viewLen / sr; const double want = secs / std::max (1.0, getWidth() / 140.0);
        static const double steps[] = { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600 }; double step = 600; for (double s : steps) if (s >= want) { step = s; break; }
        g.setFont (monoFont (11.f));
        for (double t = std::ceil (wave.viewStart / sr / step) * step; t < (wave.viewStart + wave.viewLen) / sr; t += step)
        {
            const int x = wave.xAt (t * sr); g.setColour (kDim); g.drawVerticalLine (x, 0.f, 8.f);
            g.drawText (fmtPos (t * sr, sr, wave.unit, true), x + 3, 4, 90, 14, juce::Justification::centredLeft);
        }
    }
private:
    MeowcorderProcessor& proc; WaveView& wave;
};

class Overview : public juce::Component
{
public:
    Overview (MeowcorderProcessor& p, WaveView& w) : proc (p), wave (w) {}
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff08080a)); const auto& a = proc.ed.audio; const int w = getWidth(), h = getHeight();
        if (a.frames() > 0)
        {
            if (key != proc.version || (int) pk.mx.size() != w) { pk = mc::peakOverview (a, 0, a.frames(), (size_t) w); key = proc.version; }
            g.setColour (kDim.withAlpha (0.7f));
            for (int x = 0; x < w; ++x) { const float hi = juce::jlimit (0.f, 1.f, pk.mx[(size_t) x]), lo = juce::jlimit (-1.f, 0.f, pk.mn[(size_t) x]); g.drawVerticalLine (x, (float) h * 0.5f - hi * (float) h * 0.45f, (float) h * 0.5f - lo * (float) h * 0.45f + 1.f); }
            const double n = (double) a.frames(); const int x0 = (int) (wave.viewStart / n * w), x1 = (int) ((wave.viewStart + wave.viewLen) / n * w);
            g.setColour (kBlue.withAlpha (0.18f)); g.fillRect (x0, 0, std::max (3, x1 - x0), h); g.setColour (kBlue); g.drawRect (x0, 0, std::max (3, x1 - x0), h, 1);
        }
        g.setColour (kLine); g.drawRect (getLocalBounds(), 1);
    }
    void mouseDown (const juce::MouseEvent& e) override { move (e); }
    void mouseDrag (const juce::MouseEvent& e) override { move (e); }
private:
    void move (const juce::MouseEvent& e) { const double n = wave.frames(); if (n <= 0) return; wave.viewStart = (double) e.x / std::max (1, getWidth()) * n - wave.viewLen * 0.5; wave.clampView(); repaint(); }
    MeowcorderProcessor& proc; WaveView& wave; mc::Peaks pk; uint64_t key = ~0ull;
};

// The spectral view: a picture of the frequencies (log scale, 20 Hz up) with tools that work on boxes of time and frequency.
class SpecView : public juce::Component
{
public:
    enum Tool { toolMouse, toolSelect, toolScissors, toolCutter, toolCopy, toolPaste, toolAmp };
    SpecView (MeowcorderProcessor& p, WaveView& w, std::function<void()> edited, std::function<void()> changed) : proc (p), wave (w), onEdited (std::move (edited)), onChange (std::move (changed)) {}
    int tool = toolMouse; double ampDb = 6.0; bool hasBox = false; mc::SRegion box { 0, 0, 20, 20 }; std::function<void (int)> setToolBack;

    static juce::Colour palette (float db)
    {
        const float t = juce::jlimit (0.f, 1.f, (db + 90.f) / 90.f); static const float pos[] = { 0.f, 0.25f, 0.5f, 0.72f, 0.88f, 1.f };
        static const juce::Colour col[] = { juce::Colour (0xff0a0a0c), juce::Colour (0xff25164a), juce::Colour (0xff8a1fa3), juce::Colour (0xffff1383), juce::Colour (0xffff9000), juce::Colour (0xfffff3b0) };
        int i = 0; while (i < 4 && t > pos[i + 1]) ++i; return col[i].interpolatedWith (col[i + 1], (t - pos[i]) / (pos[i + 1] - pos[i]));
    }
    double fMax() const { return std::max (1000.0, proc.ed.audio.sr * 0.5); }
    double hzAtY (int y) const { const int h = std::max (2, getHeight()); return 20.0 * std::pow (fMax() / 20.0, 1.0 - (double) juce::jlimit (0, h - 1, y) / (double) (h - 1)); }
    int yAtHz (double hz) const { const int h = std::max (2, getHeight()); return (int) std::lround ((1.0 - std::log (std::max (20.0, hz) / 20.0) / std::log (fMax() / 20.0)) * (h - 1)); }
    void applyBox (bool cut) { if (hasBox && proc.ed.spectralEdit (cut, { box }, ampDb)) onEdited(); }
    void copyBox() { if (hasBox && proc.ed.spectralCopy (box)) { setTool (toolPaste); repaint(); } }
    void setTool (int t) { tool = t; if (setToolBack) setToolBack (t); repaint(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff0a0a0c)); const auto& a = proc.ed.audio; const int w = getWidth(), h = getHeight();
        if (a.frames() == 0) { g.setColour (kDim); g.setFont (labelFont (18.f)); g.drawText ("Record something or drop a file here to see its frequencies.", getLocalBounds(), juce::Justification::centred); return; }
        buildImage (w, h); g.drawImageAt (img, 0, 0);
        g.setFont (monoFont (10.f));
        for (double f : { 100.0, 1000.0, 10000.0 }) { if (f > fMax()) continue; const int y = yAtHz (f); g.setColour (juce::Colour (0x22ffffff)); g.drawHorizontalLine (y, 0.f, (float) w); g.setColour (kDim); g.drawText (f >= 1000 ? juce::String (f / 1000.0, 0) + " kHz" : juce::String (f, 0) + " Hz", 4, y - 12, 60, 11, juce::Justification::centredLeft); }
        if (proc.ed.hasSelection()) { const int x0 = wave.xAt ((double) proc.ed.selA), x1 = wave.xAt ((double) proc.ed.selB); g.setColour (kPink.withAlpha (0.14f)); g.fillRect (x0, 0, std::max (1, x1 - x0), h); g.setColour (kPink); g.drawVerticalLine (x0, 0.f, (float) h); g.drawVerticalLine (x1, 0.f, (float) h); }
        else { g.setColour (kInk.withAlpha (0.8f)); g.drawVerticalLine (wave.xAt ((double) proc.ed.cursor), 0.f, (float) h); }
        if (hasBox) drawBox (g, box, kBlue, kBlue.withAlpha (0.13f), true);
        if (dragging && tool >= toolSelect && tool != toolPaste) { auto b = dragBox(); if (tool == toolScissors) { b.f0 = 20; b.f1 = fMax(); } drawBox (g, b, tool == toolCutter || tool == toolScissors ? kPink : (tool == toolAmp ? kYellow : kBlue), juce::Colour (0x1cffffff), false); }
        if (proc.isPlaying()) { const int x = wave.xAt (proc.playheadFrames()); if (x >= 0 && x < w) { g.setColour (kYellow); g.drawVerticalLine (x, 0.f, (float) h); } }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (proc.ed.audio.frames() == 0) return; if (auto* top = getParentComponent()) top->grabKeyboardFocus();
        p0 = e.getPosition(); p1 = p0; dragging = true;
        if (tool == toolMouse) { const size_t f = (size_t) juce::jlimit (0.0, wave.frames(), wave.frameAt (e.x)); anchor = f; proc.ed.setCursor (f); if (proc.isPlaying()) proc.setPlayhead ((double) f); onChange(); }
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return; p1 = e.getPosition();
        if (tool == toolMouse && e.getDistanceFromDragStart() > 3) { const size_t f = (size_t) juce::jlimit (0.0, wave.frames(), wave.frameAt (e.x)); proc.ed.setSelection (anchor, f); onChange(); }
        repaint();
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! dragging) return; dragging = false; p1 = e.getPosition(); const bool tiny = std::abs (p1.x - p0.x) < 4 || std::abs (p1.y - p0.y) < 4; auto b = dragBox();
        switch (tool)
        {
            case toolSelect: if (tiny) hasBox = false; else { hasBox = true; box = b; } break;
            case toolScissors: if (std::abs (p1.x - p0.x) >= 4) { proc.ed.setSelection ((size_t) b.t0, (size_t) b.t1); proc.ed.deleteSelection(); onEdited(); } break;
            case toolCutter: if (! tiny) { box = b; hasBox = true; applyBox (true); } break;
            case toolAmp: if (! tiny) { box = b; hasBox = true; applyBox (false); } break;
            case toolCopy: if (! tiny) { box = b; hasBox = true; copyBox(); } break;
            case toolPaste: if (proc.ed.spectralPaste (wave.frameAt (e.x), hzAtY (e.y))) onEdited(); break;
            default: break;
        }
        repaint(); onChange();
    }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { wave.mouseWheelMove (e, w); }
private:
    mc::SRegion dragBox() const
    {
        const double ta = wave.frameAt (p0.x), tb = wave.frameAt (p1.x), ha = hzAtY (p0.y), hb = hzAtY (p1.y);
        return { std::min (ta, tb), std::max (ta, tb), std::min (ha, hb), std::max (ha, hb) };
    }
    void drawBox (juce::Graphics& g, const mc::SRegion& r, juce::Colour edge, juce::Colour fill, bool label)
    {
        const int x0 = wave.xAt (r.t0), x1 = wave.xAt (r.t1), y0 = yAtHz (r.f1), y1 = yAtHz (r.f0); auto rr = juce::Rectangle<int> (x0, y0, std::max (1, x1 - x0), std::max (1, y1 - y0));
        g.setColour (fill); g.fillRect (rr); g.setColour (edge); g.drawRect (rr, 1);
        if (label) { g.setFont (monoFont (10.f)); g.drawText (juce::String ((int) r.f0) + " to " + juce::String ((int) r.f1) + " Hz", rr.getX() + 3, rr.getY() + 2, 160, 12, juce::Justification::centredLeft); }
    }
    void buildImage (int w, int h)
    {
        const auto& a = proc.ed.audio; const uint64_t key = proc.version * 1000003ull + (uint64_t) wave.viewStart * 31ull + (uint64_t) wave.viewLen * 17ull + (uint64_t) w * 7ull + (uint64_t) h;
        if (key == cacheKey && img.isValid() && img.getWidth() == w && img.getHeight() == h) return; cacheKey = key;
        const size_t from = (size_t) wave.viewStart, to = (size_t) std::ceil (wave.viewStart + wave.viewLen); const int hop = std::max (64, (int) ((to - from) / (size_t) std::max (1, w)));
        const auto sp = mc::spectrogram (a, from, to, 1024, hop);
        img = juce::Image (juce::Image::RGB, std::max (1, w), std::max (1, h), false); juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        static const std::array<juce::Colour, 256> pal = [] { std::array<juce::Colour, 256> p; for (int i = 0; i < 256; ++i) p[(size_t) i] = palette (-90.f + (float) i / 255.f * 90.f); return p; }();
        std::vector<int> row ((size_t) h); for (int y = 0; y < h; ++y) { const double f = 20.0 * std::pow (fMax() / 20.0, 1.0 - (double) y / (double) std::max (1, h - 1)); row[(size_t) y] = juce::jlimit (0, sp.bins - 1, (int) std::lround (f / fMax() * (sp.bins - 1))); }
        for (int x = 0; x < w; ++x)
        {
            const int fr = juce::jlimit (0, sp.frames - 1, (int) ((double) x / (double) w * sp.frames));
            for (int y = 0; y < h; ++y) { const float db = sp.db[(size_t) fr * (size_t) sp.bins + (size_t) row[(size_t) y]]; bd.setPixelColour (x, y, pal[(size_t) juce::jlimit (0, 255, (int) std::lround ((db + 90.f) / 90.f * 255.f))]); }
        }
    }
    MeowcorderProcessor& proc; WaveView& wave; std::function<void()> onEdited, onChange; juce::Image img; uint64_t cacheKey = ~0ull;
    juce::Point<int> p0, p1; bool dragging = false; size_t anchor = 0;
};

// The pitch view: a piano roll of the notes found in the sound. Drag a note up or down to change its pitch, the sound follows.
class PitchView : public juce::Component
{
public:
    PitchView (MeowcorderProcessor& p, WaveView& w, std::function<void()> edited) : proc (p), wave (w), onEdited (std::move (edited)) {}
    static juce::String noteName (double m) { static const char* nn[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; const long r = std::lround (m); return juce::String (nn[((r % 12) + 12) % 12]) + juce::String ((int) std::floor ((double) r / 12.0) - 1); }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff0a0a0c)); const auto& a = proc.ed.audio; const int w = getWidth(), h = getHeight();
        if (a.frames() == 0) { g.setColour (kDim); g.setFont (labelFont (18.f)); g.drawText ("Record or open a sound with a clear pitch to see its notes.", getLocalBounds(), juce::Justification::centred); return; }
        if (a.frames() > (size_t) (a.sr * 120)) { g.setColour (kDim); g.setFont (labelFont (16.f)); g.drawText ("The pitch view works on sounds up to two minutes. Select a part and crop it first.", getLocalBounds(), juce::Justification::centred); return; }
        refreshNotes(); computeRange(); const double rowH = (double) h / (hi - lo);
        for (int m = (int) lo; m < (int) hi; ++m)
        {
            const int y = (int) ((hi - m - 1) * rowH); const bool black = (m % 12 == 1 || m % 12 == 3 || m % 12 == 6 || m % 12 == 8 || m % 12 == 10);
            g.setColour (black ? juce::Colour (0xff0e0e11) : juce::Colour (0xff131418)); g.fillRect (0, y, w, (int) std::ceil (rowH)); g.setColour (juce::Colour (0xff1d1e24)); g.drawHorizontalLine (y, 0.f, (float) w);
            if (m % 12 == 0) { g.setColour (kDim); g.setFont (monoFont (10.f)); g.drawText (noteName ((double) m), 4, y + (int) rowH - 13, 40, 12, juce::Justification::centredLeft); }
        }
        g.setColour (kBlue.withAlpha (0.55f));
        for (size_t i = 0; i < result.track.midi.size(); ++i) { const float m = result.track.midi[i]; if (std::isnan (m)) continue; g.fillRect (wave.xAt ((double) result.track.t[i]), (int) ((hi - (double) m - 0.5) * rowH), 2, 2); }
        for (size_t i = 0; i < result.notes.size(); ++i)
        {
            const auto& n = result.notes[i]; const bool drag = (int) i == dragIdx; const double mm = n.m + (drag ? shiftSemis : 0);
            const int x0 = wave.xAt ((double) n.t0), x1 = wave.xAt ((double) n.t1); auto r = juce::Rectangle<float> ((float) x0, (float) ((hi - mm - 0.5) * rowH - rowH * 0.4), (float) std::max (3, x1 - x0), (float) (rowH * 0.8));
            g.setColour ((drag ? kPink : kGreen).withAlpha (0.35f)); g.fillRoundedRectangle (r, 3.f); g.setColour (drag ? kPink : kGreen); g.drawRoundedRectangle (r, 3.f, 1.2f);
            g.setFont (monoFont (10.f)); g.drawText (noteName (mm) + (drag && shiftSemis != 0 ? juce::String (shiftSemis > 0 ? " +" : " ") + juce::String (shiftSemis) + " st" : juce::String()), r.toNearestInt().reduced (3, 0), juce::Justification::centredLeft);
        }
        if (result.notes.empty()) { g.setColour (kDim); g.setFont (labelFont (16.f)); g.drawText ("No clear notes found in this sound.", getLocalBounds(), juce::Justification::centred); }
        if (proc.isPlaying()) { const int x = wave.xAt (proc.playheadFrames()); if (x >= 0 && x < w) { g.setColour (kYellow); g.drawVerticalLine (x, 0.f, (float) h); } }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (auto* top = getParentComponent()) top->grabKeyboardFocus();
        dragIdx = -1; shiftSemis = 0; const double f = wave.frameAt (e.x), rowH = (double) getHeight() / (hi - lo);
        for (size_t i = 0; i < result.notes.size(); ++i) { const auto& n = result.notes[i]; const double y = (hi - n.m - 0.5) * rowH; if (f >= (double) n.t0 && f <= (double) n.t1 && std::fabs ((double) e.y - y) <= rowH) { dragIdx = (int) i; break; } }
        if (dragIdx < 0 && proc.ed.audio.frames() > 0) { proc.ed.setCursor ((size_t) juce::jlimit (0.0, wave.frames(), f)); if (proc.isPlaying()) proc.setPlayhead (f); }
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (dragIdx >= 0) { shiftSemis = (int) std::lround (-(double) e.getDistanceFromDragStartY() / ((double) getHeight() / (hi - lo))); repaint(); } }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragIdx >= 0 && shiftSemis != 0 && (size_t) dragIdx < result.notes.size())
        {
            const auto n = result.notes[(size_t) dragIdx]; const double semis = shiftSemis;
            proc.ed.transform ("Pitch of a note", [&] (mc::Audio& a) { mc::pitchShiftRange (a, (size_t) n.t0, (size_t) n.t1, semis, true, 0.0); });
            dragIdx = -1; shiftSemis = 0; onEdited();
        }
        dragIdx = -1; shiftSemis = 0; repaint();
    }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { wave.mouseWheelMove (e, w); }
private:
    void refreshNotes() { if (key == proc.version) return; key = proc.version; result = mc::detectNotes (proc.ed.audio); }
    void computeRange()
    {
        if (result.notes.empty()) { lo = 40; hi = 76; return; }
        double mn = 200, mx = 0; for (auto& n : result.notes) { mn = std::min (mn, n.m); mx = std::max (mx, n.m); }
        lo = std::floor (mn) - 3; hi = std::ceil (mx) + 4; if (hi - lo < 24) { const double c = 0.5 * (hi + lo); lo = std::floor (c - 12); hi = lo + 24; }
    }
    MeowcorderProcessor& proc; WaveView& wave; std::function<void()> onEdited; mc::NoteResult result; uint64_t key = 0; double lo = 40, hi = 76; int dragIdx = -1, shiftSemis = 0;
};

// Warp and transpose: speed with four ways to do it, pitch, fine tune and formants. Preview plays the result, Apply writes it into the sound.
class WarpPanel : public juce::Component
{
public:
    WarpPanel (MeowcorderProcessor& p, std::function<void (const mc::WarpOptions&, bool)> run, std::function<void()> finished) : proc (p), runWarp (std::move (run)), done (std::move (finished))
    {
        struct Def { const char* name; double lo, hi, def; const char* unit; const char* tip; };
        static const Def defs[] = { { "Speed", 50, 200, 100, "%", "How fast it plays. 100 is as recorded. More is faster and shorter." }, { "Transpose", -24, 24, 0, "st", "Pitch change in semitones." },
                                    { "Fine", -100, 100, 0, "ct", "Fine tune in cents." }, { "Formant shift", -12, 12, 0, "st", "Moves the vowel shape of a voice, when Formants is on Keep." } };
        for (int i = 0; i < 4; ++i)
        {
            auto lab = std::make_unique<juce::Label> (juce::String(), juce::String (defs[i].name)); lab->setFont (labelFont (16.f, true)); lab->setColour (juce::Label::textColourId, kInk); lab->setTooltip (defs[i].tip); addAndMakeVisible (*lab);
            auto sl = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight); sl->setRange (defs[i].lo, defs[i].hi, i == 0 ? 0.1 : (i == 2 ? 1.0 : 0.01)); sl->setValue (saved[i], juce::dontSendNotification);
            sl->setDoubleClickReturnValue (true, defs[i].def); sl->setTextValueSuffix (juce::String (" ") + defs[i].unit); sl->setColour (juce::Slider::trackColourId, kGreen); sl->setColour (juce::Slider::thumbColourId, kInk); sl->setColour (juce::Slider::backgroundColourId, kLine);
            sl->setColour (juce::Slider::textBoxTextColourId, kInk); sl->setColour (juce::Slider::textBoxOutlineColourId, kLine); sl->setTooltip (defs[i].tip); addAndMakeVisible (*sl);
            labels[i] = lab.get(); sliders[i] = sl.get(); owned.push_back (std::move (lab)); owned.push_back (std::move (sl));
        }
        for (auto* cb : { &mode, &formant }) addAndMakeVisible (*cb);
        mode.addItemList ({ "Stretch (pitch stays)", "Repitch (like tape)", "Beats (hits stay sharp)", "Texture (grains)" }, 1); mode.setSelectedId (savedMode + 1, juce::dontSendNotification); mode.setTooltip ("How the speed is changed. Stretch and Beats keep the pitch, Repitch changes both, Texture smears it into grains.");
        formant.addItemList ({ "Follow the pitch", "Keep the formants" }, 1); formant.setSelectedId (savedFormant + 1, juce::dontSendNotification); formant.setTooltip ("Keep the formants so a transposed voice does not sound smaller or bigger.");
        for (auto* b : { &preview, &apply, &cancel }) addAndMakeVisible (*b);
        preview.onClick = [this] { togglePreview(); }; apply.onClick = [this] { proc.endPreview(); remember(); runWarp (options(), false); done(); }; cancel.onClick = [this] { proc.endPreview(); done(); };
        setWantsKeyboardFocus (true); setSize (600, 330);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (getLocalBounds().toFloat().translated (0, 4), 8.f);
        g.setColour (kPanel); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 8.f); g.setColour (kBtnLine); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 8.f, 1.f);
        g.setColour (kInk); g.setFont (titleFont (22.f)); g.drawText ("Warp and transpose", 20, 10, getWidth() - 40, 28, juce::Justification::centredLeft);
        g.setColour (kDim); g.setFont (labelFont (14.f)); g.drawText ("Changes the speed and the pitch of the sound. Long sounds take a moment to process.", 20, 38, getWidth() - 40, 20, juce::Justification::centredLeft);
        g.setFont (labelFont (16.f)); g.setColour (kInk); g.drawText ("Method", 20, 80 + 38 * 4, 130, 28, juce::Justification::centredLeft); g.drawText ("Formants", 20, 80 + 38 * 5 - 0, 130, 28, juce::Justification::centredLeft);
        g.setColour (kDim); g.setFont (monoFont (11.f)); g.drawText (proc.ed.hasSelection() ? "Applies to the selection" : "Applies to the whole sound", 20, getHeight() - 34, 260, 20, juce::Justification::centredLeft);
    }
    void resized() override
    {
        int y = 80; for (int i = 0; i < 4; ++i) { labels[i]->setBounds (20, y, 130, 28); sliders[i]->setBounds (150, y, getWidth() - 170, 28); y += 38; }
        mode.setBounds (150, y, 260, 28); formant.setBounds (150, y + 38, 260, 28);
        preview.setBounds (getWidth() - 304, getHeight() - 40, 96, 28); apply.setBounds (getWidth() - 200, getHeight() - 40, 88, 28); cancel.setBounds (getWidth() - 104, getHeight() - 40, 88, 28);
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { proc.endPreview(); done(); return true; } if (k == juce::KeyPress::spaceKey) { togglePreview(); return true; }
        if (k == juce::KeyPress::returnKey) { proc.endPreview(); remember(); runWarp (options(), false); done(); return true; } return false;
    }
private:
    mc::WarpOptions options() const
    {
        mc::WarpOptions o; o.ratio = sliders[0]->getValue() / 100.0; o.mode = mode.getSelectedId() - 1; o.semis = sliders[1]->getValue(); o.cents = sliders[2]->getValue(); o.formant = formant.getSelectedId() == 2; o.fshift = sliders[3]->getValue(); return o;
    }
    void remember() { for (int i = 0; i < 4; ++i) saved[i] = sliders[i]->getValue(); savedMode = mode.getSelectedId() - 1; savedFormant = formant.getSelectedId() - 1; }
    void togglePreview() { if (proc.isPreviewing() && proc.isPlaying()) { proc.endPreview(); preview.setButtonText ("Preview"); return; } remember(); runWarp (options(), true); preview.setButtonText ("Stop"); }
    MeowcorderProcessor& proc; std::function<void (const mc::WarpOptions&, bool)> runWarp; std::function<void()> done;
    std::vector<std::unique_ptr<juce::Component>> owned; juce::Label* labels[4] = {}; juce::Slider* sliders[4] = {}; juce::ComboBox mode, formant; juce::TextButton preview { "Preview" }, apply { "Apply" }, cancel { "Cancel" };
    static inline double saved[4] = { 100, 0, 0, 0 }; static inline int savedMode = 0, savedFormant = 1;
};

// Settings of one operation: sliders and choices straight from the operation table, Apply and Cancel.
class OpPanel : public juce::Component
{
public:
    OpPanel (MeowcorderProcessor& p, int opIndex, std::function<void (bool)> finished) : proc (p), op (opIndex), done (std::move (finished))
    {
        const auto& info = mc::kOps[op];
        for (int i = 0; i < info.nParams; ++i)
        {
            const auto& pr = info.params[i];
            auto lab = std::make_unique<juce::Label> (juce::String(), juce::String (pr.name)); lab->setFont (labelFont (16.f, true)); lab->setColour (juce::Label::textColourId, kInk);
            lab->setTooltip (pr.tip); addAndMakeVisible (*lab); labels[i] = lab.get(); owned.push_back (std::move (lab));
            if (pr.choice)
            {
                auto cb = std::make_unique<juce::ComboBox>(); juce::StringArray items; items.addTokens (pr.choices, "|", ""); for (int k = 0; k < items.size(); ++k) cb->addItem (items[k], k + 1);
                cb->setSelectedId ((int) std::lround (proc.opParams[op][i]) + 1, juce::dontSendNotification); cb->setTooltip (pr.tip); addAndMakeVisible (*cb); combos[i] = cb.get(); owned.push_back (std::move (cb));
            }
            else
            {
                auto sl = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight); sl->setRange (pr.lo, pr.hi, (pr.hi - pr.lo) > 200 ? 1.0 : ((pr.hi - pr.lo) > 20 ? 0.1 : 0.01));
                if (pr.lo > 0 && pr.hi / pr.lo > 20.f) sl->setSkewFactorFromMidPoint (std::sqrt ((double) pr.lo * pr.hi));
                sl->setDoubleClickReturnValue (true, pr.def); sl->setValue (proc.opParams[op][i], juce::dontSendNotification); sl->setTextValueSuffix (juce::String (" ") + pr.unit);
                sl->setColour (juce::Slider::trackColourId, kGreen); sl->setColour (juce::Slider::thumbColourId, kInk); sl->setColour (juce::Slider::backgroundColourId, kLine);
                sl->setColour (juce::Slider::textBoxTextColourId, kInk); sl->setColour (juce::Slider::textBoxOutlineColourId, kLine); sl->setTooltip (pr.tip);
                addAndMakeVisible (*sl); sliders[i] = sl.get(); owned.push_back (std::move (sl));
            }
        }
        addAndMakeVisible (apply); addAndMakeVisible (cancel); addAndMakeVisible (preview); setWantsKeyboardFocus (true);
        preview.setTooltip ("Plays the result without changing the sound. Press again to stop."); preview.onClick = [this] { togglePreview(); };
        apply.onClick = [this] { proc.endPreview(); commit(); done (true); }; cancel.onClick = [this] { proc.endPreview(); done (false); };
        setSize (600, 120 + 38 * std::max (1, info.nParams));
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (getLocalBounds().toFloat().translated (0, 4), 8.f);
        g.setColour (kPanel); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 8.f); g.setColour (kBtnLine); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 8.f, 1.f);
        g.setColour (kInk); g.setFont (titleFont (22.f)); g.drawText (mc::kOps[op].name, 20, 10, getWidth() - 40, 28, juce::Justification::centredLeft);
        g.setColour (kDim); g.setFont (labelFont (14.f)); g.drawFittedText (mc::kOps[op].tip, 20, 38, getWidth() - 40, 34, juce::Justification::topLeft, 2);
        g.setColour (kDim); g.setFont (monoFont (11.f)); g.drawText (proc.ed.hasSelection() ? "Applies to the selection" : "Applies to the whole sound", 20, getHeight() - 34, 260, 20, juce::Justification::centredLeft);
    }
    void resized() override
    {
        int y = 78; const auto& info = mc::kOps[op];
        for (int i = 0; i < info.nParams; ++i)
        {
            labels[i]->setBounds (20, y, 130, 28);
            if (sliders[i] != nullptr) sliders[i]->setBounds (150, y, getWidth() - 170, 28);
            if (combos[i] != nullptr) combos[i]->setBounds (150, y, 200, 28);
            y += 38;
        }
        preview.setBounds (getWidth() - 304, getHeight() - 40, 96, 28); apply.setBounds (getWidth() - 200, getHeight() - 40, 88, 28); cancel.setBounds (getWidth() - 104, getHeight() - 40, 88, 28);
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::returnKey) { proc.endPreview(); commit(); done (true); return true; }
        if (k == juce::KeyPress::escapeKey) { proc.endPreview(); done (false); return true; }
        if (k == juce::KeyPress::spaceKey) { togglePreview(); return true; } return false;
    }
    void togglePreview()
    {
        if (proc.isPreviewing() && proc.isPlaying()) { proc.endPreview(); preview.setButtonText ("Preview"); return; }
        commit(); mc::Audio tmp = proc.ed.audio; if (tmp.frames() == 0) return;
        const size_t s = proc.ed.rangeStart(), e = proc.ed.rangeEnd(); const size_t ne = mc::applyOp (tmp, op, s, e, proc.opParams[op]);
        proc.startPreview (tmp, s, ne); preview.setButtonText ("Stop");
    }
    void commit()
    {
        const auto& info = mc::kOps[op];
        for (int i = 0; i < info.nParams; ++i) { if (sliders[i] != nullptr) proc.opParams[op][i] = (float) sliders[i]->getValue(); if (combos[i] != nullptr) proc.opParams[op][i] = (float) (combos[i]->getSelectedId() - 1); }
    }
private:
    MeowcorderProcessor& proc; int op; std::function<void (bool)> done;
    std::vector<std::unique_ptr<juce::Component>> owned; juce::Label* labels[mc::kMaxOpParams] = {}; juce::Slider* sliders[mc::kMaxOpParams] = {}; juce::ComboBox* combos[mc::kMaxOpParams] = {};
    juce::TextButton apply { "Apply" }, cancel { "Cancel" }, preview { "Preview" };
};
} // namespace

// ---------------------------------------------------------------- everything on screen
class MeowcorderEditor::Content : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    Content (MeowcorderEditor& owner, MeowcorderProcessor& pr)
        : editor (owner), p (pr), wave (pr, [this] { refresh(); }), spec (pr, wave, [this] { changed(); }, [this] { refresh(); }), pitch (pr, wave, [this] { changed(); }), meters (pr), live (pr), ruler (pr, wave), overview (pr, wave)
    {
        setWantsKeyboardFocus (true); setSize (layout::kW, layout::kH);
        addAndMakeVisible (wave); addChildComponent (spec); addChildComponent (pitch); addAndMakeVisible (meters); addAndMakeVisible (live); addAndMakeVisible (ruler); addAndMakeVisible (overview);
        wave.onZoom = [this] { ruler.repaint(); overview.repaint(); spec.repaint(); pitch.repaint(); refresh(); };
        wave.unit = 0; wave.audioReplaced();
        setBoundsOf (live, 55, 272, 1155, 33); setBoundsOf (wave, 55, 337, 1155, 459); setBoundsOf (spec, 55, 337, 1155, 459); setBoundsOf (pitch, 55, 337, 1155, 459); setBoundsOf (ruler, 55, 797, 1155, 21); setBoundsOf (overview, 55, 818, 1155, 33); setBoundsOf (meters, 1224, 68, 100, 778);

        // menu bar
        const struct { const char* name; layout::R r; } menuDefs[] = { { "File", layout::mFile }, { "Edit", layout::mEdit }, { "Select", layout::mSelect }, { "View", layout::mView }, { "Spectral", layout::mSpectral },
                                                                         { "Warp", layout::mWarp }, { "Play", layout::mPlay }, { "Record", layout::mRecord }, { "Process", layout::mProcess }, { "Help", layout::mHelp } };
        int mi = 0;
        for (auto& d : menuDefs)
        {
            auto b = std::make_unique<TextBtn> (d.name, TextBtn::menu); b->setBounds (rc (d.r)); const int idx = mi++; auto* raw = b.get();
            b->onClick = [this, idx, raw] { showMenu (idx, raw); }; 
            addAndMakeVisible (*b); texts.push_back (std::move (b));
        }

        // transport
        add ("bLoop", layout::bLoop, [this] { toggleParam ("loop"); }); add ("bPlay", layout::bPlay, [this] { doPlay(); }); add ("bStop", layout::bStop, [this] { doStop(); });
        add ("bRec", layout::bRec, [this] { doRecord(); }, kPink);
        { auto b = std::make_unique<TextBtn> ("opts", TextBtn::menu); b->setBounds (rc (layout::bRecOpt)); b->label = juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbe")); b->setTooltip ("Record options"); b->onClick = [this] { showTakeMenu(); }; addAndMakeVisible (*b); texts.push_back (std::move (b)); }
        const struct { const char* n; layout::R r; } segs[] = { { "Now", layout::tNow }, { "On input", layout::tOnInput }, { "Count-in", layout::tCountIn } };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<TextBtn> (segs[i].n, TextBtn::segment); b->setBounds (rc (segs[i].r)); auto* raw = b.get(); segBtns[i] = raw;
            b->onClick = [this, i] { p.startSel = i; refresh(); };
            b->setTooltip (i == 0 ? "Recording starts the moment you press record." : i == 1 ? "Recording waits until the sound passes the threshold." : "Four clicks at the tempo of the Tempo knob, then recording starts.");
            addAndMakeVisible (*b); texts.push_back (std::move (b));
        }
        takeBtn = std::make_unique<TextBtn> ("take", TextBtn::take); takeBtn->setBounds (rc (layout::bTake)); takeBtn->onClick = [this] { showTakeMenu(); }; takeBtn->setTooltip ("What a new recording does to the sound. Click to change."); addAndMakeVisible (*takeBtn);

        // knobs
        const struct { const char* id; layout::R k; layout::R v; const char* tip; } kn[] = {
            { "threshold", layout::kThreshold, layout::vThreshold, "Threshold: level that starts a recording when Start is on Threshold. -80.0 dB to 0.0 dB, default -40.0 dB. Double click resets." },
            { "tempo", layout::kTempo, layout::vTempo, "Tempo: speed of the count-in click, 4/4. 40 bpm to 240 bpm, default 120 bpm." },
            { "preroll", layout::kPreroll, layout::vPreroll, "Pre-roll: how much sound from before the start is included in the recording. 0.0 ms to 3.00 s, default 200 ms." } };
        for (int i = 0; i < 3; ++i)
        {
            auto* prm = p.apvts.getParameter (kn[i].id); const juce::String id = kn[i].id;
            knobs[i] = std::make_unique<Knob> (*prm, [id] (float v) { return id == "threshold" ? juce::String (v, 1) + " dB" : id == "tempo" ? juce::String (v, 0) + " bpm" : (v >= 1000.f ? juce::String (v / 1000.f, 2) + " s" : juce::String (v, 0) + " ms"); }, nullptr);
            knobs[i]->setBounds (rc (kn[i].k)); knobs[i]->setTooltip (kn[i].tip); addAndMakeVisible (*knobs[i]); valueRects[i] = rc (kn[i].v);
        }

        // ops grid
        for (int o = 0; o < mc::opCount; ++o)
        {
            const auto& m = layout::kOpMeta[o]; auto b = std::make_unique<IconButton> (juce::String ("op") + juce::String (o), juce::Colour ((juce::uint32) (0xff000000 | m.colour)));
            b->setBounds (rc (layout::opCell (o))); b->setTooltip (juce::String (m.name) + (juce::String (m.shortcut).isNotEmpty() ? " (" + juce::String (m.shortcut) + ")" : "") + ": " + m.tip + " Click for settings, double click to apply at once.");
            b->onClick = [this, o] { openPanel (o); };
            b->addMouseListener (this, false); opButtons[o] = b.get(); addAndMakeVisible (*b); icons.push_back (std::move (b));
        }

        // toolbar
        add ("bNew", layout::bNew, [this] { doNew(); }); add ("bOpen", layout::bOpen, [this] { doOpen(); }); add ("bSave", layout::bSave, [this] { doSave(); });
        add ("bUndo", layout::bUndo, [this] { doUndo(); }); add ("bRedo", layout::bRedo, [this] { doRedo(); }); add ("bHist", layout::bHist, [this] { showHistory(); });
        add ("bCut", layout::bCut, [this] { doCut(); }); add ("bCopy", layout::bCopy, [this] { doCopy(); }); add ("bPaste", layout::bPaste, [this] { doPaste(); });
        add ("bDel", layout::bDel, [this] { doDelete(); }); add ("bCrop", layout::bCrop, [this] { doCrop(); });
        add ("bAll", layout::bAll, [this] { doSelectAll(); }); add ("bSnap", layout::bSnap, [this] { doSnap(); });
        add ("bZin", layout::bZin, [this] { wave.zoom (0.5, centreFrame()); }); add ("bZout", layout::bZout, [this] { wave.zoom (2.0, centreFrame()); });
        add ("bFit", layout::bFit, [this] { wave.fit(); }); add ("bSel", layout::bSel, [this] { wave.zoomToSelection(); });
        add ("bRepeat", layout::bRepeat, [this] { doRepeat(); }); add ("bGrab", layout::bGrab, [this] { doGrab(); });
        add ("bMono", layout::bMono, [this] { p.monoTakes = ! p.monoTakes; refresh(); }); add ("bInput", layout::bInput, [] {}); add ("bMon", layout::bMon, [this] { toggleParam ("monitor"); }, kGreen); add ("bSpec", layout::bSpec, [] {});
        for (auto* k : { "bInput", "bSpec" }) { byKey[k]->setActive (false); byKey[k]->setTooltip (juce::String (byKey[k]->getTooltip()).upToFirstOccurrenceOf (":", false, false) + ": " + kSoon); }
        add ("vComb", layout::vComb, [] {}); add ("vPen", layout::vPen, [] {}); byKey["vComb"]->setActive (false); byKey["vPen"]->setActive (false);
        {
            const struct { const char* n; layout::R r; } tabs[] = { { "Waveform", layout::tWave }, { "Spectral", { 161, 310, 61, 21 } }, { "Pitch", { 221, 310, 47, 21 } } };
            for (int i = 0; i < 3; ++i)
            {
                auto b = std::make_unique<TextBtn> (tabs[i].n, TextBtn::tab); b->setBounds (rc (tabs[i].r)); tabBtns[i] = b.get();
                b->onClick = [this, i] { setViewMode (i); };
                addAndMakeVisible (*b); texts.push_back (std::move (b));
            }
            const char* tn[] = { "Mouse", "Select", "Scissors", "Cutter", "Copy", "Paste", "Amp" };
            const char* tt[] = { "Click for the cursor, drag to select a time range.", "Drag a box over time and frequency. Use Cutter, Amp or Copy on the box from the Spectral menu.", "Drag across time: that slice is cut out and the gap closes.",
                                 "Drag a box: the sound inside it is removed, the length stays.", "Drag a box to copy that piece, then click with Paste where it should land.", "Click where the copied piece should go. Left edge is time, height is frequency.", "Drag a box: it gets the level change set in the Spectral menu." };
            int x = 352; for (int i = 0; i < 7; ++i)
            {
                auto b = std::make_unique<TextBtn> (tn[i], TextBtn::tab); const int wd = 58; b->setBounds (x, 310, wd, 21); x += wd + 2; b->setTooltip (tt[i]); const int t = i; b->onClick = [this, t] { spec.setTool (t); toolHint = toolHints[t]; refresh(); };
                b->setVisible (false); addChildComponent (*b); toolBtns.push_back (std::move (b)); toolHints[i] = tt[i];
            }
            spec.setToolBack = [this] (int) { refresh(); };
        }
        // info chips
        const struct { const char* n; layout::R r; int kind; } chipDefs[] = { { "ch", layout::cCh, 0 }, { "sr", layout::cSr, 0 }, { "bits", layout::cBits, 1 }, { "unit", layout::cUnit, 2 }, { "bpm", layout::cBpm, 0 }, { "sync", layout::cSync, 0 }, { "warp", layout::cWarp, 0 } };
        for (auto& c : chipDefs)
        {
            auto b = std::make_unique<TextBtn> (c.n, TextBtn::chip); b->setBounds (rc (c.r)); chips[c.n] = b.get(); const int kind = c.kind;
            if (kind == 1) { b->setTooltip ("File format of Save: 16 bit, 24 bit or 32 bit float. Click to change."); b->onClick = [this] { showBitsMenu(); }; }
            else if (kind == 2) { b->setTooltip ("How positions are shown: time, seconds or samples. Click to change."); b->onClick = [this] { showUnitMenu(); }; }
            else if (std::string (c.n) == "warp") { b->setTooltip ("Change the speed and the pitch of the sound. Opens the warp panel."); b->onClick = [this] { openWarp(); }; }
            else { b->setActive (false); b->setTooltip (std::string (c.n) == "ch" || std::string (c.n) == "sr" ? "Channels and sample rate of this sound." : std::string (c.n) == "bpm" ? "Tempo found in the hits of this sound." : kSoon); }
            addAndMakeVisible (*b); texts.push_back (std::move (b));
        }
        // header: UI size
        { auto b = std::make_unique<TextBtn> ("scale", TextBtn::chip); b->setBounds (rc (layout::bZoom)); chips["scale"] = b.get(); b->setTooltip ("Size of the window. You can also drag its corners."); b->onClick = [this] { showScaleMenu(); }; addAndMakeVisible (*b); texts.push_back (std::move (b)); }

        applyBubbles();
        ready = true; refresh (true); startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (kGround);
        for (int side = 0; side < 2; ++side)
        {
            const int x = side ? layout::kW - 27 : 0; juce::ColourGradient wood (juce::Colour (0xff3d2210), (float) x, 0, juce::Colour (0xff5a3419), (float) x + 27.f, 0, false); wood.addColour (0.4, juce::Colour (0xff7b4a26));
            g.setGradientFill (wood); g.fillRect (x, 0, 27, layout::kH); g.setColour (juce::Colours::black.withAlpha (0.14f)); for (int k = 2; k < 27; k += 4) g.drawVerticalLine (x + k, 0.f, (float) layout::kH);
        }
        auto panel = [&] (int x, int y, int w, int h) { g.setColour (kPanel); g.fillRoundedRectangle ((float) x, (float) y, (float) w, (float) h, 4.f); g.setColour (kLine); g.drawRoundedRectangle ((float) x + 0.5f, (float) y + 0.5f, (float) w - 1.f, (float) h - 1.f, 4.f, 1.f); };
        panel (39, 64, 1183, 155); panel (39, 266, 1183, 592); panel (1231, 64, 111, 807 + 0);
        // header
        if (auto xml = juce::parseXML (layout::kLogoSvg)) if (auto d = juce::Drawable::createFromSVG (*xml)) d->drawWithin (g, rc (layout::logo).toFloat(), juce::RectanglePlacement::centred, 1.f);
        g.setColour (kInk); g.setFont (titleFont (24.f)); g.drawText ("Meowcorder", 93, 14, 170, 30, juce::Justification::centredLeft);
        g.setColour (kDim); g.setFont (labelFont (13.f)); g.drawText ("STUDIO 87", 230, 22, 100, 18, juce::Justification::centredLeft);
        g.setColour (kDim); g.setFont (labelFont (14.f)); g.drawText (p.soundName + (p.dirty ? " *" : ""), 560, 15, 620, 31, juce::Justification::centredRight);
        // knob values and labels
        g.setFont (monoFont (11.5f)); const char* names[] = { "THRESHOLD", "TEMPO", "PRE-ROLL" };
        for (int i = 0; i < 3; ++i) { g.setColour (kDim); g.setFont (labelFont (11.f)); g.drawText (names[i], knobs[i]->getX() - 14, knobs[i]->getY() - 14, knobs[i]->getWidth() + 28, 12, juce::Justification::centred);
                                      g.setColour (kInk); g.setFont (monoFont (12.f)); g.drawText (knobs[i]->valueText(), valueRects[i].expanded (8, 0), juce::Justification::centred); }
        g.setColour (kDim); g.setFont (labelFont (12.5f)); g.drawFittedText ("Operations work on everything (nothing selected). Click one for its settings, double click or Shift+letter to apply at once.", 795, 134, 410, 36, juce::Justification::topLeft, 2);
        // info line
        const auto& a = p.ed.audio; const double sr = a.sr; int x = 43; const int y = 221;
        auto seg = [&] (const juce::String& k, const juce::String& v) { g.setFont (monoFont (12.5f)); g.setColour (kDim); const int kw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), k + " "); g.drawText (k, x, y, kw + 2, 18, juce::Justification::centredLeft); x += kw;
                                                                         g.setColour (kInk); const int vw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), v); g.drawText (v, x, y, vw + 4, 18, juce::Justification::centredLeft); x += vw + 26; };
        seg ("Length", fmtPos ((double) a.frames(), sr, wave.unit));
        seg ("Selection", p.ed.hasSelection() ? fmtPos ((double) p.ed.selA, sr, wave.unit) + " to " + fmtPos ((double) p.ed.selB, sr, wave.unit) + " (" + fmtPos ((double) (p.ed.selB - p.ed.selA), sr, wave.unit) + ")" : "none, operations use everything");
        seg ("Cursor", fmtPos ((double) p.ed.cursor, sr, wave.unit)); seg ("Peak", fmtDb (a.peak (0, a.frames())) + " dBFS");
        g.setColour (kDim); g.setFont (labelFont (12.f)); g.drawText ("VIEW", 43, 311, 40, 18, juce::Justification::centredLeft);
        if (viewMode == 1) { g.setFont (labelFont (12.5f)); g.drawText (toolHint, 770, 309, 430, 22, juce::Justification::centredLeft); }
    }

    // ---- actions
    void setViewMode (int m)
    {
        viewMode = m; wave.setVisible (m == 0); spec.setVisible (m == 1); pitch.setVisible (m == 2); for (auto& b : toolBtns) b->setVisible (m == 1);
        if (m == 1) toolHint = toolHints[juce::jlimit (0, 6, spec.tool)]; repaint (juce::Rectangle<int> (55, 300, 1155, 40)); refresh();
    }
    void openWarp()
    {
        if (warpPanel || panel || p.ed.audio.frames() == 0) return;
        warpPanel = std::make_unique<WarpPanel> (p, [this] (const mc::WarpOptions& o, bool previewOnly) { runWarp (o, previewOnly); }, [this] { auto old = std::move (warpPanel); removeChildComponent (old.get()); grabKeyboardFocus(); });
        warpPanel->setCentrePosition (layout::kW / 2, layout::kH / 2 - 40); addAndMakeVisible (*warpPanel); warpPanel->grabKeyboardFocus();
    }
    void runWarp (mc::WarpOptions o, bool previewOnly)
    {
        const size_t s = p.ed.rangeStart(), e = p.ed.rangeEnd(); if (e <= s) return;
        mc::Audio seg = p.ed.audio.slice (s, e);
        if (o.mode == 2) for (size_t h : mc::detectTransients (seg, 0.5f)) o.hits.push_back (h);
        const mc::Audio res = mc::warpAudio (seg, o);
        auto put = [&] (mc::Audio& a) { for (int c = 0; c < 2; ++c) { a.ch[c].erase (a.ch[c].begin() + (std::ptrdiff_t) s, a.ch[c].begin() + (std::ptrdiff_t) e); a.ch[c].insert (a.ch[c].begin() + (std::ptrdiff_t) s, res.ch[c].begin(), res.ch[c].end()); } };
        if (previewOnly) { mc::Audio tmp = p.ed.audio; put (tmp); p.startPreview (tmp, s, s + res.frames()); return; }
        const bool sel = p.ed.hasSelection();
        p.ed.transform ("Warp", [&] (mc::Audio& a) { put (a); if (sel) p.ed.selB = s + res.frames(); });
        changed(); wave.fit();
    }
    void doNew() { if (p.ed.audio.frames() > 0) { p.ed.setSelection (0, p.ed.audio.frames()); p.ed.deleteSelection(); } p.soundName = "new sound, not saved"; changed(); }
    void doOpen()
    {
        chooser = std::make_unique<juce::FileChooser> ("Open an audio file", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) openFile (fc.getResult()); });
    }
    void openFile (const juce::File& f) { juce::String err; if (! p.loadFile (f, err)) { juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Meowcorder", err); return; } wave.fit(); changed(); }
    void doSave()
    {
        if (p.ed.audio.frames() == 0) return;
        chooser = std::make_unique<juce::FileChooser> ("Save as WAV", juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("meowcorder.wav"), "*.wav");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult(); if (f == juce::File()) return; if (! f.hasFileExtension ("wav")) f = f.withFileExtension ("wav");
            juce::String err; if (! p.saveWav (f, p.exportBits, err)) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Meowcorder", err); else { p.soundName = f.getFileName(); p.dirty = false; refresh(); }
        });
    }
    void doUndo() { p.ed.undo(); changed(); } void doRedo() { p.ed.redo(); changed(); }
    void doCut() { p.ed.cut(); changed(); } void doCopy() { p.ed.copy(); refresh(); } void doPaste() { p.ed.paste(); changed(); }
    void doDelete() { p.ed.deleteSelection(); changed(); } void doCrop() { p.ed.cropToSelection(); wave.fit(); changed(); }
    void doSelectAll() { p.ed.setSelection (0, p.ed.audio.frames()); refresh(); }
    void doSelectNone() { p.ed.setCursor (p.ed.cursor); refresh(); }
    void doSnap() { wave.snapZero = ! wave.snapZero; if (wave.snapZero && p.ed.hasSelection()) { const size_t r = (size_t) std::max (32.0, p.ed.audio.sr * 0.01); p.ed.setSelection (p.ed.snapToZero (p.ed.selA, r), p.ed.snapToZero (p.ed.selB, r)); } refresh(); }
    void doRepeat() { if (p.ed.repeatLast()) changed(); }
    void doPlay() { if (p.isPlaying()) p.stopPlayback(); else p.play(); refresh(); }
    void doStop() { p.stopPlayback(); if (p.recState() != MeowcorderProcessor::recOff) { p.stopRecording(); wave.audioReplaced(); changed(); } refresh(); }
    void doRecord()
    {
        if (p.recState() == MeowcorderProcessor::recOff) p.startRecording (p.startSel == 0 ? MeowcorderProcessor::startNow : p.startSel == 1 ? MeowcorderProcessor::startOnInput : MeowcorderProcessor::startCountIn);
        else { p.stopRecording(); wave.audioReplaced(); changed(); }
        refresh();
    }
    void doGrab() { if (! p.grabLastSeconds()) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Meowcorder", "Nothing was heard yet. Play something through the plugin first."); else { wave.audioReplaced(); changed(); } }
    void toggleParam (const char* id) { if (auto* prm = p.apvts.getParameter (id)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.f : 1.f); prm->endChangeGesture(); } refresh(); }
    double centreFrame() const { return wave.viewStart + wave.viewLen * 0.5; }
    void changed() { p.audioChanged(); wave.audioReplaced(); wave.dirtyPeaks(); overview.repaint(); ruler.repaint(); refresh(); }

    void applyNow (int op) { if (p.ed.apply (op, p.opParams[op])) changed(); }
    void openPanel (int op)
    {
        if (panel) return;
        panel = std::make_unique<OpPanel> (p, op, [this, op] (bool ok) { auto old = std::move (panel); if (ok) applyNow (op); removeChildComponent (old.get()); grabKeyboardFocus(); });
        panel->setCentrePosition (layout::kW / 2, layout::kH / 2 - 40); addAndMakeVisible (*panel); panel->grabKeyboardFocus();
    }
    // double click on an op button: apply at once with the last settings (the single click already opened the panel; close it first)
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        for (int o = 0; o < mc::opCount; ++o) if (e.eventComponent == opButtons[o]) { if (panel) { removeChildComponent (panel.get()); panel.reset(); } applyNow (o); return; }
    }

    void showMenu (int which, juce::Component* anchor)
    {
        juce::PopupMenu m; const bool hasA = p.ed.audio.frames() > 0, sel = p.ed.hasSelection();
        auto item = [&] (juce::PopupMenu& mm, const juce::String& t, const juce::String& sc, bool en, std::function<void()> f) { juce::PopupMenu::Item it (t); it.setEnabled (en); it.shortcutKeyDescription = sc; it.setAction (std::move (f)); mm.addItem (it); };
        switch (which)
        {
            case 0: item (m, "New empty sound", "Alt+N", true, [this] { doNew(); }); item (m, "Open file...", "Ctrl+O", true, [this] { doOpen(); }); item (m, "Save as WAV...", "Ctrl+S", hasA, [this] { doSave(); }); break;
            case 1: item (m, "Undo", "Ctrl+Z", p.ed.canUndo(), [this] { doUndo(); }); item (m, "Redo", "Ctrl+Y", p.ed.canRedo(), [this] { doRedo(); }); item (m, "Repeat last operation", "R", p.ed.last.op >= 0, [this] { doRepeat(); }); m.addSeparator();
                    item (m, "Cut", "Ctrl+X", sel, [this] { doCut(); }); item (m, "Copy", "Ctrl+C", sel, [this] { doCopy(); }); item (m, "Paste", "Ctrl+V", p.ed.clipboard().frames() > 0, [this] { doPaste(); }); item (m, "Delete", "Del", sel, [this] { doDelete(); }); item (m, "Crop", "T", sel, [this] { doCrop(); }); break;
            case 2: item (m, "Select all", "Ctrl+A", hasA, [this] { doSelectAll(); }); item (m, "Select none", "", hasA, [this] { doSelectNone(); }); item (m, "Snap to zero crossings", "Z", true, [this] { doSnap(); }); break;
            case 3: item (m, "Zoom in", "+", hasA, [this] { wave.zoom (0.5, centreFrame()); }); item (m, "Zoom out", "-", hasA, [this] { wave.zoom (2.0, centreFrame()); }); item (m, "Show everything", "F", hasA, [this] { wave.fit(); }); item (m, "Zoom to selection", "S", sel, [this] { wave.zoomToSelection(); }); break;
            case 4:
            {
                item (m, viewMode == 1 ? "Show the waveform" : "Show the spectral view", "", true, [this] { setViewMode (viewMode == 1 ? 0 : 1); }); m.addSeparator();
                const char* tn[] = { "Mouse", "Select box", "Scissors", "Cutter", "Copy", "Paste", "Amp" };
                for (int i = 0; i < 7; ++i) { juce::PopupMenu::Item it (tn[i]); it.setTicked (spec.tool == i); it.setEnabled (hasA); it.setAction ([this, i] { setViewMode (1); spec.setTool (i); toolHint = toolHints[i]; refresh(); }); m.addItem (it); }
                m.addSeparator(); juce::PopupMenu amp; for (int db : { -24, -12, -6, 3, 6, 12 }) { juce::PopupMenu::Item it (juce::String (db > 0 ? "+" : "") + juce::String (db) + " dB"); it.setTicked ((int) spec.ampDb == db); it.setAction ([this, db] { spec.ampDb = db; }); amp.addItem (it); }
                m.addSubMenu ("Amp level", amp); m.addSeparator();
                item (m, "Cut the box", "", spec.hasBox, [this] { spec.applyBox (true); }); item (m, "Amp the box", "", spec.hasBox, [this] { spec.applyBox (false); }); item (m, "Copy the box", "", spec.hasBox, [this] { spec.copyBox(); });
                break;
            }
            case 5: item (m, "Warp and transpose...", "", hasA, [this] { openWarp(); }); break;
            case 6: item (m, p.isPlaying() ? "Stop" : "Play", "Space", hasA, [this] { doPlay(); }); item (m, "Loop", "L", true, [this] { toggleParam ("loop"); }); break;
            case 7: item (m, p.recState() == MeowcorderProcessor::recOff ? "Record" : "Stop recording", "Enter", true, [this] { doRecord(); }); item (m, "Grab the last seconds", "G", true, [this] { doGrab(); }); m.addSeparator(); showTakeItems (m); break;
            case 8:
            {
                juce::StringArray groups; for (int o = 0; o < mc::opCount; ++o) groups.addIfNotAlreadyThere (mc::kOps[o].group);
                for (auto& gname : groups) { juce::PopupMenu sub; for (int o = 0; o < mc::opCount; ++o) if (gname == mc::kOps[o].group) item (sub, mc::kOps[o].name, layout::kOpMeta[o].shortcut, hasA, [this, o] { openPanel (o); }); m.addSubMenu (gname, sub); }
                break;
            }
            case 9: item (m, p.infoBubbles ? "Info bubbles: on" : "Info bubbles: off", "", true, [this] { p.infoBubbles = ! p.infoBubbles; applyBubbles(); }); m.addSeparator(); item (m, "Keyboard shortcuts", "", true, [this] { showShortcuts(); }); item (m, "About Meowcorder", "", true, [this] { showAbout(); }); break;
            default: break;
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (anchor));
    }
    void showTakeItems (juce::PopupMenu& m)
    {
        const char* names[] = { "Replace everything", "Append at the end", "Punch in over the selection", "Insert at the selection" };
        for (int i = 0; i < 4; ++i) { juce::PopupMenu::Item it (names[i]); it.setTicked (p.takeMode == i); it.setAction ([this, i] { p.takeMode = i; refresh(); }); m.addItem (it); }
    }
    void showHistory()
    {
        juce::PopupMenu m; const size_t steps = p.ed.historySteps();
        for (size_t k = steps; k-- > 0;)
        {
            juce::PopupMenu::Item it (juce::String ((int) k) + "  " + (k == 0 ? "Start" : p.ed.historyLabel (k))); it.setTicked (k == p.ed.historyPos());
            it.setAction ([this, k] { p.ed.jumpToStep (k); changed(); }); m.addItem (it);
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (byKey["bHist"]).withMaximumNumColumns (1));
    }
    void showTakeMenu() { juce::PopupMenu m; showTakeItems (m); m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (takeBtn.get())); }
    void showBitsMenu() { juce::PopupMenu m; for (int b : { 16, 24, 32 }) { juce::PopupMenu::Item it (b == 32 ? "32 bit float" : juce::String (b) + " bit"); it.setTicked (p.exportBits == b); it.setAction ([this, b] { p.exportBits = b; refresh(); }); m.addItem (it); } m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (chips["bits"])); }
    void showUnitMenu() { juce::PopupMenu m; const char* n[] = { "Time", "Seconds", "Samples" }; for (int i = 0; i < 3; ++i) { juce::PopupMenu::Item it (n[i]); it.setTicked (wave.unit == i); it.setAction ([this, i] { wave.unit = i; refresh(); ruler.repaint(); }); m.addItem (it); } m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (chips["unit"])); }
    void showScaleMenu() { juce::PopupMenu m; for (int s : { 75, 100, 125, 150, 200 }) { juce::PopupMenu::Item it (juce::String (s) + "%"); it.setTicked (p.uiScalePercent == s); it.setAction ([this, s] { editor.setScalePercent (s); }); m.addItem (it); } m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (chips["scale"])); }
    void applyBubbles() { if (p.infoBubbles) { if (! tips) tips = std::make_unique<juce::TooltipWindow> (this, 600); } else tips.reset(); }
    void showShortcuts()
    {
        juce::String t = "Space: play or stop\nEnter: record or stop recording\nL: loop\nG: grab the last seconds\nCtrl+Z / Ctrl+Y: undo / redo\nR: repeat last operation\nCtrl+X, C, V, Del: cut, copy, paste, delete\nT: crop   Z: snap to zero   Ctrl+A: select all\n+ / -: zoom   F: show everything   S: zoom to selection\nMouse wheel: scroll, Ctrl+wheel: zoom\nShift+letter: apply an operation (see Process menu)";
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Keyboard shortcuts", t);
    }
    void showAbout()
    {
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::InfoIcon).withTitle ("About Meowcorder").withMessage (
            "Meowcorder " + juce::String (JucePlugin_VersionString) + " by Studio 87\nA free audio recorder and editor.\n\nFree software under the GNU AGPL v3. It works offline: no account, no network, nothing is collected or sent anywhere.\n"
            "The sound you record or open lives inside the plugin while it is open. Save it as a WAV file to keep it.\nSource and manual: github.com/SpeakingColors/Meowcorder\n\n" + juce::String (studio87link::aboutLine()))
            .withButton ("Support on Ko-fi").withButton (studio87link::buttonText()).withButton (studio87link::githubButtonText()).withButton ("Close"),
            [] (int pick) { if (pick == 1) juce::URL ("https://ko-fi.com/studio87").launchInDefaultBrowser(); else if (pick == 2) studio87link::openUpdater ("Meowcorder"); else if (pick == 3) studio87link::openGitHub ("Meowcorder"); });
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        const auto mods = k.getModifiers(); const int c = k.getKeyCode(); const bool ctrl = mods.isCtrlDown() || mods.isCommandDown();
        if (panel || warpPanel) return false;
        if (ctrl && c == 'Z') { if (mods.isShiftDown()) doRepeat(); else doUndo(); return true; }
        if (ctrl && c == 'Y') { doRedo(); return true; } if (ctrl && c == 'X') { doCut(); return true; } if (ctrl && c == 'C') { doCopy(); return true; } if (ctrl && c == 'V') { doPaste(); return true; }
        if (ctrl && c == 'A') { doSelectAll(); return true; } if (ctrl && c == 'O') { doOpen(); return true; } if (ctrl && c == 'S') { doSave(); return true; }
        if (k == juce::KeyPress::spaceKey) { doPlay(); return true; } if (k == juce::KeyPress::returnKey) { doRecord(); return true; }
        if (k == juce::KeyPress::deleteKey || k == juce::KeyPress::backspaceKey) { doDelete(); return true; }
        if (mods.isShiftDown() && ! ctrl)
            for (int o = 0; o < mc::opCount; ++o) { const juce::String sc = layout::kOpMeta[o].shortcut; if (sc.startsWith ("Shift+") && sc.length() == 7 && (int) sc[6] == c) { applyNow (o); return true; } }
        if (! ctrl && ! mods.isShiftDown() && ! mods.isAltDown())
        {
            if (c == 'L') { toggleParam ("loop"); return true; } if (c == 'G') { doGrab(); return true; } if (c == 'R') { doRepeat(); return true; } if (c == 'T') { doCrop(); return true; }
            if (c == 'Z') { doSnap(); return true; } if (c == 'F') { wave.fit(); return true; } if (c == 'S') { wave.zoomToSelection(); return true; }
            if (c == '+' || c == '=') { wave.zoom (0.5, centreFrame()); return true; } if (c == '-') { wave.zoom (2.0, centreFrame()); return true; }
        }
        return false;
    }
    bool isInterestedInFileDrag (const juce::StringArray& files) override { for (auto& f : files) if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg")) return true; return false; }
    void filesDropped (const juce::StringArray& files, int, int) override { if (files.size() > 0) openFile (juce::File (files[0])); }
    void mouseDown (const juce::MouseEvent&) override { grabKeyboardFocus(); }

private:
    void timerCallback() override { refresh(); }
    void refresh (bool force = false)
    {
        if (! ready) return;
        const bool rec = p.recState() != MeowcorderProcessor::recOff, has = p.ed.audio.frames() > 0, sel = p.ed.hasSelection();
        auto on = [&] (const char* k, bool v) { if (byKey.count (k)) byKey[k]->setOn (v); }; auto act = [&] (const char* k, bool v) { if (byKey.count (k)) byKey[k]->setActive (v); };
        on ("bRec", rec); on ("bPlay", p.isPlaying()); on ("bLoop", p.apvts.getRawParameterValue ("loop")->load() > 0.5f); on ("bMon", p.apvts.getRawParameterValue ("monitor")->load() > 0.5f); on ("bSnap", wave.snapZero); on ("bMono", p.monoTakes); act ("bHist", p.ed.historySteps() > 1);
        act ("bUndo", p.ed.canUndo()); act ("bRedo", p.ed.canRedo()); act ("bCut", sel); act ("bCopy", sel); act ("bPaste", p.ed.clipboard().frames() > 0); act ("bDel", sel); act ("bCrop", sel);
        act ("bAll", has); act ("bZin", has); act ("bZout", has); act ("bFit", has); act ("bSel", sel); act ("bRepeat", p.ed.last.op >= 0); act ("bSave", has); act ("bPlay", has);
        for (int i = 0; i < 3; ++i) segBtns[i]->setOn (p.startSel == i); for (int i = 0; i < 3; ++i) if (tabBtns[i] != nullptr) tabBtns[i]->setOn (viewMode == i); for (size_t i = 0; i < toolBtns.size(); ++i) toolBtns[i]->setOn (spec.tool == (int) i);
        const char* takes[] = { "Replace", "Append", "Punch in", "Insert" }; takeBtn->label = takes[juce::jlimit (0, 3, p.takeMode)]; takeBtn->repaint();
        chips["ch"]->label = "Stereo"; chips["sr"]->label = juce::String (p.ed.audio.sr / 1000.0, p.ed.audio.sr == 44100.0 ? 1 : 0) + " kHz"; chips["bits"]->label = p.exportBits == 32 ? "32 bit float" : juce::String (p.exportBits) + " bit";
        chips["unit"]->label = wave.unit == 0 ? "Time" : wave.unit == 1 ? "Seconds" : "Samples"; if (bpmKey != p.version) { bpmKey = p.version; bpmValue = mc::detectBpm (p.ed.audio); } chips["bpm"]->label = bpmValue > 0.f ? juce::String (bpmValue, 1) + " bpm" : juce::String ("no tempo"); chips["sync"]->label = "Free"; chips["warp"]->label = "Warp >";
        chips["scale"]->label = juce::String (p.uiScalePercent) + "%"; for (auto& kv : chips) kv.second->repaint();
        meters.thresholdDb = live.thresholdDb = p.apvts.getRawParameterValue ("threshold")->load(); meters.repaint(); live.repaint();
        if (p.isPlaying() || force) { wave.repaint(); if (viewMode == 1) spec.repaint(); if (viewMode == 2) pitch.repaint(); }
        repaint (juce::Rectangle<int> (39, 64, 1183, 155)); repaint (juce::Rectangle<int> (39, 222, 1183, 28));
    }
    IconButton* add (const juce::String& key, const layout::R& r, std::function<void()> onClick, juce::Colour accent = kBlue)
    {
        auto b = std::make_unique<IconButton> (key, accent); b->setBounds (rc (r)); b->onClick = std::move (onClick);
        for (auto& t : layout::kTips) if (key == t.key) b->setTooltip (juce::String (t.name) + (juce::String (t.shortcut).isNotEmpty() ? " (" + juce::String (t.shortcut) + ")" : "") + ": " + t.text);
        byKey[key] = b.get(); addAndMakeVisible (*b); icons.push_back (std::move (b)); return byKey[key];
    }
    static void setBoundsOf (juce::Component& c, int x, int y, int w, int h) { c.setBounds (x, y, w, h); }

    bool ready = false; uint64_t bpmKey = 0; float bpmValue = 0.f;
    MeowcorderEditor& editor; MeowcorderProcessor& p;
    WaveView wave; SpecView spec; PitchView pitch; Meters meters; LiveStrip live; Ruler ruler; Overview overview; int viewMode = 0;
    std::vector<std::unique_ptr<TextBtn>> toolBtns; TextBtn* tabBtns[3] = {}; juce::String toolHint; juce::String toolHints[7];
    std::vector<std::unique_ptr<IconButton>> icons; std::vector<std::unique_ptr<TextBtn>> texts; std::map<juce::String, IconButton*> byKey; std::map<juce::String, TextBtn*> chips;
    IconButton* opButtons[mc::opCount] = {}; TextBtn* segBtns[3] = {}; std::unique_ptr<TextBtn> takeBtn; std::unique_ptr<Knob> knobs[3]; juce::Rectangle<int> valueRects[3];
    std::unique_ptr<OpPanel> panel; std::unique_ptr<WarpPanel> warpPanel; std::unique_ptr<juce::FileChooser> chooser; std::unique_ptr<juce::TooltipWindow> tips;
};

// ---------------------------------------------------------------- editor shell
MeowcorderEditor::MeowcorderEditor (MeowcorderProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    content = std::make_unique<Content> (*this, p); addAndMakeVisible (*content);
    constrainer.setFixedAspectRatio ((double) layout::kW / layout::kH); constrainer.setSizeLimits (layout::kW / 2, layout::kH / 2, layout::kW * 2, layout::kH * 2);
    setConstrainer (&constrainer); setResizable (true, true);
    setSize (layout::kW * proc.uiScalePercent / 100, layout::kH * proc.uiScalePercent / 100);
}

MeowcorderEditor::~MeowcorderEditor() = default;

void MeowcorderEditor::resized()
{
    content->setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) layout::kW));
    content->setBounds (0, 0, layout::kW, layout::kH);
}

void MeowcorderEditor::setScalePercent (int percent) { proc.uiScalePercent = juce::jlimit (50, 200, percent); setSize (layout::kW * proc.uiScalePercent / 100, layout::kH * proc.uiScalePercent / 100); }

void MeowcorderEditor::showView (int mode) { content->setViewMode (mode); }
