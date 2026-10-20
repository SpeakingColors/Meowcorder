// Checks the spectral editing in docs/mockups/engine.js. Run: node tools/spectral_test.js
const MC = require('../docs/mockups/engine.js'); let bad = 0;
const ok = (c, m) => { if (!c) { ++bad; console.log('FAIL', m); } else console.log('ok  ', m); };
const sr = 48000, n = 3 * sr, mk = () => { const a = new MC.Audio(sr, n); for (let i = 0; i < n; ++i) { const t = i / sr, v = .3 * Math.sin(2 * Math.PI * 1000 * t) + .3 * Math.sin(2 * Math.PI * 3000 * t); a.l[i] = v; a.r[i] = v * .5; } return a; };
const tone = (a, f, s0, s1) => { let re = 0, im = 0; for (let i = s0; i < s1; ++i) { const w = 2 * Math.PI * f * i / sr; re += a.l[i] * Math.cos(w); im += a.l[i] * Math.sin(w); } return 20 * Math.log10(Math.max(1e-9, 2 * Math.hypot(re, im) / (s1 - s0))); };
let a = mk(), ref = mk(); MC.spectralEdit(a, [{ t0: 10000, t1: 20000, f0: 100, f1: 200 }], 'amp', 0);
let d = 0; for (let i = 0; i < n; ++i) d = Math.max(d, Math.abs(a.l[i] - ref.l[i]), Math.abs(a.r[i] - ref.r[i])); ok(d < 1e-4, 'no change gives the same sound, max diff ' + d.toExponential(2));
a = mk(); const R = { t0: sr * .5, t1: sr * 1.5, f0: 2500, f1: 3500 }; MC.spectralEdit(a, [R], 'cut');
const in3 = tone(a, 3000, sr * .7, sr * 1.3), in1 = tone(a, 1000, sr * .7, sr * 1.3), out3 = tone(a, 3000, sr * 2, sr * 2.5), pre3 = tone(a, 3000, 0, sr * .3);
ok(in3 < -50, '3 kHz inside the box removed: ' + in3.toFixed(1) + ' dB'); ok(Math.abs(in1 - tone(ref, 1000, sr * .7, sr * 1.3)) < .2, '1 kHz inside the box kept: ' + in1.toFixed(1) + ' dB');
ok(Math.abs(out3 - tone(ref, 3000, sr * 2, sr * 2.5)) < .1 && Math.abs(pre3 - tone(ref, 3000, 0, sr * .3)) < .1, '3 kHz outside the time range kept');
a = mk(); MC.spectralEdit(a, [R], 'amp', 6); const g = tone(a, 3000, sr * .7, sr * 1.3) - tone(ref, 3000, sr * .7, sr * 1.3); ok(Math.abs(g - 6) < .3, 'amp +6 dB gives ' + g.toFixed(2) + ' dB');
a = mk(); const clip = MC.spectralCopy(a, { t0: sr * .5, t1: sr * 1.0, f0: 2500, f1: 3500 }); const b = new MC.Audio(sr, n); ok(MC.spectralPaste(b, clip, sr * 2, clip.fc), 'paste runs');
const p3 = tone(b, 3000, sr * 2.1, sr * 2.4), p1 = tone(b, 1000, sr * 2.1, sr * 2.4), e3 = tone(b, 3000, sr * .5, sr * 1); ok(Math.abs(p3 - tone(ref, 3000, sr * .5, sr * 1)) < 1, 'pasted 3 kHz level matches: ' + p3.toFixed(1)); ok(p1 < -50 && e3 < -80, 'nothing else pasted');
const c2 = new MC.Audio(sr, n); MC.spectralPaste(c2, clip, sr * 2, clip.fc + 2000); const up = 3000 + Math.round(2000 * 2048 / sr) * sr / 2048; ok(tone(c2, up, sr * 2.1, sr * 2.4) > tone(c2, 3000, sr * 2.1, sr * 2.4) + 40, 'paste can move up in frequency to 5 kHz');
const ed = new MC.Editor([]); ed.load(mk()); ed.spectralEdit('cut', [R]); ok(ed.canUndo(), 'editor edit can be undone'); ed.undo(); let dd = 0; for (let i = 0; i < n; i += 97) dd = Math.max(dd, Math.abs(ed.audio.l[i] - ref.l[i])); ok(dd === 0, 'undo restores exactly');
console.log(bad ? bad + ' failed' : 'all ok'); process.exit(bad ? 1 : 0);
