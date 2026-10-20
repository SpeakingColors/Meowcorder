(function () {
'use strict';
const $ = (s, r) => (r || document).querySelector(s), $$ = (s, r) => Array.from((r || document).querySelectorAll(s));
const store = { get(k) { try { return localStorage.getItem(k); } catch (e) { return null; } }, set(k, v) { try { localStorage.setItem(k, v); } catch (e) {} } };
const clamp = (v, a, b) => v < a ? a : (v > b ? b : v);
const OPS = window.MC_OPS, MC = window.MC;
OPS.forEach(o => o.params.forEach(d => { d.choices = d.choices || []; d.log = !d.choice && d.lo > 0 && d.hi / d.lo >= 40; }));
const AC = window.AudioContext || window.webkitAudioContext;
const gridCol = '#2a2d36', dim = '#8d92a0';
let ctx = null, bus = null, analyser = null, inBus = null, procNode = null, demoSrc = null, micStream = null, micNode = null, playSrc = null;
let tipOn = true, powerSave = false, showHits = true, showSpec = true;
const ed = new MC.Editor(OPS), rec = new MC.Recorder();
const state = { scale: 1, ver: 0, viewA: 0, viewB: 1, playing: false, playA: 0, playN: 0, playT0: 0, loop: false, curOp: 0, takeMode: 1, startMode: 2, recOn: false, input: 'demo', lvl: 0, lvlHold: 0, lvlT: 0, drag: null, fname: 'demo loop, not saved' };
const opP = OPS.map(o => o.params.map(d => d.def)), recP = { thresh: -40, pre: 200 };

// ---------- demo sound ----------
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
const SR = (AC ? (new AC()).sampleRate : 48000);
let demo = makeDemo(SR);
ed.load(demo.clone()); ed.audio.sr = SR; rec.prepare(SR, 30);

// ---------- formatting ----------
function fmt(d, v) {
  if (d.choice) return d.choices[clamp(Math.round(v), 0, d.choices.length - 1)];
  const u = d.unit, a = Math.abs(v);
  if (u === 'ms') return a >= 1000 ? (v / 1000).toFixed(2) + ' s' : (a >= 100 ? v.toFixed(0) : v.toFixed(1)) + ' ms';
  if (u === 's') return v.toFixed(2) + ' s'; if (u === 'Hz') return a >= 1000 ? (v / 1000).toFixed(2) + ' kHz' : (a >= 100 ? v.toFixed(0) : v.toFixed(1)) + ' Hz';
  if (u === 'st') return (v > 0 ? '+' : '') + v.toFixed(1) + ' st'; if (u === 'ct') return (v > 0 ? '+' : '') + v.toFixed(0) + ' ct'; if (u === 'dB') return (v > 0 ? '+' : '') + v.toFixed(1) + ' dB';
  if (u === '%') return v.toFixed(0) + '%'; if (u === 'x') return v.toFixed(2) + 'x'; if (u === ':1') return v.toFixed(1) + ':1';
  if (d.id === 'bits' || d.id === 'stages') return String(Math.round(v)); return v.toFixed(2);
}
const toNorm = (d, v) => clamp(d.log ? (Math.log(v) - Math.log(d.lo)) / (Math.log(d.hi) - Math.log(d.lo)) : (v - d.lo) / (d.hi - d.lo), 0, 1);
const fromNorm = (d, n) => { n = clamp(n, 0, 1); let v = d.log ? Math.exp(Math.log(d.lo) + n * (Math.log(d.hi) - Math.log(d.lo))) : d.lo + n * (d.hi - d.lo); if (d.id === 'bits' || d.id === 'stages') v = Math.round(v); return clamp(v, d.lo, d.hi); };
function parseVal(d, s) { s = String(s).trim().toLowerCase(); const m = s.match(/^(-?[\d.]+)\s*(k?hz|s|ms|%|db|st|ct|x)?$/); if (!m) return null; let v = parseFloat(m[1]); if (isNaN(v)) return null; const u = m[2] || ''; if (u === 'khz') v *= 1000; else if (u === 's' && d.unit === 'ms') v *= 1000; return clamp(v, d.lo, d.hi); }
const fmtTime = (fr, sr) => { const t = fr / sr, m = Math.floor(t / 60), s = t - m * 60; return (m ? m + ':' + (s < 10 ? '0' : '') : '') + s.toFixed(m ? 2 : 3) + (m ? '' : ' s'); };

// ---------- knobs ----------
const colOf = { Level: '--green', Edit: '--blue', Stereo: '--violet', Time: '--blue', Colour: '--orange', Tone: '--yellow', Space: '--pink', Dynamics: '--green', Record: '--pink' };
function arcPath(cx, cy, r, a0, a1) { const p = a => [cx + r * Math.cos(a), cy + r * Math.sin(a)], s = p(a0), e = p(a1); return `M${s[0].toFixed(2)} ${s[1].toFixed(2)}A${r} ${r} 0 ${Math.abs(a1 - a0) > Math.PI ? 1 : 0} 1 ${e[0].toFixed(2)} ${e[1].toFixed(2)}`; }
function makeKnob(d, group, get, set) {
  const w = document.createElement('div'); w.className = 'knobwrap'; const col = `var(${colOf[group] || '--pink'})`;
  w.innerHTML = `<label>${d.name}</label><div class="knob" tabindex="0" role="slider" aria-label="${d.name}"><svg viewBox="0 0 50 50"><path class="trk" fill="none" stroke="#343741" stroke-width="4" stroke-linecap="round"/><path class="vl" fill="none" stroke="${col}" stroke-width="4" stroke-linecap="round"/><circle cx="25" cy="25" r="14" fill="#23252c" stroke="#3c3f4a"/><line class="ptr" x1="25" y1="25" x2="25" y2="13" stroke="#f4f5f7" stroke-width="2.4" stroke-linecap="round"/></svg></div><button class="val"></button>`;
  const knob = $('.knob', w), A0 = Math.PI * .75, A1 = Math.PI * 2.25; knob.dataset.tip = '@' + d.tip + '|' + (d.choice ? d.choices.join(' / ') : fmt(d, d.lo) + ' to ' + fmt(d, d.hi) + ', default ' + fmt(d, d.def)) + '|' + d.name;
  $('.trk', w).setAttribute('d', arcPath(25, 25, 21, A0, A1)); const bip = d.lo < 0 && d.hi > 0, zero = bip ? toNorm(d, 0) : 0;
  const draw = () => { const n = toNorm(d, get()), a = A0 + n * (A1 - A0), from = bip ? A0 + zero * (A1 - A0) : A0, lo = Math.min(from, a), hi = Math.max(from, a);
    $('.vl', w).setAttribute('d', hi - lo < .01 ? '' : arcPath(25, 25, 21, lo, hi)); $('.ptr', w).setAttribute('transform', `rotate(${(a - A0) / Math.PI * 180 - 135} 25 25)`); $('.val', w).textContent = fmt(d, get()); knob.setAttribute('aria-valuetext', fmt(d, get())); };
  const put = v => { v = clamp(v, d.lo, d.hi); set(v); draw(); }; draw(); w._draw = draw;
  let dragY = 0, dragN = 0, active = false;
  knob.addEventListener('pointerdown', e => { active = true; dragY = e.clientY; dragN = toNorm(d, get()); knob.setPointerCapture(e.pointerId); e.preventDefault(); hideTip(); });
  knob.addEventListener('pointermove', e => { if (active) put(fromNorm(d, dragN + (dragY - e.clientY) / (state.scale || 1) / (e.shiftKey ? 900 : 190))); });
  knob.addEventListener('pointerup', () => { active = false; }); knob.addEventListener('pointercancel', () => { active = false; });
  knob.addEventListener('dblclick', () => put(d.def));
  knob.addEventListener('wheel', e => { e.preventDefault(); put(fromNorm(d, toNorm(d, get()) + (e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? .002 : .01))); }, { passive: false });
  knob.addEventListener('keydown', e => { let dn = 0; if (e.key === 'ArrowUp' || e.key === 'ArrowRight') dn = .01; else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') dn = -.01; if (dn) { e.preventDefault(); put(fromNorm(d, toNorm(d, get()) + dn * (e.shiftKey ? .2 : 1))); } });
  $('.val', w).addEventListener('click', () => { const b = $('.val', w), inp = document.createElement('input'); inp.className = 'valin'; inp.value = fmt(d, get()); b.replaceWith(inp); inp.focus(); inp.select();
    const done = ok => { if (ok) { const v = parseVal(d, inp.value); if (v !== null) put(v); } inp.replaceWith(b); draw(); }; inp.addEventListener('keydown', e => { if (e.key === 'Enter') done(true); else if (e.key === 'Escape') done(false); }); inp.addEventListener('blur', () => done(true)); });
  return w;
}
function makeChoice(d, get, set) {
  const w = document.createElement('div'); w.className = 'choice'; w.innerHTML = `<label>${d.name}</label><div class="seg">${d.choices.map((l, i) => `<button data-v="${i}">${l}</button>`).join('')}</div>`;
  $('.seg', w).dataset.tip = '@' + d.tip + '|' + d.choices.join(' / ') + '|' + d.name;
  const draw = () => $$('button', w).forEach(b => b.classList.toggle('on', +b.dataset.v === Math.round(get()))); $$('button', w).forEach(b => b.addEventListener('click', () => { set(+b.dataset.v); draw(); })); draw(); return w;
}

// ---------- tips ----------
let tipT = 0; const tip = $('#tip');
function hideTip() { clearTimeout(tipT); tip.hidden = true; }
function showTip(el) {
  const t = el.dataset.tip; if (t[0] === '@') { const [txt, rng, name] = t.slice(1).split('|'); tip.innerHTML = `<b>${name}</b>${txt}<i>${rng}</i>`; } else tip.textContent = t;
  tip.hidden = false; const r = el.getBoundingClientRect(), tw = tip.offsetWidth, th = tip.offsetHeight;
  tip.style.left = clamp(r.left + r.width / 2 - tw / 2, 6, innerWidth - tw - 6) + 'px'; tip.style.top = (r.bottom + th + 12 < innerHeight ? r.bottom + 8 : r.top - th - 8) + 'px';
}
document.addEventListener('pointerover', e => { const el = e.target.closest && e.target.closest('[data-tip]'); if (!el || !tipOn) return; clearTimeout(tipT); tipT = setTimeout(() => showTip(el), 1000); });
document.addEventListener('pointerout', e => { const el = e.target.closest && e.target.closest('[data-tip]'); if (el) hideTip(); });
document.addEventListener('pointerdown', hideTip);

// ---------- modal ----------
function modal(title, body, buttons, onOpen) {
  const bg = document.createElement('div'); bg.className = 'modalbg'; const m = document.createElement('div'); m.className = 'modal'; m.setAttribute('role', 'dialog'); m.setAttribute('aria-label', title); m.innerHTML = `<h2>${title}</h2><div class="mb">${body}</div><div class="btns"></div>`; bg.appendChild(m); document.body.appendChild(bg);
  const close = () => { bg.remove(); document.removeEventListener('keydown', esc); }, esc = e => { if (e.key === 'Escape') close(); }; document.addEventListener('keydown', esc); bg.addEventListener('pointerdown', e => { if (e.target === bg) close(); });
  const bt = $('.btns', m); buttons.forEach(([label, fn, cls]) => { const b = document.createElement('button'); b.textContent = label; if (cls) b.className = cls; b.addEventListener('click', () => { if (fn && fn(m) === false) return; close(); }); bt.appendChild(b); }); if (onOpen) onOpen(m); return { close, m };
}

// ---------- audio graph ----------
async function powerOn() {
  if (!AC) return; if (!ctx) {
    ctx = new AC(); bus = ctx.createGain(); analyser = ctx.createAnalyser(); analyser.fftSize = 1024; bus.connect(analyser); bus.connect(ctx.destination);
    inBus = ctx.createGain(); procNode = ctx.createScriptProcessor(4096, 2, 2); const zero = ctx.createGain(); zero.gain.value = 0; inBus.connect(procNode); procNode.connect(zero); zero.connect(ctx.destination);
    procNode.onaudioprocess = ev => { const ib = ev.inputBuffer, l = ib.getChannelData(0), r = ib.numberOfChannels > 1 ? ib.getChannelData(1) : l; rec.process(l, r, l.length); };
    if (ctx.sampleRate !== SR) { demo = makeDemo(ctx.sampleRate); ed.audio = demo.clone(); rec.prepare(ctx.sampleRate, 30); }
  }
  if (ctx.state !== 'running') await ctx.resume(); $('#overlay').hidden = true; $('#power').classList.add('on'); startInput();
}
function startInput() {
  if (!ctx) return; try { if (demoSrc) demoSrc.stop(); } catch (e) {} demoSrc = null; if (micNode) { try { micNode.disconnect(); } catch (e) {} micNode = null; }
  if (state.input === 'demo') {
    const b = ctx.createBuffer(2, demo.frames, demo.sr); b.copyToChannel(demo.l, 0); b.copyToChannel(demo.r, 1); demoSrc = ctx.createBufferSource(); demoSrc.buffer = b; demoSrc.loop = true; demoSrc.connect(inBus);
    const mon = ctx.createGain(); mon.gain.value = .6; demoSrc.connect(mon); mon.connect(bus); demoSrc.start();
  } else if (micStream) { micNode = ctx.createMediaStreamSource(micStream); micNode.connect(inBus); }
}
async function chooseInput(v) {
  state.input = v;
  if (v === 'mic' && !micStream) { try { micStream = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } }); } catch (e) { $('#recHint').textContent = 'Microphone not available here. Using the demo loop.'; state.input = 'demo'; $('#inSel').value = 'demo'; } }
  startInput();
}

// ---------- recording ----------
function armOrRecord() {
  if (!ctx || ctx.state !== 'running') { powerOn(); return; }
  if (rec.isRecording() || rec.isArmed()) { stopRecording(); return; }
  stopPlay(); rec.arm(state.startMode, recP.thresh, recP.pre); state.recOn = true; syncRec();
}
function stopRecording() {
  const a = rec.stop(); state.recOn = false; if (a.frames > 0) { a.sr = ed.audio.sr || a.sr; ed.takeRecording(a, state.takeMode); state.fname = 'new recording, not saved'; viewFit(); changed(); }
  syncRec();
}
function syncRec() { const b = $('#bRec'), on = rec.isRecording(); b.classList.toggle('on', on); b.innerHTML = on ? '&#9632; Stop' : (rec.isArmed() ? '&#9632; Armed, waiting' : '&#9679; Record'); $('#recHint').textContent = rec.isArmed() ? 'Waiting for the sound to pass ' + recP.thresh.toFixed(0) + ' dB.' : (on ? 'Recording ' + fmtTime(rec.recordedFrames(), rec.sr) : ''); }

// ---------- playback ----------
function playRange() { const a = ed.hasSelection() ? ed.selA : Math.min(ed.cursor, Math.max(0, ed.audio.frames - 1)), b = ed.hasSelection() ? ed.selB : ed.audio.frames; return [a, b]; }
function playAudio(au, from, loop) {
  stopPlay(); if (!ctx || ctx.state !== 'running') { powerOn().then(() => playAudio(au, from, loop)); return; } if (!au.frames) return;
  const b = ctx.createBuffer(2, au.frames, au.sr); b.copyToChannel(au.l, 0); b.copyToChannel(au.r, 1); playSrc = ctx.createBufferSource(); playSrc.buffer = b; playSrc.loop = loop; playSrc.connect(bus);
  playSrc.onended = () => { if (playSrc === s) { state.playing = false; syncPlay(); } }; const s = playSrc; playSrc.start(); state.playing = true; state.playA = from; state.playN = au.frames; state.playT0 = ctx.currentTime; state.playSr = au.sr; syncPlay();
}
function togglePlay() { if (state.playing) { stopPlay(); return; } const [a, b] = playRange(); playAudio(ed.audio.slice(a, b), a, state.loop); }
function stopPlay() { if (playSrc) { try { playSrc.onended = null; playSrc.stop(); } catch (e) {} playSrc = null; } state.playing = false; syncPlay(); }
function syncPlay() { $('#bPlay').classList.toggle('on', state.playing); $('#bPlay').innerHTML = state.playing ? '&#9632; Stop' : '&#9654; Play'; }
const playheadFrame = () => { if (!state.playing || !ctx) return -1; const el = (ctx.currentTime - state.playT0) * state.playSr; return state.playA + (state.loop ? el % state.playN : Math.min(el, state.playN)); };

// ---------- view ----------
function viewFit() { state.viewA = 0; state.viewB = Math.max(1, ed.audio.frames); }
function clampView() { const n = Math.max(1, ed.audio.frames), len = clamp(state.viewB - state.viewA, Math.min(64, n), n); state.viewA = clamp(state.viewA, 0, n - len); state.viewB = state.viewA + len; }
function zoomAt(f, anchor) { const len = state.viewB - state.viewA, nl = clamp(len * f, Math.min(64, ed.audio.frames), Math.max(1, ed.audio.frames)); state.viewA = anchor - (anchor - state.viewA) / len * nl; state.viewB = state.viewA + nl; clampView(); state.ver++; }
const xToFrame = (x, w) => Math.round(state.viewA + x / w * (state.viewB - state.viewA)), frameToX = (f, w) => (f - state.viewA) / (state.viewB - state.viewA) * w;

// ---------- drawing ----------
const dprOf = c => { const d = Math.min(2, window.devicePixelRatio || 1), w = c.clientWidth || 100, h = c.clientHeight || parseFloat(c.style.height) || 100, W = Math.round(w * d), H = Math.round(h * d); if (c.width !== W || c.height !== H) { c.width = W; c.height = H; } return d; };
function prep(c) { const d = dprOf(c), g = c.getContext('2d'); g.setTransform(d, 0, 0, d, 0, 0); return { g, w: c.width / d, h: c.height / d }; }
function fmtTicks(len, sr, w) { const secs = len / sr, steps = [.001, .002, .005, .01, .02, .05, .1, .2, .5, 1, 2, 5, 10, 30, 60, 120, 300]; const want = secs / (w / 90); return steps.find(s => s >= want) || 300; }
function drawWave() {
  const c = $('#wave'), { g, w, h } = prep(c), a = ed.audio, bh = h - 18, mid = bh / 2; g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h);
  g.strokeStyle = gridCol; g.lineWidth = 1; g.beginPath(); g.moveTo(0, mid + .5); g.lineTo(w, mid + .5); g.stroke();
  if (!a.frames) { g.fillStyle = dim; g.font = '14px Barlow Condensed, sans-serif'; g.textAlign = 'center'; g.fillText('Nothing here yet. Record, open a file or drop one here.', w / 2, mid); return; }
  const pk = MC.peakOverview(a, Math.floor(state.viewA), Math.ceil(state.viewB), Math.floor(w));
  g.fillStyle = '#02EBBA'; g.globalAlpha = .9; for (let x = 0; x < pk.mx.length; ++x) { const y0 = mid - pk.mx[x] * mid * .96, y1 = mid - pk.mn[x] * mid * .96; g.fillRect(x, y0, 1, Math.max(1, y1 - y0)); } g.globalAlpha = 1;
  if (showHits) { const hits = state.hits || []; g.fillStyle = '#FF9000'; for (const f of hits) { if (f < state.viewA || f > state.viewB) continue; g.fillRect(Math.round(frameToX(f, w)), 0, 1, 12); } }
  if (ed.hasSelection()) { const x0 = frameToX(ed.selA, w), x1 = frameToX(ed.selB, w); g.fillStyle = '#6cc9f533'; g.fillRect(x0, 0, x1 - x0, bh); g.fillStyle = '#6cc9f5'; g.fillRect(x0, 0, 1, bh); g.fillRect(x1 - 1, 0, 1, bh); }
  else { const x = frameToX(ed.cursor, w); g.fillStyle = '#f4f5f7'; g.fillRect(Math.round(x), 0, 1, bh); }
  const ph = playheadFrame(); if (ph >= 0) { g.fillStyle = '#FF1383'; g.fillRect(Math.round(frameToX(ph, w)), 0, 2, bh); }
  g.fillStyle = '#101216'; g.fillRect(0, bh, w, 18); g.fillStyle = dim; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left';
  const step = fmtTicks(state.viewB - state.viewA, a.sr, w), t0 = Math.ceil(state.viewA / a.sr / step) * step; for (let t = t0; t * a.sr < state.viewB; t += step) { const x = Math.round(frameToX(t * a.sr, w)); g.fillRect(x, bh, 1, 5); g.fillText(step < 1 ? t.toFixed(step < .01 ? 3 : 2) + 's' : (t >= 60 ? Math.floor(t / 60) + ':' + ('0' + Math.round(t % 60)).slice(-2) : t + 's'), x + 3, bh + 13); }
}
function palette(db) { const t = clamp((db + 90) / 90, 0, 1), st = [[0, '#0a0a0c'], [.25, '#25164a'], [.5, '#8a1fa3'], [.72, '#ff1383'], [.88, '#ff9000'], [1, '#fff3b0']]; let i = 0; while (i < st.length - 2 && t > st[i + 1][0]) ++i; const a = st[i], b = st[i + 1], f = (t - a[0]) / (b[0] - a[0]); const p = s => [parseInt(s.slice(1, 3), 16), parseInt(s.slice(3, 5), 16), parseInt(s.slice(5, 7), 16)], A = p(a[1]), B = p(b[1]); return A.map((v, k) => Math.round(v + (B[k] - v) * clamp(f, 0, 1))); }
const PAL = Array.from({ length: 256 }, (_, i) => palette(-90 + i / 255 * 90));
let specKey = '', specImg = null;
function drawSpec() {
  const c = $('#spec'), { g, w, h } = prep(c), a = ed.audio; g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); if (!a.frames || !showSpec) return;
  const key = state.ver + '|' + Math.round(state.viewA) + '|' + Math.round(state.viewB) + '|' + Math.round(w) + '|' + Math.round(h);
  if (key !== specKey) {
    specKey = key; const W = Math.floor(w), H = Math.floor(h), from = Math.floor(state.viewA), to = Math.ceil(state.viewB), N = 1024, hop = Math.max(64, Math.floor((to - from) / W)), sp = MC.spectrogram(a, from, to, N, hop);
    specImg = g.createImageData(W, H); const fMax = a.sr / 2, row = new Int32Array(H); for (let y = 0; y < H; ++y) { const f = 20 * Math.pow(fMax / 20, 1 - y / (H - 1)); row[y] = clamp(Math.round(f / fMax * (sp.bins - 1)), 0, sp.bins - 1); }
    for (let x = 0; x < W; ++x) { const fr = clamp(Math.floor(x / W * sp.frames), 0, sp.frames - 1); for (let y = 0; y < H; ++y) { const db = sp.db[fr * sp.bins + row[y]], p = PAL[clamp(Math.round((db + 90) / 90 * 255), 0, 255)], o = (y * W + x) * 4; specImg.data[o] = p[0]; specImg.data[o + 1] = p[1]; specImg.data[o + 2] = p[2]; specImg.data[o + 3] = 255; } }
  }
  const d = Math.min(2, window.devicePixelRatio || 1); g.setTransform(1, 0, 0, 1, 0, 0); const tmp = document.createElement('canvas'); tmp.width = specImg.width; tmp.height = specImg.height; tmp.getContext('2d').putImageData(specImg, 0, 0); g.drawImage(tmp, 0, 0, c.width, c.height); g.setTransform(d, 0, 0, d, 0, 0);
  g.fillStyle = dim; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; const fMax = a.sr / 2; for (const f of [100, 1000, 10000]) { if (f > fMax) continue; const y = (1 - Math.log(f / 20) / Math.log(fMax / 20)) * (h - 1); g.fillStyle = '#ffffff22'; g.fillRect(0, Math.round(y), w, 1); g.fillStyle = '#c8ccd8'; g.fillText(f >= 1000 ? f / 1000 + 'k' : f, 3, y - 2); }
  if (ed.hasSelection()) { const x0 = frameToX(ed.selA, w), x1 = frameToX(ed.selB, w); g.fillStyle = '#6cc9f522'; g.fillRect(x0, 0, x1 - x0, h); }
  const ph = playheadFrame(); if (ph >= 0) { g.fillStyle = '#FF1383'; g.fillRect(Math.round(frameToX(ph, w)), 0, 2, h); }
}
function drawLive() {
  const c = $('#live'), { g, w, h } = prep(c); g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); if (!ctx || !rec.ring) { g.fillStyle = dim; g.font = '12px Barlow Condensed, sans-serif'; g.fillText('Input appears here after Start audio', 8, h / 2 + 4); return; }
  const live = rec.captureLast(3), secs = 3, cols = Math.floor(w); g.strokeStyle = gridCol; g.beginPath(); g.moveTo(0, h / 2 + .5); g.lineTo(w, h / 2 + .5); g.stroke();
  const pre = clamp(recP.pre / 1000 / secs, 0, 1) * w; g.fillStyle = '#ff138322'; g.fillRect(w - pre, 0, pre, h);
  if (live.frames > 0) { const total = secs * rec.sr, off = total - live.frames, pk = MC.peakOverview(live, 0, live.frames, Math.max(1, Math.floor(cols * live.frames / total))); g.fillStyle = rec.isRecording() ? '#FF1383' : '#6CC9F5'; const x0 = off / total * w; for (let x = 0; x < pk.mx.length; ++x) { const y0 = h / 2 - pk.mx[x] * h / 2 * .95, y1 = h / 2 - pk.mn[x] * h / 2 * .95; g.fillRect(x0 + x, y0, 1, Math.max(1, y1 - y0)); } }
  if (state.startMode === 2) { const y = h / 2 - Math.pow(10, recP.thresh / 20) * h / 2; g.fillStyle = '#FFD100aa'; g.fillRect(0, y, w, 1); g.fillRect(0, h - y, w, 1); }
}
function dbY(db, h) { return 3 + (1 - (clamp(db, -60, 0) + 60) / 60) * (h - 20); }
function drawMeter() {
  const c = $('#meter'), { g, w, h } = prep(c); g.clearRect(0, 0, w, h); let pk = 0; if (analyser) { const buf = new Float32Array(analyser.fftSize); analyser.getFloatTimeDomainData(buf); for (const v of buf) pk = Math.max(pk, Math.abs(v)); }
  if (rec.ring && (rec.isRecording() || rec.isArmed() || state.input === 'mic')) { const cl = rec.captureLast(.05); pk = Math.max(pk, cl.peak ? 0 : 0); let m = 0; for (let i = 0; i < cl.frames; ++i) m = Math.max(m, Math.abs(cl.l[i]), Math.abs(cl.r[i])); pk = Math.max(pk, m); }
  state.lvl = Math.max(pk, state.lvl * .9); const now = performance.now(); if (pk > state.lvlHold || now > state.lvlT) { state.lvlHold = pk; state.lvlT = now + 1500; }
  const bw = 20, x1 = 44; g.fillStyle = '#0a0a0c'; g.fillRect(x1, 2, bw, h - 17); const l2d = v => 20 * Math.log10(Math.max(1e-6, v)); g.font = '10px "JetBrains Mono", monospace'; g.textAlign = 'right'; g.fillStyle = dim;
  for (const db of [0, -6, -12, -24, -36, -48, -60]) { const y = dbY(db, h); g.fillText(String(db), 38, y + 3); g.strokeStyle = gridCol; g.beginPath(); g.moveTo(40, y + .5); g.lineTo(x1 + bw + 3, y + .5); g.stroke(); }
  const yP = dbY(l2d(state.lvl), h), grad = g.createLinearGradient(0, dbY(0, h), 0, dbY(-60, h)); grad.addColorStop(0, '#ff1383'); grad.addColorStop(.06, '#ff1383'); grad.addColorStop(.09, '#b43dff'); grad.addColorStop(.26, '#a43dff'); grad.addColorStop(.30, '#1ea7ff'); grad.addColorStop(1, '#00d4ff');
  g.fillStyle = grad; g.fillRect(x1 + 3, yP, bw - 6, h - 15 - yP); g.fillStyle = state.lvlHold >= .98 ? '#ff1383' : '#fff'; g.fillRect(x1, dbY(l2d(state.lvlHold), h), bw, 2);
  if (state.startMode === 2) { const y = dbY(recP.thresh, h); g.fillStyle = '#FFD100'; g.fillRect(x1 - 4, y, bw + 8, 2); }
  g.textAlign = 'center'; g.fillStyle = dim; g.fillText('dBFS', 40, h - 2);
}
function drawReadout() {
  const a = ed.audio, sel = ed.hasSelection(); $('#readout').innerHTML = `<span>Length <b>${fmtTime(a.frames, a.sr || 48000)}</b></span><span>Selection <b>${sel ? fmtTime(ed.selA, a.sr) + ' to ' + fmtTime(ed.selB, a.sr) + ' (' + fmtTime(ed.selB - ed.selA, a.sr) + ')' : 'none, operations use everything'}</b></span><span>Cursor <b>${fmtTime(ed.cursor, a.sr || 48000)}</b></span><span>Peak <b>${a.frames ? MC.linToDb(a.peak(sel ? ed.selA : 0, sel ? ed.selB : a.frames)).toFixed(1) + ' dBFS' : '-'}</b></span><span><b>${(a.sr / 1000).toFixed(1)} kHz, stereo</b></span><span>${rec.isRecording() ? '<b style="color:var(--pink)">REC ' + fmtTime(rec.recordedFrames(), rec.sr) + '</b>' : ''}</span>`;
}

// ---------- editing UI ----------
function changed() { state.ver++; state.hits = showHits ? MC.detectTransients(ed.audio, .6) : []; clampView(); syncEdit(); }
function syncEdit() {
  const sel = ed.hasSelection(); $('#bUndo').disabled = !ed.canUndo(); $('#bRedo').disabled = !ed.canRedo(); $('#bCut').disabled = !sel; $('#bCopy').disabled = !sel; $('#bDel').disabled = !sel; $('#bCrop').disabled = !sel; $('#bSnap').disabled = !sel; $('#bSel').disabled = !sel;
  $('#bPaste').disabled = !ed.clip.frames; $('#bRepeat').disabled = !ed.last; $('#bRepeat').textContent = 'Repeat last: ' + (ed.last ? OPS[ed.last.op].name : 'none'); $('#fname').textContent = state.fname;
  $('#opHint').textContent = sel ? 'on the selection' : 'on everything (nothing selected)';
}
function doOp() { stopPlay(); if (!ed.apply(state.curOp, opP[state.curOp])) return; state.fname = state.fname.replace(/, not saved$/, '') + ', edited, not saved'; changed(); }
function previewOp() {
  const [a, b] = ed.hasSelection() ? [ed.selA, ed.selB] : [0, ed.audio.frames]; const c = ed.audio.slice(a, b); MC.applyOp(c, state.curOp, 0, c.frames, opP[state.curOp]); playAudio(c, a, false);
}
function buildOps() {
  const host = $('#oplist'), groups = [], by = {}; OPS.forEach((o, i) => { if (!by[o.group]) { by[o.group] = []; groups.push(o.group); } by[o.group].push(i); });
  groups.forEach(gn => { const d = document.createElement('div'); d.className = 'opgrp'; d.innerHTML = `<h4>${gn}</h4>`; by[gn].forEach(i => { const b = document.createElement('button'); b.textContent = OPS[i].name; b.dataset.op = i; b.dataset.tip = OPS[i].tip; b.addEventListener('click', () => selectOp(i)); b.addEventListener('dblclick', () => { selectOp(i); doOp(); }); d.appendChild(b); }); host.appendChild(d); });
}
function selectOp(i) {
  state.curOp = i; $$('#oplist button').forEach(b => b.classList.toggle('on', +b.dataset.op === i)); const o = OPS[i]; $('#opName').textContent = o.name; $('#opTip').textContent = o.tip + (o.changesLength ? ' This one can change the length.' : ''); const host = $('#opK'); host.innerHTML = '';
  o.params.forEach((d, k) => host.appendChild(d.choice ? makeChoice(d, () => opP[i][k], v => { opP[i][k] = v; }) : makeKnob(d, o.group, () => opP[i][k], v => { opP[i][k] = v; })));
  if (!o.params.length) host.innerHTML = '<div class="hint">No settings.</div>';
}
function buildRec() {
  const h = $('#recK'); h.appendChild(makeKnob({ id: 'thresh', name: 'Threshold', lo: -80, hi: 0, def: -40, unit: 'dB', tip: 'Level that starts a recording when Start is on Threshold.', choice: false, choices: [] }, 'Record', () => recP.thresh, v => { recP.thresh = v; }));
  h.appendChild(makeKnob({ id: 'pre', name: 'Pre-roll', lo: 0, hi: 3000, def: 200, unit: 'ms', tip: 'How much sound from before the start is included in the recording.', choice: false, choices: [] }, 'Record', () => recP.pre, v => { recP.pre = v; }));
}

// ---------- pointer on the waveform ----------
const wave = $('#wave');
const wx = e => { const r = wave.getBoundingClientRect(); return (e.clientX - r.left) / (r.width || 1) * (wave.clientWidth || 1000); };
wave.addEventListener('pointerdown', e => { if (!ed.audio.frames) return; wave.setPointerCapture(e.pointerId); const w = wave.clientWidth, f = clamp(xToFrame(wx(e), w), 0, ed.audio.frames); state.drag = { f0: e.shiftKey && ed.hasSelection() ? (Math.abs(f - ed.selA) < Math.abs(f - ed.selB) ? ed.selB : ed.selA) : f, moved: false, x0: wx(e) }; if (e.shiftKey && ed.hasSelection()) ed.setSelection(state.drag.f0, f); hideTip(); });
wave.addEventListener('pointermove', e => { const d = state.drag; if (!d) return; const w = wave.clientWidth, f = clamp(xToFrame(wx(e), w), 0, ed.audio.frames); if (Math.abs(wx(e) - d.x0) > 3) d.moved = true; if (d.moved) { ed.setSelection(d.f0, f); syncEdit(); } });
wave.addEventListener('pointerup', e => { const d = state.drag; state.drag = null; if (!d) return; if (!d.moved && !e.shiftKey) { ed.setCursor(clamp(xToFrame(wx(e), wave.clientWidth), 0, ed.audio.frames)); } syncEdit(); });
wave.addEventListener('wheel', e => { e.preventDefault(); if (e.altKey || e.shiftKey) { const len = state.viewB - state.viewA, d = (e.deltaY + e.deltaX) / 400 * len; state.viewA += d; state.viewB += d; clampView(); state.ver++; } else { const w = wave.clientWidth; zoomAt(e.deltaY < 0 ? .8 : 1.25, xToFrame(wx(e), w)); } }, { passive: false });
wave.addEventListener('dblclick', () => { ed.setSelection(0, ed.audio.frames); syncEdit(); });

// ---------- files ----------
async function openFile(f) {
  if (!f) return; const buf = new Uint8Array(await f.arrayBuffer()); let a = MC.decodeWav(buf);
  if (!a) { try { const c = ctx || new AC(); const d = await c.decodeAudioData(buf.buffer.slice(0)); a = new MC.Audio(d.sampleRate, d.length); d.copyFromChannel(a.l, 0); a.r = d.numberOfChannels > 1 ? (d.copyFromChannel(a.r, 1), a.r) : a.l.slice(); } catch (e) { modal('Open', 'This browser cannot read that file. Try a WAV file.', [['OK', null, 'on']]); return; } }
  stopPlay(); ed.pushUndo(); const keep = ed.undoStack.slice(); ed.load(a); ed.undoStack = keep; state.fname = f.name; viewFit(); changed();
}
$('#bOpen').addEventListener('click', () => $('#file').click()); $('#file').addEventListener('change', e => { openFile(e.target.files[0]); e.target.value = ''; });
const dw = $('#dispwrap'); ['dragenter', 'dragover'].forEach(t => dw.addEventListener(t, e => { e.preventDefault(); dw.classList.add('hot'); })); ['dragleave', 'drop'].forEach(t => dw.addEventListener(t, e => { e.preventDefault(); dw.classList.remove('hot'); }));
dw.addEventListener('drop', e => openFile(e.dataTransfer.files[0]));
$('#bSave').addEventListener('click', () => { const bytes = MC.encodeWav(ed.audio, +$('#bits').value), url = URL.createObjectURL(new Blob([bytes], { type: 'audio/wav' })), a = document.createElement('a'); a.href = url; a.download = (state.fname.replace(/[^\w.-]+/g, '_').replace(/_?(not_saved|edited)/g, '') || 'meowcorder') + '.wav'; document.body.appendChild(a); a.click(); a.remove(); setTimeout(() => URL.revokeObjectURL(url), 2000); state.fname = state.fname.replace(/, not saved$/, ''); syncEdit(); });
$('#bNew').addEventListener('click', () => { stopPlay(); ed.pushUndo(); const keep = ed.undoStack.slice(), a = new MC.Audio(ed.audio.sr || 48000); ed.load(a); ed.undoStack = keep; state.fname = 'empty, not saved'; viewFit(); changed(); });

// ---------- buttons ----------
$('#bStart').addEventListener('click', powerOn); $('#power').addEventListener('click', () => { if (ctx && ctx.state === 'running') { ctx.suspend(); $('#power').classList.remove('on'); $('#overlay').hidden = false; } else powerOn(); });
$('#inSel').addEventListener('change', e => chooseInput(e.target.value)); $('#bRec').addEventListener('click', armOrRecord);
$$('#startSeg button').forEach(b => b.addEventListener('click', () => { state.startMode = +b.dataset.v; syncSegs(); })); $$('#takeSeg button').forEach(b => b.addEventListener('click', () => { state.takeMode = +b.dataset.v; syncSegs(); }));
function syncSegs() { $$('#startSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === state.startMode)); $$('#takeSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === state.takeMode)); }
$('#bGrab').addEventListener('click', () => { if (!rec.ring) { powerOn(); return; } const a = rec.captureLast(Math.max(.2, recP.pre / 1000 * 1 + 0) || 3); if (a.frames) { ed.takeRecording(a, state.takeMode); state.fname = 'grabbed, not saved'; viewFit(); changed(); } });
$('#bPlay').addEventListener('click', togglePlay); $('#bLoop').addEventListener('click', () => { state.loop = !state.loop; $('#bLoop').classList.toggle('on', state.loop); if (state.playing) { const [a, b] = playRange(); playAudio(ed.audio.slice(a, b), a, state.loop); } });
const act = (id, fn) => $(id).addEventListener('click', () => { stopPlay(); fn(); changed(); });
act('#bUndo', () => ed.undo()); act('#bRedo', () => ed.redo()); act('#bCut', () => ed.cut()); act('#bCopy', () => ed.copy()); act('#bPaste', () => ed.paste()); act('#bDel', () => ed.deleteSelection()); act('#bCrop', () => { ed.cropToSelection(); viewFit(); });
act('#bAll', () => ed.setSelection(0, ed.audio.frames)); act('#bSnap', () => { if (!ed.hasSelection()) return; const a = ed.snapToZero(ed.selA, 2000), b = ed.snapToZero(ed.selB, 2000); ed.setSelection(a, b); });
act('#bRepeat', () => ed.repeatLast());
$('#bZin').addEventListener('click', () => zoomAt(.5, (state.viewA + state.viewB) / 2)); $('#bZout').addEventListener('click', () => zoomAt(2, (state.viewA + state.viewB) / 2)); $('#bFit').addEventListener('click', () => { viewFit(); state.ver++; });
$('#bSel').addEventListener('click', () => { if (ed.hasSelection()) { state.viewA = ed.selA; state.viewB = ed.selB; clampView(); state.ver++; } });
$('#bApply').addEventListener('click', doOp); $('#bPrev').addEventListener('click', previewOp);
document.addEventListener('keydown', e => {
  if (e.target.matches('input, select, textarea') || document.querySelector('.modalbg')) return; const k = e.key.toLowerCase(), c = e.ctrlKey || e.metaKey;
  if (k === ' ') { e.preventDefault(); togglePlay(); } else if (k === 'enter') { e.preventDefault(); armOrRecord(); } else if (k === 'r' && !c) { stopPlay(); if (ed.repeatLast()) changed(); }
  else if (c && k === 'z') { e.preventDefault(); stopPlay(); e.shiftKey ? ed.redo() : ed.undo(); changed(); } else if (c && k === 'y') { e.preventDefault(); ed.redo(); changed(); }
  else if (c && k === 'x') { e.preventDefault(); ed.cut(); changed(); } else if (c && k === 'c') { e.preventDefault(); ed.copy(); syncEdit(); } else if (c && k === 'v') { e.preventDefault(); ed.paste(); changed(); }
  else if (c && k === 'a') { e.preventDefault(); ed.setSelection(0, ed.audio.frames); syncEdit(); } else if (k === 'delete' || k === 'backspace') { e.preventDefault(); ed.deleteSelection(); changed(); }
});

// ---------- dialogs ----------
$('#bGuide').addEventListener('click', () => modal('Guide', `<ol><li>Click <b>Start audio</b>. The little graph above the waveform shows the input all the time: Meowcorder is always listening.</li><li><b>Record</b> (or Enter). With Start on <b>Threshold</b> it waits for sound. The <b>Pre-roll</b> adds the sound from just before the start. <b>Grab the last seconds</b> keeps what you already heard.</li><li>Choose what a new take does: <b>Replace</b>, <b>Append</b>, <b>Punch</b> over the selection or <b>Insert</b>.</li><li>Drag on the waveform to select. Click to place the cursor. Wheel zooms, Alt wheel scrolls, double click selects all.</li><li>Pick an operation in <b>Process</b>, set it, <b>Preview</b> it, then <b>Apply</b> (or double click the operation). <b>R</b> repeats the last one on a new selection.</li><li><b>Undo</b> (Ctrl Z) always brings the old sound back.</li></ol><div class="hint">Space plays, Enter records, R repeats, Delete removes the selection.</div>`, [['Close', null, 'on']]));
$('#bSet').addEventListener('click', () => modal('Settings', `<label class="row"><input type="checkbox" id="sTip" ${tipOn ? 'checked' : ''}> Info bubbles (after 1 second)</label><label class="row"><input type="checkbox" id="sPow" ${powerSave ? 'checked' : ''}> Power saving (slower drawing)</label><label class="row"><input type="checkbox" id="sSpec" ${showSpec ? 'checked' : ''}> Show the spectrogram</label><label class="row"><input type="checkbox" id="sHit" ${showHits ? 'checked' : ''}> Mark hits (orange ticks) on the waveform</label>`, [['Reset window size', () => { setZoom(1); return false; }], ['Close', m => { tipOn = $('#sTip', m).checked; powerSave = $('#sPow', m).checked; showSpec = $('#sSpec', m).checked; showHits = $('#sHit', m).checked; store.set('mc.set', JSON.stringify({ tipOn, powerSave, showSpec, showHits })); state.ver++; changed(); }, 'on']]));
$('#bAbout').addEventListener('click', () => modal('About & Support', `<p><b>Meowcorder</b> by Studio 87, version 0.1 design mockup.</p><p>Free and open source (AGPL-3.0-or-later). It collects nothing and makes no network calls. The microphone is only used after you choose it and press Record. The button below only opens a link when you click it.</p><p>Nothing leaves this page: recordings stay in memory until you save a file.</p><p><a class="btn" href="https://ko-fi.com/studio87" target="_blank" rel="noopener">Support on Ko-fi</a></p>`, [['Close', null, 'on']]));

// ---------- resize ----------
let zoom = 1, base = 1; const ZSTEPS = [.75, 1, 1.25, 1.5, 2], DW = 1240; let DH = 800;
function layoutScale() { const u = $('#unit'); DH = u.offsetHeight; const avail = $('.viewport').clientWidth - 14; base = Math.min(1, avail / DW); const s = base * zoom; state.scale = s; u.style.transform = `scale(${s})`; const st = $('#stage'); st.style.width = DW * s + 14 + 'px'; st.style.height = DH * s + 14 + 'px'; $('#bZoom').textContent = Math.round(zoom * 100) + '%'; }
function setZoom(z) { zoom = clamp(z, .4, 3); layoutScale(); store.set('mc.zoom', String(zoom)); }
$('#bZoom').addEventListener('click', () => { const i = ZSTEPS.findIndex(z => z > zoom + .01); setZoom(i < 0 ? ZSTEPS[0] : ZSTEPS[i]); });
['#cheekL', '#cheekR'].forEach(s => $(s).addEventListener('dblclick', () => setZoom(1)));
$$('.rh').forEach(h => { let s0 = 0, x0 = 0, y0 = 0, z0 = 1; const kind = h.dataset.h;
  h.addEventListener('pointerdown', e => { h.setPointerCapture(e.pointerId); x0 = e.clientX; y0 = e.clientY; z0 = zoom; s0 = state.scale; e.preventDefault(); });
  h.addEventListener('pointermove', e => { if (!h.hasPointerCapture(e.pointerId)) return; const dx = e.clientX - x0, dy = e.clientY - y0, W0 = DW * s0, H0 = DH * s0; let fx = 1, fy = 1;
    if (kind.includes('e')) fx = (W0 + 2 * dx) / W0; if (kind.includes('w')) fx = (W0 - 2 * dx) / W0; if (kind.includes('s')) fy = (H0 + 2 * dy) / H0; if (kind.includes('n')) fy = (H0 - 2 * dy) / H0;
    const f = kind.length === 2 ? (Math.abs(fx - 1) > Math.abs(fy - 1) ? fx : fy) : (kind === 'e' || kind === 'w' ? fx : fy); setZoom(z0 * Math.max(.2, f)); }); });
addEventListener('resize', layoutScale);

// ---------- loop ----------
let visible = true, last = 0, n = 0; new IntersectionObserver(es => { visible = es[0].isIntersecting; }).observe($('#unit'));
function frame(t) {
  requestAnimationFrame(frame); if (document.hidden || !visible || t - last < (powerSave ? 50 : 16)) return; last = t; ++n;
  if (rec.ring && state.recOn && !rec.isRecording() && !rec.isArmed()) { state.recOn = false; syncRec(); }
  if (rec.isRecording() && rec.recordedFrames() / rec.sr > 1200) stopRecording(); if (rec.ring && (rec.isRecording() || rec.isArmed()) && n % 6 === 0) syncRec();
  if (state.recOn && rec.isRecording() === false && !rec.isArmed()) { /* stopped by the engine */ }
  drawLive(); drawWave(); if (n % 2 === 0 || state.playing) drawSpec(); drawMeter(); if (n % 4 === 0) drawReadout();
}

// ---------- start ----------
try { const s = JSON.parse(store.get('mc.set') || '{}'); tipOn = s.tipOn !== false; powerSave = !!s.powerSave; showSpec = s.showSpec !== false; showHits = s.showHits !== false; } catch (e) {}
buildOps(); buildRec(); selectOp(0); syncSegs(); viewFit(); changed(); syncPlay(); syncRec();
const z0 = parseFloat(store.get('mc.zoom')); zoom = z0 > 0 ? clamp(z0, .4, 3) : 1; layoutScale();
window.__mc = { ed, rec, state, opP, MC, OPS };
requestAnimationFrame(frame);
})();
