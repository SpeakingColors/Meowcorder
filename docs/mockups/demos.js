// Meowcorder demo sounds: made by code so nothing is downloaded. Used by the mockup (File, Demo sounds) and by tools/render_demos.js,
// which renders the before and after clips for the website. The sounds are generic on purpose.
(function (root) {
'use strict';
const MC = root.MC || require('./engine.js');
function rngMk(s) { let x = s >>> 0; return () => { x ^= x << 13; x >>>= 0; x ^= x >>> 17; x ^= x << 5; x >>>= 0; return x / 4294967296; }; }
function makeDemo(sr) {
  const len = Math.floor(4 * sr), a = new MC.Audio(sr, len), rnd = rngMk(1234), l = a.l, r = a.r;
  for (let b = 0; b < 8; ++b) { const t0 = Math.floor(b * .5 * sr); for (let i = 0; i < .35 * sr && t0 + i < len; ++i) { const t = i / sr, ph = 2 * Math.PI * (45 * t + 110 * (1 - Math.exp(-t * 28)) / 28), s = Math.sin(ph) * Math.exp(-t * 7) * .9; l[t0 + i] += s; r[t0 + i] += s; } }
  for (let b = 0; b < 8; ++b) { const t0 = Math.floor((b * .5 + .25) * sr); let lp = 0; for (let i = 0; i < .06 * sr && t0 + i < len; ++i) { const n = rnd() * 2 - 1, h = n - lp; lp += (n - lp) * .55; const s = h * Math.exp(-i / sr * 70) * .28; l[t0 + i] += s; r[t0 + i] += s * .8; } }
  const notes = [57, 60, 64, 67]; for (let b = 0; b < 4; ++b) { const t0 = Math.floor((b * .5 + .5) * sr * .5 * 2 / 2 * 1), root = [0, 0, -2, -4][b];
    for (let i = 0; i < .45 * sr && t0 + i < len; ++i) { const t = i / sr; let s = 0; notes.forEach((n, k) => { const f = 440 * Math.pow(2, (n + root - 69) / 12), ph = (t * f + k * .13) % 1; s += ph * 2 - 1; }); s *= Math.exp(-t * 6) * .07; l[t0 + i] += s * 1.2; r[t0 + i] += s * .8; } }
  let pk = 0; for (let i = 0; i < len; ++i) pk = Math.max(pk, Math.abs(l[i]), Math.abs(r[i])); const g = pk > 0 ? .8 / pk : 1; for (let i = 0; i < len; ++i) { l[i] *= g; r[i] *= g; }
  return a;
}
function makeMelody(sr, cents) {
  const len = Math.floor(4 * sr), a = new MC.Audio(sr, len), seq = [57, 60, 64, 62, 60, 59, 57, 64]; let ph = 0;
  for (let n = 0; n < 8; ++n) {
    const t0 = Math.floor(n * .5 * sr), L = Math.floor(.46 * sr), f0 = 440 * Math.pow(2, (seq[n] - 69 + (cents || 0) / 100) / 12);
    for (let i = 0; i < L && t0 + i < len; ++i) {
      const t = i / sr, f = f0 * (1 + .006 * Math.sin(2 * Math.PI * 5.5 * t) * Math.min(1, t * 4)); ph += 2 * Math.PI * f / sr; let s = 0;
      for (let h = 1; h <= 16; ++h) { const hf = f * h; if (hf > sr * .45) break; s += Math.sin(h * ph) / h * (Math.exp(-Math.pow((hf - 800) / 500, 2)) + .6 * Math.exp(-Math.pow((hf - 2400) / 700, 2)) + .06); }
      a.l[t0 + i] = a.r[t0 + i] = s * Math.min(1, t / .02) * Math.min(1, (L - i) / (.05 * sr));
    }
  }
  let pk = 0; for (let i = 0; i < len; ++i) pk = Math.max(pk, Math.abs(a.l[i])); const g = pk > 0 ? .8 / pk : 1; for (let i = 0; i < len; ++i) { a.l[i] *= g; a.r[i] *= g; } return a;
}

// 6 s of warm pad and room hiss with a steady whistle on top (about 2.6 kHz with a little vibrato). The spectral view shows the whistle as a line.
function makeWhistle(sr) {
  const len = Math.floor(6 * sr), a = new MC.Audio(sr, len), rnd = rngMk(77); let lp = 0, ph = 0, wph = 0;
  const pad = [110, 130.81, 164.81, 196]; 
  for (let i = 0; i < len; ++i) {
    const t = i / sr; let s = 0;
    pad.forEach((f, k) => { for (let h = 1; h <= 5; ++h) s += Math.sin(2 * Math.PI * f * h * t + k * 1.7 + h) / (h * h) * (.8 + .2 * Math.sin(2 * Math.PI * .25 * t + k)); });
    s *= .11; const n = rnd() * 2 - 1; lp += (n - lp) * .08; s += lp * .05;
    const env = Math.min(1, Math.max(0, (t - .8) / .15)) * Math.min(1, Math.max(0, (5.6 - t) / .15));
    wph += 2 * Math.PI * (2600 + 35 * Math.sin(2 * Math.PI * 5 * t)) / sr; s += Math.sin(wph) * .22 * env;
    a.l[i] = s; a.r[i] = s * .98;
  }
  return a;
}
// The box that covers the whistle: seconds and Hz.
const whistleBox = { t0: .75, t1: 5.75, f0: 2300, f1: 2950 };

// 9 s take: a click, a short silence, a sung phrase, and a long silence with hiss after it.
function makeTake(sr) {
  const len = Math.floor(9 * sr), a = new MC.Audio(sr, len), rnd = rngMk(5); let lp = 0, ph = 0;
  const notes = [[1.8, 2.6, 220], [2.7, 3.5, 261.63], [3.6, 5.0, 329.63]];
  for (let i = 0; i < len; ++i) {
    const t = i / sr, n = rnd() * 2 - 1; lp += (n - lp) * .2; let s = lp * .006;
    for (const [t0, t1, f] of notes) if (t >= t0 && t < t1) {
      const e = Math.min(1, (t - t0) / .04) * Math.min(1, (t1 - t) / .12), ff = f * (1 + .004 * Math.sin(2 * Math.PI * 5.2 * t)); ph = 2 * Math.PI * ff * t;
      let v = 0; for (let h = 1; h <= 10; ++h) v += Math.sin(h * ph) / h * Math.exp(-Math.pow((ff * h - 900) / 700, 2)); s += v * .3 * e;
    }
    a.l[i] = s; a.r[i] = s;
  }
  const c = Math.floor(.45 * sr); for (let i = 0; i < 40; ++i) { const v = .95 * Math.exp(-i / 9) * (i % 2 ? -1 : 1); a.l[c + i] += v; a.r[c + i] += v; }
  return a;
}
const takeClick = { t0: .44, t1: .47 };

root.DEMOS = { rngMk, makeDemo, makeMelody, makeWhistle, whistleBox, makeTake, takeClick };
if (typeof module !== 'undefined' && module.exports) module.exports = root.DEMOS;
})(typeof window !== 'undefined' ? window : globalThis);
