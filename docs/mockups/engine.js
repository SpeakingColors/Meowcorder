// Meowcorder engine, JavaScript copy of src/core/MeowCore.h (same maths, same order of operations). Used by the mockup and
// checked against the C++ core by tools/parity.js. Audio = { l, r, sr } with Float32Array channels.
(function (root) {
'use strict';
const PI = 3.14159265358979, clampf = (v, lo, hi) => v < lo ? lo : (v > hi ? hi : v), dbToLin = db => Math.pow(10, db / 20), linToDb = x => 20 * Math.log10(Math.max(1e-9, x));
const f32 = Math.fround;

class Audio {
  constructor(sr, n) { this.sr = sr || 48000; this.l = new Float32Array(n || 0); this.r = new Float32Array(n || 0); }
  get frames() { return this.l.length; }
  clone() { const a = new Audio(this.sr); a.l = this.l.slice(); a.r = this.r.slice(); return a; }
  slice(a, b) { b = Math.min(b, this.frames); a = Math.min(a, b); const o = new Audio(this.sr); o.l = this.l.slice(a, b); o.r = this.r.slice(a, b); return o; }
  splice(s, e, nl, nr) { // replace frames [s,e) with new channel data
    const cat = (c, n) => { const o = new Float32Array(c.length - (e - s) + n.length); o.set(c.subarray(0, s), 0); o.set(n, s); o.set(c.subarray(e), s + n.length); return o; };
    this.l = cat(this.l, nl); this.r = cat(this.r, nr);
  }
  erase(a, b) { b = Math.min(b, this.frames); a = Math.min(a, b); this.splice(a, b, new Float32Array(0), new Float32Array(0)); }
  insertSilence(at, n) { at = Math.min(at, this.frames); this.splice(at, at, new Float32Array(n), new Float32Array(n)); }
  insert(at, x) { at = Math.min(at, this.frames); this.splice(at, at, x.l, x.r); }
  peak(a, b) { let m = 0; b = Math.min(b, this.frames); for (let i = a; i < b; ++i) m = Math.max(m, Math.abs(this.l[i]), Math.abs(this.r[i])); return m; }
}

// ---------- DSP blocks ----------
class Biquad {
  constructor() { this.b0 = 1; this.b1 = 0; this.b2 = 0; this.a1 = 0; this.a2 = 0; this.z1 = 0; this.z2 = 0; }
  norm(B0, B1, B2, A0, A1, A2) { this.b0 = f32(B0 / A0); this.b1 = f32(B1 / A0); this.b2 = f32(B2 / A0); this.a1 = f32(A1 / A0); this.a2 = f32(A2 / A0); }
  peak(sr, f, g, q) { const A = Math.pow(10, g / 40), w = 2 * f32(PI) * f / sr, al = Math.sin(w) / (2 * q), c = Math.cos(w); this.norm(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A); }
  lowShelf(sr, f, g) { const A = Math.pow(10, g / 40), w = 2 * f32(PI) * f / sr, c = Math.cos(w), s = Math.sin(w), al = s / 2 * Math.SQRT2, t = 2 * Math.sqrt(A) * al;
    this.norm(A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t), (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t); }
  highShelf(sr, f, g) { const A = Math.pow(10, g / 40), w = 2 * f32(PI) * f / sr, c = Math.cos(w), s = Math.sin(w), al = s / 2 * Math.SQRT2, t = 2 * Math.sqrt(A) * al;
    this.norm(A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t), (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t); }
  tick(x) { const y = f32(this.b0 * x + this.z1); this.z1 = f32(this.b1 * x - this.a1 * y + this.z2); this.z2 = f32(this.b2 * x - this.a2 * y); return y; }
}
class Svf {
  constructor() { this.g = .1; this.k = 1; this.a1 = 0; this.a2 = 0; this.a3 = 0; this.ic1 = 0; this.ic2 = 0; this.lp = 0; this.bp = 0; this.hp = 0; }
  set(sr, f, q) { f = Math.min(f, sr * .45); this.g = Math.tan(f32(PI) * f / sr); this.k = 1 / q; this.a1 = 1 / (1 + this.g * (this.g + this.k)); this.a2 = this.g * this.a1; this.a3 = this.g * this.a2; }
  tick(x) { const v3 = x - this.ic2, v1 = this.a1 * this.ic1 + this.a2 * v3, v2 = this.ic2 + this.a2 * this.ic1 + this.a3 * v3; this.ic1 = 2 * v1 - this.ic1; this.ic2 = 2 * v2 - this.ic2; this.bp = v1; this.lp = v2; this.hp = x - this.k * v1 - v2; }
}
const Colour = {
  s01: t => { t = t < 0 ? 0 : (t > 1 ? 1 : t); return t * t * (3 - 2 * t); },
  asym: (u, nl) => u >= 0 ? Math.tanh(u) : nl * Math.tanh(u / nl),
  shaper(x, drive, xs, fuzz) {
    const dn = this.s01((drive - .8) * 5), xn = this.s01((xs - .8) * 5), fn = this.s01((fuzz - .8) * 5); let y = x;
    if (drive > .001) { y = Math.tanh(x * (1 + drive * 5 + dn * 30)); if (dn > 0) y += dn * (Math.tanh(y * (2 + 10 * dn)) - y); }
    if (xs > .001) { const nl = Math.max(.2, 1 - .3 * xs - .25 * xn), u = y * (1 + xs * 5 + xn * 30); y = this.asym(u, nl); y += (.3 * xs + .25 * xn) * y * y;
      if (xn > 0) { const y2 = this.asym(y * (2 + 10 * xn) + .15 * xn, nl); y += xn * (y2 - y); } }
    if (fuzz > .001) { const fz = 1 + fuzz * 14 + fn * 70, bf = .5 * fuzz; y = Math.tanh(y * fz + bf) - Math.tanh(bf);
      if (fn > 0) { const r = Math.sqrt(y * y + .02) - .1414; y += fn * .7 * (r - .3); y = Math.tanh(y * (1 + .8 * fn)); } }
    return y;
  },
  makeup(drive, xs, fuzz, ref) {
    ref = ref || .35; let a = 0, b = 0;
    for (let i = 0; i < 64; ++i) { const x = ref * Math.sin(6.2831853 * i / 64), y = this.shaper(x, drive, xs, fuzz); a += x * x; b += y * y; }
    return clampf(b > 1e-9 ? Math.sqrt(a / b) : 1, .25, 4);
  }
};

// ---------- WAV ----------
function encodeWav(a, bits) {
  const flt = bits === 32, bps = bits / 8, n = a.frames, dataBytes = n * 2 * bps, buf = new ArrayBuffer(44 + dataBytes), v = new DataView(buf);
  const tag = (p, t) => { for (let i = 0; i < 4; ++i) v.setUint8(p + i, t.charCodeAt(i)); };
  tag(0, 'RIFF'); v.setUint32(4, 36 + dataBytes, true); tag(8, 'WAVE'); tag(12, 'fmt '); v.setUint32(16, 16, true); v.setUint16(20, flt ? 3 : 1, true); v.setUint16(22, 2, true);
  v.setUint32(24, a.sr, true); v.setUint32(28, a.sr * 2 * bps, true); v.setUint16(32, 2 * bps, true); v.setUint16(34, bits, true); tag(36, 'data'); v.setUint32(40, dataBytes, true);
  let p = 44;
  for (let i = 0; i < n; ++i) for (let c = 0; c < 2; ++c) {
    const x = clampf((c ? a.r : a.l)[i], -1, 1);
    if (flt) { v.setFloat32(p, x, true); p += 4; }
    else if (bits === 16) { v.setInt16(p, Math.round(x * 32767), true); p += 2; }
    else { const q = Math.round(x * 8388607); v.setUint8(p, q & 255); v.setUint8(p + 1, (q >> 8) & 255); v.setUint8(p + 2, (q >> 16) & 255); p += 3; }
  }
  return new Uint8Array(buf);
}
function decodeWav(d) {
  const v = new DataView(d.buffer, d.byteOffset, d.byteLength), n = d.length, tg = p => String.fromCharCode(d[p], d[p + 1], d[p + 2], d[p + 3]);
  if (n < 12 || tg(0) !== 'RIFF' || tg(8) !== 'WAVE') return null;
  let fmt = 0, chans = 0, rate = 0, bits = 0, dataPos = 0, dataLen = 0, p = 12;
  while (p + 8 <= n) {
    const sz = v.getUint32(p + 4, true), body = p + 8;
    if (tg(p) === 'fmt ' && sz >= 16 && body + 16 <= n) { fmt = v.getUint16(body, true); chans = v.getUint16(body + 2, true); rate = v.getUint32(body + 4, true); bits = v.getUint16(body + 14, true); if (fmt === 0xFFFE && sz >= 26 && body + 26 <= n) fmt = v.getUint16(body + 24, true); }
    else if (tg(p) === 'data') { dataPos = body; dataLen = Math.min(sz, n - body); break; }
    p = body + sz + (sz & 1);
  }
  if (!dataPos || !chans || !rate || (fmt !== 1 && fmt !== 3)) return null;
  const bps = bits / 8; if (!bps || (fmt === 1 && ![8, 16, 24, 32].includes(bits)) || (fmt === 3 && ![32, 64].includes(bits))) return null;
  const frames = Math.floor(dataLen / (bps * chans)), a = new Audio(rate, frames);
  for (let i = 0; i < frames; ++i) for (let c = 0; c < Math.min(chans, 2); ++c) {
    const q = dataPos + (i * chans + c) * bps; let x;
    if (fmt === 3 && bits === 32) x = v.getFloat32(q, true); else if (fmt === 3) x = v.getFloat64(q, true);
    else if (bits === 8) x = (d[q] - 128) / 128; else if (bits === 16) x = v.getInt16(q, true) / 32768;
    else if (bits === 24) { const s = (d[q] | (d[q + 1] << 8) | (d[q + 2] << 16)) << 8 >> 8; x = s / 8388608; } else x = v.getInt32(q, true) / 2147483648;
    (c ? a.r : a.l)[i] = x;
  }
  if (chans === 1) a.r = a.l.slice();
  return a;
}

// ---------- analysis ----------
function fft(re, im) {
  const n = re.length;
  for (let i = 1, j = 0; i < n; ++i) { let bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) { let t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; } }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = -2 * Math.PI / len, wr = Math.cos(ang), wi = Math.sin(ang);
    for (let i = 0; i < n; i += len) { let cr = 1, ci = 0; for (let k = 0; k < len / 2; ++k) { const a = i + k, b = i + k + len / 2, tr = re[b] * cr - im[b] * ci, ti = re[b] * ci + im[b] * cr; re[b] = re[a] - tr; im[b] = im[a] - ti; re[a] += tr; im[a] += ti; const nr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = nr; } }
  }
}
function spectrogram(a, from, to, fftSize, hop) {
  to = Math.min(to, a.frames); from = Math.min(from, to); const n = to - from, bins = fftSize / 2 + 1, frames = n <= fftSize ? 1 : Math.floor((n - fftSize) / hop) + 1, db = new Float32Array(frames * bins).fill(-120);
  const w = new Float64Array(fftSize); let ws = 0; for (let i = 0; i < fftSize; ++i) { w[i] = .5 - .5 * Math.cos(2 * Math.PI * i / fftSize); ws += w[i]; }
  const re = new Float64Array(fftSize), im = new Float64Array(fftSize);
  for (let f = 0; f < frames; ++f) {
    const st = from + f * hop; for (let i = 0; i < fftSize; ++i) { const k = st + i; re[i] = (k < to ? .5 * (a.l[k] + a.r[k]) : 0) * w[i]; im[i] = 0; }
    fft(re, im); for (let b = 0; b < bins; ++b) db[f * bins + b] = Math.max(-120, 20 * Math.log10(Math.max(1e-9, Math.hypot(re[b], im[b]) * 2 / ws)));
  }
  return { bins, frames, db };
}
function peakOverview(a, from, to, cols) {
  const mn = new Float32Array(cols), mx = new Float32Array(cols); to = Math.min(to, a.frames); if (!cols || to <= from) return { mn, mx };
  const per = (to - from) / cols;
  for (let c = 0; c < cols; ++c) { const x0 = from + Math.floor(c * per); let x1 = from + Math.floor((c + 1) * per); if (x1 <= x0) x1 = x0 + 1; x1 = Math.min(x1, to); let lo = 0, hi = 0; for (let i = x0; i < x1; ++i) { lo = Math.min(lo, a.l[i], a.r[i]); hi = Math.max(hi, a.l[i], a.r[i]); } mn[c] = lo; mx[c] = hi; }
  return { mn, mx };
}

// ---------- spectral editing: STFT, Hann window, 75 percent overlap ----------
const SN = 2048, SH = 512, SWIN = (() => { const w = new Float64Array(SN); for (let i = 0; i < SN; ++i) w[i] = .5 - .5 * Math.cos(2 * PI * i / SN); return w; })();
function ifft(re, im) { const n = re.length; for (let i = 0; i < n; ++i) im[i] = -im[i]; fft(re, im); for (let i = 0; i < n; ++i) { re[i] /= n; im[i] = -im[i] / n; } }
// region r = { t0, t1 (frames), f0, f1 (Hz) }. The edge is soft: half a window in time, two bins in frequency.
function maskBins(regions, c, sr, m) {
  m.fill(0); let any = false; const fe = SN / 2;
  for (const r of regions) {
    const tw = clampf((c - r.t0) / fe + .5, 0, 1) * clampf((r.t1 - c) / fe + .5, 0, 1); if (tw <= 0) continue;
    const kLo = r.f0 * SN / sr, kHi = r.f1 * SN / sr, a = Math.max(0, Math.floor(kLo) - 2), b = Math.min(SN / 2, Math.ceil(kHi) + 2);
    for (let k = a; k <= b; ++k) { const fw = clampf((k - kLo) / 2 + .5, 0, 1) * clampf((kHi - k) / 2 + .5, 0, 1), v = tw * fw; if (v > m[k]) { m[k] = v; any = true; } }
  }
  return any;
}
function stft(a, s, e, edit) {
  const L = e - s, outs = [new Float64Array(L), new Float64Array(L)], chans = [a.l, a.r], re = new Float64Array(SN), im = new Float64Array(SN), n = a.frames;
  const j0 = Math.floor((s - SN) / SH) + 1, j1 = Math.floor((e - 1) / SH);
  for (let j = j0; j <= j1; ++j) {
    const st = j * SH, c = st + SN / 2;
    for (let ch = 0; ch < 2; ++ch) {
      const x = chans[ch]; for (let i = 0; i < SN; ++i) { const p = st + i; re[i] = (p >= 0 && p < n ? x[p] : 0) * SWIN[i]; im[i] = 0; }
      fft(re, im); edit(re, im, c, ch, j); ifft(re, im); const o = outs[ch];
      for (let i = 0; i < SN; ++i) { const p = st + i - s; if (p >= 0 && p < L) o[p] += re[i] * SWIN[i]; }
    }
  }
  for (let i = 0; i < L; ++i) { a.l[s + i] = outs[0][i] / 1.5; a.r[s + i] = outs[1][i] / 1.5; }
}
function regionSpan(regions, a) { let t0 = 1e18, t1 = -1e18; for (const r of regions) { t0 = Math.min(t0, r.t0); t1 = Math.max(t1, r.t1); } return [Math.max(0, Math.floor(t0) - 2 * SN), Math.min(a.frames, Math.ceil(t1) + 2 * SN)]; }
// mode 'cut': remove what is inside the regions. mode 'amp': change its level by gainDb.
function spectralEdit(a, regions, mode, gainDb) {
  if (!regions.length || !a.frames) return; const [s, e] = regionSpan(regions, a); if (e <= s) return;
  const g = dbToLin(gainDb || 0), m = new Float64Array(SN / 2 + 1); let lastC = null, any = false;
  stft(a, s, e, (re, im, c, ch) => {
    if (ch === 0) { lastC = c; any = maskBins(regions, c, a.sr, m); } if (!any) return;
    for (let k = 0; k <= SN / 2; ++k) { const v = m[k]; if (v <= 0) continue; const gk = mode === 'cut' ? 1 - v : 1 + v * (g - 1); re[k] *= gk; im[k] *= gk; if (k > 0 && k < SN / 2) { re[SN - k] *= gk; im[SN - k] *= gk; } }
  });
}
function spectralCopy(a, r) {
  if (!a.frames) return null; const sr = a.sr, kLo = Math.max(0, Math.floor(r.f0 * SN / sr) - 2), kHi = Math.min(SN / 2, Math.ceil(r.f1 * SN / sr) + 2), nb = kHi - kLo + 1;
  const j0 = Math.ceil((r.t0 - SN / 4 - SN / 2) / SH), j1 = Math.floor((r.t1 + SN / 4 - SN / 2) / SH), nf = Math.max(1, j1 - j0 + 1), m = new Float64Array(SN / 2 + 1), re = new Float64Array(SN), im = new Float64Array(SN), n = a.frames;
  const clip = { nf, nb, kLo, sr, fc: Math.sqrt(Math.max(20, r.f0) * r.f1), ch: [0, 1].map(() => ({ re: new Float32Array(nf * nb), im: new Float32Array(nf * nb) })) };
  for (let f = 0; f < nf; ++f) {
    const st = (j0 + f) * SH, c = st + SN / 2; maskBins([r], c, sr, m);
    for (let ch = 0; ch < 2; ++ch) { const x = ch ? a.r : a.l; for (let i = 0; i < SN; ++i) { const p = st + i; re[i] = (p >= 0 && p < n ? x[p] : 0) * SWIN[i]; im[i] = 0; } fft(re, im); for (let b = 0; b < nb; ++b) { const k = kLo + b; clip.ch[ch].re[f * nb + b] = re[k] * m[k]; clip.ch[ch].im[f * nb + b] = im[k] * m[k]; } }
  }
  return clip;
}
// Adds the copied piece back, its first frame at atFrame, its middle frequency at destHz.
function spectralPaste(a, clip, atFrame, destHz) {
  if (!clip || !a.frames) return false; const sr = a.sr, jt = Math.round((atFrame - SN / 2) / SH), dk = Math.round((destHz - clip.fc) * SN / sr);
  const s = Math.max(0, jt * SH - 2 * SN), e = Math.min(a.frames, (jt + clip.nf) * SH + 3 * SN); if (e <= s) return false;
  stft(a, s, e, (re, im, c, ch, j) => {
    const f = j - jt; if (f < 0 || f >= clip.nf) return; const C = clip.ch[ch], th = 2 * PI * dk * SH * f / SN, cs = Math.cos(th), sn = Math.sin(th);
    for (let b = 0; b < clip.nb; ++b) { const kk = clip.kLo + b + dk; if (kk < 1 || kk >= SN / 2) continue; const ur = C.re[f * clip.nb + b], ui = C.im[f * clip.nb + b], vr = ur * cs - ui * sn, vi = ur * sn + ui * cs; re[kk] += vr; im[kk] += vi; re[SN - kk] += vr; im[SN - kk] -= vi; }
  });
  return true;
}
function peakOverviewCh(a, ch, from, to, cols) {
  const x = ch ? a.r : a.l, mn = new Float32Array(cols), mx = new Float32Array(cols); to = Math.min(to, a.frames); if (!cols || to <= from) return { mn, mx };
  const per = (to - from) / cols;
  for (let c = 0; c < cols; ++c) { const x0 = from + Math.floor(c * per); let x1 = from + Math.floor((c + 1) * per); if (x1 <= x0) x1 = x0 + 1; x1 = Math.min(x1, to); let lo = 0, hi = 0; for (let i = x0; i < x1; ++i) { const v = x[i]; if (v < lo) lo = v; if (v > hi) hi = v; } mn[c] = lo; mx[c] = hi; }
  return { mn, mx };
}
function detectTransients(a, sens) {
  const out = [], hop = 256, frames = Math.floor(a.frames / hop); if (frames < 4) return out; const e = new Float32Array(frames);
  for (let f = 0; f < frames; ++f) { let s = 0; for (let i = 0; i < hop; ++i) { const x = .5 * (a.l[f * hop + i] + a.r[f * hop + i]); s += x * x; } e[f] = Math.sqrt(s / hop); }
  const ratio = 1.4 + 2.2 * (1 - clampf(sens, 0, 1)), minGap = Math.floor(.08 * a.sr / hop) + 1; let last = 0, any = false;
  for (let f = 2; f + 1 < frames; ++f) {
    let avg = 0; const a0 = f > 24 ? f - 24 : 0; for (let k = a0; k < f; ++k) avg += e[k]; avg /= (f - a0);
    const onset = e[f] > ratio * avg + .01 && e[f] > e[f - 1] && e[f - 1] <= e[f - 2] * 1.5 + 1e-4;
    if (onset && (!any || f - last >= minGap)) { out.push(f * hop); last = f; any = true; }
  }
  return out;
}

// ---------- operations ----------
function hermite(d, pos) {
  const n = d.length; if (!n) return 0; const i1 = Math.floor(pos), f = pos - i1, at = i => d[Math.max(0, Math.min(n - 1, i))];
  const y0 = at(i1 - 1), y1 = at(i1), y2 = at(i1 + 1), y3 = at(i1 + 2), c1 = .5 * (y2 - y0), c2 = y0 - 2.5 * y1 + 2 * y2 - .5 * y3, c3 = .5 * (y3 - y0) + 1.5 * (y1 - y2);
  return ((c3 * f + c2) * f + c1) * f + y1;
}
const ID = ['gain', 'normalize', 'fadeIn', 'fadeOut', 'reverse', 'invert', 'dcRemove', 'silence', 'mono', 'pan', 'rate', 'bitcrush', 'drive', 'eq', 'filter', 'delay', 'chorus', 'phaser', 'reverb', 'compressor', 'limiter', 'gate', 'trimSilence', 'loopXfade'];
function rng() { let s = 2463534242 >>> 0; return () => { s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0; return (s >>> 8) * (1 / 16777216); }; }
const lround = x => x < 0 ? -Math.round(-x) : Math.round(x);

// Applies one operation to frames [s, e). Returns the new end of that range.
function applyOp(a, op, s, e, p) {
  e = Math.min(e, a.frames); if (s >= e) return e; const sr = a.sr, n = e - s, ch = [a.l, a.r];
  switch (ID[op]) {
    case 'gain': { const g = dbToLin(p[0]); for (const c of ch) for (let i = s; i < e; ++i) c[i] *= g; break; }
    case 'normalize': {
      let g = 1;
      if (p[1] < .5) { const pk = a.peak(s, e); if (pk < 1e-9) break; g = dbToLin(p[0]) / pk; }
      else { let sum = 0; for (const c of ch) for (let i = s; i < e; ++i) sum += c[i] * c[i]; const rms = Math.sqrt(sum / (2 * n)); if (rms < 1e-9) break; g = dbToLin(p[0]) / rms; const pk = a.peak(s, e); if (pk * g > 1) g = 1 / pk; }
      for (const c of ch) for (let i = s; i < e; ++i) c[i] *= g; break;
    }
    case 'fadeIn': case 'fadeOut': for (let i = 0; i < n; ++i) { let t = n > 1 ? i / (n - 1) : 1; if (ID[op] === 'fadeOut') t = 1 - t; const g = Math.pow(t, p[0]); a.l[s + i] *= g; a.r[s + i] *= g; } break;
    case 'reverse': for (const c of ch) { let i = s, j = e - 1; while (i < j) { const t = c[i]; c[i] = c[j]; c[j] = t; ++i; --j; } } break;
    case 'invert': for (const c of ch) for (let i = s; i < e; ++i) c[i] = -c[i]; break;
    case 'dcRemove': for (const c of ch) { let m = 0; for (let i = s; i < e; ++i) m += c[i]; const mean = m / n; for (let i = s; i < e; ++i) c[i] -= mean; } break;
    case 'silence': for (const c of ch) c.fill(0, s, e); break;
    case 'mono': { const m = lround(p[0]); for (let i = s; i < e; ++i) { const v = m === 0 ? .5 * (a.l[i] + a.r[i]) : (m === 1 ? a.l[i] : a.r[i]); a.l[i] = a.r[i] = v; } break; }
    case 'pan': { const b = clampf(p[0] * .01, -1, 1), gl = b > 0 ? 1 - b : 1, gr = b < 0 ? 1 + b : 1; for (let i = s; i < e; ++i) { a.l[i] *= gl; a.r[i] *= gr; } break; }
    case 'rate': {
      const ratio = Math.pow(2, (p[0] + p[1] / 100) / 12), newN = Math.max(1, lround(n / ratio)), out = [];
      for (const c of ch) {
        const src = c.slice(s, e);
        if (ratio > 1) { const f1 = new Svf(), f2 = new Svf(), fc = .45 * sr / ratio; f1.set(sr, fc, .707); f2.set(sr, fc, .707); for (let i = 0; i < src.length; ++i) { f1.tick(src[i]); f2.tick(f1.lp); src[i] = f2.lp; } }
        const dst = new Float32Array(newN); for (let i = 0; i < newN; ++i) dst[i] = hermite(src, i * ratio); out.push(dst);
      }
      a.splice(s, e, out[0], out[1]); return s + newN;
    }
    case 'bitcrush': {
      const bits = Math.round(clampf(p[0], 1, 16)), levels = Math.pow(2, bits - 1), down = Math.max(1, lround(p[1])), dith = p[2] >= .5, mix = p[3] * .01;
      for (const c of ch) { const r = rng(); let held = 0; for (let i = 0; i < n; ++i) { const x = c[s + i]; if (i % down === 0) { const d = dith ? (r() - r()) : 0; held = Math.round(x * levels + d) / levels; } c[s + i] = x * (1 - mix) + held * mix; } }
      break;
    }
    case 'drive': {
      const d = p[0] * .01, x = p[1] * .01, f = p[2] * .01, mk = Colour.makeup(d, x, f);
      for (const c of ch) { let px = 0, py = 0; for (let i = s; i < e; ++i) { const y = Colour.shaper(c[i], d, x, f) * mk, o = y - px + .9995 * py; px = y; py = o; c[i] = o; } }
      break;
    }
    case 'eq': for (const c of ch) {
      const lo = new Biquad(), mid = new Biquad(), hi = new Biquad(), l = Math.abs(p[0]) > .01, m = Math.abs(p[1]) > .01, h = Math.abs(p[4]) > .01;
      if (l) lo.lowShelf(sr, 120, p[0]); if (m) mid.peak(sr, p[2], p[1], p[3]); if (h) hi.highShelf(sr, 8000, p[4]);
      for (let i = s; i < e; ++i) { let y = c[i]; if (l) y = lo.tick(y); if (m) y = mid.tick(y); if (h) y = hi.tick(y); c[i] = y; }
    } break;
    case 'filter': for (const c of ch) { const f = new Svf(); f.set(sr, p[1], p[2]); const t = lround(p[0]); for (let i = s; i < e; ++i) { f.tick(c[i]); c[i] = t === 0 ? f.lp : (t === 1 ? f.hp : f.bp * f.k); } } break;
    case 'delay': {
      const tail = lround(p[3] * sr); if (tail) { a.insertSilence(e, tail); e += tail; }
      const d = Math.max(1, lround(p[0] * sr / 1000)), fb = p[1] * .01, mix = p[2] * .01;
      for (const c of [a.l, a.r]) { const buf = new Float32Array(d); let w = 0; for (let i = s; i < e; ++i) { const inp = c[i], y = buf[w]; buf[w] = inp + y * fb; c[i] = inp + y * mix; if (++w >= d) w = 0; } }
      return e;
    }
    case 'chorus': {
      const base = .015 * sr, depth = p[1] * .001 * sr, mix = p[2] * .01, L = Math.floor(.05 * sr) + 8;
      [a.l, a.r].forEach((c, k) => { const buf = new Float32Array(L), ph0 = k ? .25 : 0; let w = 0;
        for (let i = 0; i < n; ++i) { const inp = c[s + i]; buf[w] = inp; const dl = base + depth * Math.sin(2 * Math.PI * (p[0] * i / sr + ph0)); let rp = w - dl; while (rp < 0) rp += L;
          const i0 = Math.floor(rp) % L, i1 = (i0 + 1) % L, fr = rp - Math.floor(rp), wet = buf[i0] + (buf[i1] - buf[i0]) * fr; c[s + i] = inp * (1 - mix) + wet * mix; if (++w >= L) w = 0; } });
      break;
    }
    case 'phaser': {
      let stages = lround(p[2 + 1]); stages = Math.max(2, Math.min(12, stages & ~1)); const depth = p[1] * .01, fb = p[2] * .01, mix = p[4] * .01;
      [a.l, a.r].forEach((c, k) => { const z = new Float32Array(12), ph0 = k ? .25 : 0; let last = 0;
        for (let i = 0; i < n; ++i) { const lfo = .5 + .5 * Math.sin(2 * Math.PI * (p[0] * i / sr + ph0)), f = 200 * Math.pow(20, depth * lfo), t = Math.tan(Math.PI * Math.min(f, sr * .45) / sr), cf = f32((t - 1) / (t + 1)), inp = c[s + i]; let x = inp + fb * last;
          for (let q = 0; q < stages; ++q) { const y = -cf * x + z[q]; z[q] = x + cf * y; x = y; } last = x; c[s + i] = inp * (1 - mix) + x * mix; } });
      break;
    }
    case 'reverb': {
      const tail = lround(p[3] * sr); if (tail) { a.insertSilence(e, tail); e += tail; }
      const combT = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617], apT = [556, 441, 341, 225], sc = sr / 44100, size = p[0] * .01, fbk = .7 + .28 * size, damp = .4 * p[1] * .01, mix = p[2] * .01;
      const len = e - s, mono = new Float32Array(len), dry = [a.l.slice(s, e), a.r.slice(s, e)]; for (let i = 0; i < len; ++i) mono[i] = (a.l[s + i] + a.r[s + i]) * .015;
      [a.l, a.r].forEach((c, k) => {
        const spread = k ? 23 : 0, cb = combT.map(t => ({ b: new Float32Array(Math.max(8, Math.round((t + spread) * sc))), i: 0, st: 0 })), ap = apT.map(t => ({ b: new Float32Array(Math.max(8, Math.round((t + spread) * sc))), i: 0 }));
        for (let i = 0; i < len; ++i) { let o = 0;
          for (const q of cb) { const y = q.b[q.i]; q.st = y * (1 - damp) + q.st * damp; q.b[q.i] = mono[i] + q.st * fbk; if (++q.i >= q.b.length) q.i = 0; o += y; }
          for (const q of ap) { const bo = q.b[q.i], y = -o + bo; q.b[q.i] = o + bo * .5; if (++q.i >= q.b.length) q.i = 0; o = y; }
          c[s + i] = dry[k][i] * (1 - mix) + o * 3 * mix; }
      });
      return e;
    }
    case 'compressor': {
      const thr = p[0], ratio = p[1], makeup = p[4], knee = 6, ca = 1 - Math.exp(-1 / (Math.max(.1, p[2]) * .001 * sr)), cr = 1 - Math.exp(-1 / (Math.max(1, p[3]) * .001 * sr)); let gr = 0;
      for (let i = s; i < e; ++i) { const lv = linToDb(Math.max(Math.abs(a.l[i]), Math.abs(a.r[i]))), over = lv - thr; let target = 0;
        if (2 * over > knee) target = (1 / ratio - 1) * over; else if (2 * Math.abs(over) <= knee) target = (1 / ratio - 1) * (over + knee * .5) * (over + knee * .5) / (2 * knee);
        gr += (target - gr) * (target < gr ? ca : cr); const g = dbToLin(gr + makeup); a.l[i] *= g; a.r[i] *= g; }
      break;
    }
    case 'limiter': {
      const ceil = dbToLin(p[0]), W = Math.max(1, lround(.002 * sr)), rc = 1 - Math.exp(-1 / (Math.max(1, p[1]) * .001 * sr)), g = new Float32Array(n), hm = new Float32Array(n);
      for (let i = 0; i < n; ++i) { const m = Math.max(Math.abs(a.l[s + i]), Math.abs(a.r[s + i])); g[i] = m > ceil ? ceil / m : 1; }
      for (let i = 0; i < n; ++i) { let m = 1; const hi = Math.min(n - 1, i + W); for (let j = i; j <= hi; ++j) m = Math.min(m, g[j]); hm[i] = m; }
      let r = 1;
      for (let i = 0; i < n; ++i) { let sum = 0; for (let k = 0; k <= W; ++k) sum += hm[Math.max(0, i - k)]; const ma = sum / (W + 1); r = Math.min(ma, r + (1 - r) * rc); a.l[s + i] *= r; a.r[s + i] *= r; }
      break;
    }
    case 'gate': {
      const thr = dbToLin(p[0]), range = dbToLin(p[4]), ca = 1 - Math.exp(-1 / (Math.max(.1, p[1]) * .001 * sr)), cr = 1 - Math.exp(-1 / (Math.max(1, p[3]) * .001 * sr)), holdN = lround(p[2] * .001 * sr); let hold = 0, g = range;
      for (let i = s; i < e; ++i) { const det = Math.max(Math.abs(a.l[i]), Math.abs(a.r[i])); let open = false; if (det >= thr) { hold = holdN; open = true; } else if (hold > 0) { --hold; open = true; }
        const t = open ? 1 : range; g += (t - g) * (t > g ? ca : cr); a.l[i] *= g; a.r[i] *= g; }
      break;
    }
    case 'trimSilence': {
      const thr = dbToLin(p[0]); let first = e, last = s, any = false;
      for (let i = s; i < e; ++i) if (Math.max(Math.abs(a.l[i]), Math.abs(a.r[i])) >= thr) { if (!any) first = i; last = i; any = true; }
      if (!any) break; a.erase(last + 1, e); a.erase(s, first); return s + (last - first + 1);
    }
    case 'loopXfade': {
      const xf = Math.min(lround(p[0] * .001 * sr), Math.floor(n / 2)); if (xf < 1) break;
      for (const c of ch) for (let k = 0; k < xf; ++k) { const t = (k + .5) / xf, wi = Math.sin(t * PI * .5), wo = Math.cos(t * PI * .5); c[s + k] = c[s + k] * wi + c[e - xf + k] * wo; }
      a.erase(e - xf, e); return e - xf;
    }
  }
  return e;
}
function defaultParams(OPS, op) { return OPS[op].params.map(q => q.def); }

// ---------- editor ----------
class Editor {
  constructor(OPS) { this.OPS = OPS; this.audio = new Audio(48000); this.selA = 0; this.selB = 0; this.cursor = 0; this.undoStack = []; this.redoStack = []; this.clip = new Audio(48000); this.last = null; this.limitBytes = 400 * 1024 * 1024; }
  load(a) { this.audio = a; this.selA = this.selB = 0; this.cursor = 0; this.undoStack = []; this.redoStack = []; this.last = null; }
  hasSelection() { return this.selB > this.selA; }
  setSelection(a, b) { if (a > b) [a, b] = [b, a]; this.selA = Math.min(a, this.audio.frames); this.selB = Math.min(b, this.audio.frames); this.cursor = this.selA; }
  setCursor(c) { this.cursor = Math.min(c, this.audio.frames); this.selA = this.selB = 0; }
  rangeStart() { return this.hasSelection() ? this.selA : 0; } rangeEnd() { return this.hasSelection() ? this.selB : this.audio.frames; }
  snap() { return { a: this.audio.clone(), sa: this.selA, sb: this.selB }; }
  restore(s) { this.audio = s.a; this.selA = s.sa; this.selB = s.sb; }
  pushUndo() { this.undoStack.push(this.snap()); this.redoStack = []; let bytes = 0; for (const s of this.undoStack) bytes += s.a.frames * 8; while (this.undoStack.length > 1 && bytes > this.limitBytes) { bytes -= this.undoStack[0].a.frames * 8; this.undoStack.shift(); } }
  apply(op, p) {
    if (op < 0 || op >= this.OPS.length || !this.audio.frames) return false;
    this.pushUndo(); const ne = applyOp(this.audio, op, this.rangeStart(), this.rangeEnd(), p);
    if (this.hasSelection()) this.selB = ne; else if (this.OPS[op].changesLength) this.selA = this.selB = 0;
    this.last = { op, p: p.slice() }; return true;
  }
  repeatLast() { return this.last ? this.apply(this.last.op, this.last.p) : false; }
  canUndo() { return this.undoStack.length > 0; } canRedo() { return this.redoStack.length > 0; }
  undo() { if (!this.undoStack.length) return; this.redoStack.push(this.snap()); this.restore(this.undoStack.pop()); }
  redo() { if (!this.redoStack.length) return; this.undoStack.push(this.snap()); this.restore(this.redoStack.pop()); }
  copy() { if (this.hasSelection()) this.clip = this.audio.slice(this.selA, this.selB); }
  cut() { if (!this.hasSelection()) return; this.clip = this.audio.slice(this.selA, this.selB); this.pushUndo(); this.audio.erase(this.selA, this.selB); this.cursor = this.selA; this.selB = this.selA; }
  deleteSelection() { if (!this.hasSelection()) return; this.pushUndo(); this.audio.erase(this.selA, this.selB); this.cursor = this.selA; this.selB = this.selA; }
  cropToSelection() { if (!this.hasSelection()) return; this.pushUndo(); this.audio = this.audio.slice(this.selA, this.selB); this.selA = 0; this.selB = this.audio.frames; }
  paste() { if (!this.clip.frames) return; this.pushUndo(); const at = this.hasSelection() ? this.selA : Math.min(this.cursor, this.audio.frames); this.audio.insert(at, this.clip); this.selA = at; this.selB = at + this.clip.frames; }
  insertSilenceAtSelection(n) { this.pushUndo(); const at = this.rangeStart(); this.audio.insertSilence(at, n); this.selA = at; this.selB = at + n; }
  // mode: 0 replace everything, 1 append at the end, 2 replace the selection (punch), 3 insert at the start of the selection
  takeRecording(rec, mode) {
    if (!rec.frames) return; if (mode === 0 || !this.audio.frames) { this.load(rec); return; }
    this.pushUndo();
    if (mode === 1) { const at = this.audio.frames; this.audio.insert(at, rec); this.selA = at; this.selB = at + rec.frames; }
    else if (mode === 2 && this.hasSelection()) { const a = this.selA; this.audio.erase(this.selA, this.selB); this.audio.insert(a, rec); this.selA = a; this.selB = a + rec.frames; }
    else { const at = this.rangeStart(); this.audio.insert(at, rec); this.selA = at; this.selB = at + rec.frames; }
  }
  spectralEdit(mode, regions, db) { if (!this.audio.frames || !regions.length) return false; this.pushUndo(); spectralEdit(this.audio, regions, mode, db); return true; }
  spectralCopy(r) { const c = spectralCopy(this.audio, r); if (c) this.sclip = c; return !!c; }
  spectralPaste(atFrame, destHz) { if (!this.sclip || !this.audio.frames) return false; this.pushUndo(); spectralPaste(this.audio, this.sclip, atFrame, destHz); return true; }
  snapToZero(pos, radius) {
    const n = this.audio.frames; if (n < 2) return pos; pos = Math.min(pos, n - 1); const m = i => .5 * (this.audio.l[i] + this.audio.r[i]);
    for (let d = 0; d <= radius; ++d) { if (pos >= d && pos - d + 1 < n && m(pos - d) * m(pos - d + 1) <= 0) return pos - d; if (pos + d + 1 < n && m(pos + d) * m(pos + d + 1) <= 0) return pos + d; }
    return pos;
  }
}

// ---------- recorder (always listening) ----------
class Recorder {
  constructor() { this.maxFrames = 48000 * 60 * 20; this.mode = 0; this.recording = false; this.pl = []; this.pr = []; }
  prepare(sr, secs) { this.sr = sr; this.cap = Math.floor((secs || 30) * sr) + 1; this.ring = [new Float32Array(this.cap), new Float32Array(this.cap)]; this.w = 0; this.filled = 0; this.recording = false; this.mode = 0; this.pl = []; this.pr = []; }
  // mode: 0 off, 1 immediate, 2 threshold
  arm(mode, thresholdDb, preRollMs) { this.mode = mode; this.thr = dbToLin(thresholdDb === undefined ? -40 : thresholdDb); this.preRoll = Math.round(Math.max(0, preRollMs || 0) * .001 * this.sr); this.pl = []; this.pr = []; this.recording = false; if (mode === 1) this.begin(); }
  isRecording() { return this.recording; } isArmed() { return this.mode !== 0 && !this.recording; }
  recordedFrames() { return this.pl.length; }
  begin() { this.recording = true; const pre = this.captureLast(Math.min(this.preRoll, this.filled) / this.sr); this.pl = Array.from(pre.l); this.pr = Array.from(pre.r); }
  process(l, r, n) {
    for (let i = 0; i < n; ++i) {
      const x = l[i], y = r ? r[i] : l[i];
      if (this.mode === 2 && !this.recording && Math.max(Math.abs(x), Math.abs(y)) >= this.thr) this.begin();
      if (this.recording && this.pl.length < this.maxFrames) { this.pl.push(x); this.pr.push(y); }
      this.ring[0][this.w] = x; this.ring[1][this.w] = y; if (++this.w >= this.cap) this.w = 0; if (this.filled < this.cap) ++this.filled;
    }
  }
  stop() { this.recording = false; this.mode = 0; const o = new Audio(this.sr); o.l = Float32Array.from(this.pl); o.r = Float32Array.from(this.pr); this.pl = []; this.pr = []; return o; }
  captureLast(seconds) { const o = new Audio(this.sr), want = Math.min(this.filled, Math.round(seconds * this.sr)); o.l = new Float32Array(want); o.r = new Float32Array(want); for (let i = 0; i < want; ++i) { const k = (this.w + this.cap - want + i) % this.cap; o.l[i] = this.ring[0][k]; o.r[i] = this.ring[1][k]; } return o; }
}

const MC = { peakOverviewCh, spectralEdit, spectralCopy, spectralPaste, SN, SH, Audio, Editor, Recorder, applyOp, defaultParams, encodeWav, decodeWav, spectrogram, peakOverview, detectTransients, ID, Colour, dbToLin, linToDb };
if (typeof module !== 'undefined' && module.exports) module.exports = MC; else root.MC = MC;
})(typeof window !== 'undefined' ? window : globalThis);
