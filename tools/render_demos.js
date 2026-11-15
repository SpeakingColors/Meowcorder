// Renders the before and after clips for the website with the same engine the mockup uses.
// Run: node tools/render_demos.js   (needs ffmpeg). Writes docs/audio/*.mp3 and docs/audio/clips.json.
const fs = require('fs'), path = require('path'), cp = require('child_process');
const MC = require('../docs/mockups/engine.js'), D = require('../docs/mockups/demos.js'), OPS = require('../docs/mockups/ops.json');
const SR = 44100, out = path.join(__dirname, '..', 'docs', 'audio'), tmp = fs.mkdtempSync(path.join(require('os').tmpdir(), 'mcdemo-'));
fs.mkdirSync(out, { recursive: true });
const opIdx = id => MC.ID.indexOf(id), opDef = id => OPS[opIdx(id)].params.map(p => p.def);
const peak = a => { let p = 0; for (let i = 0; i < a.frames; ++i) p = Math.max(p, Math.abs(a.l[i]), Math.abs(a.r[i])); return p; };
const band = (a, f0, f1, t0) => { // level in dB of a band, from a windowed transform of one second of the left channel
  const n = 1 << 16, s = Math.floor(t0 * a.sr), x = new Float64Array(n); for (let i = 0; i < n; ++i) x[i] = (a.l[s + i] || 0) * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / n));
  let e = 0; for (let k = Math.floor(f0 * n / a.sr); k <= Math.ceil(f1 * n / a.sr); ++k) { let re = 0, im = 0; const w2 = 2 * Math.PI * k / n; for (let i = 0; i < n; i += 4) { re += x[i] * Math.cos(w2 * i); im -= x[i] * Math.sin(w2 * i); } e += re * re + im * im; }
  return 10 * Math.log10(e + 1e-12); };
const tile = (a, n, secs) => { const o = new MC.Audio(a.sr, Math.floor(secs * a.sr)); for (let i = 0; i < o.frames; ++i) { o.l[i] = a.l[i % a.frames]; o.r[i] = a.r[i % a.frames]; } return o; };
const edit = (a, id, s, e, p) => { const ed = new MC.Editor(); ed.OPS = OPS; ed.load(a); ed.setSelection(s, e); ed.apply(opIdx(id), p || opDef(id)); return ed.audio; };
const clips = [], checks = [];
const ok = (name, cond, info) => { checks.push([name, cond, info]); console.log((cond ? 'ok   ' : 'FAIL ') + name + (info ? '  ' + info : '')); };

function save(file, a, gain) {
  const g = gain || .89 / Math.max(1e-6, peak(a)), wav = path.join(tmp, file + '.wav'), mp3 = path.join(out, file + '.mp3'), b = a.clone();
  for (let i = 0; i < b.frames; ++i) { b.l[i] *= g; b.r[i] *= g; }
  fs.writeFileSync(wav, Buffer.from(MC.encodeWav(b, 16)));
  cp.execFileSync('ffmpeg', ['-y', '-v', 'error', '-i', wav, '-codec:a', 'libmp3lame', '-b:a', '96k', '-ac', '2', '-metadata', 'artist=Studio 87', '-metadata', 'comment=Made with the Meowcorder engine', mp3]);
  return Math.round(fs.statSync(mp3).size / 1024);
}
function pair(key, title, before, after, bDesc, aDesc) {
  const g = .89 / Math.max(peak(before), peak(after));       // one shared gain, so the level difference you hear is real
  for (const [suffix, a, d, lab] of [['before', before, bDesc, 'Before'], ['after', after, aDesc, 'After']]) {
    const file = 'meowcorder-' + key + '-' + suffix, kb = save(file, a, g);
    clips.push({ file: file + '.mp3', pair: key, title, name: title + ', ' + lab.toLowerCase(), seconds: +(a.frames / a.sr).toFixed(1), description: d });
    console.log('wrote', file + '.mp3', (a.frames / a.sr).toFixed(1) + ' s', kb + ' KB');
  }
}

// 1 spectral erase: a whistle on a pad, erased with one box in the spectral view
{
  const before = D.makeWhistle(SR), after = before.clone(), r = D.whistleBox;
  MC.spectralEdit(after, [{ t0: Math.floor(r.t0 * SR), t1: Math.floor(r.t1 * SR), f0: r.f0, f1: r.f1 }], 'cut', 0);
  const w0 = band(before, 2400, 2800, 2, 3), w1 = band(after, 2400, 2800, 2, 3), p0 = band(before, 100, 700, 2, 3), p1 = band(after, 100, 700, 2, 3);
  ok('whistle erased by at least 30 dB', w0 - w1 >= 30, (w0 - w1).toFixed(1) + ' dB');
  ok('pad untouched (within 1 dB)', Math.abs(p0 - p1) < 1, (p1 - p0).toFixed(2) + ' dB');
  pair('spectral-erase', 'Spectral erase', before, after, 'A warm pad with a whistle on top, about 2.6 kHz.', 'The same sound after one box in the spectral view erased the whistle. The pad is untouched.');
}
// 2 warp: a loop slowed from 120 to 96 bpm with the pitch kept (Beats mode keeps the drum hits sharp)
{
  const before = tile(D.makeDemo(SR), 2, 7.5), hits = MC.detectTransients(before, .6), ratio = 96 / 120;
  const after = MC.warpAudio(before, { ratio, mode: 2, hits });
  ok('warp slows by 25 percent', Math.abs(after.frames / before.frames - 1 / ratio) < .02, (after.frames / before.frames).toFixed(3));
  ok('warp stays within 10 s', after.frames / SR <= 10, (after.frames / SR).toFixed(1) + ' s');
  pair('warp', 'Warp', before, after, 'A drum loop at its own tempo, 120 bpm.', 'The same loop slowed to 96 bpm with the pitch kept. Warp mode Beats keeps the drum hits sharp.');
}
// 3 pitch fix: a melody 40 cents flat, corrected by +0.4 semitone
{
  const before = tile(D.makeMelody(SR, -40), 2, 8), after = MC.pitchShift(before, .4, 0, 0);
  const dn = a => { const d = MC.detectNotes(a); return d.notes.reduce((s, n) => s + (n.m - Math.round(n.m)), 0) / Math.max(1, d.notes.length) * 100; };
  const e0 = dn(before), e1 = dn(after);
  ok('melody starts about 40 cents flat', e0 < -25 && e0 > -55, e0.toFixed(1) + ' cents');
  ok('melody ends within 10 cents', Math.abs(e1) < 10, e1.toFixed(1) + ' cents');
  pair('pitch-fix', 'Pitch fix', before, after, 'A short synth melody 40 cents flat.', 'The same melody moved up 0.4 semitone: now in tune.');
}
// 4 cleanup: click silenced, silence trimmed, fade out, normalized
{
  const before = D.makeTake(SR); let a = before.clone(); const c = D.takeClick;
  a = edit(a, 'silence', Math.floor(c.t0 * SR), Math.floor(c.t1 * SR));
  ok('click silenced', (() => { let p = 0; for (let i = Math.floor(c.t0 * SR); i < Math.floor(c.t1 * SR); ++i) p = Math.max(p, Math.abs(a.l[i])); return p === 0; })());
  a = edit(a, 'trimSilence', 0, a.frames, [-40]);
  a = edit(a, 'fadeOut', Math.max(0, a.frames - Math.floor(.5 * SR)), a.frames, [1]);
  a = edit(a, 'normalize', 0, a.frames, [-1, 0]);
  ok('cleanup is shorter', a.frames < before.frames * .5, (a.frames / SR).toFixed(1) + ' s from ' + (before.frames / SR).toFixed(1) + ' s');
  pair('cleanup', 'Cut and fade', before, a, 'A take with a click at the start, a pause, a short tone phrase and a long silence after it.', 'The click silenced, the silence trimmed away, a fade out and the level normalized.');
}

fs.writeFileSync(path.join(out, 'clips.json'), JSON.stringify(clips, null, 2) + '\n');
fs.rmSync(tmp, { recursive: true, force: true });
const bad = checks.filter(c => !c[1]).length; console.log(bad ? bad + ' check(s) failed' : 'all checks passed'); process.exit(bad ? 1 : 0);
