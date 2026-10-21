// Checks the JS-only DSP of the mockup engine: resampling, stretch, pitch shift, formants, pitch and tempo detection, format tools.
const MC = require('../docs/mockups/engine.js'); let bad = 0; const ok = (n, c, x) => { console.log((c ? 'ok   ' : 'FAIL ') + n + (x !== undefined ? '  ' + x : '')); if (!c) bad++; };
const sine = (sr, f, secs, amp) => { const a = new MC.Audio(sr, Math.round(sr * secs)); for (let i = 0; i < a.frames; ++i) a.l[i] = a.r[i] = (amp || .5) * Math.sin(2 * Math.PI * f * i / sr); return a; };
// frequency by zero crossings in the middle part
const freq = (x, sr) => { const a = Math.floor(x.length * .25), b = Math.floor(x.length * .75); let z = 0, first = -1, last = -1; for (let i = a + 1; i < b; ++i) if (x[i - 1] <= 0 && x[i] > 0) { if (first < 0) first = i; last = i; z++; } return (z - 1) * sr / (last - first); };
const rms = (x, a, b) => { let s = 0; for (let i = a; i < b; ++i) s += x[i] * x[i]; return Math.sqrt(s / (b - a)); };
let a = sine(48000, 440, 1); let r = MC.resample(a, 44100);
ok('resample length', r.frames === 44100, r.frames); ok('resample keeps the pitch', Math.abs(freq(r.l, 44100) - 440) < 1.5, freq(r.l, 44100).toFixed(1)); ok('resample keeps the level', Math.abs(rms(r.l, 5000, 30000) - .5 / Math.SQRT2) < .01);
r = MC.resample(sine(48000, 440, 1), 96000); ok('resample up', r.frames === 96000 && Math.abs(freq(r.l, 96000) - 440) < 1.5);
let st = MC.pvStretch(a.l, 1.5); ok('stretch length', st.length === 72000, st.length); ok('stretch keeps the pitch', Math.abs(freq(st, 48000) - 440) < 3, freq(st, 48000).toFixed(1)); ok('stretch keeps the level', Math.abs(rms(st, 20000, 50000) - .3535) < .03, rms(st, 20000, 50000).toFixed(3));
st = MC.pvStretch(a.l, .75); ok('shorten length', st.length === 36000 && Math.abs(freq(st, 48000) - 440) < 3);
let ps = MC.pitchShift(a, 12, 0, 0); ok('pitch +12', ps.frames === a.frames && Math.abs(freq(ps.l, 48000) - 880) < 6, freq(ps.l, 48000).toFixed(1)); ps = MC.pitchShift(a, -5, 1, 0); ok('pitch -5 with formants', Math.abs(freq(ps.l, 48000) - 440 * Math.pow(2, -5 / 12)) < 4, freq(ps.l, 48000).toFixed(1) + ' level ' + rms(ps.l, 12000, 36000).toFixed(3));
let w = MC.warpAudio(a, { ratio: 2, mode: 1 }); ok('repitch is faster and higher', w.frames === 24000 && Math.abs(freq(w.l, 48000) - 880) < 6);
w = MC.warpAudio(a, { ratio: 1.25, mode: 0 }); ok('stretch mode', w.frames === 38400 && Math.abs(freq(w.l, 48000) - 440) < 3);
w = MC.warpAudio(a, { ratio: .8, mode: 3 }); ok('texture length', w.frames === 60000 && rms(w.l, 10000, 50000) > .15, rms(w.l, 10000, 50000).toFixed(3));
// beats: clicks stay sharp
const cl = new MC.Audio(48000, 48000 * 2); const hits = []; for (let b = 0; b < 4; ++b) { const t0 = b * 24000; hits.push(t0); for (let i = 0; i < 6000; ++i) cl.l[t0 + i] = cl.r[t0 + i] = Math.exp(-i / 800) * Math.sin(2 * Math.PI * 200 * i / 48000); }
w = MC.warpAudio(cl, { ratio: .8, mode: 2, hits }); ok('beats length', w.frames === 120000, w.frames); let pk = 0, pi = 0; for (let i = 0; i < 2000; ++i) if (Math.abs(w.l[i]) > pk) { pk = Math.abs(w.l[i]); pi = i; } ok('beats keep the hit', pk > .9 && rms(w.l, 30000 + 0, 30000 + 100) > 0, pk.toFixed(2));
// pitch detect
const d = MC.detectNotes(sine(48000, 220, 1.2, .5)); ok('pitch of 220 Hz is note 57', d.notes.length === 1 && Math.abs(d.notes[0].m - 57) < .1, JSON.stringify(d.notes.map(n => [n.t0, n.t1, +n.m.toFixed(2)])));
const two = new MC.Audio(48000, 48000 * 2); for (let i = 0; i < two.frames; ++i) { const f = i < 48000 ? 220 : 330; two.l[i] = two.r[i] = .4 * Math.sin(2 * Math.PI * f * i / 48000 * 1); } // second half phase jump is fine
const dn = MC.detectNotes(two); ok('two notes found', dn.notes.length === 2 && Math.abs(dn.notes[0].m - 57) < .2 && Math.abs(dn.notes[1].m - 64.02) < .3, JSON.stringify(dn.notes.map(n => +n.m.toFixed(2))));
const sh = MC.pitchShift(sine(48000, 220, 1.5, .5), 3, 1, 0); const dsh = MC.detectNotes(sh); ok('shifted note is 3 up', dsh.notes.length >= 1 && Math.abs(dsh.notes[0].m - 60) < .2, dsh.notes.map(n => n.m.toFixed(2)).join(','));
// range shift
const rg = sine(48000, 220, 1.5, .5); MC.pitchShiftRange(rg, 24000, 48000, 12, 1, 0); const seg = rg.l.subarray(30000, 42000); ok('range shift changes only the range', Math.abs(freq(seg, 48000) - 440) < 6 && Math.abs(freq(rg.l.subarray(2000, 20000), 48000) - 220) < 2, freq(seg, 48000).toFixed(1));
// bpm
const dr = new MC.Audio(48000, 48000 * 4); for (let b = 0; b < 8; ++b) { const t0 = b * 24000; for (let i = 0; i < 12000; ++i) dr.l[t0 + i] = dr.r[t0 + i] = Math.exp(-i / 2500) * Math.sin(2 * Math.PI * 60 * i / 48000); } const bp = MC.detectBpm(dr); ok('tempo of a 120 bpm kick', Math.abs(bp - 120) < 1, bp);
ok('too short has no tempo', MC.detectBpm(sine(48000, 100, .3)) === null);
// quantize
const q = sine(48000, 440, .2, .9); MC.quantize(q, 8, false); const lv = new Set(); for (let i = 0; i < q.frames; ++i) lv.add(Math.round(q.l[i] * 128)); ok('8 bit has few steps', lv.size <= 256 && Math.abs(q.l[100] * 128 - Math.round(q.l[100] * 128)) < 1e-6);
// channels
const c = new MC.Audio(48000, 4); c.l.set([.5, .2, -.4, 0]); c.r.set([.1, .2, .4, 1]); const cc = c.clone(); MC.channelOp(cc, 'enc'); MC.channelOp(cc, 'dec'); let e = 0; for (let i = 0; i < 4; ++i) e = Math.max(e, Math.abs(cc.l[i] - c.l[i]), Math.abs(cc.r[i] - c.r[i])); ok('mid side round trip', e < 1e-6); const m = c.clone(); MC.channelOp(m, 'mix'); ok('mix to mono', m.l[0] === m.r[0] && Math.abs(m.l[0] - .3) < 1e-6 && MC.isMonoAudio(m));
// history
const ed = new MC.Editor([{ name: 'X', params: [], changesLength: false }]); ed.load(sine(48000, 100, .1)); ed.transform('One', x => { x.l[0] = 1; }); ed.transform('Two', x => { x.l[0] = 2; }); ed.transform('Three', x => { x.l[0] = 3; });
let h = ed.history(); ok('history labels', h.done.join() === 'One,Two,Three' && h.undone.length === 0); ed.jumpTo(1); h = ed.history(); ok('jump back', ed.audio.l[0] === 1 && h.done.join() === 'One' && h.undone.join() === 'Two,Three', h.undone.join()); ed.jumpTo(3); ok('jump forward', ed.audio.l[0] === 3);
// pencils
const pa = new MC.Audio(48000, 48000); MC.pencilTone(pa, 0, 24000, 1000, 1000, .5, 0, 100, 100); ok('pencil tone has the pitch', Math.abs(freq(pa.l.subarray(2000, 20000), 48000) - 1000) < 15 && Math.max(...pa.l) <= .5001 && pa.l[30000] === 0);
const pw = new MC.Audio(100, 100); MC.pencilWave(pw, 10, 0, 20, 1, 0); ok('pencil wave sets one side', Math.abs(pw.l[15] - .5) < .01 && pw.r[15] === 0); MC.pencilWave(pw, 30, 2, 30, 2, -1); ok('pencil wave clamps both sides', pw.l[30] === 1 && pw.r[30] === 1);
const w8 = MC.encodeWav(sine(48000, 440, .01, .5), 8); ok('8 bit wav is 1 byte per sample', w8.length === 44 + 480 * 2 && w8[34] === 8);
console.log(bad ? bad + ' FAILED' : 'all ok'); process.exit(bad ? 1 : 0);
