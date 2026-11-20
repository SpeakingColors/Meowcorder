// Meowcorder core tests. Pure C++17: sh tests/run.sh
#include "../src/core/MeowCore.h"
#include "../src/core/MeowDsp.h"
#include <cstdio>
#include <set>
using namespace mc;
static int checks = 0, failed = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++failed; std::printf ("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static bool near (double a, double b, double t) { return std::fabs (a - b) <= t; }

static Audio sine (double hz, double secs, float amp = 0.5f, double sr = 48000.0)
{
    Audio a; a.sr = sr; a.resize ((size_t) (secs * sr));
    for (size_t i = 0; i < a.frames(); ++i) a.ch[0][i] = a.ch[1][i] = amp * (float) std::sin (2 * 3.14159265358979323846 * hz * (double) i / sr);
    return a;
}
static float rms (const Audio& a, size_t s, size_t e) { double t = 0; for (size_t i = s; i < e; ++i) t += (double) a.ch[0][i] * a.ch[0][i]; return (float) std::sqrt (t / (double) (e - s)); }
static bool finite (const Audio& a) { for (auto& c : a.ch) for (float x : c) if (!std::isfinite (x)) return false; return true; }
static int zeroCrossings (const Audio& a) { int n = 0; for (size_t i = 1; i < a.frames(); ++i) if ((a.ch[0][i - 1] < 0) != (a.ch[0][i] < 0)) ++n; return n; }
static Audio run (const Audio& in, int op, std::initializer_list<float> p = {})
{
    Audio a = in; float v[kMaxOpParams]; defaultParams (op, v); int i = 0; for (float x : p) v[i++] = x; applyOp (a, op, 0, a.frames(), v); return a;
}

static void testTable()
{
    std::set<std::string> ids;
    CHECK (opCount == 24);
    for (int i = 0; i < opCount; ++i)
    {
        CHECK (kOps[i].id && kOps[i].name && kOps[i].tip && kOps[i].nParams <= kMaxOpParams);
        CHECK (ids.insert (kOps[i].id).second);
        float p[kMaxOpParams]; defaultParams (i, p);
        for (int k = 0; k < kOps[i].nParams; ++k) { const auto& d = kOps[i].params[k]; CHECK (d.lo <= d.def && d.def <= d.hi); CHECK (p[k] == d.def); CHECK (d.tip && d.tip[0]); }
    }
}

static void testWav()
{
    Audio a = sine (440, 0.1f, 0.8f);
    for (int bits : { 16, 24, 32 })
    {
        const auto w = encodeWav (a, bits); Audio b; CHECK (decodeWav (w.data(), w.size(), b));
        CHECK (b.frames() == a.frames() && b.sr == a.sr);
        double err = 0; for (size_t i = 0; i < a.frames(); ++i) err = std::max (err, (double) std::fabs (a.ch[0][i] - b.ch[0][i]));
        CHECK (err < (bits == 16 ? 1e-4 : (bits == 24 ? 1e-6 : 1e-9)));
    }
    uint8_t junk[20] = { 1, 2, 3 }; Audio b; CHECK (! decodeWav (junk, 20, b)); CHECK (! decodeWav (junk, 3, b));
    auto w = encodeWav (a, 16); w.resize (60); CHECK (decodeWav (w.data(), w.size(), b) && b.frames() > 0 && b.frames() < a.frames());   // truncated file still reads what is there
    // mono file: duplicated to both sides
    std::vector<uint8_t> m; putTag (m, "RIFF"); put32 (m, 36 + 4); putTag (m, "WAVE"); putTag (m, "fmt "); put32 (m, 16); put16 (m, 1); put16 (m, 1); put32 (m, 44100); put32 (m, 88200); put16 (m, 2); put16 (m, 16); putTag (m, "data"); put32 (m, 4); put16 (m, 16384); put16 (m, 0);
    Audio c; CHECK (decodeWav (m.data(), m.size(), c)); CHECK (c.frames() == 2 && c.sr == 44100.0 && near (c.ch[0][0], 0.5, 1e-4) && c.ch[1][0] == c.ch[0][0]);
}

static void testOps()
{
    const Audio s = sine (1000, 0.5f, 0.25f);
    CHECK (near (run (s, opGain, { 6.0f }).peak (0, s.frames()), 0.25 * 1.9953, 0.002));
    CHECK (near (run (s, opNormalize, { -1.0f, 0.0f }).peak (0, s.frames()), std::pow (10.0, -1.0 / 20), 0.002));
    { Audio r = run (s, opNormalize, { -3.0f, 1.0f }); CHECK (r.peak (0, r.frames()) <= 1.0001f); CHECK (near (rms (r, 0, r.frames()), std::pow (10.0, -3.0 / 20), 0.02)); }
    { Audio q = s; for (auto& c : q.ch) std::fill (c.begin(), c.end(), 0.f); CHECK (run (q, opNormalize).peak (0, q.frames()) == 0.f); }
    { Audio f = run (s, opFadeIn); CHECK (f.ch[0][0] == 0.f); CHECK (near (f.ch[0][f.frames() - 1], s.ch[0][s.frames() - 1], 1e-5)); Audio g = run (s, opFadeOut); CHECK (g.ch[0][g.frames() - 1] == 0.f); }
    { Audio r = run (run (s, opReverse), opReverse); CHECK (r.ch[0] == s.ch[0]); CHECK (run (s, opReverse).ch[0][0] == s.ch[0][s.frames() - 1]); }
    { Audio r = run (s, opInvert); CHECK (r.ch[0][100] == -s.ch[0][100]); }
    { Audio d = s; for (auto& c : d.ch) for (auto& x : c) x += 0.2f; Audio r = run (d, opDcRemove); double m = 0; for (float x : r.ch[0]) m += x; CHECK (std::fabs (m / (double) r.frames()) < 1e-4); }
    { Audio r = run (s, opSilence); CHECK (r.peak (0, r.frames()) == 0.f && r.frames() == s.frames()); }
    { Audio l = s; for (auto& x : l.ch[1]) x = 0; Audio r = run (l, opMono, { 0.0f }); CHECK (near (r.ch[0][50], 0.5 * s.ch[0][50], 1e-6) && r.ch[0][50] == r.ch[1][50]); Audio rl = run (l, opMono, { 1.0f }); CHECK (rl.ch[1][50] == s.ch[0][50]); }
    { Audio r = run (s, opPan, { 100.0f }); CHECK (r.ch[0][50] == 0.f && r.ch[1][50] == s.ch[1][50]); }
    // pitch and speed: +12 st halves the length and doubles the frequency
    { Audio r = run (s, opRate, { 12.0f, 0.0f }); CHECK (near ((double) r.frames(), (double) s.frames() / 2, 2)); CHECK (near (zeroCrossings (r) / (r.frames() / 48000.0) / 2, 2000, 40)); Audio d = run (s, opRate, { -12.0f, 0.0f }); CHECK (near ((double) d.frames(), (double) s.frames() * 2, 2)); CHECK (finite (r) && finite (d)); }
    // bitcrush: 2 bits leaves only a few levels
    { Audio r = run (s, opBitcrush, { 2.0f, 1.0f, 0.0f, 100.0f }); std::set<float> v (r.ch[0].begin(), r.ch[0].end()); CHECK (v.size() <= 4); Audio dry = run (s, opBitcrush, { 2.0f, 1.0f, 0.0f, 0.0f }); CHECK (dry.ch[0] == s.ch[0]); }
    { Audio r = run (s, opBitcrush, { 8.0f, 4.0f, 0.0f, 100.0f }); CHECK (r.ch[0][1] == r.ch[0][0] && r.ch[0][2] == r.ch[0][0] && r.ch[0][3] == r.ch[0][0]); }
    { Audio r = run (s, opDrive, { 80.0f, 0.0f, 0.0f }); CHECK (finite (r)); CHECK (near (rms (r, 4800, r.frames()) / rms (s, 4800, s.frames()), 1.0, 0.35)); }
    // EQ and filter on sines
    { const Audio hi = sine (10000, 0.3f, 0.5f); const Audio lo = sine (100, 0.3f, 0.5f); const Audio sub = sine (30, 0.5f, 0.5f);
      CHECK (rms (run (hi, opFilter, { 0.0f, 500.0f, 0.707f }), 4800, hi.frames()) < 0.01f);                // LP kills 10 kHz
      CHECK (rms (run (lo, opFilter, { 1.0f, 2000.0f, 0.707f }), 4800, lo.frames()) < 0.01f);               // HP kills 100 Hz
      CHECK (rms (run (lo, opFilter, { 0.0f, 2000.0f, 0.707f }), 4800, lo.frames()) > 0.3f);
      CHECK (near (rms (run (sub, opEq, { 12.0f, 0.0f, 1000.0f, 1.0f, 0.0f }), 12000, sub.frames()) / rms (sub, 12000, sub.frames()), 3.98, 0.3));   // +12 dB low shelf, full gain well below 120 Hz
      CHECK (near (rms (run (hi, opEq), 4800, hi.frames()), rms (hi, 4800, hi.frames()), 1e-4)); }
    // delay and reverb add a tail and keep the start
    { Audio r = run (s, opDelay, { 100.0f, 50.0f, 50.0f, 1.0f }); CHECK (r.frames() == s.frames() + 48000); CHECK (rms (r, s.frames(), r.frames()) > 0.001f); CHECK (finite (r)); }
    { Audio r = run (s, opDelay, { 100.0f, 50.0f, 0.0f, 0.0f }); CHECK (r.ch[0] == s.ch[0]); }
    { Audio r = run (s, opReverb, { 50.0f, 40.0f, 50.0f, 2.0f }); CHECK (r.frames() == s.frames() + 96000); CHECK (rms (r, s.frames() + 4800, r.frames()) > 1e-4f); CHECK (finite (r)); Audio z = run (s, opReverb, { 50.0f, 40.0f, 0.0f, 0.0f }); CHECK (z.ch[0] == s.ch[0]); }
    { Audio r = run (s, opChorus); CHECK (finite (r) && r.frames() == s.frames()); Audio p = run (s, opPhaser); CHECK (finite (p) && p.frames() == s.frames() && rms (p, 4800, p.frames()) > 0.05f); }
    // dynamics
    { const Audio loud = sine (200, 0.5f, 0.8f);
      Audio c = run (loud, opCompressor, { -20.0f, 8.0f, 1.0f, 100.0f, 0.0f }); CHECK (rms (c, 12000, c.frames()) < 0.6f * rms (loud, 12000, loud.frames()));
      Audio m = run (loud, opCompressor, { 0.0f, 8.0f, 1.0f, 100.0f, 0.0f }); CHECK (near (rms (m, 12000, m.frames()), rms (loud, 12000, loud.frames()), 0.02));   // below threshold: untouched
      Audio l = run (loud, opLimiter, { -6.0f, 60.0f }); CHECK (l.peak (0, l.frames()) <= std::pow (10.0, -6.0 / 20) + 1e-4);
      Audio q = loud; for (auto& ch : q.ch) for (auto& x : ch) x *= 0.01f; Audio g = run (q, opGate, { -30.0f, 1.0f, 10.0f, 50.0f, -90.0f }); CHECK (g.peak (12000, g.frames()) < 1e-3f);
      Audio g2 = run (loud, opGate, { -30.0f, 1.0f, 10.0f, 50.0f, -90.0f }); CHECK (near (rms (g2, 12000, g2.frames()), rms (loud, 12000, loud.frames()), 0.02)); }
    // trim silence and loop crossfade change the length
    { Audio t; t.sr = 48000; t.resize (30000); for (size_t i = 10000; i < 20000; ++i) t.ch[0][i] = t.ch[1][i] = 0.5f; Audio r = run (t, opTrimSilence, { -60.0f }); CHECK (r.frames() == 10000 && r.ch[0][0] == 0.5f); Audio e; e.sr = 48000; e.resize (1000); CHECK (run (e, opTrimSilence).frames() == 1000); }
    { Audio r = run (s, opLoopXfade, { 50.0f }); CHECK (r.frames() == s.frames() - 2400); CHECK (finite (r)); }
    // every operation at default and at both ends of every parameter: finite, on a very short sound too
    const Audio tiny = sine (500, 0.002f, 0.5f);
    for (int op = 0; op < opCount; ++op)
        for (int mode = 0; mode < 3; ++mode)
            for (const Audio* in : { &s, &tiny })
            {
                float p[kMaxOpParams]; defaultParams (op, p);
                for (int k = 0; k < kOps[op].nParams; ++k) p[k] = mode == 0 ? kOps[op].params[k].lo : (mode == 1 ? kOps[op].params[k].hi : kOps[op].params[k].def);
                Audio a = *in; applyOp (a, op, 0, a.frames(), p); CHECK (finite (a));
                Audio e; e.sr = 48000; e.resize (0); applyOp (e, op, 0, 0, p); CHECK (e.frames() == 0);       // empty sound does nothing and does not crash
            }
    // operations stay inside the selection
    { Audio r = s; float p[kMaxOpParams]; defaultParams (opSilence, p); applyOp (r, opSilence, 1000, 2000, p); CHECK (r.ch[0][999] == s.ch[0][999] && r.ch[0][1500] == 0.f && r.ch[0][2000] == s.ch[0][2000]); }
}

static void testEditor()
{
    Editor ed; ed.load (sine (440, 1.0f)); const size_t n = ed.audio.frames(); const Audio orig = ed.audio;
    float p[kMaxOpParams]; defaultParams (opGain, p); p[0] = -6.0f;
    CHECK (! ed.canUndo()); ed.apply (opGain, p); CHECK (ed.canUndo() && near (ed.audio.peak (0, n), 0.25, 0.002));
    ed.undo(); CHECK (ed.audio.ch[0] == orig.ch[0]); CHECK (ed.canRedo()); ed.redo(); CHECK (near (ed.audio.peak (0, n), 0.25, 0.002));
    ed.undo(); CHECK (! ed.apply (-1, p) && ! ed.apply (opCount, p));
    // selection limits an operation, and repeat last applies it again
    ed.setSelection (12000, 24000); ed.apply (opSilence, p); CHECK (ed.audio.ch[0][13000] == 0.f && ed.audio.ch[0][11999] == orig.ch[0][11999]);
    ed.setSelection (30000, 36000); CHECK (ed.repeatLast()); CHECK (ed.audio.ch[0][31000] == 0.f);
    // length changing op inside a selection: selection follows
    ed.load (orig); ed.setSelection (0, 24000); float r[kMaxOpParams]; defaultParams (opRate, r); r[0] = 12.0f; ed.apply (opRate, r); CHECK (ed.audio.frames() == n - 12000 && ed.selB == 12000);
    // cut, paste, copy, delete, crop
    ed.load (orig); ed.setSelection (1000, 3000); ed.copy(); CHECK (ed.clipboard().frames() == 2000);
    ed.cut(); CHECK (ed.audio.frames() == n - 2000 && ! ed.hasSelection()); ed.paste(); CHECK (ed.audio.frames() == n && ed.audio.ch[0] == orig.ch[0]);   // paste goes back where the cut was
    ed.undo(); ed.undo(); CHECK (ed.audio.ch[0] == orig.ch[0]);
    ed.setCursor (5000); ed.copy(); ed.setSelection (1000, 3000); ed.copy(); ed.setCursor (5000); ed.paste(); CHECK (ed.audio.frames() == n + 2000 && ed.audio.ch[0][5000] == orig.ch[0][1000]);
    ed.load (orig);
    ed.setSelection (500, 1500); ed.deleteSelection (); CHECK (ed.audio.frames() == n - 1000); ed.undo (); CHECK (ed.audio.frames() == n);
    ed.setSelection (100, 600); ed.cropToSelection(); CHECK (ed.audio.frames() == 500 && ed.selA == 0 && ed.selB == 500);
    ed.setSelection (900, 100); CHECK (ed.selA <= ed.selB); ed.setSelection (0, 99999999); CHECK (ed.selB == ed.audio.frames());
    // recordings: replace, append, punch, insert
    ed.load (orig); Audio rec = sine (880, 0.1f); const size_t rn = rec.frames();
    ed.takeRecording (rec, 1); CHECK (ed.audio.frames() == n + rn && ed.selA == n);
    ed.load (orig); ed.setSelection (1000, 2000); ed.takeRecording (rec, 2); CHECK (ed.audio.frames() == n - 1000 + rn);
    ed.load (orig); ed.setSelection (1000, 2000); ed.takeRecording (rec, 3); CHECK (ed.audio.frames() == n + rn);
    ed.load (orig); ed.takeRecording (rec, 0); CHECK (ed.audio.frames() == rn && ! ed.canUndo());
    Audio none; none.sr = 48000; ed.takeRecording (none, 1); CHECK (ed.audio.frames() == rn);
    // snap to zero crossing
    ed.load (sine (100, 0.1f)); const size_t z = ed.snapToZero (123, 400); CHECK (std::fabs (ed.audio.ch[0][z]) < 0.05f);
    // undo memory is limited but never empties the newest step
    ed.load (orig); ed.setUndoLimitBytes (n * 8 * 3); for (int i = 0; i < 20; ++i) ed.apply (opGain, p); int u = 0; while (ed.canUndo()) { ed.undo(); ++u; } CHECK (u >= 1 && u <= 4);
}

static void testRecorder()
{
    Recorder r; r.prepare (48000.0, 2.0);
    std::vector<float> quiet (4800, 0.f), loud (4800, 0.5f);
    r.arm (Recorder::Threshold, -20.0f, 50.0);
    CHECK (r.isArmed() && ! r.isRecording());
    r.process (quiet.data(), nullptr, quiet.size()); CHECK (! r.isRecording());
    r.process (loud.data(), nullptr, loud.size()); CHECK (r.isRecording());
    Audio a = r.stop(); CHECK (a.frames() == 2400 + 4800);   // 50 ms of pre-roll plus the loud part
    CHECK (a.ch[0][0] == 0.f && a.ch[0][2400] == 0.5f && a.ch[1][2400] == 0.5f);
    CHECK (! r.isRecording() && ! r.isArmed());
    r.arm (Recorder::Immediate); CHECK (r.isRecording()); r.process (loud.data(), loud.data(), 100); CHECK (r.recordedFrames() == 100); CHECK (r.stop().frames() == 100);
    // always listening: capture last 0.05 s without recording; limited by what exists and by the ring size
    r.prepare (48000.0, 1.0); r.process (quiet.data(), nullptr, quiet.size()); r.process (loud.data(), nullptr, loud.size());
    Audio c = r.captureLast (0.05); CHECK (c.frames() == 2400 && c.ch[0][0] == 0.5f);
    CHECK (r.captureLast (100.0).frames() == 9600);
    for (int i = 0; i < 20; ++i) r.process (loud.data(), nullptr, loud.size());
    CHECK (r.captureLast (100.0).frames() <= 48001);
    // recording stops adding at the maximum
    r.prepare (48000.0, 1.0); r.maxFrames = 1000; r.arm (Recorder::Immediate); r.process (loud.data(), nullptr, loud.size()); CHECK (r.recordedFrames() == 1000);
}

static void testAnalysis()
{
    const Audio s = sine (1000, 0.5f, 1.0f);
    const Spectrogram sp = spectrogram (s, 0, s.frames(), 4096, 1024);
    int best = 0; for (int b = 0; b < sp.bins; ++b) if (sp.at (2, b) > sp.at (2, best)) best = b;
    CHECK (near (best * 48000.0 / 4096, 1000, 12)); CHECK (near (sp.at (2, best), -6.0, 1.5) || near (sp.at (2, best), 0.0, 1.5));   // a full-scale mono mix reads about 0 dB
    CHECK (sp.frames == (int) ((s.frames() - 4096) / 1024) + 1 && sp.bins == 2049);
    Audio sil; sil.sr = 48000; sil.resize (8192); CHECK (spectrogram (sil, 0, 8192, 1024, 512).at (0, 10) <= -119.0f);
    const Peaks pk = peakOverview (s, 0, s.frames(), 100); CHECK (pk.mx.size() == 100 && pk.mx[10] > 0.9f && pk.mn[10] < -0.9f);
    CHECK (peakOverview (s, 10, 10, 50).mx[0] == 0.f && peakOverview (s, 0, 100, 0).mx.empty());
    Audio clk; clk.sr = 48000; clk.resize (48000 * 2);
    for (size_t t : { 10000u, 34000u, 60000u, 82000u }) for (size_t i = 0; i < 600; ++i) clk.ch[0][t + i] = clk.ch[1][t + i] = (i % 2 ? -0.8f : 0.8f) * (1.0f - (float) i / 600.0f);
    const auto hits = detectTransients (clk, 0.5f); CHECK (hits.size() == 4);
    for (size_t k = 0; k < hits.size() && k < 4; ++k) { static const size_t at[4] = { 10000, 34000, 60000, 82000 }; CHECK (near ((double) hits[k], (double) at[k], 600)); }
    Audio short_; short_.sr = 48000; short_.resize (100); CHECK (detectTransients (short_, 0.5f).empty());
}

static void testAudio()
{
    Audio a = sine (440, 0.01f); const size_t n = a.frames();
    a.insertSilence (100, 50); CHECK (a.frames() == n + 50 && a.ch[0][120] == 0.f);
    a.erase (100, 150); CHECK (a.frames() == n);
    CHECK (a.slice (10, 20).frames() == 10 && a.slice (20, 10).frames() == 0 && a.slice (0, 1 << 30).frames() == n);
    a.erase (n, 5 * n); CHECK (a.frames() == n);
}

static void testHistoryAndTempo()
{
    // history: every change is a step, the list can be walked, a new change after going back drops the later steps
    Editor ed; ed.load (sine (440, 0.5f));
    CHECK (ed.historySteps() == 1 && ed.historyPos() == 0);
    float g[kMaxOpParams]; defaultParams (opGain, g); g[0] = -6.f;
    ed.apply (opGain, g); ed.setSelection (1000, 5000); ed.cut(); ed.paste();
    CHECK (ed.historySteps() == 4 && ed.historyPos() == 3);
    CHECK (std::string (ed.historyLabel (1)) == "Gain" && std::string (ed.historyLabel (2)) == "Cut" && std::string (ed.historyLabel (3)) == "Paste");
    const size_t full = ed.audio.frames();
    ed.jumpToStep (1); CHECK (ed.historyPos() == 1 && ed.audio.frames() > full - 5000 && ed.canRedo());
    ed.jumpToStep (0); CHECK (ed.historyPos() == 0 && ed.audio.peak (0, ed.audio.frames()) > 0.45f);          // back to the loaded sound, not quieter
    ed.jumpToStep (3); CHECK (ed.historyPos() == 3 && ed.audio.frames() == full);
    ed.jumpToStep (1); defaultParams (opReverse, g); ed.apply (opReverse, g);
    CHECK (ed.historySteps() == 3 && std::string (ed.historyLabel (2)) == "Reverse" && ! ed.canRedo());                // a new change drops the old future
    ed.undo(); CHECK (ed.historyPos() == 1);
    ed.load (sine (440, 0.1f)); CHECK (ed.historySteps() == 1);

    // tempo: a kick every half second is 120 bpm; too short or silent has none
    Audio dr; dr.sr = 48000.0; dr.resize (48000 * 4);
    for (int b = 0; b < 8; ++b) for (int i = 0; i < 12000; ++i) dr.ch[0][(size_t) (b * 24000 + i)] = dr.ch[1][(size_t) (b * 24000 + i)] = std::exp (-(float) i / 2500.f) * std::sin (2.f * 3.14159265f * 60.f * (float) i / 48000.f);
    CHECK (near (detectBpm (dr), 120.0, 1.0));
    CHECK (detectBpm (sine (100, 0.3f)) == 0.f);
    Audio silent; silent.sr = 48000.0; silent.resize (48000 * 4); CHECK (detectBpm (silent) == 0.f);
    Audio dr2; dr2.sr = 48000.0; dr2.resize (48000 * 4);                                                              // 90 bpm: a hit every 2/3 s
    for (int b = 0; b < 6; ++b) for (int i = 0; i < 8000; ++i) { const size_t k = (size_t) (b * 32000 + i); dr2.ch[0][k] = dr2.ch[1][k] = std::exp (-(float) i / 1800.f) * std::sin (2.f * 3.14159265f * 70.f * (float) i / 48000.f); }
    CHECK (near (detectBpm (dr2), 90.0, 1.5));
}

// Port of tools/spectral_test.js: the C++ spectral editing must behave like the one in the mockup.
static double toneDb (const Audio& a, double f, size_t s0, size_t s1, double sr)
{
    double re = 0, im = 0; for (size_t i = s0; i < s1; ++i) { const double w = 2 * 3.14159265358979323846 * f * (double) i / sr; re += (double) a.ch[0][i] * std::cos (w); im += (double) a.ch[0][i] * std::sin (w); }
    return 20 * std::log10 (std::max (1e-9, 2 * std::hypot (re, im) / (double) (s1 - s0)));
}
static void testSpectral()
{
    const double sr = 48000; const size_t n = (size_t) (3 * sr);
    auto mk = [&] { Audio a; a.sr = sr; a.resize (n); for (size_t i = 0; i < n; ++i) { const double t = (double) i / sr, v = 0.3 * std::sin (2 * 3.14159265358979323846 * 1000 * t) + 0.3 * std::sin (2 * 3.14159265358979323846 * 3000 * t); a.ch[0][i] = (float) v; a.ch[1][i] = (float) (v * 0.5); } return a; };
    Audio ref = mk(), a = mk(); spectralEdit (a, { { 10000, 20000, 100, 200 } }, false, 0.0);
    double d = 0; for (size_t i = 0; i < n; ++i) d = std::max ({ d, (double) std::fabs (a.ch[0][i] - ref.ch[0][i]), (double) std::fabs (a.ch[1][i] - ref.ch[1][i]) });
    CHECK (d < 1e-4);                                                                                                  // no change gives the same sound
    const SRegion R { sr * 0.5, sr * 1.5, 2500, 3500 };
    a = mk(); spectralEdit (a, { R }, true, 0.0);
    CHECK (toneDb (a, 3000, (size_t) (sr * 0.7), (size_t) (sr * 1.3), sr) < -50);                                       // 3 kHz inside the box removed
    CHECK (near (toneDb (a, 1000, (size_t) (sr * 0.7), (size_t) (sr * 1.3), sr), toneDb (ref, 1000, (size_t) (sr * 0.7), (size_t) (sr * 1.3), sr), 0.2));   // 1 kHz kept
    CHECK (near (toneDb (a, 3000, (size_t) (sr * 2), (size_t) (sr * 2.5), sr), toneDb (ref, 3000, (size_t) (sr * 2), (size_t) (sr * 2.5), sr), 0.1));      // outside the time range kept
    a = mk(); spectralEdit (a, { R }, false, 6.0);
    CHECK (near (toneDb (a, 3000, (size_t) (sr * 0.7), (size_t) (sr * 1.3), sr) - toneDb (ref, 3000, (size_t) (sr * 0.7), (size_t) (sr * 1.3), sr), 6.0, 0.3));   // amp +6 dB
    a = mk(); SpectralClip clip; CHECK (spectralCopy (a, { sr * 0.5, sr * 1.0, 2500, 3500 }, clip));
    Audio b; b.sr = sr; b.resize (n); CHECK (spectralPaste (b, clip, sr * 2, clip.fc));
    CHECK (near (toneDb (b, 3000, (size_t) (sr * 2.1), (size_t) (sr * 2.4), sr), toneDb (ref, 3000, (size_t) (sr * 0.5), (size_t) (sr * 1), sr), 1.0));        // pasted level matches
    CHECK (toneDb (b, 1000, (size_t) (sr * 2.1), (size_t) (sr * 2.4), sr) < -50 && toneDb (b, 3000, (size_t) (sr * 0.5), (size_t) (sr * 1), sr) < -80);       // nothing else pasted
    Audio c2; c2.sr = sr; c2.resize (n); spectralPaste (c2, clip, sr * 2, clip.fc + 2000);
    const double up = 3000 + std::round (2000.0 * 2048 / sr) * sr / 2048;
    CHECK (toneDb (c2, up, (size_t) (sr * 2.1), (size_t) (sr * 2.4), sr) > toneDb (c2, 3000, (size_t) (sr * 2.1), (size_t) (sr * 2.4), sr) + 40);                // paste moves up to 5 kHz
    Editor ed; ed.load (mk()); CHECK (ed.spectralEdit (true, { R }, 0.0)); CHECK (ed.canUndo() && std::string (ed.historyLabel (1)) == "Spectral cut");
    ed.undo(); double dd = 0; for (size_t i = 0; i < n; i += 97) dd = std::max (dd, (double) std::fabs (ed.audio.ch[0][i] - ref.ch[0][i])); CHECK (dd == 0.0);   // undo restores exactly
}

// Port of tools/dsp_test.js: the C++ resampler, stretch, pitch shift, warp and note detection must behave like the mockup's.
static double zcFreq (const std::vector<float>& x, double sr)
{
    const size_t a = (size_t) ((double) x.size() * 0.25), b = (size_t) ((double) x.size() * 0.75); int z = 0; long first = -1, last = -1;
    for (size_t i = a + 1; i < b; ++i) if (x[i - 1] <= 0 && x[i] > 0) { if (first < 0) first = (long) i; last = (long) i; ++z; }
    return (z - 1) * sr / (double) (last - first);
}
static double rmsOf (const std::vector<float>& x, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i]; return std::sqrt (s / (double) (b - a)); }
static void testDsp()
{
    const double sr = 48000; Audio a = sine (440, 1.0, 0.5f, sr);
    Audio r = resampleAudio (a, 44100); CHECK (r.frames() == 44100 && near (zcFreq (r.ch[0], 44100), 440, 1.5) && near (rmsOf (r.ch[0], 5000, 30000), 0.5 / std::sqrt (2.0), 0.01));
    r = resampleAudio (a, 96000); CHECK (r.frames() == 96000 && near (zcFreq (r.ch[0], 96000), 440, 1.5));
    auto st = pvStretch (a.ch[0], 1.5); CHECK (st.size() == 72000 && near (zcFreq (st, sr), 440, 3) && near (rmsOf (st, 20000, 50000), 0.3535, 0.03));
    st = pvStretch (a.ch[0], 0.75); CHECK (st.size() == 36000 && near (zcFreq (st, sr), 440, 3));
    Audio ps = pitchShift (a, 12, false, 0); CHECK (ps.frames() == a.frames() && near (zcFreq (ps.ch[0], sr), 880, 6));
    ps = pitchShift (a, -5, true, 0); CHECK (near (zcFreq (ps.ch[0], sr), 440 * std::pow (2.0, -5.0 / 12), 4));
    WarpOptions wo; wo.ratio = 2; wo.mode = 1; Audio w = warpAudio (a, wo); CHECK (w.frames() == 24000 && near (zcFreq (w.ch[0], sr), 880, 6));
    wo = WarpOptions(); wo.ratio = 1.25; wo.mode = 0; w = warpAudio (a, wo); CHECK (w.frames() == 38400 && near (zcFreq (w.ch[0], sr), 440, 3));
    wo = WarpOptions(); wo.ratio = 0.8; wo.mode = 3; w = warpAudio (a, wo); CHECK (w.frames() == 60000 && rmsOf (w.ch[0], 10000, 50000) > 0.15);
    // beats: clicks stay sharp
    Audio cl; cl.sr = sr; cl.resize (48000 * 2); std::vector<size_t> hits;
    for (int b = 0; b < 4; ++b) { const size_t t0 = (size_t) b * 24000; hits.push_back (t0); for (int i = 0; i < 6000; ++i) cl.ch[0][t0 + (size_t) i] = cl.ch[1][t0 + (size_t) i] = std::exp (-(float) i / 800.f) * std::sin (2.f * 3.14159265f * 200.f * (float) i / 48000.f); }
    wo = WarpOptions(); wo.ratio = 0.8; wo.mode = 2; wo.hits = hits; w = warpAudio (cl, wo); float pk = 0; for (size_t i = 0; i < 2000; ++i) pk = std::max (pk, std::fabs (w.ch[0][i]));
    CHECK (w.frames() == 120000 && pk > 0.9f);
    // note detection
    auto d = detectNotes (sine (220, 1.2, 0.5f, sr)); CHECK (d.notes.size() == 1 && near (d.notes[0].m, 57, 0.1));
    Audio two; two.sr = sr; two.resize (48000 * 2); for (size_t i = 0; i < two.frames(); ++i) { const double f = i < 48000 ? 220 : 330; two.ch[0][i] = two.ch[1][i] = (float) (0.4 * std::sin (2 * kPi * f * (double) i / sr)); }
    auto dn = detectNotes (two); CHECK (dn.notes.size() == 2 && near (dn.notes[0].m, 57, 0.2) && near (dn.notes[1].m, 64.02, 0.3));
    auto dsh = detectNotes (pitchShift (sine (220, 1.5, 0.5f, sr), 3, true, 0)); CHECK (dsh.notes.size() >= 1 && near (dsh.notes[0].m, 60, 0.2));
    // the editor can run such a change as one undo step with its own label
    { Editor ed; ed.load (sine (220, 0.5, 0.5f, sr)); const size_t n0 = ed.audio.frames(); WarpOptions w2; w2.ratio = 2; w2.mode = 1;
      CHECK (ed.transform ("Warp", [&] (Audio& x) { x = warpAudio (x, w2); })); CHECK (ed.audio.frames() == n0 / 2 && std::string (ed.historyLabel (1)) == "Warp"); ed.undo(); CHECK (ed.audio.frames() == n0); }
    // range shift touches only the range
    Audio rg = sine (220, 1.5, 0.5f, sr); pitchShiftRange (rg, 24000, 48000, 12, true, 0);
    CHECK (near (zcFreq (std::vector<float> (rg.ch[0].begin() + 30000, rg.ch[0].begin() + 42000), sr), 440, 6) && near (zcFreq (std::vector<float> (rg.ch[0].begin() + 2000, rg.ch[0].begin() + 20000), sr), 220, 2));
}

static void testPen()
{
    mc::Editor e; e.audio.sr = 48000.0; e.audio.resize (100);
    CHECK (e.beginPen());
    e.pen (0, 10, 0.f, 20, 1.f);                                   // a ramp: every sample between is set, none outside
    CHECK (std::fabs (e.audio.ch[0][15] - 0.5f) < 1e-6f); CHECK (e.audio.ch[0][9] == 0.f && e.audio.ch[0][21] == 0.f); CHECK (e.audio.ch[1][15] == 0.f);
    e.pen (0, 20, 1.f, 12, -3.f);                                  // backwards, and past full scale: clamped
    CHECK (e.audio.ch[0][12] == -1.f); CHECK (e.audio.ch[0][20] == 1.f);
    e.pen (1, -50, 0.5f, 500, 0.5f);                               // far outside the sound: no crash, only real samples change
    CHECK (e.audio.ch[1][0] == 0.5f && e.audio.ch[1][99] == 0.5f);
    e.undo (); CHECK (e.audio.peak (0, 100) == 0.f);               // the whole stroke is one undo step
    mc::Editor empty; CHECK (! empty.beginPen()); empty.pen (0, 0, 1.f, 5, 1.f);
}

int main()
{
    testTable(); testAudio(); testWav(); testOps(); testEditor(); testRecorder(); testAnalysis(); testHistoryAndTempo(); testSpectral(); testDsp(); testPen();
    std::printf ("%d checks, %d failed\n", checks, failed); return failed ? 1 : 0;
}
