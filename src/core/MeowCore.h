// Meowcorder core: an audio recorder and editor engine. Pure C++17, no JUCE.
// Audio buffer, WAV in and out, recorder with pre-roll, edit operations on a selection, undo, repeat last operation,
// peak overview, spectrogram and transient detection. Everything here is offline and deterministic.
#pragma once
#include "MeowOps.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace mc
{
constexpr float kPi = 3.14159265358979f;
inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float dbToLin (float db) { return std::pow (10.0f, db / 20.0f); }
inline float linToDb (float x) { return 20.0f * std::log10 (std::max (1e-9f, x)); }

struct Rng
{
    uint32_t s = 2463534242u;
    float next01() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    float bi() { return next01() * 2.0f - 1.0f; }
};

// ---------- the sound ----------
struct Audio
{
    std::vector<float> ch[2];
    double sr = 48000.0;
    size_t frames() const { return ch[0].size(); }
    void resize (size_t n) { ch[0].assign (n, 0.f); ch[1].assign (n, 0.f); }
    void insertSilence (size_t at, size_t n)
    {
        at = std::min (at, frames());
        for (auto& c : ch) c.insert (c.begin() + (std::ptrdiff_t) at, n, 0.f);
    }
    void erase (size_t a, size_t b)
    {
        b = std::min (b, frames()); a = std::min (a, b);
        for (auto& c : ch) c.erase (c.begin() + (std::ptrdiff_t) a, c.begin() + (std::ptrdiff_t) b);
    }
    Audio slice (size_t a, size_t b) const
    {
        b = std::min (b, frames()); a = std::min (a, b);
        Audio o; o.sr = sr; for (int c = 0; c < 2; ++c) o.ch[c].assign (ch[c].begin() + (std::ptrdiff_t) a, ch[c].begin() + (std::ptrdiff_t) b);
        return o;
    }
    float peak (size_t a, size_t b) const
    {
        float m = 0; b = std::min (b, frames());
        for (int c = 0; c < 2; ++c) for (size_t i = a; i < b; ++i) m = std::max (m, std::fabs (ch[c][i]));
        return m;
    }
};

// ---------- small DSP blocks (same maths as the other Studio 87 plugins) ----------
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void norm (double B0, double B1, double B2, double A0, double A1, double A2)
    { b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0); }
    void peak (double sr, double f, double gDb, double q)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double sr, double f, double gDb)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), t = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
              (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
    }
    void highShelf (double sr, double f, double gDb)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), t = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
              (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
    }
    float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};

struct Svf
{
    float g = 0.1f, k = 1.f, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0, lp = 0, bp = 0, hp = 0;
    void set (double sr, double f, double q)
    { f = std::min (f, sr * 0.45); g = (float) std::tan (kPi * f / sr); k = (float) (1.0 / q); a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    void tick (float x)
    { const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; bp = v1; lp = v2; hp = x - k * v1 - v2; }
    void reset() { ic1 = ic2 = 0; }
};

// PurrOne colour stages: Drive (symmetric clip), Transistor (asymmetric, even harmonics), Fuzz (lopsided, nasty above 80 percent).
struct Colour
{
    static float smoothstep01 (float t) { t = t < 0 ? 0 : (t > 1 ? 1 : t); return t * t * (3 - 2 * t); }
    static float asymTanh (float u, float nl) { return u >= 0 ? std::tanh (u) : nl * std::tanh (u / nl); }
    static float shaper (float x, float drive, float xs, float fuzz)
    {
        const float dn = smoothstep01 ((drive - 0.8f) * 5.0f), xn = smoothstep01 ((xs - 0.8f) * 5.0f), fn = smoothstep01 ((fuzz - 0.8f) * 5.0f);
        float y = x;
        if (drive > 0.001f)
        {
            y = std::tanh (x * (1.0f + drive * 5.0f + dn * 30.0f));
            if (dn > 0.0f) y += dn * (std::tanh (y * (2.0f + 10.0f * dn)) - y);
        }
        if (xs > 0.001f)
        {
            const float nl = std::max (0.2f, 1.0f - 0.3f * xs - 0.25f * xn);
            const float u = y * (1.0f + xs * 5.0f + xn * 30.0f);
            y = asymTanh (u, nl);
            y += (0.3f * xs + 0.25f * xn) * y * y;
            if (xn > 0.0f) { const float y2 = asymTanh (y * (2.0f + 10.0f * xn) + 0.15f * xn, nl); y += xn * (y2 - y); }
        }
        if (fuzz > 0.001f)
        {
            const float fz = 1.0f + fuzz * 14.0f + fn * 70.0f, bf = 0.5f * fuzz;
            y = std::tanh (y * fz + bf) - std::tanh (bf);
            if (fn > 0.0f)
            {
                const float r = std::sqrt (y * y + 0.02f) - 0.1414f;
                y += fn * 0.7f * (r - 0.3f);
                y = std::tanh (y * (1.0f + 0.8f * fn));
            }
        }
        return y;
    }
    // Gain that keeps the level about the same as the clean path, so colour adds grit and not volume.
    static float makeup (float drive, float xs, float fuzz, float ref = 0.35f)
    {
        double a = 0, b = 0;
        for (int i = 0; i < 64; ++i)
        {
            const float x = ref * std::sin (6.2831853f * (float) i / 64.0f);
            const float y1 = shaper (x, drive, xs, fuzz);
            a += (double) x * x; b += (double) y1 * y1;
        }
        const float m = b > 1e-9 ? (float) std::sqrt (a / b) : 1.0f;
        return clampf (m, 0.25f, 4.0f);
    }
};

// ---------- WAV ----------
inline void put16 (std::vector<uint8_t>& o, uint32_t v) { o.push_back ((uint8_t) (v & 255)); o.push_back ((uint8_t) ((v >> 8) & 255)); }
inline void put32 (std::vector<uint8_t>& o, uint32_t v) { put16 (o, v & 0xffff); put16 (o, v >> 16); }
inline void putTag (std::vector<uint8_t>& o, const char* t) { for (int i = 0; i < 4; ++i) o.push_back ((uint8_t) t[i]); }

// bits: 16, 24 (integer) or 32 (float). Always stereo.
inline std::vector<uint8_t> encodeWav (const Audio& a, int bits)
{
    const bool flt = bits == 32; const int bps = bits / 8; const uint32_t n = (uint32_t) a.frames(), dataBytes = n * 2 * (uint32_t) bps;
    std::vector<uint8_t> o; o.reserve (44 + dataBytes);
    putTag (o, "RIFF"); put32 (o, 36 + dataBytes); putTag (o, "WAVE"); putTag (o, "fmt "); put32 (o, 16);
    put16 (o, flt ? 3 : 1); put16 (o, 2); put32 (o, (uint32_t) a.sr); put32 (o, (uint32_t) a.sr * 2 * (uint32_t) bps); put16 (o, (uint32_t) (2 * bps)); put16 (o, (uint32_t) bits);
    putTag (o, "data"); put32 (o, dataBytes);
    for (uint32_t i = 0; i < n; ++i)
        for (int c = 0; c < 2; ++c)
        {
            const float x = clampf (a.ch[c][i], -1.0f, 1.0f);
            if (flt) { uint32_t u; std::memcpy (&u, &x, 4); put32 (o, u); }
            else if (bits == 16) { const int v = (int) std::lrintf (x * 32767.0f); put16 (o, (uint32_t) (v & 0xffff)); }
            else { const int v = (int) std::lrintf (x * 8388607.0f); o.push_back ((uint8_t) (v & 255)); o.push_back ((uint8_t) ((v >> 8) & 255)); o.push_back ((uint8_t) ((v >> 16) & 255)); }
        }
    return o;
}

// Reads PCM 8/16/24/32 bit and float 32/64 bit WAV, mono or more (first two channels kept). Returns false if it cannot.
inline bool decodeWav (const uint8_t* d, size_t n, Audio& out)
{
    auto r16 = [&] (size_t p) { return (uint32_t) d[p] | ((uint32_t) d[p + 1] << 8); };
    auto r32 = [&] (size_t p) { return r16 (p) | (r16 (p + 2) << 16); };
    if (n < 12 || std::memcmp (d, "RIFF", 4) || std::memcmp (d + 8, "WAVE", 4)) return false;
    uint32_t fmt = 0, chans = 0, rate = 0, bits = 0; size_t dataPos = 0, dataLen = 0; size_t p = 12;
    while (p + 8 <= n)
    {
        const uint32_t sz = r32 (p + 4); const size_t body = p + 8;
        if (!std::memcmp (d + p, "fmt ", 4) && sz >= 16 && body + 16 <= n)
        {
            fmt = r16 (body); chans = r16 (body + 2); rate = r32 (body + 4); bits = r16 (body + 14);
            if (fmt == 0xFFFE && sz >= 26 && body + 26 <= n) fmt = r16 (body + 24);
        }
        else if (!std::memcmp (d + p, "data", 4)) { dataPos = body; dataLen = std::min<size_t> (sz, n - body); break; }
        p = body + sz + (sz & 1);
    }
    if (!dataPos || !chans || !rate || (fmt != 1 && fmt != 3)) return false;
    const size_t bps = bits / 8; if (bps == 0 || (fmt == 1 && bits != 8 && bits != 16 && bits != 24 && bits != 32) || (fmt == 3 && bits != 32 && bits != 64)) return false;
    const size_t frames = dataLen / (bps * chans);
    out.sr = rate; out.resize (frames);
    for (size_t i = 0; i < frames; ++i)
        for (uint32_t c = 0; c < std::min<uint32_t> (chans, 2); ++c)
        {
            const uint8_t* q = d + dataPos + (i * chans + c) * bps; float v = 0;
            if (fmt == 3 && bits == 32) { uint32_t u = (uint32_t) q[0] | ((uint32_t) q[1] << 8) | ((uint32_t) q[2] << 16) | ((uint32_t) q[3] << 24); std::memcpy (&v, &u, 4); }
            else if (fmt == 3) { uint64_t u = 0; for (int k = 7; k >= 0; --k) u = (u << 8) | q[k]; double dv; std::memcpy (&dv, &u, 8); v = (float) dv; }
            else if (bits == 8) v = ((float) q[0] - 128.0f) / 128.0f;
            else if (bits == 16) v = (float) (int16_t) (q[0] | (q[1] << 8)) / 32768.0f;
            else if (bits == 24) { int32_t s = (int32_t) (((uint32_t) q[0] << 8) | ((uint32_t) q[1] << 16) | ((uint32_t) q[2] << 24)); v = (float) (s >> 8) / 8388608.0f; }
            else { int32_t s = (int32_t) ((uint32_t) q[0] | ((uint32_t) q[1] << 8) | ((uint32_t) q[2] << 16) | ((uint32_t) q[3] << 24)); v = (float) ((double) s / 2147483648.0); }
            out.ch[c][i] = v;
        }
    if (chans == 1) out.ch[1] = out.ch[0];
    return true;
}

// ---------- analysis ----------
inline void fft (std::vector<std::complex<double>>& x)
{
    const size_t n = x.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (x[i], x[j]); }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * 3.14159265358979323846 / (double) len; const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w (1, 0); for (size_t k = 0; k < len / 2; ++k) { const auto u = x[i + k], v = x[i + k + len / 2] * w; x[i + k] = u + v; x[i + k + len / 2] = u - v; w *= wl; } }
    }
}

struct Spectrogram { int bins = 0, frames = 0; std::vector<float> db; float at (int f, int b) const { return db[(size_t) f * (size_t) bins + (size_t) b]; } };

// Magnitude in dBFS: a full-scale sine reads 0 dB at its bin. fftSize must be a power of two.
inline Spectrogram spectrogram (const Audio& a, size_t from, size_t to, int fftSize, int hop)
{
    Spectrogram s; to = std::min (to, a.frames()); from = std::min (from, to); const size_t n = to - from;
    s.bins = fftSize / 2 + 1; s.frames = n <= (size_t) fftSize ? 1 : (int) ((n - (size_t) fftSize) / (size_t) hop) + 1; s.db.assign ((size_t) s.frames * (size_t) s.bins, -120.f);
    std::vector<double> w ((size_t) fftSize); double ws = 0; for (int i = 0; i < fftSize; ++i) { w[(size_t) i] = 0.5 - 0.5 * std::cos (2 * 3.14159265358979323846 * i / fftSize); ws += w[(size_t) i]; }
    std::vector<std::complex<double>> buf ((size_t) fftSize);
    for (int f = 0; f < s.frames; ++f)
    {
        const size_t st = from + (size_t) f * (size_t) hop;
        for (int i = 0; i < fftSize; ++i) { const size_t k = st + (size_t) i; const double m = k < to ? 0.5 * ((double) a.ch[0][k] + (double) a.ch[1][k]) : 0.0; buf[(size_t) i] = m * w[(size_t) i]; }
        fft (buf);
        for (int b = 0; b < s.bins; ++b) { const double mag = std::abs (buf[(size_t) b]) * 2.0 / ws; s.db[(size_t) f * (size_t) s.bins + (size_t) b] = (float) std::max (-120.0, 20.0 * std::log10 (std::max (1e-9, mag))); }
    }
    return s;
}

struct Peaks { std::vector<float> mn, mx; };
// Min and max over both channels for `cols` columns across [from, to).
inline Peaks peakOverview (const Audio& a, size_t from, size_t to, size_t cols)
{
    Peaks p; p.mn.assign (cols, 0.f); p.mx.assign (cols, 0.f); to = std::min (to, a.frames()); if (cols == 0 || to <= from) return p;
    const double per = (double) (to - from) / (double) cols;
    for (size_t c = 0; c < cols; ++c)
    {
        size_t x0 = from + (size_t) (c * per), x1 = from + (size_t) ((c + 1) * per); if (x1 <= x0) x1 = x0 + 1; x1 = std::min (x1, to);
        float lo = 0, hi = 0; for (size_t i = x0; i < x1; ++i) for (int k = 0; k < 2; ++k) { lo = std::min (lo, a.ch[k][i]); hi = std::max (hi, a.ch[k][i]); }
        p.mn[c] = lo; p.mx[c] = hi;
    }
    return p;
}

// Hits: frames where the short-term energy jumps well above its recent average. sens 0..1, higher finds more.
inline std::vector<size_t> detectTransients (const Audio& a, float sens)
{
    std::vector<size_t> out; const size_t hop = 256, frames = a.frames() / hop; if (frames < 4) return out;
    std::vector<float> e (frames);
    for (size_t f = 0; f < frames; ++f) { double s = 0; for (size_t i = 0; i < hop; ++i) { const float x = 0.5f * (a.ch[0][f * hop + i] + a.ch[1][f * hop + i]); s += (double) x * x; } e[f] = (float) std::sqrt (s / (double) hop); }
    const float ratio = 1.4f + 2.2f * (1.0f - clampf (sens, 0.f, 1.f)); const size_t minGap = (size_t) (0.08 * a.sr / (double) hop) + 1; size_t last = 0; bool any = false;
    for (size_t f = 2; f + 1 < frames; ++f)
    {
        float avg = 0; const size_t a0 = f > 24 ? f - 24 : 0; for (size_t k = a0; k < f; ++k) avg += e[k]; avg /= (float) (f - a0);
        const bool onset = e[f] > ratio * avg + 0.01f && e[f] > e[f - 1] && e[f - 1] <= e[f - 2] * 1.5f + 1e-4f;
        if (onset && (!any || f - last >= minGap)) { out.push_back (f * hop); last = f; any = true; }
    }
    return out;
}

// ---------- operations ----------
inline void defaultParams (int op, float* p) { for (int i = 0; i < kMaxOpParams; ++i) p[i] = i < kOps[op].nParams ? kOps[op].params[i].def : 0.f; }

namespace detail
{
inline float hermite (const std::vector<float>& d, double pos)
{
    const long n = (long) d.size(); if (n == 0) return 0.f; const long i1 = (long) std::floor (pos); const float f = (float) (pos - (double) i1);
    auto at = [&] (long i) { return d[(size_t) std::max (0L, std::min (n - 1, i))]; };
    const float y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
    const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + y1;
}
}

// Applies one operation to frames [s, e). Returns the new end of that range (changes only for ops that change length).
inline size_t applyOp (Audio& a, int op, size_t s, size_t e, const float* p)
{
    e = std::min (e, a.frames()); if (s >= e) return e; const double sr = a.sr; const size_t n = e - s;
    switch (op)
    {
        case opGain: { const float g = dbToLin (p[0]); for (auto& c : a.ch) for (size_t i = s; i < e; ++i) c[i] *= g; break; }
        case opNormalize:
        {
            float g = 1.f;
            if (p[1] < 0.5f) { const float pk = a.peak (s, e); if (pk < 1e-9f) break; g = dbToLin (p[0]) / pk; }
            else { double sum = 0; for (auto& c : a.ch) for (size_t i = s; i < e; ++i) sum += (double) c[i] * c[i]; const float rms = (float) std::sqrt (sum / (2.0 * (double) n)); if (rms < 1e-9f) break; g = dbToLin (p[0]) / rms; const float pk = a.peak (s, e); if (pk * g > 1.f) g = 1.f / pk; }
            for (auto& c : a.ch) for (size_t i = s; i < e; ++i) c[i] *= g;
            break;
        }
        case opFadeIn: case opFadeOut:
            for (size_t i = 0; i < n; ++i) { float t = n > 1 ? (float) i / (float) (n - 1) : 1.f; if (op == opFadeOut) t = 1.f - t; const float g = std::pow (t, p[0]); a.ch[0][s + i] *= g; a.ch[1][s + i] *= g; }
            break;
        case opReverse: for (auto& c : a.ch) std::reverse (c.begin() + (std::ptrdiff_t) s, c.begin() + (std::ptrdiff_t) e); break;
        case opInvert: for (auto& c : a.ch) for (size_t i = s; i < e; ++i) c[i] = -c[i]; break;
        case opDcRemove: for (auto& c : a.ch) { double m = 0; for (size_t i = s; i < e; ++i) m += c[i]; const float mean = (float) (m / (double) n); for (size_t i = s; i < e; ++i) c[i] -= mean; } break;
        case opSilence: for (auto& c : a.ch) for (size_t i = s; i < e; ++i) c[i] = 0.f; break;
        case opMono:
            for (size_t i = s; i < e; ++i) { const int m = (int) std::lround (p[0]); const float v = m == 0 ? 0.5f * (a.ch[0][i] + a.ch[1][i]) : (m == 1 ? a.ch[0][i] : a.ch[1][i]); a.ch[0][i] = a.ch[1][i] = v; }
            break;
        case opPan: { const float b = clampf (p[0] * 0.01f, -1.f, 1.f), gl = b > 0 ? 1.f - b : 1.f, gr = b < 0 ? 1.f + b : 1.f; for (size_t i = s; i < e; ++i) { a.ch[0][i] *= gl; a.ch[1][i] *= gr; } break; }
        case opRate:
        {
            const double ratio = std::pow (2.0, ((double) p[0] + (double) p[1] / 100.0) / 12.0); const size_t newN = std::max<size_t> (1, (size_t) std::llround ((double) n / ratio));
            for (auto& c : a.ch)
            {
                std::vector<float> src (c.begin() + (std::ptrdiff_t) s, c.begin() + (std::ptrdiff_t) e);
                if (ratio > 1.0) { Svf f1, f2; const double fc = 0.45 * sr / ratio; f1.set (sr, fc, 0.707); f2.set (sr, fc, 0.707); for (auto& x : src) { f1.tick (x); f2.tick (f1.lp); x = f2.lp; } }
                std::vector<float> dst (newN); for (size_t i = 0; i < newN; ++i) dst[i] = detail::hermite (src, (double) i * ratio);
                c.erase (c.begin() + (std::ptrdiff_t) s, c.begin() + (std::ptrdiff_t) e); c.insert (c.begin() + (std::ptrdiff_t) s, dst.begin(), dst.end());
            }
            return s + newN;
        }
        case opBitcrush:
        {
            const float bits = std::round (clampf (p[0], 1.f, 16.f)), levels = std::pow (2.f, bits - 1.f); const int down = std::max (1, (int) std::lround (p[1])); const bool dith = p[2] >= 0.5f; const float mix = p[3] * 0.01f;
            for (int c = 0; c < 2; ++c)
            {
                Rng rng; float held = 0;
                for (size_t i = 0; i < n; ++i)
                {
                    const float x = a.ch[c][s + i];
                    if (i % (size_t) down == 0) { const float d = dith ? (rng.next01() - rng.next01()) : 0.f; held = std::round (x * levels + d) / levels; }
                    a.ch[c][s + i] = x * (1.f - mix) + held * mix;
                }
            }
            break;
        }
        case opDrive:
        {
            const float d = p[0] * 0.01f, x = p[1] * 0.01f, f = p[2] * 0.01f, mk = Colour::makeup (d, x, f);
            for (int c = 0; c < 2; ++c) { float px = 0, py = 0; for (size_t i = s; i < e; ++i) { const float y = Colour::shaper (a.ch[c][i], d, x, f) * mk, o = y - px + 0.9995f * py; px = y; py = o; a.ch[c][i] = o; } }
            break;
        }
        case opEq:
            for (int c = 0; c < 2; ++c)
            {
                Biquad lo, mid, hi; const bool l = std::fabs (p[0]) > 0.01f, m = std::fabs (p[1]) > 0.01f, h = std::fabs (p[4]) > 0.01f;
                if (l) lo.lowShelf (sr, 120.0, p[0]);
                if (m) mid.peak (sr, p[2], p[1], p[3]);
                if (h) hi.highShelf (sr, 8000.0, p[4]);
                for (size_t i = s; i < e; ++i) { float y = a.ch[c][i]; if (l) y = lo.tick (y); if (m) y = mid.tick (y); if (h) y = hi.tick (y); a.ch[c][i] = y; }
            }
            break;
        case opFilter:
            for (int c = 0; c < 2; ++c)
            {
                Svf f; f.set (sr, p[1], p[2]); const int t = (int) std::lround (p[0]);
                for (size_t i = s; i < e; ++i) { f.tick (a.ch[c][i]); a.ch[c][i] = t == 0 ? f.lp : (t == 1 ? f.hp : f.bp * f.k); }
            }
            break;
        case opDelay:
        {
            const size_t tail = (size_t) std::llround ((double) p[3] * sr); if (tail) { a.insertSilence (e, tail); e += tail; }
            const size_t d = std::max<size_t> (1, (size_t) std::llround ((double) p[0] * sr / 1000.0)); const float fb = p[1] * 0.01f, mix = p[2] * 0.01f;
            for (int c = 0; c < 2; ++c) { std::vector<float> buf (d, 0.f); size_t w = 0; for (size_t i = s; i < e; ++i) { const float in = a.ch[c][i], y = buf[w]; buf[w] = in + y * fb; a.ch[c][i] = in + y * mix; if (++w >= d) w = 0; } }
            return e;
        }
        case opChorus:
        {
            const double base = 0.015 * sr, depth = (double) p[1] * 0.001 * sr; const float mix = p[2] * 0.01f; const size_t L = (size_t) (0.05 * sr) + 8;
            for (int c = 0; c < 2; ++c)
            {
                std::vector<float> buf (L, 0.f); size_t w = 0; const double ph0 = c ? 0.25 : 0.0;
                for (size_t i = 0; i < n; ++i)
                {
                    const float in = a.ch[c][s + i]; buf[w] = in;
                    const double dl = base + depth * std::sin (2.0 * 3.14159265358979323846 * ((double) p[0] * (double) i / sr + ph0));
                    double rp = (double) w - dl; while (rp < 0) rp += (double) L; const size_t i0 = (size_t) rp % L, i1 = (i0 + 1) % L; const float fr = (float) (rp - std::floor (rp)), wet = buf[i0] + (buf[i1] - buf[i0]) * fr;
                    a.ch[c][s + i] = in * (1.f - mix) + wet * mix; if (++w >= L) w = 0;
                }
            }
            break;
        }
        case opPhaser:
        {
            int stages = (int) std::lround (p[3]); stages = std::max (2, std::min (12, stages & ~1)); const float depth = p[1] * 0.01f, fb = p[2] * 0.01f, mix = p[4] * 0.01f;
            for (int c = 0; c < 2; ++c)
            {
                float z[12] = { 0 }; float last = 0; const double ph0 = c ? 0.25 : 0.0;
                for (size_t i = 0; i < n; ++i)
                {
                    const double lfo = 0.5 + 0.5 * std::sin (2.0 * 3.14159265358979323846 * ((double) p[0] * (double) i / sr + ph0));
                    const double f = 200.0 * std::pow (20.0, (double) depth * lfo), t = std::tan (3.14159265358979323846 * std::min (f, sr * 0.45) / sr), co = (float) ((t - 1.0) / (t + 1.0)) * 1.0;
                    const float cf = (float) co, in = a.ch[c][s + i]; float x = in + fb * last;
                    for (int k = 0; k < stages; ++k) { const float y = -cf * x + z[k]; z[k] = x + cf * y; x = y; }
                    last = x; a.ch[c][s + i] = in * (1.f - mix) + x * mix;
                }
            }
            break;
        }
        case opReverb:
        {
            const size_t tail = (size_t) std::llround ((double) p[3] * sr); if (tail) { a.insertSilence (e, tail); e += tail; }
            static const int combT[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }, apT[4] = { 556, 441, 341, 225 };
            const double sc = sr / 44100.0; const float size = p[0] * 0.01f, fbk = 0.7f + 0.28f * size, damp = 0.4f * p[1] * 0.01f, mix = p[2] * 0.01f;
            struct Comb { std::vector<float> b; size_t i = 0; float st = 0; };
            struct Ap { std::vector<float> b; size_t i = 0; };
            // keep an untouched mono input and the dry sound, then render each channel
            std::vector<float> mono (e - s); for (size_t i = s; i < e; ++i) mono[i - s] = (a.ch[0][i] + a.ch[1][i]) * 0.015f;
            std::vector<float> dryL (a.ch[0].begin() + (std::ptrdiff_t) s, a.ch[0].begin() + (std::ptrdiff_t) e), dryR (a.ch[1].begin() + (std::ptrdiff_t) s, a.ch[1].begin() + (std::ptrdiff_t) e);
            for (int c = 0; c < 2; ++c)
            {
                Comb cb[8]; Ap ap[4]; const int spread = c ? 23 : 0;
                for (int k = 0; k < 8; ++k) cb[k].b.assign ((size_t) std::max (8.0, std::round ((combT[k] + spread) * sc)), 0.f);
                for (int k = 0; k < 4; ++k) ap[k].b.assign ((size_t) std::max (8.0, std::round ((apT[k] + spread) * sc)), 0.f);
                for (size_t i = 0; i < mono.size(); ++i)
                {
                    float o = 0;
                    for (int k = 0; k < 8; ++k) { Comb& q = cb[k]; const float y = q.b[q.i]; q.st = y * (1.f - damp) + q.st * damp; q.b[q.i] = mono[i] + q.st * fbk; if (++q.i >= q.b.size()) q.i = 0; o += y; }
                    for (int k = 0; k < 4; ++k) { Ap& q = ap[k]; const float bo = q.b[q.i]; const float y = -o + bo; q.b[q.i] = o + bo * 0.5f; if (++q.i >= q.b.size()) q.i = 0; o = y; }
                    const float dry = c ? dryR[i] : dryL[i];
                    a.ch[c][s + i] = dry * (1.f - mix) + o * 3.0f * mix;
                }
            }
            return e;
        }
        case opCompressor:
        {
            const float thr = p[0], ratio = p[1], makeup = p[4], knob = 6.0f; const float ca = 1.f - std::exp (-1.f / (std::max (0.1f, p[2]) * 0.001f * (float) sr)), cr = 1.f - std::exp (-1.f / (std::max (1.f, p[3]) * 0.001f * (float) sr)); float gr = 0;
            for (size_t i = s; i < e; ++i)
            {
                const float lv = linToDb (std::max (std::fabs (a.ch[0][i]), std::fabs (a.ch[1][i]))), over = lv - thr; float target = 0;
                if (2.f * over > knob) target = (1.f / ratio - 1.f) * over; else if (2.f * std::fabs (over) <= knob) target = (1.f / ratio - 1.f) * (over + knob * 0.5f) * (over + knob * 0.5f) / (2.f * knob);
                gr += (target - gr) * (target < gr ? ca : cr); const float g = dbToLin (gr + makeup); a.ch[0][i] *= g; a.ch[1][i] *= g;
            }
            break;
        }
        case opLimiter:
        {
            const float ceilL = dbToLin (p[0]); const size_t W = std::max<size_t> (1, (size_t) std::llround (0.002 * sr)); const float rc = 1.f - std::exp (-1.f / (std::max (1.f, p[1]) * 0.001f * (float) sr));
            std::vector<float> g (n), hm (n); for (size_t i = 0; i < n; ++i) { const float m = std::max (std::fabs (a.ch[0][s + i]), std::fabs (a.ch[1][s + i])); g[i] = m > ceilL ? ceilL / m : 1.f; }
            for (size_t i = 0; i < n; ++i) { float m = 1.f; const size_t hi = std::min (n - 1, i + W); for (size_t j = i; j <= hi; ++j) m = std::min (m, g[j]); hm[i] = m; }
            float r = 1.f;
            for (size_t i = 0; i < n; ++i)
            {
                // moving average of hm over [i-W, i]; before the start the first value is repeated
                double sum = 0; for (size_t k = 0; k <= W; ++k) { const long idx = (long) i - (long) k; sum += hm[(size_t) std::max (0L, idx)]; }
                const float ma = (float) (sum / (double) (W + 1)); r = std::min (ma, r + (1.f - r) * rc);
                a.ch[0][s + i] *= r; a.ch[1][s + i] *= r;
            }
            break;
        }
        case opGate:
        {
            const float thr = dbToLin (p[0]), range = dbToLin (p[4]); const float ca = 1.f - std::exp (-1.f / (std::max (0.1f, p[1]) * 0.001f * (float) sr)), cr = 1.f - std::exp (-1.f / (std::max (1.f, p[3]) * 0.001f * (float) sr));
            const long holdN = (long) std::llround ((double) p[2] * 0.001 * sr); long hold = 0; float g = range;
            for (size_t i = s; i < e; ++i)
            {
                const float det = std::max (std::fabs (a.ch[0][i]), std::fabs (a.ch[1][i])); bool open = false;
                if (det >= thr) { hold = holdN; open = true; } else if (hold > 0) { --hold; open = true; }
                const float t = open ? 1.f : range; g += (t - g) * (t > g ? ca : cr); a.ch[0][i] *= g; a.ch[1][i] *= g;
            }
            break;
        }
        case opTrimSilence:
        {
            const float thr = dbToLin (p[0]); size_t first = e, last = s; bool any = false;
            for (size_t i = s; i < e; ++i) if (std::max (std::fabs (a.ch[0][i]), std::fabs (a.ch[1][i])) >= thr) { if (!any) first = i; last = i; any = true; }
            if (!any) break;
            a.erase (last + 1, e); a.erase (s, first); return s + (last - first + 1);
        }
        case opLoopXfade:
        {
            const size_t xf = std::min<size_t> ((size_t) std::llround ((double) p[0] * 0.001 * sr), n / 2); if (xf < 1) break;
            for (int c = 0; c < 2; ++c)
            {
                for (size_t k = 0; k < xf; ++k) { const float t = ((float) k + 0.5f) / (float) xf, wi = std::sin (t * kPi * 0.5f), wo = std::cos (t * kPi * 0.5f); a.ch[c][s + k] = a.ch[c][s + k] * wi + a.ch[c][e - xf + k] * wo; }
            }
            a.erase (e - xf, e); return e - xf;
        }
        default: break;
    }
    return e;
}

// ---------- the editor: selection, undo, clipboard, repeat last operation ----------
class Editor
{
public:
    Audio audio; size_t selA = 0, selB = 0; // no selection when selA == selB: operations then use the whole sound
    size_t cursor = 0;                      // where Paste goes when nothing is selected; set by clicks, Cut and Delete

    struct LastOp { int op = -1; float p[kMaxOpParams] = { 0 }; } last;

    void load (const Audio& a) { audio = a; selA = selB = 0; cursor = 0; undoStack.clear(); redoStack.clear(); last = LastOp(); }
    bool hasSelection() const { return selB > selA; }
    void setSelection (size_t a, size_t b) { if (a > b) std::swap (a, b); selA = std::min (a, audio.frames()); selB = std::min (b, audio.frames()); cursor = selA; }
    void setCursor (size_t c) { cursor = std::min (c, audio.frames()); selA = selB = 0; }
    size_t rangeStart() const { return hasSelection() ? selA : 0; }
    size_t rangeEnd() const { return hasSelection() ? selB : audio.frames(); }

    bool apply (int op, const float* p)
    {
        if (op < 0 || op >= opCount || audio.frames() == 0) return false;
        pushUndo(); const size_t s = rangeStart(), e = rangeEnd(); const size_t ne = applyOp (audio, op, s, e, p);
        if (hasSelection()) selB = ne; else if (kOps[op].changesLength) { selA = selB = 0; }
        last.op = op; for (int i = 0; i < kMaxOpParams; ++i) last.p[i] = p[i]; return true;
    }
    bool repeatLast() { return last.op >= 0 && apply (last.op, last.p); }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void undo() { if (undoStack.empty()) return; redoStack.push_back (snap()); restore (undoStack.back()); undoStack.pop_back(); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back (snap()); restore (redoStack.back()); redoStack.pop_back(); }

    void copy() { if (hasSelection()) clip = audio.slice (selA, selB); }
    void cut() { if (!hasSelection()) return; clip = audio.slice (selA, selB); pushUndo(); audio.erase (selA, selB); cursor = selA; selB = selA; }
    void deleteSelection() { if (!hasSelection()) return; pushUndo(); audio.erase (selA, selB); cursor = selA; selB = selA; }
    void cropToSelection() { if (!hasSelection()) return; pushUndo(); audio = audio.slice (selA, selB); selA = 0; selB = audio.frames(); }
    void paste() { if (clip.frames() == 0) return; pushUndo(); const size_t at = hasSelection() ? selA : std::min (cursor, audio.frames()); insert (at, clip); selA = at; selB = at + clip.frames(); }
    void insertSilenceAtSelection (size_t n) { pushUndo(); const size_t at = rangeStart(); audio.insertSilence (at, n); selA = at; selB = at + n; }
    // Puts a recording into the sound. mode: 0 replace everything, 1 append at the end, 2 replace the selection (punch), 3 insert at the start of the selection.
    void takeRecording (const Audio& rec, int mode)
    {
        if (rec.frames() == 0) return;
        if (mode == 0 || audio.frames() == 0) { load (rec); return; }
        pushUndo();
        if (mode == 1) { const size_t at = audio.frames(); insert (at, rec); selA = at; selB = at + rec.frames(); }
        else if (mode == 2 && hasSelection()) { const size_t a = selA; audio.erase (selA, selB); insert (a, rec); selA = a; selB = a + rec.frames(); }
        else { const size_t at = rangeStart(); insert (at, rec); selA = at; selB = at + rec.frames(); }
    }
    // Moves a position to the nearest point where the wave crosses zero, within radius frames.
    size_t snapToZero (size_t pos, size_t radius) const
    {
        const size_t n = audio.frames(); if (n < 2) return pos; pos = std::min (pos, n - 1); auto m = [&] (size_t i) { return 0.5f * (audio.ch[0][i] + audio.ch[1][i]); };
        for (size_t d = 0; d <= radius; ++d)
        {
            if (pos >= d && pos - d + 1 < n && m (pos - d) * m (pos - d + 1) <= 0.f) return pos - d;
            if (pos + d + 1 < n && m (pos + d) * m (pos + d + 1) <= 0.f) return pos + d;
        }
        return pos;
    }
    const Audio& clipboard() const { return clip; }
    void setUndoLimitBytes (size_t b) { limitBytes = b; }

private:
    struct Snap { Audio a; size_t sa, sb; };
    std::vector<Snap> undoStack, redoStack; Audio clip; size_t limitBytes = 400u * 1024u * 1024u;
    Snap snap() const { return { audio, selA, selB }; }
    void restore (const Snap& s) { audio = s.a; selA = s.sa; selB = s.sb; }
    void insert (size_t at, const Audio& x) { for (int c = 0; c < 2; ++c) audio.ch[c].insert (audio.ch[c].begin() + (std::ptrdiff_t) std::min (at, audio.frames()), x.ch[c].begin(), x.ch[c].end()); }
    void pushUndo()
    {
        undoStack.push_back (snap()); redoStack.clear();
        size_t bytes = 0; for (auto& s : undoStack) bytes += s.a.frames() * 8; while (undoStack.size() > 1 && bytes > limitBytes) { bytes -= undoStack.front().a.frames() * 8; undoStack.erase (undoStack.begin()); }
    }
};

// ---------- the recorder: always listening, so it can include what happened just before you pressed record ----------
class Recorder
{
public:
    enum Mode { Off, Immediate, Threshold };
    void prepare (double sampleRate, double captureSeconds = 30.0)
    {
        sr = sampleRate; cap = (size_t) (captureSeconds * sampleRate) + 1; ring[0].assign (cap, 0.f); ring[1].assign (cap, 0.f); w = 0; filled = 0; recording = false; mode = Off; rec = Audio(); rec.sr = sr;
    }
    // Immediate: starts at once. Threshold: waits until the signal passes thresholdDb. preRollMs of earlier sound is included.
    void arm (Mode m, float thresholdDb = -40.f, double preRollMs = 0.0)
    {
        mode = m; thr = dbToLin (thresholdDb); preRoll = (size_t) std::llround (std::max (0.0, preRollMs) * 0.001 * sr); rec = Audio(); rec.sr = sr; recording = false;
        if (m == Immediate) begin();
    }
    bool isRecording() const { return recording; }
    bool isArmed() const { return mode != Off && !recording; }
    size_t recordedFrames() const { return rec.frames(); }
    void process (const float* l, const float* r, size_t n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            const float x = l[i], y = r ? r[i] : l[i];
            if (mode == Threshold && !recording && std::max (std::fabs (x), std::fabs (y)) >= thr) begin();
            if (recording && rec.frames() < maxFrames) { rec.ch[0].push_back (x); rec.ch[1].push_back (y); }
            ring[0][w] = x; ring[1][w] = y; if (++w >= cap) w = 0; if (filled < cap) ++filled;
        }
    }
    Audio stop() { recording = false; mode = Off; Audio o = std::move (rec); rec = Audio(); rec.sr = sr; return o; }
    // The last `seconds` of sound, even if nothing was recording.
    Audio captureLast (double seconds) const
    {
        Audio o; o.sr = sr; size_t want = std::min (filled, (size_t) std::llround (seconds * sr)); o.resize (want);
        for (size_t i = 0; i < want; ++i) { const size_t k = (w + cap - want + i) % cap; o.ch[0][i] = ring[0][k]; o.ch[1][i] = ring[1][k]; }
        return o;
    }
    size_t maxFrames = (size_t) 48000 * 60 * 20; // 20 minutes at 48 kHz, then it stops adding
private:
    double sr = 48000; size_t cap = 1, w = 0, filled = 0, preRoll = 0; std::vector<float> ring[2]; Audio rec; bool recording = false; Mode mode = Off; float thr = 0.01f;
    void begin() { recording = true; const Audio pre = captureLast ((double) std::min (preRoll, filled) / sr); rec.ch[0] = pre.ch[0]; rec.ch[1] = pre.ch[1]; }
};
} // namespace mc
