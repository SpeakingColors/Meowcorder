// Meowcorder DSP tests: spectral editing, resampling, stretch, pitch, warp, detection, history. Pure C++17.
#include "../src/core/MeowCore.h"
#include <cstdio>
using namespace mc;
static int checks = 0, failed = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++failed; std::printf ("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static bool near (double a, double b, double t) { return std::fabs (a - b) <= t; }
static const double PI = 3.14159265358979323846;

static Audio sines (std::initializer_list<double> hz, double secs, float amp = 0.3f, double sr = 48000.0)
{
    Audio a; a.sr = sr; a.resize ((size_t) (secs * sr));
    for (size_t i = 0; i < a.frames(); ++i) { double v = 0; for (double f : hz) v += std::sin (2 * PI * f * (double) i / sr); a.ch[0][i] = a.ch[1][i] = amp * (float) v; }
    return a;
}
// level of one frequency (dB), by correlation over [s, e)
static double level (const Audio& a, double hz, size_t s, size_t e)
{
    double re = 0, im = 0; for (size_t i = s; i < e; ++i) { const double ph = 2 * PI * hz * (double) i / a.sr; re += a.ch[0][i] * std::cos (ph); im += a.ch[0][i] * std::sin (ph); }
    return 20 * std::log10 (std::max (1e-9, 2 * std::hypot (re, im) / (double) (e - s)));
}
static double medianMidi (const Audio& a)
{
    auto p = detectPitch (a); std::vector<float> v; for (float m : p.midi) if (!std::isnan (m)) v.push_back (m); if (v.empty()) return -1; std::sort (v.begin(), v.end()); return v[v.size() / 2];
}
static bool finite (const Audio& a) { for (auto& c : a.ch) for (float x : c) if (!std::isfinite (x)) return false; return true; }

static void testResample()
{
    const Audio a = sines ({ 1000 }, 0.5); const Audio b = resample (a, 44100.0);
    CHECK (b.sr == 44100.0 && near ((double) b.frames(), 22050.0, 1.0)); CHECK (finite (b));
    CHECK (near (level (b, 1000, 4000, 18000), level (a, 1000, 4000, 18000), 0.5));
    const Audio c = resample (b, 48000.0); CHECK (near ((double) c.frames(), (double) a.frames(), 2.0));
    CHECK (near (level (c, 1000, 4000, 18000), level (a, 1000, 4000, 18000), 0.5));
    // mono stays identical on both sides
    CHECK (isMonoAudio (b));
}
static void testStretchAndPitch()
{
    const Audio a = sines ({ 440 }, 1.0, 0.4f);
    const auto st = pvStretch (a.ch[0], 1.5); CHECK (near ((double) st.size(), 72000.0, 2.0));
    Audio s; s.sr = a.sr; s.ch[0] = st; s.ch[1] = st; CHECK (finite (s)); CHECK (near (medianMidi (s), 69.0, 0.3));
    const Audio up = pitchShift (a, 12, false, 0); CHECK (up.frames() == a.frames()); CHECK (near (medianMidi (up), 81.0, 0.3));
    const Audio dn = pitchShift (a, -5, false, 0); CHECK (near (medianMidi (dn), 64.0, 0.3));
    const Audio fm = pitchShift (a, 7, true, 0); CHECK (finite (fm)); CHECK (near (medianMidi (fm), 76.0, 0.4));
    Audio r = a; pitchShiftRange (r, 12000, 36000, 12, false, 0); CHECK (finite (r)); CHECK (near ((double) r.ch[0][100], (double) a.ch[0][100], 1e-6));
    CHECK (near (level (r, 880, 18000, 30000), level (a, 440, 18000, 30000), 3.0));
}
static void testWarp()
{
    const Audio a = sines ({ 330 }, 2.0, 0.4f); WarpOpts o; o.ratio = 2.0;
    for (int mode = 0; mode < 4; ++mode)
    {
        o.mode = mode; if (mode == 2) o.hits = { 24000, 60000 };
        const Audio w = warpAudio (a, o); CHECK (near ((double) w.frames(), 48000.0, 2.0)); CHECK (finite (w));
    }
    o = WarpOpts(); o.ratio = 2.0; o.mode = 0; CHECK (near (medianMidi (warpAudio (a, o)), 64.0, 0.3));          // pitch kept
    o.mode = 1; CHECK (near (medianMidi (warpAudio (a, o)), 76.0, 0.4));                                              // repitch: one octave up
    o = WarpOpts(); o.semis = 3; o.cents = 50; CHECK (near (medianMidi (warpAudio (a, o)), 64.0 + 3.5, 0.4));
    o = WarpOpts(); CHECK (warpAudio (a, o).frames() == a.frames());
}
static void testSpectral()
{
    const Audio base = sines ({ 1000, 4000 }, 1.0, 0.25f); Audio a = base;
    SpecRegion r; r.t0 = 0; r.t1 = (double) a.frames(); r.f0 = 3000; r.f1 = 5000;
    spectralEdit (a, { r }, true, 0.f); CHECK (finite (a));
    CHECK (level (a, 4000, 8000, 40000) < level (base, 4000, 8000, 40000) - 25.0); CHECK (near (level (a, 1000, 8000, 40000), level (base, 1000, 8000, 40000), 1.0));
    Audio b = base; spectralEdit (b, { r }, false, -12.f); CHECK (near (level (b, 4000, 8000, 40000), level (base, 4000, 8000, 40000) - 12.0, 2.0)); CHECK (near (level (b, 1000, 8000, 40000), level (base, 1000, 8000, 40000), 1.0));
    Audio c = base; SpecRegion part; part.t0 = 20000; part.t1 = 30000; part.f0 = 3000; part.f1 = 5000; spectralEdit (c, { part }, true, 0.f);
    CHECK (level (c, 4000, 22000, 28000) < level (base, 4000, 22000, 28000) - 15.0); CHECK (near (level (c, 4000, 2000, 8000), level (base, 4000, 2000, 8000), 1.0)); CHECK (near (level (c, 4000, 40000, 46000), level (base, 4000, 40000, 46000), 1.0));
    // copy the 4 kHz piece and paste it at 2 kHz into silence
    SpecRegion cr; cr.t0 = 12000; cr.t1 = 36000; cr.f0 = 3500; cr.f1 = 4500; const SpecClip clip = spectralCopy (base, cr); CHECK (clip.valid());
    Audio sil; sil.sr = 48000; sil.resize (48000); CHECK (spectralPaste (sil, clip, 12000, 2000)); CHECK (finite (sil));
    // the shift is a whole number of bins: 4000 Hz minus 84 bins of 23.4375 Hz is 2031.25 Hz
    CHECK (near (level (sil, 2031.25, 18000, 30000), level (base, 4000, 18000, 30000), 3.0)); CHECK (level (sil, 2031.25, 18000, 30000) > level (sil, 4000, 18000, 30000) + 25.0);
    Audio empty; CHECK (!spectralPaste (empty, clip, 0, 1000)); CHECK (!spectralCopy (empty, cr).valid());
}
static void testDetect()
{
    Audio a; a.sr = 48000; a.resize (48000); const double f[2] = { 220.0, 261.63 };
    for (size_t i = 0; i < a.frames(); ++i) { const int k = i < 24000 ? 0 : 1; const size_t j = i < 24000 ? i : i - 24000; const float env = (j < 480 ? (float) j / 480 : 1.f) * (j > 23000 ? (float) (24000 - j) / 1000 : 1.f); a.ch[0][i] = a.ch[1][i] = 0.4f * env * (float) std::sin (2 * PI * f[k] * (double) j / a.sr); }
    const auto r = detectNotes (a); CHECK (r.notes.size() == 2);
    if (r.notes.size() == 2) { CHECK (near (r.notes[0].m, 57.0, 0.3)); CHECK (near (r.notes[1].m, 60.0, 0.3)); CHECK (r.notes[0].t0 < 3000 && near ((double) r.notes[0].t1, 24000.0, 3000.0)); CHECK (near ((double) r.notes[1].t0, 24000.0, 3000.0)); }
    Audio click; click.sr = 48000; click.resize (48000 * 8); const size_t beat = 24000; // 120 bpm
    for (size_t t = 0; t + 600 < click.frames(); t += beat) for (size_t i = 0; i < 600; ++i) click.ch[0][t + i] = click.ch[1][t + i] = 0.8f * (float) std::exp (-(double) i / 100.0) * (float) std::sin (2 * PI * 1500.0 * (double) i / 48000.0);
    CHECK (near (detectBpm (click), 120.0, 2.0)); Audio shortA = click.slice (0, 5000); CHECK (detectBpm (shortA) == 0.0);
}
static void testFormat()
{
    Audio a = sines ({ 300 }, 0.1, 0.5f); Audio q = a; quantize (q, 8, false); std::vector<float> lv; for (float x : q.ch[0]) { if (std::find (lv.begin(), lv.end(), x) == lv.end()) lv.push_back (x); } CHECK (lv.size() <= 256 && lv.size() > 20);
    for (float x : q.ch[0]) CHECK (near (x * 128.0, std::floor (x * 128.0 + 0.5), 1e-4));
    Audio s; s.sr = 48000; s.resize (4); s.ch[0] = { 1.f, 0.f, 0.5f, -1.f }; s.ch[1] = { 0.f, 1.f, 0.5f, 1.f };
    Audio m = s; channelOp (m, ChannelKind::Mix); CHECK (m.ch[0][0] == 0.5f && m.ch[1][0] == 0.5f && isMonoAudio (m));
    Audio l = s; channelOp (l, ChannelKind::Left); CHECK (l.ch[1][0] == 1.f); Audio r = s; channelOp (r, ChannelKind::Right); CHECK (r.ch[0][0] == 0.f && r.ch[0][1] == 1.f);
    Audio w = s; channelOp (w, ChannelKind::Swap); CHECK (w.ch[0][0] == 0.f && w.ch[1][0] == 1.f);
    Audio ms = s; channelOp (ms, ChannelKind::MidSideEnc); channelOp (ms, ChannelKind::MidSideDec); for (int c = 0; c < 2; ++c) for (size_t i = 0; i < 4; ++i) CHECK (near (ms.ch[c][i], s.ch[c][i], 1e-6));
}
static void testPencil()
{
    Audio a; a.sr = 48000; a.resize (1000); pencilWave (a, 100, 0.0, 200, 1.0, -1); CHECK (near (a.ch[0][150], 0.5, 0.02) && a.ch[1][200] == 1.f && a.ch[0][99] == 0.f);
    pencilWave (a, 400, 0.5, 300, -0.5, 0); CHECK (a.ch[0][330] != 0.f && a.ch[1][330] == 0.f && near (a.ch[0][330], -0.2, 0.02));
    pencilWave (a, -50, 2.0, 5, 2.0, -1); CHECK (a.ch[0][0] == 1.f);   // clipped to full scale and to the sound
    Audio t; t.sr = 48000; t.resize (48000); double ph = pencilTone (t, 0, 48000, 1000, 1000, 0.5, 0, 0, 0); CHECK (ph >= 0 && ph < 2 * PI); CHECK (near (level (t, 1000, 1000, 40000), 20 * std::log10 (0.5), 0.5));
    Audio u; u.sr = 48000; u.resize (4800); pencilTone (u, 0, 4800, 500, 500, 1.0, 0, 480, 480); CHECK (std::fabs (u.ch[0][10]) < 0.1f); CHECK (std::fabs (u.ch[0][4799]) < 0.1f);
}
static void testHistory()
{
    Editor e; e.load (sines ({ 440 }, 0.2, 0.5f)); float v[kMaxOpParams]; defaultParams (opGain, v); v[0] = -6.f; const float orig = e.audio.ch[0][50];
    CHECK (e.apply (opGain, v)); CHECK (e.apply (opReverse, v)); e.setSelection (100, 400); e.deleteSelection();
    auto h = e.history(); CHECK (h.done.size() == 3 && h.done[0] == "Gain" && h.done[1] == "Reverse" && h.done[2] == "Delete" && h.undone.empty());
    e.jumpTo (1); h = e.history(); CHECK (h.done.size() == 1 && h.undone.size() == 2 && h.undone[0] == "Reverse" && h.undone[1] == "Delete");
    CHECK (near (e.audio.ch[0][50], orig * 0.5011872, 1e-4) || e.audio.frames() > 0); e.jumpTo (3); CHECK (e.history().done.size() == 3 && e.audio.frames() == 9600 - 300);
    e.jumpTo (0); CHECK (near (e.audio.ch[0][50], orig, 1e-6)); CHECK (!e.canUndo() && e.canRedo()); e.jumpTo (99); CHECK (e.history().done.size() == 3);
    // a new edit after undoing drops the undone steps
    e.jumpTo (1); e.apply (opInvert, v); h = e.history(); CHECK (h.done.size() == 2 && h.undone.empty() && h.done[1] == kOps[opInvert].name);
    // transform: one named, undoable step that may change the length
    CHECK (e.transform ("Resample", [] (Audio& a) { a = resample (a, 24000.0); })); CHECK (e.audio.sr == 24000.0 && e.history().done.back() == "Resample"); e.undo(); CHECK (e.audio.sr == 48000.0);
    // scope and mix
    Editor m; m.load (sines ({ 440 }, 0.1, 0.5f)); m.setSelection (100, 200); const float before = m.audio.ch[0][50], inside = m.audio.ch[0][150]; defaultParams (opGain, v); v[0] = -20.f;
    m.apply (opGain, v, true, 0.5f); CHECK (near (m.audio.ch[0][50], before * (0.5 + 0.5 * 0.1), 1e-4)); CHECK (near (m.audio.ch[0][150], inside * (0.5 + 0.5 * 0.1), 1e-4));
    Editor n; n.load (sines ({ 440 }, 0.1, 0.5f)); n.setSelection (100, 200); n.apply (opGain, v); CHECK (near (n.audio.ch[0][50], before, 1e-6) && near (n.audio.ch[0][150], inside * 0.1, 1e-4));
    // spectral steps are history steps too, and the spectral clipboard survives
    Editor s; s.load (sines ({ 1000, 4000 }, 0.5, 0.25f)); SpecRegion r; r.t0 = 0; r.t1 = 24000; r.f0 = 3000; r.f1 = 5000; CHECK (s.spectralEdit (true, { r }, 0.f)); CHECK (s.history().done.back() == "Spectral cut");
    CHECK (s.spectralCopy (r) && s.hasSpectralClip()); CHECK (s.spectralPaste (1000, 6000)); CHECK (s.history().done.back() == "Spectral paste"); s.undo(); s.undo(); CHECK (s.history().done.empty());
    Editor z; CHECK (!z.spectralEdit (true, { r }, 0.f) && !z.transform ("x", [] (Audio&) {}));
}
static void testHitsAndSplice()
{
    // four clicks half a second apart are found, within a few ms
    Audio c; c.sr = 48000; c.resize (96000); for (int k = 0; k < 4; ++k) for (int i = 0; i < 600; ++i) { const float v = 0.9f * std::exp (-i / 120.f) * (i % 2 ? 1.f : -1.f); c.ch[0][(size_t) (k * 24000 + 4000 + i)] = v; c.ch[1][(size_t) (k * 24000 + 4000 + i)] = v; }
    const auto h = detectTransients (c, 0.6); CHECK (h.size() == 4); for (size_t k = 0; k < h.size() && k < 4; ++k) CHECK (std::llabs ((long long) h[k] - (long long) (k * 24000 + 4000)) < 700);
    Audio quiet; quiet.sr = 48000; quiet.resize (96000); CHECK (detectTransients (quiet, 0.6).empty()); Audio tiny; tiny.resize (300); CHECK (detectTransients (tiny, 0.6).empty());
    // splice: replace the middle with a different length
    Audio a = sines ({ 440 }, 0.1, 0.5f); const size_t n = a.frames(); Audio w; w.sr = a.sr; w.resize (50); a.splice (1000, 2000, w); CHECK (a.frames() == n - 1000 + 50); CHECK (a.ch[0][1000] == 0.f && a.ch[1][1049] == 0.f);
}

int main()
{
    testResample(); testStretchAndPitch(); testWarp(); testSpectral(); testDetect(); testFormat(); testPencil(); testHistory(); testHitsAndSplice();
    std::printf ("%d checks, %d failed\n", checks, failed); return failed ? 1 : 0;
}
