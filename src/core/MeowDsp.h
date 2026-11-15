// Meowcorder core, part 2: resampling, time stretch, pitch shift, warp, pitch and note detection.
// Pure C++17, no JUCE. A port of the "v4" section of docs/mockups/engine.js: same algorithms, same constants, checked with the same cases
// (tests/test_core.cpp, testDsp). Offline and deterministic except the Texture warp mode, which uses random grain positions.
#pragma once
#include "MeowCore.h"
#include <limits>

namespace mc
{
constexpr double kPiD = 3.14159265358979323846;
inline long jsRound (double x) { return (long) std::floor (x + 0.5); }      // Math.round: halves go up
inline bool isMonoAudio (const Audio& a) { for (size_t i = 0; i < a.frames(); ++i) if (a.ch[0][i] != a.ch[1][i]) return false; return true; }

// out[i] reads the input at i / ratio. ratio = outRate / inRate. Windowed sinc, 10 zero crossings each side (more when filtering down).
inline std::vector<float> resampleCh (const std::vector<float>& x, size_t outLen, double ratio)
{
    const long n = (long) x.size(); std::vector<float> o (outLen, 0.f); if (n == 0 || outLen == 0) return o;
    if (std::fabs (ratio - 1) < 1e-9 && outLen == (size_t) n) return x;
    const double fc = std::min (1.0, ratio); const long kw = (long) std::ceil (10.0 / fc);
    for (size_t i = 0; i < outLen; ++i)
    {
        const double p = (double) i / ratio; const long k0 = (long) std::floor (p); double s = 0;
        for (long k = k0 - kw + 1; k <= k0 + kw; ++k)
        {
            if (k < 0 || k >= n) continue; const double d = p - (double) k, xs = d * fc;
            s += (double) x[(size_t) k] * (xs == 0 ? 1.0 : std::sin (kPiD * xs) / (kPiD * xs)) * (0.5 + 0.5 * std::cos (kPiD * d / (double) kw));
        }
        o[i] = (float) (s * fc);
    }
    return o;
}

template <class Fn>
inline Audio perChannel (const Audio& a, Fn fn)
{
    Audio o; o.sr = a.sr; o.ch[0] = fn (a.ch[0]); o.ch[1] = isMonoAudio (a) ? o.ch[0] : fn (a.ch[1]); return o;
}
inline Audio resampleAudio (const Audio& a, double newSr)
{
    const double ratio = newSr / a.sr; const size_t n = (size_t) jsRound ((double) a.frames() * ratio);
    return perChannel (a, [&] (const std::vector<float>& x) { return resampleCh (x, n, ratio); });
}

// Phase vocoder time stretch with peak phase locking. factor = output length / input length. Pitch stays.
inline std::vector<float> pvStretch (const std::vector<float>& x, double factor)
{
    const long n = (long) x.size(), N = kSN, Hs = 512; const double Ha = (double) Hs / factor; const size_t outLen = (size_t) std::max (1L, jsRound ((double) n * factor)); const int K = N / 2 + 1;
    if (n < 64) return resampleCh (x, outLen, (double) outLen / (double) std::max (1L, n));
    const long base = N; std::vector<double> out (outLen + 3 * (size_t) N, 0.0), nrm (outLen + 3 * (size_t) N, 0.0);
    std::vector<std::complex<double>> buf ((size_t) N); const auto& w = detail::swin();
    std::vector<double> mag ((size_t) K), ph ((size_t) K), pph ((size_t) K, 0.0), sph ((size_t) K, 0.0), nph ((size_t) K, 0.0); std::vector<int> pk;
    long prevSt = 0; const long frames = (long) std::ceil ((double) outLen / Hs) + 3; const double TP = 2 * kPiD;
    for (long j = 0; j < frames; ++j)
    {
        const long st = jsRound ((double) j * Ha) - N / 2, os = j * Hs - N / 2;
        for (long i = 0; i < N; ++i) { const long p = st + i; buf[(size_t) i] = { (p >= 0 && p < n ? (double) x[(size_t) p] : 0.0) * w[(size_t) i], 0.0 }; }
        fft (buf);
        for (int k = 0; k < K; ++k) { mag[(size_t) k] = std::abs (buf[(size_t) k]); ph[(size_t) k] = std::arg (buf[(size_t) k]); }
        if (j == 0) sph = ph;
        else
        {
            const double ha = (double) (st - prevSt); pk.clear();
            for (int k = 0; k < K; ++k)
            {
                const double m = mag[(size_t) k];
                if (m > 1e-9 && m >= (k > 0 ? mag[(size_t) k - 1] : 0.0) && m >= (k > 1 ? mag[(size_t) k - 2] : 0.0) && m >= (k + 1 < K ? mag[(size_t) k + 1] : 0.0) && m >= (k + 2 < K ? mag[(size_t) k + 2] : 0.0)) pk.push_back (k);
            }
            if (pk.empty()) sph = ph;
            else
            {
                for (int k : pk) { const double ex = TP * k * ha / N; double d = ph[(size_t) k] - pph[(size_t) k] - ex; d -= TP * std::floor (d / TP + 0.5); nph[(size_t) k] = sph[(size_t) k] + (ex + d) / ha * Hs; }
                size_t pi = 0;
                for (int k = 0; k < K; ++k) { while (pi + 1 < pk.size() && std::abs (pk[pi + 1] - k) < std::abs (pk[pi] - k)) ++pi; const int p = pk[pi]; sph[(size_t) k] = nph[(size_t) p] + (ph[(size_t) k] - ph[(size_t) p]); }
            }
        }
        pph = ph; prevSt = st;
        for (int k = 0; k < K; ++k) { buf[(size_t) k] = std::polar (mag[(size_t) k], sph[(size_t) k]); if (k > 0 && k < N / 2) buf[(size_t) (N - k)] = std::conj (buf[(size_t) k]); }
        buf[0] = { buf[0].real(), 0.0 }; buf[(size_t) N / 2] = { buf[(size_t) N / 2].real(), 0.0 }; detail::ifft (buf);
        for (long i = 0; i < N; ++i) { const long q = base + os + i; if (q >= 0 && q < (long) out.size()) { out[(size_t) q] += buf[(size_t) i].real() * w[(size_t) i]; nrm[(size_t) q] += w[(size_t) i] * w[(size_t) i]; } }
    }
    std::vector<float> o (outLen); for (size_t i = 0; i < outLen; ++i) o[i] = (float) (out[(size_t) base + i] / std::max (0.1, nrm[(size_t) base + i])); return o;
}

// Gives y the spectral envelope (the vowel shape, the formants) of x, moved by fs semitones. x and y have the same length.
inline std::vector<float> formantCorrect (const std::vector<float>& x, const std::vector<float>& y, double fs)
{
    const long n = (long) y.size(), N = kSN; const int K = N / 2 + 1, W = 14; const double fr = std::pow (2.0, fs / 12.0); const auto& win = detail::swin();
    std::vector<double> o ((size_t) n + 3 * (size_t) N, 0.0), nr ((size_t) n + 3 * (size_t) N, 0.0);
    std::vector<std::complex<double>> b1 ((size_t) N), b2 ((size_t) N); std::vector<double> m1 ((size_t) K), m2 ((size_t) K), c1 ((size_t) K + 1), c2 ((size_t) K + 1), e1 ((size_t) K), e2 ((size_t) K);
    auto env = [&] (const std::vector<double>& m, std::vector<double>& c, std::vector<double>& e)
    {
        c[0] = 0; for (int k = 0; k < K; ++k) c[(size_t) k + 1] = c[(size_t) k] + m[(size_t) k] * m[(size_t) k];
        for (int k = 0; k < K; ++k) { const int a = std::max (0, k - W), b = std::min (K - 1, k + W); e[(size_t) k] = std::sqrt ((c[(size_t) b + 1] - c[(size_t) a]) / (double) (b - a + 1)); }
    };
    for (long st = -N; st < n; st += kSH)
    {
        bool any = false;
        for (long i = 0; i < N; ++i) { const long p = st + i; const double ww = win[(size_t) i], a = p >= 0 && p < n ? (double) x[(size_t) p] : 0.0, b = p >= 0 && p < n ? (double) y[(size_t) p] : 0.0; b1[(size_t) i] = { a * ww, 0.0 }; b2[(size_t) i] = { b * ww, 0.0 }; if (b != 0) any = true; }
        if (! any) continue; fft (b1); fft (b2);
        for (int k = 0; k < K; ++k) { m1[(size_t) k] = std::abs (b1[(size_t) k]); m2[(size_t) k] = std::abs (b2[(size_t) k]); }
        env (m1, c1, e1); env (m2, c2, e2);
        for (int k = 0; k < K; ++k)
        {
            const double pos = k / fr; const int k0 = (int) std::floor (pos); const double f = pos - k0;
            const double tgt = k0 + 1 < K ? e1[(size_t) k0] * (1 - f) + e1[(size_t) k0 + 1] * f : (k0 < K ? e1[(size_t) k0] : 0.0);
            const double g = std::min (8.0, std::max (0.05, tgt / std::max (e2[(size_t) k], 1e-7)));
            b2[(size_t) k] *= g; if (k > 0 && k < N / 2) b2[(size_t) (N - k)] *= g;
        }
        detail::ifft (b2);
        for (long i = 0; i < N; ++i) { const long q = st + i + N; if (q >= 0 && q < (long) o.size()) { o[(size_t) q] += b2[(size_t) i].real() * win[(size_t) i]; nr[(size_t) q] += win[(size_t) i] * win[(size_t) i]; } }
    }
    std::vector<float> out ((size_t) n); for (long i = 0; i < n; ++i) out[(size_t) i] = nr[(size_t) (i + N)] > 0.1 ? (float) (o[(size_t) (i + N)] / nr[(size_t) (i + N)]) : y[(size_t) i]; return out;
}

// formant: false follows the pitch (the voice gets smaller or bigger), true keeps the original formants, moved by fshift semitones.
inline std::vector<float> pitchShiftCh (const std::vector<float>& x, double semis, bool formant, double fshift)
{
    const size_t n = x.size(); if (n == 0) return x; const double r = std::pow (2.0, semis / 12.0); std::vector<float> y = x;
    if (std::fabs (semis) > 1e-4) y = resampleCh (pvStretch (x, r), n, 1.0 / r);
    if (formant && (std::fabs (semis) > 1e-4 || std::fabs (fshift) > 1e-4)) y = formantCorrect (x, y, fshift);
    return y;
}
inline Audio pitchShift (const Audio& a, double semis, bool formant, double fshift)
{
    return perChannel (a, [&] (const std::vector<float>& x) { return pitchShiftCh (x, semis, formant, fshift); });
}
// Pitch shifts frames [s, e) only; 15 ms cross fades hide the joins.
inline void pitchShiftRange (Audio& a, size_t s, size_t e, double semis, bool formant, double fshift)
{
    e = std::min (e, a.frames()); if (e < s + 256) return;
    const size_t pad = std::min ({ (size_t) 4096, s, a.frames() - e }), s0 = s - pad, e0 = e + pad; const Audio seg = pitchShift (a.slice (s0, e0), semis, formant, fshift);
    const double f = std::max (1.0, (double) std::min<size_t> ((size_t) jsRound (0.015 * a.sr), (e - s) >> 1));
    for (size_t i = s; i < e; ++i)
    {
        const double k = (double) (i - s), t = std::min ({ 1.0, k / f, (double) (e - 1 - i) / f }), w = t * t * (3 - 2 * t);
        for (int c = 0; c < 2; ++c) a.ch[c][i] = (float) ((double) a.ch[c][i] * (1 - w) + (double) seg.ch[c][i - s0] * w);
    }
}

struct WarpOptions { double ratio = 1.0; int mode = 0; double semis = 0, cents = 0; bool formant = false; double fshift = 0; std::vector<size_t> hits; };

// Tempo change: ratio = speed multiplier (more is faster and shorter). mode: 0 stretch (pitch kept), 1 repitch, 2 beats (hits stay sharp), 3 texture (grains).
inline std::vector<float> warpCh (const std::vector<float>& x, double ratio, int mode, double sr, const std::vector<size_t>& hits)
{
    const long n = (long) x.size(); const size_t outLen = (size_t) std::max (1L, jsRound ((double) n / ratio)); if (std::fabs (ratio - 1) < 1e-6) return x;
    if (mode == 1) return resampleCh (x, outLen, 1.0 / ratio);
    if (mode == 3)
    {
        const long G = jsRound (0.09 * sr), hop = G >> 2; std::vector<float> o (outLen + (size_t) G, 0.f); Rng r;
        for (long g = 0; (size_t) (g * hop) < outLen; ++g)
        {
            const long op = g * hop, sp = jsRound ((double) op * ratio + (r.next01() - 0.5) * (double) G * 0.35);
            for (long i = 0; i < G; ++i) { const long q = sp + i; if (q < 0 || q >= n) continue; o[(size_t) (op + i)] += x[(size_t) q] * (float) (0.5 - 0.5 * std::cos (2 * kPiD * (double) i / (double) G)) * 0.5f; }
        }
        o.resize (outLen); return o;
    }
    if (mode == 2 && ! hits.empty())
    {
        std::vector<float> o (outLen, 0.f); std::vector<long> cuts { 0 }; for (size_t h : hits) if (h > 0 && (long) h < n) cuts.push_back ((long) h); cuts.push_back (n);
        const long hd = jsRound (0.03 * sr), xf = 256;
        for (size_t c = 0; c + 1 < cuts.size(); ++c)
        {
            const long a = cuts[c], b = cuts[c + 1], L = b - a, O = jsRound ((double) a / ratio), Lt = std::min (jsRound ((double) b / ratio) - O, (long) outLen - O); if (Lt <= 0 || L <= 0) continue;
            const std::vector<float> seg (x.begin() + a, x.begin() + b);
            const std::vector<float> S = L < 3000 || Lt < 3000 ? resampleCh (seg, (size_t) Lt, (double) Lt / (double) L) : pvStretch (seg, (double) Lt / (double) L);
            for (long i = 0; i < Lt; ++i) o[(size_t) (O + i)] = S[(size_t) i];
            if (L >= 3000 && Lt >= 3000) { const long hdn = std::min ({ hd, L, Lt }); for (long i = 0; i < hdn; ++i) { const double t = i < hdn - xf ? 0.0 : (double) (i - (hdn - xf)) / (double) xf; o[(size_t) (O + i)] = (float) ((double) seg[(size_t) i] * (1 - t) + (double) S[(size_t) i] * t); } }
            const long fo = std::min (48L, Lt); for (long i = 0; i < fo; ++i) o[(size_t) (O + Lt - 1 - i)] *= (float) ((double) i / (double) fo);
        }
        return o;
    }
    return pvStretch (x, 1.0 / ratio);
}
inline Audio warpAudio (const Audio& a, const WarpOptions& o)
{
    Audio r = o.ratio != 0 && std::fabs (o.ratio - 1) > 1e-6 ? perChannel (a, [&] (const std::vector<float>& x) { return warpCh (x, o.ratio, o.mode, a.sr, o.hits); }) : a;
    const double st = o.semis + o.cents / 100.0;
    if (std::fabs (st) > 1e-4 || (o.fshift != 0 && o.formant)) r = pitchShift (r, st, o.formant, o.fshift);
    return r;
}

// Pitch track (YIN) on a 12 kHz mono copy. midi is NaN where there is no clear pitch.
struct PitchTrack { std::vector<float> t, midi; int hop = 128; };
inline PitchTrack detectPitch (const Audio& a)
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const long D = std::max (1L, jsRound (a.sr / 12000.0)); const double sr2 = a.sr / (double) D; const long m = (long) (a.frames() / (size_t) D); std::vector<float> x ((size_t) std::max (0L, m));
    for (long i = 0; i < m; ++i) { double s = 0; for (long d = 0; d < D; ++d) s += 0.5 * ((double) a.ch[0][(size_t) (i * D + d)] + (double) a.ch[1][(size_t) (i * D + d)]); x[(size_t) i] = (float) (s / (double) D); }
    const long W = 512, hop = 128, tmin = std::max (2L, (long) std::floor (sr2 / 1500)), tmax = (long) std::ceil (sr2 / 60); const long F = std::max (0L, (m - W - tmax) / hop + 1);
    PitchTrack pt; pt.hop = (int) (hop * D); pt.midi.assign ((size_t) F, (float) nan); pt.t.assign ((size_t) F, 0.f); std::vector<double> d ((size_t) tmax + 2, 0.0), cm ((size_t) tmax + 2, 0.0);
    for (long f = 0; f < F; ++f)
    {
        const long t0 = f * hop; double e = 0; for (long j = 0; j < W; ++j) e += (double) x[(size_t) (t0 + j)] * x[(size_t) (t0 + j)];
        pt.t[(size_t) f] = (float) ((double) (t0 + W / 2 + 100) * (double) D); if (std::sqrt (e / (double) W) < 0.004) continue;
        for (long tau = 1; tau <= tmax; ++tau) { double s = 0; for (long j = 0; j < W; ++j) { const double v = (double) x[(size_t) (t0 + j)] - (double) x[(size_t) (t0 + j + tau)]; s += v * v; } d[(size_t) tau] = s; }
        double run = 0; cm[0] = 1; for (long tau = 1; tau <= tmax; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? d[(size_t) tau] * (double) tau / run : 1.0; }
        long best = -1; for (long tau = tmin; tau < tmax; ++tau) { if (cm[(size_t) tau] < 0.15) { while (tau + 1 < tmax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; } }
        if (best < 0) { double mn = 1e9; long mt = -1; for (long tau = tmin; tau < tmax; ++tau) if (cm[(size_t) tau] < mn) { mn = cm[(size_t) tau]; mt = tau; } if (mn < 0.25) best = mt; }
        if (best < 0) continue;
        double tau = (double) best; if (best > 1 && best < tmax) { const double a0 = cm[(size_t) best - 1], b0 = cm[(size_t) best], c0 = cm[(size_t) best + 1], den = a0 - 2 * b0 + c0; if (std::fabs (den) > 1e-12) tau = (double) best + 0.5 * (a0 - c0) / den; }
        pt.midi[(size_t) f] = (float) (69 + 12 * std::log2 (sr2 / tau / 440)); pt.t[(size_t) f] = (float) ((double) (t0 + W / 2) * (double) D + tau / 2 * (double) D);
    }
    return pt;
}

struct Note { long t0 = 0, t1 = 0; double m = 0; };
struct NoteResult { std::vector<Note> notes; PitchTrack track; };
// Groups the pitch track into notes: t0 and t1 in frames, m the middle pitch in (fractional) MIDI notes.
inline NoteResult detectNotes (const Audio& a)
{
    NoteResult res; res.track = detectPitch (a); const auto& p = res.track; const long F = (long) p.midi.size(); const double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> sm ((size_t) F, nan);
    for (long f = 0; f < F; ++f)
    {
        std::vector<double> v; for (long k = -2; k <= 2; ++k) { const long q = f + k; if (q >= 0 && q < F && ! std::isnan (p.midi[(size_t) q])) v.push_back (p.midi[(size_t) q]); }
        if (! std::isnan (p.midi[(size_t) f]) && v.size() >= 3) { std::sort (v.begin(), v.end()); sm[(size_t) f] = v[v.size() >> 1]; }
    }
    struct Cur { double t0, t1; std::vector<double> v; }; bool have = false; Cur cur { 0, 0, {} }; int gap = 0;
    auto med = [] (std::vector<double> v) { std::sort (v.begin(), v.end()); return v[v.size() >> 1]; };
    auto close = [&] { if (have && cur.v.size() >= 4) res.notes.push_back ({ std::max (0L, jsRound (cur.t0 - p.hop / 2.0)), std::min ((long) a.frames(), jsRound (cur.t1 + p.hop / 2.0)), med (cur.v) }); have = false; };
    for (long f = 0; f < F; ++f)
    {
        const double v = sm[(size_t) f];
        if (std::isnan (v)) { if (have && ++gap > 2) close(); continue; } gap = 0;
        if (have) { std::vector<double> last (cur.v.end() - (long) std::min<size_t> (8, cur.v.size()), cur.v.end()); if (std::fabs (v - med (last)) > 0.75) close(); }
        if (! have) { cur = { (double) p.t[(size_t) f], (double) p.t[(size_t) f], { v } }; have = true; } else { cur.t1 = (double) p.t[(size_t) f]; cur.v.push_back (v); }
    }
    close(); return res;
}
} // namespace mc
