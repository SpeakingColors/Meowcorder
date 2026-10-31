// Spectral editing, resampling, time stretch, pitch shift, warp, pitch and tempo detection, channel and format tools, pencils.
// A C++ port of the v4 engine of the mockup (docs/mockups/engine.js). Included by MeowCore.h inside namespace mc, after Audio and fft.
// No locks, no JUCE. These run on the message thread, never in the audio callback.

constexpr double kPiD = 3.14159265358979323846;
constexpr int kSN = 2048, kSH = 512;   // spectral window and hop (75 percent overlap)

inline const std::vector<double>& swin()
{
    static const std::vector<double> w = [] { std::vector<double> v ((size_t) kSN); for (int i = 0; i < kSN; ++i) v[(size_t) i] = 0.5 - 0.5 * std::cos (2 * kPiD * i / kSN); return v; }();
    return w;
}
inline long long floorDiv (long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline long long rnd (double x) { return (long long) std::floor (x + 0.5); }

// In place FFT on separate real and imaginary arrays (power of two length).
inline void fftRI (double* re, double* im, int n)
{
    for (int i = 1, j = 0; i < n; ++i) { int bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) { std::swap (re[i], re[j]); std::swap (im[i], im[j]); } }
    for (int len = 2; len <= n; len <<= 1)
    {
        const double ang = -2 * kPiD / len, wr = std::cos (ang), wi = std::sin (ang);
        for (int i = 0; i < n; i += len)
        {
            double cr = 1, ci = 0;
            for (int k = 0; k < len / 2; ++k)
            {
                const int a = i + k, b = i + k + len / 2; const double tr = re[b] * cr - im[b] * ci, ti = re[b] * ci + im[b] * cr;
                re[b] = re[a] - tr; im[b] = im[a] - ti; re[a] += tr; im[a] += ti; const double nr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = nr;
            }
        }
    }
}
inline void ifftRI (double* re, double* im, int n) { for (int i = 0; i < n; ++i) im[i] = -im[i]; fftRI (re, im, n); for (int i = 0; i < n; ++i) { re[i] /= n; im[i] = -im[i] / n; } }

// ---------- spectral editing: a box in time (frames) and frequency (Hz) ----------
struct SpecRegion { double t0 = 0, t1 = 0, f0 = 0, f1 = 0; };

// The edge is soft: half a window in time, two bins in frequency. Returns true when any bin is touched.
inline bool maskBins (const std::vector<SpecRegion>& regions, double c, double sr, std::vector<double>& m)
{
    std::fill (m.begin(), m.end(), 0.0); bool any = false; const double fe = kSN / 2;
    auto cl = [] (double v) { return v < 0 ? 0.0 : (v > 1 ? 1.0 : v); };
    for (const auto& r : regions)
    {
        const double tw = cl ((c - r.t0) / fe + 0.5) * cl ((r.t1 - c) / fe + 0.5); if (tw <= 0) continue;
        const double kLo = r.f0 * kSN / sr, kHi = r.f1 * kSN / sr; const int a = std::max (0, (int) std::floor (kLo) - 2), b = std::min (kSN / 2, (int) std::ceil (kHi) + 2);
        for (int k = a; k <= b; ++k) { const double fw = cl ((k - kLo) / 2 + 0.5) * cl ((kHi - k) / 2 + 0.5), v = tw * fw; if (v > m[(size_t) k]) { m[(size_t) k] = v; any = true; } }
    }
    return any;
}

// Runs edit(re, im, centreFrame, channel, frameIndex) on every STFT frame touching [s, e) and writes the result back.
inline void stft (Audio& a, long long s, long long e, const std::function<void (double*, double*, double, int, long long)>& edit)
{
    const long long L = e - s, n = (long long) a.frames(); std::vector<double> outs[2] = { std::vector<double> ((size_t) L, 0.0), std::vector<double> ((size_t) L, 0.0) };
    std::vector<double> re ((size_t) kSN), im ((size_t) kSN); const auto& W = swin();
    const long long j0 = floorDiv (s - kSN, kSH) + 1, j1 = floorDiv (e - 1, kSH);
    for (long long j = j0; j <= j1; ++j)
    {
        const long long st = j * kSH; const double c = (double) (st + kSN / 2);
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto& x = a.ch[ch]; for (int i = 0; i < kSN; ++i) { const long long p = st + i; re[(size_t) i] = (p >= 0 && p < n ? (double) x[(size_t) p] : 0.0) * W[(size_t) i]; im[(size_t) i] = 0; }
            fftRI (re.data(), im.data(), kSN); edit (re.data(), im.data(), c, ch, j); ifftRI (re.data(), im.data(), kSN);
            for (int i = 0; i < kSN; ++i) { const long long p = st + i - s; if (p >= 0 && p < L) outs[ch][(size_t) p] += re[(size_t) i] * W[(size_t) i]; }
        }
    }
    for (long long i = 0; i < L; ++i) { a.ch[0][(size_t) (s + i)] = (float) (outs[0][(size_t) i] / 1.5); a.ch[1][(size_t) (s + i)] = (float) (outs[1][(size_t) i] / 1.5); }
}

// cut: remove what is inside the boxes. Otherwise change its level by gainDb.
inline void spectralEdit (Audio& a, const std::vector<SpecRegion>& regions, bool cut, float gainDb)
{
    if (regions.empty() || a.frames() == 0) return;
    double t0 = 1e18, t1 = -1e18; for (auto& r : regions) { t0 = std::min (t0, r.t0); t1 = std::max (t1, r.t1); }
    const long long s = std::max<long long> (0, (long long) std::floor (t0) - 2 * kSN), e = std::min<long long> ((long long) a.frames(), (long long) std::ceil (t1) + 2 * kSN); if (e <= s) return;
    const double g = std::pow (10.0, gainDb / 20.0); std::vector<double> m ((size_t) kSN / 2 + 1); bool any = false;
    stft (a, s, e, [&] (double* re, double* im, double c, int ch, long long)
    {
        if (ch == 0) any = maskBins (regions, c, a.sr, m); if (!any) return;
        for (int k = 0; k <= kSN / 2; ++k)
        {
            const double v = m[(size_t) k]; if (v <= 0) continue; const double gk = cut ? 1 - v : 1 + v * (g - 1); re[k] *= gk; im[k] *= gk;
            if (k > 0 && k < kSN / 2) { re[kSN - k] *= gk; im[kSN - k] *= gk; }
        }
    });
}

struct SpecClip
{
    int nf = 0, nb = 0, kLo = 0; double sr = 48000, fc = 1000; std::vector<float> re[2], im[2];
    bool valid() const { return nf > 0 && nb > 0; }
};
inline SpecClip spectralCopy (const Audio& a, const SpecRegion& r)
{
    SpecClip clip; if (a.frames() == 0) return clip; const double sr = a.sr; const long long n = (long long) a.frames();
    const int kLo = std::max (0, (int) std::floor (r.f0 * kSN / sr) - 2), kHi = std::min (kSN / 2, (int) std::ceil (r.f1 * kSN / sr) + 2), nb = kHi - kLo + 1; if (nb < 1) return clip;
    const long long j0 = (long long) std::ceil ((r.t0 - kSN / 4.0 - kSN / 2.0) / kSH), j1 = (long long) std::floor ((r.t1 + kSN / 4.0 - kSN / 2.0) / kSH); const int nf = (int) std::max<long long> (1, j1 - j0 + 1);
    clip.nf = nf; clip.nb = nb; clip.kLo = kLo; clip.sr = sr; clip.fc = std::sqrt (std::max (20.0, r.f0) * r.f1);
    for (int c = 0; c < 2; ++c) { clip.re[c].assign ((size_t) nf * (size_t) nb, 0.f); clip.im[c].assign ((size_t) nf * (size_t) nb, 0.f); }
    std::vector<double> m ((size_t) kSN / 2 + 1), re ((size_t) kSN), im ((size_t) kSN); const auto& W = swin(); const std::vector<SpecRegion> one { r };
    for (int f = 0; f < nf; ++f)
    {
        const long long st = (j0 + f) * kSH; const double c = (double) (st + kSN / 2); maskBins (one, c, sr, m);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < kSN; ++i) { const long long p = st + i; re[(size_t) i] = (p >= 0 && p < n ? (double) a.ch[ch][(size_t) p] : 0.0) * W[(size_t) i]; im[(size_t) i] = 0; }
            fftRI (re.data(), im.data(), kSN);
            for (int b = 0; b < nb; ++b) { const int k = kLo + b; clip.re[ch][(size_t) f * (size_t) nb + (size_t) b] = (float) (re[(size_t) k] * m[(size_t) k]); clip.im[ch][(size_t) f * (size_t) nb + (size_t) b] = (float) (im[(size_t) k] * m[(size_t) k]); }
        }
    }
    return clip;
}
// Adds the copied piece back, its first frame at atFrame, its middle frequency at destHz.
inline bool spectralPaste (Audio& a, const SpecClip& clip, double atFrame, double destHz)
{
    if (!clip.valid() || a.frames() == 0) return false; const double sr = a.sr; const long long jt = rnd ((atFrame - kSN / 2.0) / kSH), dk = rnd ((destHz - clip.fc) * kSN / sr);
    const long long s = std::max<long long> (0, jt * kSH - 2 * kSN), e = std::min<long long> ((long long) a.frames(), (jt + clip.nf) * kSH + 3 * kSN); if (e <= s) return false;
    stft (a, s, e, [&] (double* re, double* im, double, int ch, long long j)
    {
        const long long f = j - jt; if (f < 0 || f >= clip.nf) return; const double th = 2 * kPiD * (double) dk * kSH * (double) f / kSN, cs = std::cos (th), sn = std::sin (th);
        for (int b = 0; b < clip.nb; ++b)
        {
            const long long kk = clip.kLo + b + dk; if (kk < 1 || kk >= kSN / 2) continue;
            const double ur = clip.re[ch][(size_t) f * (size_t) clip.nb + (size_t) b], ui = clip.im[ch][(size_t) f * (size_t) clip.nb + (size_t) b], vr = ur * cs - ui * sn, vi = ur * sn + ui * cs;
            re[kk] += vr; im[kk] += vi; re[kSN - kk] += vr; im[kSN - kk] -= vi;
        }
    });
    return true;
}

// ---------- resampling ----------
inline double sincw (double x) { return x == 0 ? 1.0 : std::sin (kPiD * x) / (kPiD * x); }
// out[i] reads the input at i / ratio. ratio = outRate / inRate. Windowed sinc, 10 zero crossings each side (more when filtering down).
inline std::vector<float> resampleCh (const std::vector<float>& x, size_t outLen, double ratio)
{
    const long long n = (long long) x.size(); std::vector<float> o (outLen, 0.f); if (n == 0 || outLen == 0) return o;
    if (std::fabs (ratio - 1) < 1e-9 && outLen == (size_t) n) return x;
    const double fc = std::min (1.0, ratio); const long long kw = (long long) std::ceil (10 / fc);
    for (size_t i = 0; i < outLen; ++i)
    {
        const double p = (double) i / ratio; const long long k0 = (long long) std::floor (p); double s = 0;
        for (long long k = k0 - kw + 1; k <= k0 + kw; ++k) { if (k < 0 || k >= n) continue; const double d = p - (double) k; s += (double) x[(size_t) k] * sincw (d * fc) * (0.5 + 0.5 * std::cos (kPiD * d / (double) kw)); }
        o[i] = (float) (s * fc);
    }
    return o;
}
inline bool isMonoAudio (const Audio& a) { for (size_t i = 0; i < a.frames(); ++i) if (a.ch[0][i] != a.ch[1][i]) return false; return true; }
inline Audio perChannel (const Audio& a, const std::function<std::vector<float> (const std::vector<float>&, int)>& fn)
{
    Audio o; o.sr = a.sr; o.ch[0] = fn (a.ch[0], 0); o.ch[1] = isMonoAudio (a) ? o.ch[0] : fn (a.ch[1], 1); return o;
}
inline Audio resample (const Audio& a, double newSr)
{
    const double ratio = newSr / a.sr; const size_t n = (size_t) rnd ((double) a.frames() * ratio); Audio o; o.sr = newSr;
    o.ch[0] = resampleCh (a.ch[0], n, ratio); o.ch[1] = isMonoAudio (a) ? o.ch[0] : resampleCh (a.ch[1], n, ratio); return o;
}

// ---------- time stretch and pitch shift ----------
// Phase vocoder with peak phase locking. factor = output length / input length. Pitch stays.
inline std::vector<float> pvStretch (const std::vector<float>& x, double factor)
{
    const long long n = (long long) x.size(), N = kSN, Hs = 512; const double Ha = (double) Hs / factor; const long long outLen = std::max<long long> (1, rnd ((double) n * factor)); const int K = (int) N / 2 + 1;
    if (n < 64) return resampleCh (x, (size_t) outLen, (double) outLen / (double) std::max<long long> (1, n));
    const long long base = N; std::vector<double> out ((size_t) (outLen + 3 * N), 0.0), nrm ((size_t) (outLen + 3 * N), 0.0), re ((size_t) N), im ((size_t) N);
    std::vector<double> mag ((size_t) K), ph ((size_t) K), pph ((size_t) K, 0.0), sph ((size_t) K, 0.0), nph ((size_t) K, 0.0); std::vector<int> pk; const auto& W = swin();
    long long prevSt = 0; const long long frames = (outLen + Hs - 1) / Hs + 3; const double TP = 2 * kPiD;
    for (long long j = 0; j < frames; ++j)
    {
        const long long st = rnd ((double) j * Ha) - N / 2, os = j * Hs - N / 2;
        for (long long i = 0; i < N; ++i) { const long long p = st + i; re[(size_t) i] = (p >= 0 && p < n ? (double) x[(size_t) p] : 0.0) * W[(size_t) i]; im[(size_t) i] = 0; }
        fftRI (re.data(), im.data(), (int) N);
        for (int k = 0; k < K; ++k) { mag[(size_t) k] = std::hypot (re[(size_t) k], im[(size_t) k]); ph[(size_t) k] = std::atan2 (im[(size_t) k], re[(size_t) k]); }
        if (j == 0) sph = ph;
        else
        {
            const double ha = (double) (st - prevSt); pk.clear();
            for (int k = 0; k < K; ++k) { const double m = mag[(size_t) k]; if (m > 1e-9 && m >= (k > 0 ? mag[(size_t) k - 1] : 0) && m >= (k > 1 ? mag[(size_t) k - 2] : 0) && m >= (k + 1 < K ? mag[(size_t) k + 1] : 0) && m >= (k + 2 < K ? mag[(size_t) k + 2] : 0)) pk.push_back (k); }
            if (pk.empty()) sph = ph;
            else
            {
                for (int k : pk) { const double ex = TP * k * ha / (double) N; double d = ph[(size_t) k] - pph[(size_t) k] - ex; d -= TP * std::floor (d / TP + 0.5); nph[(size_t) k] = sph[(size_t) k] + (ex + d) / ha * (double) Hs; }
                size_t pi = 0; for (int k = 0; k < K; ++k) { while (pi + 1 < pk.size() && std::abs (pk[pi + 1] - k) < std::abs (pk[pi] - k)) ++pi; const int p = pk[pi]; sph[(size_t) k] = nph[(size_t) p] + (ph[(size_t) k] - ph[(size_t) p]); }
            }
        }
        pph = ph; prevSt = st;
        for (int k = 0; k < K; ++k) { re[(size_t) k] = mag[(size_t) k] * std::cos (sph[(size_t) k]); im[(size_t) k] = mag[(size_t) k] * std::sin (sph[(size_t) k]); if (k > 0 && k < N / 2) { re[(size_t) (N - k)] = re[(size_t) k]; im[(size_t) (N - k)] = -im[(size_t) k]; } }
        im[0] = 0; im[(size_t) N / 2] = 0; ifftRI (re.data(), im.data(), (int) N);
        for (long long i = 0; i < N; ++i) { const long long q = base + os + i; if (q >= 0 && q < (long long) out.size()) { out[(size_t) q] += re[(size_t) i] * W[(size_t) i]; nrm[(size_t) q] += W[(size_t) i] * W[(size_t) i]; } }
    }
    std::vector<float> o ((size_t) outLen); for (long long i = 0; i < outLen; ++i) o[(size_t) i] = (float) (out[(size_t) (base + i)] / std::max (0.1, nrm[(size_t) (base + i)])); return o;
}
// Gives y the spectral envelope (the vowel shape, the formants) of x, moved by fs semitones. x and y have the same length.
inline std::vector<float> formantCorrect (const std::vector<float>& x, const std::vector<float>& y, double fs)
{
    const long long n = (long long) y.size(), N = kSN; const int K = (int) N / 2 + 1, Wd = 14; const double fr = std::pow (2.0, fs / 12); const auto& W = swin();
    std::vector<double> o ((size_t) (n + 3 * N), 0.0), nr ((size_t) (n + 3 * N), 0.0), r1 ((size_t) N), i1 ((size_t) N), r2 ((size_t) N), i2 ((size_t) N), m1 ((size_t) K), m2 ((size_t) K), c1 ((size_t) K + 1), c2 ((size_t) K + 1), e1 ((size_t) K), e2 ((size_t) K);
    auto env = [&] (const std::vector<double>& m, std::vector<double>& c, std::vector<double>& e)
    {
        c[0] = 0; for (int k = 0; k < K; ++k) c[(size_t) k + 1] = c[(size_t) k] + m[(size_t) k] * m[(size_t) k];
        for (int k = 0; k < K; ++k) { const int a = std::max (0, k - Wd), b = std::min (K - 1, k + Wd); e[(size_t) k] = std::sqrt ((c[(size_t) b + 1] - c[(size_t) a]) / (b - a + 1)); }
    };
    for (long long st = -N; st < n; st += kSH)
    {
        bool any = false;
        for (long long i = 0; i < N; ++i) { const long long p = st + i; const double w = W[(size_t) i], a = p >= 0 && p < n ? (double) x[(size_t) p] : 0.0, b = p >= 0 && p < n ? (double) y[(size_t) p] : 0.0; r1[(size_t) i] = a * w; r2[(size_t) i] = b * w; i1[(size_t) i] = i2[(size_t) i] = 0; if (b != 0) any = true; }
        if (!any) continue; fftRI (r1.data(), i1.data(), (int) N); fftRI (r2.data(), i2.data(), (int) N);
        for (int k = 0; k < K; ++k) { m1[(size_t) k] = std::hypot (r1[(size_t) k], i1[(size_t) k]); m2[(size_t) k] = std::hypot (r2[(size_t) k], i2[(size_t) k]); } env (m1, c1, e1); env (m2, c2, e2);
        for (int k = 0; k < K; ++k)
        {
            const double pos = k / fr; const int k0 = (int) std::floor (pos); const double f = pos - k0; const double tgt = k0 + 1 < K ? e1[(size_t) k0] * (1 - f) + e1[(size_t) k0 + 1] * f : (k0 < K ? e1[(size_t) k0] : 0.0);
            const double g = std::min (8.0, std::max (0.05, tgt / std::max (e2[(size_t) k], 1e-7))); r2[(size_t) k] *= g; i2[(size_t) k] *= g; if (k > 0 && k < N / 2) { r2[(size_t) (N - k)] *= g; i2[(size_t) (N - k)] *= g; }
        }
        ifftRI (r2.data(), i2.data(), (int) N);
        for (long long i = 0; i < N; ++i) { const long long q = st + i + N; if (q >= 0 && q < (long long) o.size()) { o[(size_t) q] += r2[(size_t) i] * W[(size_t) i]; nr[(size_t) q] += W[(size_t) i] * W[(size_t) i]; } }
    }
    std::vector<float> out ((size_t) n); for (long long i = 0; i < n; ++i) out[(size_t) i] = nr[(size_t) (i + N)] > 0.1 ? (float) (o[(size_t) (i + N)] / nr[(size_t) (i + N)]) : y[(size_t) i]; return out;
}
// formant false: the formants follow the pitch (the voice gets smaller or bigger). true: they stay, moved by fshift semitones.
inline std::vector<float> pitchShiftCh (const std::vector<float>& x, double semis, bool formant, double fshift)
{
    const size_t n = x.size(); if (n == 0) return x; const double r = std::pow (2.0, semis / 12); std::vector<float> y = x;
    if (std::fabs (semis) > 1e-4) y = resampleCh (pvStretch (x, r), n, 1 / r);
    if (formant && (std::fabs (semis) > 1e-4 || std::fabs (fshift) > 1e-4)) y = formantCorrect (x, y, fshift);
    return y;
}
inline Audio pitchShift (const Audio& a, double semis, bool formant, double fshift)
{
    return perChannel (a, [&] (const std::vector<float>& x, int) { return pitchShiftCh (x, semis, formant, fshift); });
}
// Pitch shifts frames [s, e) only; 15 ms cross fades hide the joins.
inline void pitchShiftRange (Audio& a, size_t s, size_t e, double semis, bool formant, double fshift)
{
    e = std::min (e, a.frames()); if (e <= s || e - s < 256) return; const size_t pad = std::min<size_t> ({ 4096, s, a.frames() - e }), s0 = s - pad, e0 = e + pad;
    const Audio seg = pitchShift (a.slice (s0, e0), semis, formant, fshift); const double f = (double) std::min<size_t> ((size_t) rnd (0.015 * a.sr), (e - s) >> 1);
    for (size_t i = s; i < e; ++i)
    {
        const double k = (double) (i - s); const double t = f <= 0 ? 1.0 : std::min ({ 1.0, k / f, (double) (e - 1 - i) / f }), w = t * t * (3 - 2 * t);
        for (int c = 0; c < 2; ++c) a.ch[c][i] = (float) ((double) a.ch[c][i] * (1 - w) + (double) seg.ch[c][i - s0] * w);
    }
}

// Tempo change: ratio = speed multiplier (more is faster and shorter). mode: 0 stretch (pitch kept), 1 repitch, 2 beats (hits stay sharp), 3 texture (grains).
inline std::vector<float> warpCh (const std::vector<float>& x, double ratio, int mode, double sr, const std::vector<size_t>& hits)
{
    const long long n = (long long) x.size(); const long long outLen = std::max<long long> (1, rnd ((double) n / ratio)); if (std::fabs (ratio - 1) < 1e-6) return x;
    if (mode == 1) return resampleCh (x, (size_t) outLen, 1 / ratio);
    if (mode == 3)
    {
        const long long G = rnd (0.09 * sr), hop = G >> 2; std::vector<float> o ((size_t) (outLen + G), 0.f); Rng r;
        for (long long g = 0; g * hop < outLen; ++g)
        {
            const long long op = g * hop, sp = rnd ((double) op * ratio + (r.next01() - 0.5) * (double) G * 0.35);
            for (long long i = 0; i < G; ++i) { const long long q = sp + i; if (q < 0 || q >= n) continue; o[(size_t) (op + i)] += (float) ((double) x[(size_t) q] * (0.5 - 0.5 * std::cos (2 * kPiD * (double) i / (double) G)) * 0.5); }
        }
        o.resize ((size_t) outLen); return o;
    }
    if (mode == 2 && !hits.empty())
    {
        std::vector<float> o ((size_t) outLen, 0.f); std::vector<long long> cuts { 0 }; for (size_t h : hits) if ((long long) h > 0 && (long long) h < n) cuts.push_back ((long long) h); cuts.push_back (n);
        const long long hd = rnd (0.03 * sr), xf = 256;
        for (size_t c = 0; c + 1 < cuts.size(); ++c)
        {
            const long long a = cuts[c], b = cuts[c + 1], L = b - a, O = rnd ((double) a / ratio), Lt = std::min (rnd ((double) b / ratio) - O, outLen - O); if (Lt <= 0 || L <= 0) continue;
            const std::vector<float> seg (x.begin() + a, x.begin() + b); const std::vector<float> S = (L < 3000 || Lt < 3000) ? resampleCh (seg, (size_t) Lt, (double) Lt / (double) L) : pvStretch (seg, (double) Lt / (double) L);
            for (long long i = 0; i < Lt && i < (long long) S.size(); ++i) o[(size_t) (O + i)] = S[(size_t) i];
            if (L >= 3000 && Lt >= 3000) { const long long hdn = std::min ({ hd, L, Lt }); for (long long i = 0; i < hdn; ++i) { const double t = i < hdn - xf ? 0.0 : (double) (i - (hdn - xf)) / (double) xf; o[(size_t) (O + i)] = (float) ((double) seg[(size_t) i] * (1 - t) + (double) S[(size_t) i] * t); } }
            const long long fo = std::min<long long> (48, Lt); for (long long i = 0; i < fo; ++i) o[(size_t) (O + Lt - 1 - i)] *= (float) ((double) i / (double) fo);
        }
        return o;
    }
    return pvStretch (x, 1 / ratio);
}
struct WarpOpts { double ratio = 1; int mode = 0; double semis = 0, cents = 0; bool formant = false; double fshift = 0; std::vector<size_t> hits; };
inline Audio warpAudio (const Audio& a, const WarpOpts& o)
{
    Audio r = (o.ratio != 0 && std::fabs (o.ratio - 1) > 1e-6) ? perChannel (a, [&] (const std::vector<float>& x, int) { return warpCh (x, o.ratio, o.mode, a.sr, o.hits); }) : a;
    const double st = o.semis + o.cents / 100;
    if (std::fabs (st) > 1e-4 || (o.fshift != 0 && o.formant)) r = pitchShift (r, st, o.formant, o.fshift);
    return r;
}

// ---------- pitch, notes and tempo ----------
struct PitchTrack { std::vector<float> t, midi; int hop = 128; };   // midi is NaN where there is no clear pitch
inline PitchTrack detectPitch (const Audio& a)
{
    PitchTrack out; const int D = std::max (1, (int) rnd (a.sr / 12000)); const double sr2 = a.sr / D; const long long m = (long long) a.frames() / D; std::vector<float> x ((size_t) std::max<long long> (m, 0));
    for (long long i = 0; i < m; ++i) { double s = 0; for (int d = 0; d < D; ++d) s += 0.5 * ((double) a.ch[0][(size_t) (i * D + d)] + (double) a.ch[1][(size_t) (i * D + d)]); x[(size_t) i] = (float) (s / D); }
    const int W = 512, hop = 128; const int tmin = std::max (2, (int) std::floor (sr2 / 1500)), tmax = (int) std::ceil (sr2 / 60); const long long F = std::max<long long> (0, (m - W - tmax) / hop + 1 - ((m - W - tmax) < 0 ? 1 : 0));
    out.hop = hop * D; out.midi.assign ((size_t) F, std::nanf ("")); out.t.assign ((size_t) F, 0.f); std::vector<double> d ((size_t) tmax + 2), cm ((size_t) tmax + 2);
    for (long long f = 0; f < F; ++f)
    {
        const long long t0 = f * hop; double e = 0; for (int j = 0; j < W; ++j) e += (double) x[(size_t) (t0 + j)] * (double) x[(size_t) (t0 + j)];
        out.t[(size_t) f] = (float) ((t0 + W / 2 + 100) * D); if (std::sqrt (e / W) < 0.004) continue;
        for (int tau = 1; tau <= tmax; ++tau) { double s = 0; for (int j = 0; j < W; ++j) { const double v = (double) x[(size_t) (t0 + j)] - (double) x[(size_t) (t0 + j + tau)]; s += v * v; } d[(size_t) tau] = s; }
        double run = 0; cm[0] = 1; for (int tau = 1; tau <= tmax; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? d[(size_t) tau] * tau / run : 1; }
        int best = -1; for (int tau = tmin; tau < tmax; ++tau) { if (cm[(size_t) tau] < 0.15) { while (tau + 1 < tmax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; } }
        if (best < 0) { double mn = 1e9; int mt = -1; for (int tau = tmin; tau < tmax; ++tau) if (cm[(size_t) tau] < mn) { mn = cm[(size_t) tau]; mt = tau; } if (mn < 0.25) best = mt; }
        if (best < 0) continue; double tau = best;
        if (best > 1 && best < tmax) { const double a0 = cm[(size_t) best - 1], b0 = cm[(size_t) best], c0 = cm[(size_t) best + 1], den = a0 - 2 * b0 + c0; if (std::fabs (den) > 1e-12) tau = best + 0.5 * (a0 - c0) / den; }
        out.midi[(size_t) f] = (float) (69 + 12 * std::log2 (sr2 / tau / 440)); out.t[(size_t) f] = (float) ((t0 + W / 2 + tau / 2) * D);
    }
    return out;
}
struct Note { long long t0 = 0, t1 = 0; float m = 0; };   // frames, and the middle pitch in (fractional) MIDI notes
struct NoteResult { std::vector<Note> notes; PitchTrack track; };
inline NoteResult detectNotes (const Audio& a)
{
    NoteResult res; res.track = detectPitch (a); const auto& p = res.track; const long long F = (long long) p.midi.size(); std::vector<float> sm ((size_t) F, std::nanf (""));
    for (long long f = 0; f < F; ++f)
    {
        std::vector<float> v; for (int k = -2; k <= 2; ++k) { const long long q = f + k; if (q >= 0 && q < F && !std::isnan (p.midi[(size_t) q])) v.push_back (p.midi[(size_t) q]); }
        if (!std::isnan (p.midi[(size_t) f]) && v.size() >= 3) { std::sort (v.begin(), v.end()); sm[(size_t) f] = v[v.size() >> 1]; }
    }
    struct Cur { double t0, t1; std::vector<float> v; }; bool have = false; Cur cur { 0, 0, {} }; int gap = 0;
    auto med = [] (std::vector<float> v) { std::sort (v.begin(), v.end()); return v[v.size() >> 1]; };
    auto close = [&] { if (have && cur.v.size() >= 4) { Note n; n.t0 = std::max<long long> (0, rnd (cur.t0 - p.hop / 2.0)); n.t1 = std::min<long long> ((long long) a.frames(), rnd (cur.t1 + p.hop / 2.0)); n.m = med (cur.v); res.notes.push_back (n); } have = false; };
    for (long long f = 0; f < F; ++f)
    {
        const float v = sm[(size_t) f]; if (std::isnan (v)) { if (have && ++gap > 2) close(); continue; } gap = 0;
        if (have) { std::vector<float> tail (cur.v.end() - (std::ptrdiff_t) std::min<size_t> (8, cur.v.size()), cur.v.end()); if (std::fabs (v - med (tail)) > 0.75) close(); }
        if (!have) { cur = { (double) p.t[(size_t) f], (double) p.t[(size_t) f], { v } }; have = true; } else { cur.t1 = (double) p.t[(size_t) f]; cur.v.push_back (v); }
    }
    close(); return res;
}
// Hits: where the energy jumps. sens 0..1 (more sensitive finds more). Positions in frames.
inline std::vector<size_t> detectTransients (const Audio& a, double sens)
{
    std::vector<size_t> out; const int hop = 256; const long long F = (long long) a.frames() / hop; if (F < 4) return out; std::vector<double> e ((size_t) F);
    for (long long f = 0; f < F; ++f) { double s = 0; for (int i = 0; i < hop; ++i) { const double x = 0.5 * ((double) a.ch[0][(size_t) (f * hop + i)] + (double) a.ch[1][(size_t) (f * hop + i)]); s += x * x; } e[(size_t) f] = std::sqrt (s / hop); }
    const double ratio = 1.4 + 2.2 * (1 - std::max (0.0, std::min (1.0, sens))); const long long minGap = (long long) std::floor (0.08 * a.sr / hop) + 1; long long last = 0; bool any = false;
    for (long long f = 2; f + 1 < F; ++f)
    {
        double avg = 0; const long long a0 = f > 24 ? f - 24 : 0; for (long long k = a0; k < f; ++k) avg += e[(size_t) k]; avg /= (double) (f - a0);
        const bool onset = e[(size_t) f] > ratio * avg + 0.01 && e[(size_t) f] > e[(size_t) f - 1] && e[(size_t) f - 1] <= e[(size_t) f - 2] * 1.5 + 1e-4;
        if (onset && (!any || f - last >= minGap)) { out.push_back ((size_t) (f * hop)); last = f; any = true; }
    }
    return out;
}

// Tempo from the hits: autocorrelation of the onset curve, 60 to 200 bpm, leaning towards the usual 90 to 160. Returns 0 when the sound is too short.
inline double detectBpm (const Audio& a)
{
    const int hop = 256; const long long F = (long long) a.frames() / hop; if (F < 90) return 0; std::vector<double> e ((size_t) F), fl ((size_t) F, 0.0);
    for (long long f = 0; f < F; ++f) { double s = 0; for (int i = 0; i < hop; ++i) { const double v = 0.5 * ((double) a.ch[0][(size_t) (f * hop + i)] + (double) a.ch[1][(size_t) (f * hop + i)]); s += v * v; } e[(size_t) f] = std::sqrt (s / hop); }
    for (long long f = 1; f < F; ++f) fl[(size_t) f] = std::max (0.0, e[(size_t) f] - e[(size_t) f - 1]);
    const double fps = a.sr / hop; const long long lo = (long long) std::floor (fps * 60 / 200), hi = std::min<long long> ((long long) std::ceil (fps * 60 / 60), F >> 1); if (hi <= lo + 2) return 0;
    std::vector<double> ac ((size_t) hi + 2, 0.0);
    for (long long lag = lo - 1; lag <= hi + 1; ++lag) { double s = 0; for (long long i = 0; i + lag < F; ++i) s += fl[(size_t) i] * fl[(size_t) (i + lag)]; ac[(size_t) lag] = s / (double) (F - lag); }
    long long best = -1; double bv = 0;
    for (long long lag = lo; lag <= hi; ++lag) { const double bpm = 60 * fps / (double) lag, w = std::exp (-0.5 * std::pow (std::log2 (bpm / 120) / 0.9, 2)), v = ac[(size_t) lag] * w; if (v > bv && ac[(size_t) lag] >= ac[(size_t) lag - 1] && ac[(size_t) lag] >= ac[(size_t) lag + 1]) { bv = v; best = lag; } }
    if (best < 0 || bv <= 0) return 0; double lag = (double) best; const double a0 = ac[(size_t) best - 1], b0 = ac[(size_t) best], c0 = ac[(size_t) best + 1], den = a0 - 2 * b0 + c0; if (std::fabs (den) > 1e-20) lag += 0.5 * (a0 - c0) / den;
    return std::floor (600 * fps / lag + 0.5) / 10;
}

// ---------- bit depth, channels, pencils ----------
// Rounds to the steps of a smaller word, with a little triangular noise first when dither is on.
inline void quantize (Audio& a, int bits, bool dither)
{
    const double q = std::pow (2.0, bits - 1); Rng r;
    for (auto& x : a.ch) for (auto& s : x) { double v = (double) s * q; if (dither) v += (double) r.next01() - (double) r.next01(); s = (float) std::max (-1.0, std::min ((q - 1) / q, std::floor (v + 0.5) / q)); }
}
enum class ChannelKind { Mix, Left, Right, Swap, MidSideEnc, MidSideDec };
inline void channelOp (Audio& a, ChannelKind kind)
{
    for (size_t i = 0; i < a.frames(); ++i)
    {
        const float l = a.ch[0][i], r = a.ch[1][i];
        switch (kind)
        {
            case ChannelKind::Mix: a.ch[0][i] = a.ch[1][i] = 0.5f * (l + r); break;
            case ChannelKind::Left: a.ch[1][i] = l; break;
            case ChannelKind::Right: a.ch[0][i] = r; break;
            case ChannelKind::Swap: a.ch[0][i] = r; a.ch[1][i] = l; break;
            case ChannelKind::MidSideEnc: a.ch[0][i] = 0.5f * (l + r); a.ch[1][i] = 0.5f * (l - r); break;
            case ChannelKind::MidSideDec: a.ch[0][i] = l + r; a.ch[1][i] = l - r; break;
        }
    }
}
// Draws a sine into the sound (the spectral pencil), glide from hz0 to hz1, fade in and out in frames. Returns the phase to continue with.
inline double pencilTone (Audio& a, double f0, double f1, double hz0, double hz1, double amp, double ph, double fi, double fo)
{
    const long long s = std::max<long long> (0, (long long) std::floor (f0)), e = std::min<long long> ((long long) a.frames(), (long long) std::ceil (f1)); const long long n = e - s; if (n <= 0) return ph;
    const double w = 2 * kPiD / a.sr;
    for (long long i = 0; i < n; ++i)
    {
        const double hz = hz0 + (hz1 - hz0) * ((double) i / (double) n); ph += w * hz; double g = amp; if (fi > 0 && (double) i < fi) g *= (double) i / fi; if (fo > 0 && (double) i >= (double) n - fo) g *= (double) (n - i) / fo;
        const float v = (float) (std::sin (ph) * g); for (int c = 0; c < 2; ++c) { float& x = a.ch[c][(size_t) (s + i)]; x = std::max (-1.f, std::min (1.f, x + v)); }
    }
    return std::fmod (ph, 2 * kPiD);
}
// Sets samples along a line (the waveform pencil). lane: -1 both sides, 0 left, 1 right.
inline void pencilWave (Audio& a, double f0, double v0, double f1, double v1, int lane)
{
    if (a.frames() == 0) return; if (f1 < f0) { std::swap (f0, f1); std::swap (v0, v1); }
    const long long i0 = (long long) std::floor (f0), i1 = (long long) std::floor (f1);
    for (long long f = std::max<long long> (0, i0); f <= std::min<long long> ((long long) a.frames() - 1, i1); ++f)
    {
        const double t = i1 == i0 ? 1.0 : (double) (f - i0) / (double) (i1 - i0); const float v = (float) std::max (-1.0, std::min (1.0, v0 + (v1 - v0) * t));
        if (lane != 1) a.ch[0][(size_t) f] = v; if (lane != 0) a.ch[1][(size_t) f] = v;
    }
}
