#include "MeowEditor.h"
#include <catui/CatLookAndFeel.h>
#include <catui/RenderGate.h>
#include <catui/Logo.h>
#include <cmath>

using juce::Colour; using juce::Graphics; using juce::String;

namespace {
const Colour kBg(0xff101114), kPanel(0xff17181c), kEdge(0xff2c2e35), kText(0xffe8e6e1), kDim(0xff8c909a), kGold(0xffffd100), kPink(0xffff4fa0), kTeal(0xff12d6b4), kRed(0xffff3b4a), kBlue(0xff6ec6ff);
constexpr int kCheek = 30, kContentW = 1140, kBaseW = kContentW + 2 * kCheek, kBaseH = 720;
juce::Font mono(float h, bool bold = false) { return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain)); }
juce::Font sans(float h, bool bold = false) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }

class Pill : public juce::Button
{
public:
    explicit Pill(const String& t, Colour c = kPink) : juce::Button(t), col(c) {}
    Colour col; bool flash = false;
    void paintButton(Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f); const bool on = getToggleState() || flash;
        g.setColour(on ? col.withAlpha(0.22f) : (over ? Colour(0xff25272e) : Colour(0xff1b1c21))); g.fillRoundedRectangle(r, 3.f);
        g.setColour(on ? col : Colour(0xff3a3d46)); g.drawRoundedRectangle(r, 3.f, on ? 1.5f : 1.f);
        g.setColour(on ? col : (down ? kGold : kText)); g.setFont(sans(13.f, true)); g.drawFittedText(getButtonText().toUpperCase(), getLocalBounds().reduced(3, 0), juce::Justification::centred, 1);
    }
};

String timeText(double sec, bool fine = true)
{
    const int m = (int) (sec / 60.0); const double s = sec - m * 60.0;
    return String(m) + ":" + String(s, fine ? 2 : 0).paddedLeft('0', fine ? 5 : 2);
}

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
        if (frames() == 0) return; dragFrom = (size_t) juce::jlimit(0.0, (double) frames(), xToFrame((float) e.x));
        if (e.mods.isShiftDown() && (p.doc.hasSelection() || p.doc.cursor > 0)) { const size_t a = p.doc.hasSelection() ? p.doc.selA : p.doc.cursor; p.doc.setSelection(a, dragFrom); }
        else p.doc.setCursor(dragFrom);
        if (onSelection) onSelection(); repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (frames() == 0) return; const size_t f = (size_t) juce::jlimit(0.0, (double) frames(), xToFrame((float) e.x));
        if (e.getDistanceFromDragStart() > 2) p.doc.setSelection(dragFrom, f); if (onSelection) onSelection(); repaint();
    }
    void mouseDoubleClick(const juce::MouseEvent&) override { if (frames() > 0) { p.doc.setSelection(0, frames()); if (onSelection) onSelection(); repaint(); } }
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
                    const float fr = 1.f - (float) y / (float) h; const int bin = juce::jlimit(0, sp.bins - 1, (int) (std::pow(fr, 2.2f) * (float) sp.bins));
                    for (int x = 0; x < w; ++x)
                    {
                        const int fi = juce::jlimit(0, sp.frames - 1, (int) ((double) x / w * sp.frames)); const float v = juce::jlimit(0.f, 1.f, (sp.at(fi, bin) + 90.f) / 80.f);
                        spec.setPixelAt(x, y, Colour::fromHSV(0.72f - 0.72f * v, 0.85f, std::pow(v, 0.8f), 1.f));
                    }
                }
        }
        g.drawImageAt(spec, 0, 0); g.setColour(kDim); g.setFont(mono(10.f)); g.drawText("Spectral view (read only for now)", 6, 4, 260, 12, juce::Justification::left);
    }
    size_t dragFrom = 0;
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
    Impl(MeowEditor& o, MeowProcessor& pr) : owner(o), p(pr), gate(o), display(pr), inM("IN"), outM("OUT"), drag(pr, &o)
    {
        o.setLookAndFeel(&laf); o.addAndMakeVisible(content); auto& c = content;
        for (auto* b : { &mFile, &mEdit, &mProcess, &mView, &mRecord }) c.addAndMakeVisible(*b);
        for (auto* b : { &rec, &stop, &play, &loop, &monitor, &undo, &redo, &guide, &about }) c.addAndMakeVisible(*b);
        c.addAndMakeVisible(display); c.addAndMakeVisible(inM); c.addAndMakeVisible(outM); c.addAndMakeVisible(drag); c.addAndMakeVisible(info); c.addAndMakeVisible(timeL);
        for (auto* l : { &info, &timeL }) { l->setColour(juce::Label::textColourId, kDim); l->setFont(mono(13.f)); }
        timeL.setFont(mono(22.f, true)); timeL.setColour(juce::Label::textColourId, kText); timeL.setJustificationType(juce::Justification::centredLeft);
        auto knob = [&](juce::Slider& s, const char* id, const char* tip, const String& suffix) { s.setSliderStyle(juce::Slider::RotaryVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 16); s.setTooltip(tip); s.setTextValueSuffix(suffix);
            s.setColour(juce::Slider::textBoxTextColourId, kTeal); s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); c.addAndMakeVisible(s); atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, id, s)); };
        knob(thr, "thresh", "Level that starts a recording in On input mode.", " dB"); knob(pre, "preroll", "Sound from just before you pressed record that is kept in the take.", " ms");
        take.addItemList(dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter("take"))->choices, 1); take.setTooltip("Where a new recording goes: replace the sound, add it at the end, punch over the selection or insert at the selection."); c.addAndMakeVisible(take);
        takeA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "take", take);
        loopA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "loop", loop); monA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "monitor", monitor);
        loop.setClickingTogglesState(true); monitor.setClickingTogglesState(true);
        viewW.setClickingTogglesState(false); viewS.setClickingTogglesState(false); c.addAndMakeVisible(viewW); c.addAndMakeVisible(viewS); viewW.setToggleState(true, juce::dontSendNotification);
        viewW.onClick = [this] { display.spectral = false; viewW.setToggleState(true, juce::dontSendNotification); viewS.setToggleState(false, juce::dontSendNotification); display.repaint(); };
        viewS.onClick = [this] { display.spectral = true; viewS.setToggleState(true, juce::dontSendNotification); viewW.setToggleState(false, juce::dontSendNotification); display.repaint(); };
        rec.setTooltip("Record (R). Uses the start mode from the Record menu. Press again to stop and keep the take."); stop.setTooltip("Stop (Space or Esc)."); play.setTooltip("Play the selection, or from the cursor (Space).");
        loop.setTooltip("Repeat playback."); monitor.setTooltip("Hear the input while recording."); undo.setTooltip("Undo (Ctrl+Z)."); redo.setTooltip("Redo (Ctrl+Y)."); guide.setTooltip("Open the manual."); about.setTooltip("Free and open source. Opens the Ko-fi page.");
        mFile.onClick = [this] { menu(mFile, fileMenu()); }; mEdit.onClick = [this] { menu(mEdit, editMenu()); }; mProcess.onClick = [this] { menu(mProcess, processMenu()); };
        mView.onClick = [this] { menu(mView, viewMenu()); }; mRecord.onClick = [this] { menu(mRecord, recordMenu()); };
        rec.onClick = [this] { toggleRecord(); }; stop.onClick = [this] { doStop(); }; play.onClick = [this] { doPlay(); };
        undo.onClick = [this] { doUndo(); }; redo.onClick = [this] { doRedo(); };
        guide.onClick = [] { juce::URL("https://github.com/SpeakingColors/Meowcorder/blob/main/docs/Meowcorder_manual.pdf").launchInDefaultBrowser(); };
        about.onClick = [] { juce::URL("https://ko-fi.com/studio87").launchInDefaultBrowser(); };
        display.onSelection = [this] { refreshInfo(); };
        tip = std::make_unique<juce::TooltipWindow>(&o, 500);
        constrainer.setFixedAspectRatio((double) kBaseW / kBaseH); constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
        o.setConstrainer(&constrainer); o.setResizable(true, true); o.setWantsKeyboardFocus(true); o.setSize(kBaseW, kBaseH);
        gate.onFrame = [this] { tick(); }; gate.start(20);
        if (const char* f = std::getenv("MEOW_DEMO")) demo(String(f).getIntValue());   // documentation tool for screenshots, never set for users
    }
    ~Impl() { owner.setLookAndFeel(nullptr); }

    // ---- actions ----
    void doStop() { if (p.isRecording()) p.stopRecord(); p.stopPlay(); }
    void doPlay() { if (p.isPlaying()) p.stopPlay(); else { if (p.isRecording()) p.stopRecord(); p.play(); } }
    void toggleRecord() { if (p.isRecording()) p.stopRecord(); else { p.startRecord(recMode); } }
    void doUndo() { p.stopPlay(); p.doc.undo(); p.changed(); } void doRedo() { p.stopPlay(); p.doc.redo(); p.changed(); }
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
    void openOp(int op)
    {
        if (p.doc.audio.frames() == 0) return;
        auto panel = std::make_unique<OpPanel>(p, op, [this] { juce::Component::SafePointer<juce::Component> sp(box); if (sp != nullptr) { box = nullptr; if (auto* cb = dynamic_cast<juce::CallOutBox*>(sp.getComponent())) cb->dismiss(); } });
        auto& cb = juce::CallOutBox::launchAsynchronously(std::move(panel), owner.getScreenBounds().withSizeKeepingCentre(10, 10), nullptr); box = &cb;
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
        item("Delete", "Del", d.hasSelection(), chg([&d] { d.deleteSelection(); })); item("Crop to selection", "Ctrl+T", d.hasSelection(), chg([&d] { d.cropToSelection(); })); m.addSeparator();
        item("Select all", "Ctrl+A", d.audio.frames() > 0, [this] { p.doc.setSelection(0, p.doc.audio.frames()); display.repaint(); refreshInfo(); }); return m;
    }
    juce::PopupMenu processMenu()
    {
        juce::PopupMenu m; String last;
        for (int i = 0; i < mc::opCount; ++i)
        {
            const String g(mc::kOps[i].group); if (g != last) { if (last.isNotEmpty()) m.addSeparator(); m.addSectionHeader(g); last = g; }
            m.addItem(mc::kOps[i].name, p.doc.audio.frames() > 0, false, [this, i] { openOp(i); });
        }
        return m;
    }
    juce::PopupMenu viewMenu()
    {
        juce::PopupMenu m; m.addItem("Waveform", true, !display.spectral, [this] { viewW.onClick(); }); m.addItem("Spectral", true, display.spectral, [this] { viewS.onClick(); }); m.addSeparator();
        m.addItem("Zoom in", [this] { display.zoom(0.5, display.getWidth() * 0.5f); }); m.addItem("Zoom out", [this] { display.zoom(2.0, display.getWidth() * 0.5f); });
        m.addItem("Zoom to selection", p.doc.hasSelection(), false, [this] { display.zoomToSelection(); }); m.addItem("Show all", [this] { display.fitAll(); }); return m;
    }
    juce::PopupMenu recordMenu()
    {
        juce::PopupMenu m; m.addSectionHeader("Start the recording");
        m.addItem("Right away", true, recMode == MeowProcessor::RecNow, [this] { recMode = MeowProcessor::RecNow; });
        m.addItem("When the input is heard", true, recMode == MeowProcessor::RecOnInput, [this] { recMode = MeowProcessor::RecOnInput; });
        m.addItem("After a 1 bar count-in (4/4)", true, recMode == MeowProcessor::RecCountIn, [this] { recMode = MeowProcessor::RecCountIn; }); m.addSeparator();
        m.addItem("Record now", [this] { if (!p.isRecording()) p.startRecord(recMode); }); return m;
    }
    bool keys(const juce::KeyPress& k)
    {
        const auto m = k.getModifiers(); const int c = k.getKeyCode(); const bool ctrl = m.isCommandDown() || m.isCtrlDown();
        if (c == juce::KeyPress::spaceKey) { doPlay(); return true; } if (c == juce::KeyPress::escapeKey) { doStop(); return true; }
        if (!ctrl && (c == 'R' || c == 'r')) { toggleRecord(); return true; }
        if (ctrl && (c == 'Z' || c == 'z')) { if (m.isShiftDown()) { p.doc.repeatLast(); p.changed(); } else doUndo(); return true; }
        if (ctrl && (c == 'Y' || c == 'y')) { doRedo(); return true; }
        if (ctrl && (c == 'X' || c == 'x')) { p.stopPlay(); p.doc.cut(); p.changed(); return true; }
        if (ctrl && (c == 'C' || c == 'c')) { p.doc.copy(); return true; }
        if (ctrl && (c == 'V' || c == 'v')) { p.stopPlay(); p.doc.paste(); p.changed(); return true; }
        if (ctrl && (c == 'T' || c == 't')) { p.stopPlay(); p.doc.cropToSelection(); p.changed(); return true; }
        if (ctrl && (c == 'A' || c == 'a')) { p.doc.setSelection(0, p.doc.audio.frames()); display.repaint(); refreshInfo(); return true; }
        if (c == juce::KeyPress::deleteKey || c == juce::KeyPress::backspaceKey) { p.stopPlay(); p.doc.deleteSelection(); p.changed(); return true; }
        return false;
    }

    void refreshInfo()
    {
        const auto& a = p.doc.audio; if (a.frames() == 0) { info.setText("No sound", juce::dontSendNotification); return; }
        String s = String((int) a.sr) + " Hz   stereo   " + timeText((double) a.frames() / a.sr) + " s   peak " + String(mc::linToDb(a.peak(0, a.frames())), 1) + " dB";
        if (p.doc.hasSelection()) s += "   selection " + timeText((double) p.doc.selA / a.sr) + " to " + timeText((double) p.doc.selB / a.sr);
        info.setText(s, juce::dontSendNotification);
    }
    void tick()
    {
        inM.set(p.inPeak.load()); outM.set(p.outPeak.load());
        if (seenVer != p.version()) { seenVer = p.version(); refreshInfo(); display.repaint(); }
        const bool r = p.isRecording(), arm = p.isArmedOnly();
        rec.flash = r && !arm; rec.col = kRed; rec.setButtonText(p.isCountingIn() ? "Count-in" : (arm ? "Armed" : (r ? "Recording" : "Record"))); rec.repaint();
        play.setButtonText(p.isPlaying() ? "Pause" : "Play"); play.setToggleState(p.isPlaying(), juce::dontSendNotification);
        timeL.setText(r ? timeText(p.recordedSeconds()) : (p.isPlaying() ? timeText(p.playPosFrames() / std::max(1.0, p.doc.audio.sr)) : timeText((double) p.doc.cursor / std::max(1.0, p.doc.audio.sr))), juce::dontSendNotification);
        if (p.isPlaying() || r) display.repaint();
        if (status.isNotEmpty() && ++statusAge > 60) { status = {}; statusAge = 0; } content.repaint(juce::Rectangle<int>(14, 690, 900, 20));
    }
    void demo(int what)
    {
        mc::Audio a; a.sr = 48000; const size_t n = (size_t) (a.sr * 3.2); a.resize(n); mc::Rng rng;
        for (size_t i = 0; i < n; ++i) { const double t = (double) i / a.sr; const double ph = std::fmod(t, 0.8); const double env = std::exp(-ph * 7.0); const double f = 55.0 + 180.0 * std::exp(-ph * 30.0);
            const float s = (float) (std::sin(2.0 * 3.14159265 * f * ph) * env * 0.8 + rng.bi() * std::exp(-ph * 40.0) * 0.3); a.ch[0][i] = s; a.ch[1][i] = s * 0.92f; }
        p.doc.load(a); p.doc.setSelection((size_t) (a.sr * 0.7), (size_t) (a.sr * 2.1)); p.changed(); display.fitAll();
        if (what == 2) { display.spectral = true; viewS.setToggleState(true, juce::dontSendNotification); viewW.setToggleState(false, juce::dontSendNotification); }
        if (const char* out = std::getenv("MEOW_SNAPSHOT")) juce::Timer::callAfterDelay(2500, [this, path = String(out)] {
            auto img = owner.createComponentSnapshot(owner.getLocalBounds(), true, 1.5f); juce::File f(path); f.deleteFile(); juce::FileOutputStream os(f); if (os.openedOk()) juce::PNGImageFormat().writeImageToStream(img, os);
            juce::MessageManager::getInstance()->stopDispatchLoop(); });
    }

    float scale() const { return (float) owner.getWidth() / (float) kBaseW; }
    void layout()
    {
        int x = 14; for (auto* b : { &mFile, &mEdit, &mProcess, &mView, &mRecord }) { b->setBounds(x, 92, 96, 30); x += 102; }
        guide.setBounds(900, 20, 90, 34); about.setBounds(1000, 20, 130, 34); viewW.setBounds(560, 92, 110, 30); viewS.setBounds(676, 92, 110, 30);
        rec.setBounds(14, 134, 130, 40); stop.setBounds(150, 134, 80, 40); play.setBounds(236, 134, 90, 40); loop.setBounds(332, 134, 70, 40); monitor.setBounds(408, 134, 90, 40); undo.setBounds(504, 134, 70, 40); redo.setBounds(580, 134, 70, 40);
        timeL.setBounds(668, 134, 160, 40); take.setBounds(836, 138, 170, 32); thr.setBounds(922, 82, 0, 0);
        thr.setBounds(1014, 126, 56, 70); pre.setBounds(1074, 126, 56, 70);
        display.setBounds(14, 202, 1010, 440); inM.setBounds(1036, 210, 52, 438); outM.setBounds(1088, 210, 52, 438);
        info.setBounds(14, 650, 880, 22); drag.setBounds(940, 652, 186, 30);
    }
    void paintContent(Graphics& g)
    {
        g.fillAll(kBg); catui::drawCatHead(g, { 22, 14, 60, 56 }, kText, kBg);
        g.setColour(kText); g.setFont(sans(34.f, true)); g.drawText("MEOWCORDER", 96, 8, 400, 46, juce::Justification::left);
        g.setColour(kGold); g.setFont(sans(14.f, true)); g.drawText("STUDIO 87", 98, 52, 200, 20, juce::Justification::left);
        g.setColour(kEdge); g.drawHorizontalLine(84, 14.f, 1126.f);
        g.setColour(kDim); g.setFont(sans(11.f, true)); g.drawText("VIEW", 520, 92, 36, 30, juce::Justification::right);
        g.drawText("THRESHOLD", 1004, 118, 70, 12, juce::Justification::centred); g.drawText("PRE-ROLL", 1066, 118, 70, 12, juce::Justification::centred);
        g.setColour(kDim); g.setFont(mono(11.f)); g.drawText(status.isNotEmpty() ? status : String("STUDIO 87 | MEOWCORDER | start mode: ") + (recMode == 0 ? "right away" : (recMode == 1 ? "when the input is heard" : "1 bar count-in")) + " | hover a control for help", 14, 690, 900, 16, juce::Justification::left);
    }

    struct Content : juce::Component { Impl& i; explicit Content(Impl& x) : i(x) {} void paint(Graphics& g) override { i.paintContent(g); } } content { *this };
    MeowEditor& owner; MeowProcessor& p; catui::CatLookAndFeel laf; juce::ComponentBoundsConstrainer constrainer; catui::RenderGate gate;
    Pill mFile { "File", kTeal }, mEdit { "Edit", kTeal }, mProcess { "Process", kTeal }, mView { "View", kTeal }, mRecord { "Record", kTeal };
    Pill rec { "Record", kRed }, stop { "Stop", kText }, play { "Play", kGold }, loop { "Loop", kGold }, monitor { "Monitor", kTeal }, undo { "Undo", kText }, redo { "Redo", kText }, guide { "Guide" }, about { "About & Support" }, viewW { "Waveform" }, viewS { "Spectral" };
    Display display; Meter inM, outM; DragHandle drag; juce::Label info, timeL; juce::Slider thr, pre; juce::ComboBox take;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> takeA;
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
