(function () {
'use strict';
const $ = (s, r) => (r || document).querySelector(s), $$ = (s, r) => Array.from((r || document).querySelectorAll(s));
const store = { get(k) { try { return localStorage.getItem(k); } catch (e) { return null; } }, set(k, v) { try { localStorage.setItem(k, v); } catch (e) {} } };
const clamp = (v, a, b) => v < a ? a : (v > b ? b : v);
const OPS = window.MC_OPS, MC = window.MC;
OPS.forEach(o => o.params.forEach(d => { d.choices = d.choices || []; d.log = !d.choice && d.lo > 0 && d.hi / d.lo >= 40; }));

// ---------- icons (20 px, stroke) ----------
const IC = {
  loop: '<path d="M4 8V7a3 3 0 0 1 3-3h7l-2-2M16 12v1a3 3 0 0 1-3 3H6l2 2"/>', play: '<path d="M6 4l10 6-10 6z" fill="currentColor"/>', stop: '<rect x="5" y="5" width="10" height="10" rx="1" fill="currentColor"/>', rec: '<circle cx="10" cy="10" r="5.500" fill="currentColor"/>',
  new: '<path d="M5 2.500h7l3 3v12H5z"/><path d="M12 2.500v3h3"/>', open: '<path d="M2.500 15.500v-10h5l2 2h7v8z"/>', save: '<path d="M3.500 3.500h11l2 2v11h-13z"/><path d="M6 3.500v4h7v-4M6 16.500v-5h8v5"/>',
  undo: '<path d="M7 5L3.500 8.500 7 12"/><path d="M3.500 8.500H12a4.500 4.500 0 0 1 0 9H8"/>', redo: '<path d="M13 5l3.500 3.500L13 12"/><path d="M16.500 8.500H8a4.500 4.500 0 0 0 0 9h4"/>',
  cut: '<circle cx="5.500" cy="15" r="2.500"/><circle cx="14.500" cy="15" r="2.500"/><path d="M7.300 13L14 3M12.700 13L6 3"/>', copy: '<rect x="7" y="3" width="9" height="11" rx="1"/><path d="M4 7v10h8"/>',
  paste: '<rect x="4" y="4.500" width="12" height="13" rx="1"/><rect x="7.500" y="2.500" width="5" height="3.500" fill="currentColor"/>', del: '<path d="M5.500 6l9 9M14.500 6l-9 9"/>', crop: '<path d="M6 2v12h12M2 6h12v12"/>',
  all: '<rect x="3" y="4.500" width="14" height="11" stroke-dasharray="2.500 2"/><path d="M7 10h6"/>', snap: '<path d="M2 10c2-6 4-6 6 0s4 6 6 0 2-3 4-1"/><path d="M2 10h16" stroke-dasharray="1.500 2"/>',
  zin: '<circle cx="8.500" cy="8.500" r="5"/><path d="M12.500 12.500l5 5M6.500 8.500h4M8.500 6.500v4"/>', zout: '<circle cx="8.500" cy="8.500" r="5"/><path d="M12.500 12.500l5 5M6.500 8.500h4"/>', fit: '<path d="M3 10h14M3 10l3-3M3 10l3 3M17 10l-3-3M17 10l-3 3"/>',
  tosel: '<circle cx="8.500" cy="8.500" r="5"/><path d="M12.500 12.500l5 5"/><rect x="6" y="6.500" width="5" height="4" stroke-dasharray="1.500 1"/>', repeat: '<path d="M16 8a6 6 0 1 0 .8 4"/><path d="M16.500 4v4.500H12"/>', grab: '<circle cx="10" cy="10" r="7"/><path d="M10 6v4l3 2"/>',
  stereo: '<circle cx="7.500" cy="10" r="4.500"/><circle cx="12.500" cy="10" r="4.500"/>', mono: '<circle cx="10" cy="10" r="5.500"/>',
  mixer: '<path d="M4 4v12M10 4v12M16 4v12"/><rect x="2.500" y="11" width="3" height="3" fill="currentColor"/><rect x="8.500" y="6" width="3" height="3" fill="currentColor"/><rect x="14.500" y="9" width="3" height="3" fill="currentColor"/>',
  mic: '<rect x="7.500" y="2.500" width="5" height="9" rx="2.500"/><path d="M4.500 9.500a5.500 5.500 0 0 0 11 0M10 15v3"/>', spk: '<path d="M3.500 8h3l4-3.500v11l-4-3.500h-3z"/><path d="M13.500 7.500a3.500 3.500 0 0 1 0 5M15.500 5.500a6.500 6.500 0 0 1 0 9"/>', spkoff: '<path d="M3.500 8h3l4-3.500v11l-4-3.500h-3z"/><path d="M14 8l4 4M18 8l-4 4"/>',
  spec: '<path d="M3.500 16V9M7 16V5M10.500 16v-5M14 16V3M17.500 16V8" stroke-width="2"/>',
  mouse: '<path d="M5 3l10 7-4.500 1 2.500 5-2 1-2.500-5L5 16z" fill="currentColor" stroke-width=".5"/>', select: '<rect x="3.500" y="4.500" width="13" height="11" stroke-dasharray="2.500 2"/>', cutter: '<rect x="3.500" y="4.500" width="13" height="11"/><path d="M3.500 15.500l13-11"/>',
  eraser: '<path d="M3 13l7-8 7 5-5 6H6z"/><path d="M7.500 8.800l6 4.500"/>', amp: '<path d="M4 16v-4M8.500 16V9M13 16V6M17 16V3" stroke-width="2.200"/>',
  gain: '<path d="M3 8h3l4-3.500v11L6 12H3z"/><path d="M13.500 8v5M11 10.500h5"/>', normalize: '<path d="M3.500 17V9M7.500 17V4M11.500 17V7M15.500 17V5"/><path d="M2 2.500h16" stroke-dasharray="2 2"/>', fadeIn: '<path d="M3 16h14V4z"/>', fadeOut: '<path d="M3 4v12h14z"/>',
  reverse: '<path d="M16 7H5l3-3M4 13h11l-3 3"/>', invert: '<path d="M2 10c2.500-7 5-7 7.500 0"/><path d="M10.500 10c2.500 7 5 7 7.500 0"/><path d="M2 10h16" stroke-dasharray="1.500 2"/>', dcRemove: '<path d="M2 8c2-5 4-5 6 0s4 5 6 0 2-1 4-2"/><path d="M2 16h16"/>',
  silence: '<path d="M3 8h3l4-3.500v11L6 12H3z"/><path d="M13.500 8l4 4M17.500 8l-4 4"/>', monoOp: '<circle cx="10" cy="10" r="3"/><path d="M2 10h3.500M18 10h-3.500"/>', pan: '<path d="M2 10h16M5 7l-3 3 3 3M15 7l3 3-3 3"/><circle cx="10" cy="10" r="1.800" fill="currentColor"/>',
  rate: '<circle cx="10" cy="10" r="7"/><path d="M10 10l4-3.500"/><path d="M5 13.500h2M13 13.500h2"/>', bitcrush: '<path d="M2.500 15h3v-4h3V7h3v4h3V5h3"/>', drive: '<path d="M11 2L5 11h4.500L8.500 18 15 8.500h-4.500z"/>',
  eq: '<path d="M2 12c3 0 3-6 6-6s3 8 6 8 2-4 4-4"/>', filter: '<path d="M2 7h8c3 0 4 3 8 8"/>', delay: '<path d="M4 4v12M8.500 6v8M13 8v4M16.500 9v2"/>', chorus: '<path d="M2 7c3-4 5 4 8 0s5-4 8 0M2 13c3-4 5 4 8 0s5-4 8 0"/>',
  phaser: '<path d="M2 6h4c2 0 2 8 4 8s2-8 4-8h4"/>', reverb: '<path d="M5 10a5 5 0 0 1 10 0M2.500 10a7.500 7.500 0 0 1 15 0M7.500 10a2.500 2.500 0 0 1 5 0"/><circle cx="10" cy="10" r="1" fill="currentColor"/>',
  compressor: '<path d="M3 3h14M3 17h14M10 5v3.500M7.500 7l2.500 2 2.500-2M10 15v-3.500M7.500 13l2.500-2 2.500 2"/>', limiter: '<path d="M2 5h16" stroke-dasharray="3 2"/><path d="M2 15c2-6 3-9 5-9h6c2 0 3 3 5 9"/>',
  gate: '<path d="M5 4v12M15 4v12M2 10h3M15 10h3M7 10h1.500l1-3 1.500 6 1-3H13"/>', trimSilence: '<path d="M5 4v12M15 4v12M2.500 10h2M15.500 10h2"/><path d="M6.800 10c1-5 2.200-5 3.200 0s2.200 5 3.200 0"/>', loopXfade: '<path d="M3 10a5 5 0 0 1 9-3M17 10a5 5 0 0 1-9 3"/><path d="M12 3.500V7H8.500M8 16.500V13h3.500"/>'
};
const OPICON = { mono: 'monoOp' };
const svgOf = n => `<svg viewBox="0 0 20 20" aria-hidden="true">${IC[n] || ''}</svg>`;
const setTip = (el, name, desc, key) => { el.dataset.tip = '@' + desc + '|' + (key ? 'Shortcut: ' + key : '') + '|' + name; };
const GRPCLS = { Level: 'gC1', Edit: 'gC2', Stereo: 'gC3', Time: 'gC4', Colour: 'gC5', Tone: 'gC6', Space: 'gC7', Dynamics: 'gC1' };
const AC = window.AudioContext || window.webkitAudioContext;
const gridCol = '#2a2d36', dim = '#8d92a0';
let ctx = null, bus = null, analyser = null, inBus = null, procNode = null, demoSrc = null, micStream = null, micNode = null, playSrc = null, inAn = null, monNode = null;
let tipOn = true, powerSave = false, showHits = true, showSpec = false;
const ed = new MC.Editor(OPS), rec = new MC.Recorder();
const state = { monitor: true, recMono: false, tool: 'mouse', sreg: null, amp: 6, brush: 12, bars: 1, click: 1, counting: false, bits: 24, scale: 1, ver: 0, viewA: 0, viewB: 1, playing: false, playA: 0, playN: 0, playT0: 0, loop: false, curOp: 0, takeMode: 1, startMode: 2, recOn: false, input: 'demo', lvl: 0, lvlHold: 0, lvlT: 0, drag: null, fname: 'demo loop, not saved' };
const opP = OPS.map(o => o.params.map(d => d.def)), recP = { thresh: -40, pre: 200, bpm: 120 };

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
  if (u === 'bpm') return v.toFixed(0) + ' bpm'; if (u === '%') return v.toFixed(0) + '%'; if (u === 'x') return v.toFixed(2) + 'x'; if (u === ':1') return v.toFixed(1) + ':1';
  if (d.id === 'bits' || d.id === 'stages') return String(Math.round(v)); return v.toFixed(2);
}
const toNorm = (d, v) => clamp(d.log ? (Math.log(v) - Math.log(d.lo)) / (Math.log(d.hi) - Math.log(d.lo)) : (v - d.lo) / (d.hi - d.lo), 0, 1);
const fromNorm = (d, n) => { n = clamp(n, 0, 1); let v = d.log ? Math.exp(Math.log(d.lo) + n * (Math.log(d.hi) - Math.log(d.lo))) : d.lo + n * (d.hi - d.lo); if (d.id === 'bits' || d.id === 'stages' || d.id === 'bpm') v = Math.round(v); return clamp(v, d.lo, d.hi); };
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
  const t = el.dataset.tip; if (t[0] === '@') { const [txt, rng, name] = t.slice(1).split('|'); tip.innerHTML = `<b>${name}</b>${txt}${rng ? '<i>' + rng + '</i>' : ''}`; } else tip.textContent = t;
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
    inBus = ctx.createGain(); inAn = ctx.createAnalyser(); inAn.fftSize = 1024; inBus.connect(inAn); monNode = ctx.createGain(); monNode.connect(bus); monNode.gain.value = state.monitor ? .6 : 0; procNode = ctx.createScriptProcessor(4096, 2, 2); const zero = ctx.createGain(); zero.gain.value = 0; inBus.connect(procNode); procNode.connect(zero); zero.connect(ctx.destination);
    procNode.onaudioprocess = ev => { const ib = ev.inputBuffer, l = ib.getChannelData(0), r = ib.numberOfChannels > 1 ? ib.getChannelData(1) : l; rec.process(l, r, l.length); };
    if (ctx.sampleRate !== SR) { demo = makeDemo(ctx.sampleRate); ed.audio = demo.clone(); rec.prepare(ctx.sampleRate, 30); }
  }
  if (ctx.state !== 'running') await ctx.resume(); $('#overlay').hidden = true; $('#power').classList.add('on'); startInput();
}
function startInput() {
  if (!ctx) return; try { if (demoSrc) demoSrc.stop(); } catch (e) {} demoSrc = null; if (micNode) { try { micNode.disconnect(); } catch (e) {} micNode = null; }
  if (state.input === 'demo') {
    const b = ctx.createBuffer(2, demo.frames, demo.sr); b.copyToChannel(demo.l, 0); b.copyToChannel(demo.r, 1); demoSrc = ctx.createBufferSource(); demoSrc.buffer = b; demoSrc.loop = true; demoSrc.connect(inBus);
    demoSrc.connect(monNode); demoSrc.start();
  } else if (micStream) { micNode = ctx.createMediaStreamSource(micStream); micNode.connect(inBus); micNode.connect(monNode); }
}
async function chooseInput(v) {
  state.input = v; setMonitor(v === 'demo');
  if (v === 'mic' && !micStream) { try { micStream = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } }); } catch (e) { $('#recHint').textContent = 'Microphone not available here. Using the demo loop.'; state.input = 'demo'; $('#inSel').value = 'demo'; } }
  startInput();
}

function setMonitor(v) { state.monitor = v; if (monNode) monNode.gain.value = v ? .6 : 0; syncIcons(); }
// ---------- recording ----------
let clickTimer = 0, beatT = 0, beatI = 0, countTO = 0;
function clickAt(t, strong) { const o = ctx.createOscillator(), g = ctx.createGain(); o.frequency.value = strong ? 1760 : 1175; g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(.45, t + .002); g.gain.exponentialRampToValueAtTime(.001, t + .06); o.connect(g); g.connect(bus); o.start(t); o.stop(t + .08); }
function pumpClicks() { while (beatT < ctx.currentTime + .25) { if (state.clickLive || beatI < state.countBeats) clickAt(beatT, beatI % 4 === 0); beatT += 60 / recP.bpm; ++beatI; } }
function stopClicks() { clearInterval(clickTimer); clickTimer = 0; }
function startClicks(t0, countBeats) { stopClicks(); beatT = t0; beatI = 0; state.countBeats = countBeats; state.clickLive = state.click === 2; if (state.click > 0) { pumpClicks(); clickTimer = setInterval(pumpClicks, 40); } }
function armOrRecord() {
  if (!ctx || ctx.state !== 'running') { powerOn(); return; }
  if (state.counting) { cancelCount(); return; }
  if (rec.isRecording() || rec.isArmed()) { stopRecording(); return; }
  stopPlay();
  if (state.startMode === 3) { startCountIn(); return; }
  rec.arm(state.startMode === 2 ? 2 : 1, recP.thresh, state.startMode === 2 ? recP.pre : 0); state.recOn = true;
  if (state.click === 2) startClicks(ctx.currentTime + .05, 0); syncRec();
}
function startCountIn() {
  const beat = 60 / recP.bpm, n = state.bars * 4, t0 = ctx.currentTime + .15, tEnd = t0 + n * beat; state.counting = true; state.recOn = true; state.countT0 = t0; state.countBeat = beat;
  startClicks(t0, n);
  countTO = setTimeout(() => { state.counting = false; const late = Math.max(0, (ctx.currentTime - tEnd) * 1000); rec.arm(1, recP.thresh, late); if (state.click !== 2) stopClicks(); syncRec(); }, Math.max(0, (tEnd - ctx.currentTime) * 1000));
  syncRec();
}
function cancelCount() { clearTimeout(countTO); stopClicks(); state.counting = false; state.recOn = false; syncRec(); }
function stopRecording() {
  stopClicks(); const a = rec.stop(); state.recOn = false; if (a.frames > 0) { if (state.recMono) for (let i = 0; i < a.frames; ++i) a.l[i] = a.r[i] = .5 * (a.l[i] + a.r[i]); a.sr = ed.audio.sr || a.sr; ed.takeRecording(a, state.takeMode); state.fname = 'new recording, not saved'; viewFit(); changed(); }
  syncRec();
}
function syncRec() {
  const b = $('#bRec'), on = rec.isRecording(), armed = rec.isArmed(), busy = on || armed || state.counting; b.classList.toggle('on', busy); b.innerHTML = svgOf(busy ? 'stop' : 'rec');
  setTip(b, busy ? 'Stop recording' : 'Record', busy ? 'Stops and puts the take in the sound.' : 'Records what comes in from the input. Where it starts is set by the buttons next to it.', 'Enter');
  let h = ''; if (state.counting && ctx) { const k = clamp(Math.floor((ctx.currentTime - state.countT0) / state.countBeat), 0, state.bars * 4 - 1); h = 'Count-in: bar ' + (Math.floor(k / 4) + 1) + ', beat ' + (k % 4 + 1); if (ctx.currentTime < state.countT0) h = 'Count-in: get ready'; }
  else if (armed) h = 'Waiting for ' + recP.thresh.toFixed(0) + ' dB'; else if (on) h = 'Recording ' + fmtTime(rec.recordedFrames(), rec.sr);
  $('#recHint').textContent = h;
}

// ---------- playback ----------
function playRange() { const a = ed.hasSelection() ? ed.selA : Math.min(ed.cursor, Math.max(0, ed.audio.frames - 1)), b = ed.hasSelection() ? ed.selB : ed.audio.frames; return [a, b]; }
function playAudio(au, from, loop) {
  stopPlay(); if (!ctx || ctx.state !== 'running') { powerOn().then(() => playAudio(au, from, loop)); return; } if (!au.frames) return;
  const b = ctx.createBuffer(2, au.frames, au.sr); b.copyToChannel(au.l, 0); b.copyToChannel(au.r, 1); playSrc = ctx.createBufferSource(); playSrc.buffer = b; playSrc.loop = loop; playSrc.connect(bus);
  playSrc.onended = () => { if (playSrc === s) { state.playing = false; syncPlay(); } }; const s = playSrc; playSrc.start(); state.playing = true; state.playA = from; state.playN = au.frames; state.playT0 = ctx.currentTime; state.playSr = au.sr; syncPlay();
}
function togglePlay() { if (state.playing) { stopPlay(); return; } const [a, b] = playRange(); playAudio(ed.audio.slice(a, b), a, state.loop); }
function stopPlay() { if (playSrc) { try { playSrc.onended = null; playSrc.stop(); } catch (e) {} playSrc = null; } state.playing = false; syncPlay(); }
function syncPlay() { const b = $('#bPlay'); b.classList.toggle('on', state.playing); b.innerHTML = svgOf(state.playing ? 'stop' : 'play'); setTip(b, state.playing ? 'Stop' : 'Play', 'Plays the selection, or from the cursor to the end.', 'Space'); }
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
let monoKey = '', monoVal = false;
function isMono(a) { const k = state.ver + ':' + a.frames; if (k === monoKey) return monoVal; monoKey = k; monoVal = true; for (let i = 0; i < a.frames; ++i) if (a.l[i] !== a.r[i]) { monoVal = false; break; } return monoVal; }
function drawWave() {
  const c = $('#wave'), { g, w, h } = prep(c), a = ed.audio, bh = h - 18; g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h);
  if (!a.frames) { g.strokeStyle = gridCol; g.beginPath(); g.moveTo(0, bh / 2 + .5); g.lineTo(w, bh / 2 + .5); g.stroke(); g.fillStyle = dim; g.font = '14px Barlow Condensed, sans-serif'; g.textAlign = 'center'; g.fillText('Nothing here yet. Record, open a file or drop one here.', w / 2, bh / 2 - 8); return; }
  const mono = isMono(a), lanes = mono ? 1 : 2, lh = bh / lanes, cols = Math.floor(w);
  for (let ch = 0; ch < lanes; ++ch) {
    const mid = lh * (ch + .5), top = lh * ch; g.strokeStyle = gridCol; g.lineWidth = 1; g.beginPath(); g.moveTo(0, mid + .5); g.lineTo(w, mid + .5); g.stroke();
    if (ch) { g.strokeStyle = '#3a3d46'; g.beginPath(); g.moveTo(0, top + .5); g.lineTo(w, top + .5); g.stroke(); }
    const pk = MC.peakOverviewCh(a, ch, Math.floor(state.viewA), Math.ceil(state.viewB), cols); g.fillStyle = ch ? '#02D9C0' : '#02EBBA'; g.globalAlpha = .92;
    for (let x = 0; x < pk.mx.length; ++x) { const y0 = mid - pk.mx[x] * lh / 2 * .96, y1 = mid - pk.mn[x] * lh / 2 * .96; g.fillRect(x, y0, 1, Math.max(1, y1 - y0)); } g.globalAlpha = 1;
    g.fillStyle = '#ffffff55'; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; g.fillText(mono ? 'M' : (ch ? 'R' : 'L'), 4, top + 12);
  }
  if (showHits) { const hits = state.hits || []; g.fillStyle = '#FF9000'; for (const f of hits) { if (f < state.viewA || f > state.viewB) continue; g.fillRect(Math.round(frameToX(f, w)), 0, 1, 12); } }
  if (ed.hasSelection()) { const x0 = frameToX(ed.selA, w), x1 = frameToX(ed.selB, w); g.fillStyle = '#6cc9f533'; g.fillRect(x0, 0, x1 - x0, bh); g.fillStyle = '#6cc9f5'; g.fillRect(x0, 0, 1, bh); g.fillRect(x1 - 1, 0, 1, bh); }
  else { const x = frameToX(ed.cursor, w); g.fillStyle = '#f4f5f7'; g.fillRect(Math.round(x), 0, 1, bh); }
  const ph = playheadFrame(); if (ph >= 0) { g.fillStyle = '#FF1383'; g.fillRect(Math.round(frameToX(ph, w)), 0, 2, bh); }
  g.fillStyle = '#101216'; g.fillRect(0, bh, w, 18); g.fillStyle = dim; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left';
  const step = fmtTicks(state.viewB - state.viewA, a.sr, w), t0 = Math.ceil(state.viewA / a.sr / step) * step; for (let t = t0; t * a.sr < state.viewB; t += step) { const x = Math.round(frameToX(t * a.sr, w)); g.fillRect(x, bh, 1, 5); g.fillText(step < 1 ? t.toFixed(step < .01 ? 3 : 2) + 's' : (t >= 60 ? Math.floor(t / 60) + ':' + ('0' + Math.round(t % 60)).slice(-2) : t + 's'), x + 3, bh + 13); }
}
// overview and scroll bar under the display
const ovw = $('#ovw'); let ovKey = '', ovPk = null;
function drawOvw() {
  const { g, w, h } = prep(ovw), a = ed.audio; g.fillStyle = '#0d0e11'; g.fillRect(0, 0, w, h); if (!a.frames) return;
  const key = state.ver + '|' + Math.floor(w); if (key !== ovKey) { ovKey = key; ovPk = MC.peakOverview(a, 0, a.frames, Math.floor(w)); }
  g.fillStyle = '#7d8294'; const mid = h / 2; for (let x = 0; x < ovPk.mx.length; ++x) { const y0 = mid - ovPk.mx[x] * mid * .9, y1 = mid - ovPk.mn[x] * mid * .9; g.fillRect(x, y0, 1, Math.max(1, y1 - y0)); }
  const x0 = state.viewA / a.frames * w, x1 = state.viewB / a.frames * w; g.fillStyle = '#6cc9f52a'; g.fillRect(x0, 0, Math.max(3, x1 - x0), h); g.strokeStyle = '#6cc9f5'; g.strokeRect(x0 + .5, .5, Math.max(3, x1 - x0) - 1, h - 1);
  if (ed.hasSelection()) { g.fillStyle = '#FFD100'; g.fillRect(ed.selA / a.frames * w, h - 3, Math.max(2, (ed.selB - ed.selA) / a.frames * w), 3); }
  const ph = playheadFrame(); if (ph >= 0) { g.fillStyle = '#FF1383'; g.fillRect(Math.round(ph / a.frames * w), 0, 2, h); }
}
let ovDrag = null; const ovF = e => { const r = ovw.getBoundingClientRect(); return clamp((e.clientX - r.left) / (r.width || 1), 0, 1) * ed.audio.frames; };
ovw.addEventListener('pointerdown', e => { if (!ed.audio.frames) return; ovw.setPointerCapture(e.pointerId); hideTip(); const f = ovF(e), len = state.viewB - state.viewA; if (f < state.viewA || f > state.viewB) { state.viewA = f - len / 2; state.viewB = state.viewA + len; clampView(); } ovDrag = f - state.viewA; ovw.style.cursor = 'grabbing'; });
ovw.addEventListener('pointermove', e => { if (ovDrag === null) return; const len = state.viewB - state.viewA; state.viewA = ovF(e) - ovDrag; state.viewB = state.viewA + len; clampView(); });
ovw.addEventListener('pointerup', () => { ovDrag = null; ovw.style.cursor = 'grab'; });
ovw.addEventListener('wheel', e => { e.preventDefault(); zoomAt(e.deltaY < 0 ? .8 : 1.25, ovF(e)); }, { passive: false });
function palette(db) { const t = clamp((db + 90) / 90, 0, 1), st = [[0, '#0a0a0c'], [.25, '#25164a'], [.5, '#8a1fa3'], [.72, '#ff1383'], [.88, '#ff9000'], [1, '#fff3b0']]; let i = 0; while (i < st.length - 2 && t > st[i + 1][0]) ++i; const a = st[i], b = st[i + 1], f = (t - a[0]) / (b[0] - a[0]); const p = s => [parseInt(s.slice(1, 3), 16), parseInt(s.slice(3, 5), 16), parseInt(s.slice(5, 7), 16)], A = p(a[1]), B = p(b[1]); return A.map((v, k) => Math.round(v + (B[k] - v) * clamp(f, 0, 1))); }
const PAL = Array.from({ length: 256 }, (_, i) => palette(-90 + i / 255 * 90));
let specKey = '', specImg = null;
function drawSpec() {
  if (!showSpec) return; const c = $('#spec'), { g, w, h } = prep(c), a = ed.audio; g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); if (!a.frames || !showSpec) return;
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
  drawSpecOverlay(g, w, h);
}
function drawLive() {
  const c = $('#live'), { g, w, h } = prep(c); g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); if (!ctx || !rec.ring) { g.fillStyle = dim; g.font = '12px Barlow Condensed, sans-serif'; g.fillText('Input appears here after Start audio', 8, h / 2 + 4); return; }
  const live = rec.captureLast(3), secs = 3, cols = Math.floor(w); g.strokeStyle = gridCol; g.beginPath(); g.moveTo(0, h / 2 + .5); g.lineTo(w, h / 2 + .5); g.stroke();
  const pre = clamp(recP.pre / 1000 / secs, 0, 1) * w; g.fillStyle = '#ff138322'; g.fillRect(w - pre, 0, pre, h);
  if (live.frames > 0) { const total = secs * rec.sr, off = total - live.frames, pk = MC.peakOverview(live, 0, live.frames, Math.max(1, Math.floor(cols * live.frames / total))); g.fillStyle = rec.isRecording() ? '#FF1383' : '#6CC9F5'; const x0 = off / total * w; for (let x = 0; x < pk.mx.length; ++x) { const y0 = h / 2 - pk.mx[x] * h / 2 * .95, y1 = h / 2 - pk.mn[x] * h / 2 * .95; g.fillRect(x0 + x, y0, 1, Math.max(1, y1 - y0)); } }
  if (state.startMode === 2) { const y = h / 2 - Math.pow(10, recP.thresh / 20) * h / 2; g.fillStyle = '#FFD100aa'; g.fillRect(0, y, w, 1); g.fillRect(0, h - y, w, 1); }
}
const METER_LO = -72;
function dbY(db, h) { return 16 + (1 - (clamp(db, METER_LO, 0) - METER_LO) / -METER_LO) * (h - 34); }
const dbAtY = (y, h) => METER_LO + (1 - (y - 16) / (h - 34)) * -METER_LO;
const l2d = v => 20 * Math.log10(Math.max(1e-6, v));
const meterGeom = w => ({ bw: Math.min(30, (w - 36) / 2 - 6), x0: 4 });
function peakOf(an) { if (!an) return 0; const buf = new Float32Array(an.fftSize); an.getFloatTimeDomainData(buf); let pk = 0; for (let i = 0; i < buf.length; ++i) { const v = Math.abs(buf[i]); if (v > pk) pk = v; } return pk; }
const mt = { in: 0, out: 0, hin: 0, hout: 0, tin: 0, tout: 0 };
function drawMeter() {
  const c = $('#meter'), { g, w, h } = prep(c); g.clearRect(0, 0, w, h); const now = performance.now(), bw = Math.min(30, (w - 34) / 2), gap = 4, xIn = 3, xOut = w - 3 - bw, cx = w / 2;
  const pin = peakOf(inAn), pout = peakOf(analyser);
  mt.in = Math.max(pin, mt.in * .9); mt.out = Math.max(pout, mt.out * .9); if (pin > mt.hin || now > mt.tin) { mt.hin = pin; mt.tin = now + 1500; } if (pout > mt.hout || now > mt.tout) { mt.hout = pout; mt.tout = now + 1500; }
  g.font = '10px "JetBrains Mono", monospace'; g.textAlign = 'center'; g.fillStyle = dim; g.fillText('IN', xIn + bw / 2, 10); g.fillText('OUT', xOut + bw / 2, 10);
  const grad = g.createLinearGradient(0, dbY(0, h), 0, dbY(METER_LO, h)); grad.addColorStop(0, '#ff1383'); grad.addColorStop(.05, '#ff1383'); grad.addColorStop(.08, '#b43dff'); grad.addColorStop(.25, '#a43dff'); grad.addColorStop(.3, '#1ea7ff'); grad.addColorStop(1, '#00d4ff');
  for (const [x, v, hold] of [[xIn, mt.in, mt.hin], [xOut, mt.out, mt.hout]]) {
    g.fillStyle = '#0a0a0c'; g.fillRect(x, dbY(0, h), bw, dbY(METER_LO, h) - dbY(0, h)); const y = dbY(l2d(v), h); g.fillStyle = grad; g.fillRect(x + 2, y, bw - 4, dbY(METER_LO, h) - y);
    g.fillStyle = hold >= .98 ? '#ff1383' : '#fff'; g.fillRect(x, dbY(l2d(hold), h), bw, 2);
  }
  g.fillStyle = dim; g.textAlign = 'center'; for (const db of [0, -6, -12, -18, -24, -36, -48, -60, -72]) { const y = dbY(db, h); g.fillText(String(db), cx, y + 3); g.fillStyle = '#ffffff12'; g.fillRect(xIn + bw, y, xOut - xIn - bw, 1); g.fillStyle = dim; }
  g.fillText('dBFS', cx, h - 4);
  const ty = dbY(recP.thresh, h); g.fillStyle = state.startMode === 2 ? '#FFD100' : '#FFD10077'; g.fillRect(xIn - 3, ty - 1, bw + 6, 3); g.beginPath(); g.moveTo(xIn + bw + 3, ty - 5); g.lineTo(xIn + bw + 10, ty); g.lineTo(xIn + bw + 3, ty + 5); g.fill();
  if (state.thrDrag) { g.fillStyle = '#FFD100'; g.textAlign = 'left'; g.fillText(recP.thresh.toFixed(0) + ' dB', xIn + bw + 12, ty - 6); }
}
const meterC = $('#meter');
meterC.addEventListener('pointerdown', e => { const r = meterC.getBoundingClientRect(), h = meterC.clientHeight, y = (e.clientY - r.top) / (r.height || 1) * h; if (Math.abs(y - dbY(recP.thresh, h)) < 10) { state.thrDrag = true; meterC.setPointerCapture(e.pointerId); hideTip(); e.preventDefault(); } });
meterC.addEventListener('pointermove', e => { const r = meterC.getBoundingClientRect(), h = meterC.clientHeight, y = (e.clientY - r.top) / (r.height || 1) * h; if (state.thrDrag) { recP.thresh = clamp(Math.round(dbAtY(y, h)), -72, 0); rebuildRec(); } else meterC.style.cursor = Math.abs(y - dbY(recP.thresh, h)) < 10 ? 'ns-resize' : 'default'; });
meterC.addEventListener('pointerup', () => { state.thrDrag = false; });
function drawReadout() {
  const a = ed.audio, sel = ed.hasSelection(); $('#readout').innerHTML = `<span>Length <b>${fmtTime(a.frames, a.sr || 48000)}</b></span><span>Selection <b>${sel ? fmtTime(ed.selA, a.sr) + ' to ' + fmtTime(ed.selB, a.sr) + ' (' + fmtTime(ed.selB - ed.selA, a.sr) + ')' : 'none, operations use everything'}</b></span><span>Cursor <b>${fmtTime(ed.cursor, a.sr || 48000)}</b></span><span>Peak <b>${a.frames ? MC.linToDb(a.peak(sel ? ed.selA : 0, sel ? ed.selB : a.frames)).toFixed(1) + ' dBFS' : '-'}</b></span><span><b>${(a.sr / 1000).toFixed(1)} kHz, ${isMono(a) ? 'mono' : 'stereo'}</b></span><span>${rec.isRecording() ? '<b style="color:var(--pink)">REC ' + fmtTime(rec.recordedFrames(), rec.sr) + '</b>' : ''}</span>`;
}

// ---------- editing UI ----------
function changed() { state.ver++; state.hits = showHits ? MC.detectTransients(ed.audio, .6) : []; clampView(); syncEdit(); }
function syncEdit() {
  const sel = ed.hasSelection(); $('#bUndo').disabled = !ed.canUndo(); $('#bRedo').disabled = !ed.canRedo(); $('#bCut').disabled = !sel; $('#bCopy').disabled = !sel; $('#bDel').disabled = !sel; $('#bCrop').disabled = !sel; $('#bSnap').disabled = !sel; $('#bSel').disabled = !sel;
  $('#bPaste').disabled = !ed.clip.frames; $('#bRepeat').disabled = !ed.last; setTip($('#bRepeat'), 'Repeat last operation', ed.last ? 'Does ' + OPS[ed.last.op].name + ' again with the same settings, on the current selection.' : 'Nothing to repeat yet. Apply an operation first.', 'R'); $('#fname').textContent = state.fname;
  $('#opHint').textContent = (sel ? 'Operations work on the selection. ' : 'Operations work on everything (nothing selected). ') + 'Click one for its settings, double click or Shift+letter to apply at once.';
}
function doOp() { stopPlay(); if (!ed.apply(state.curOp, opP[state.curOp])) return; state.fname = state.fname.replace(/, not saved$/, '') + ', edited, not saved'; changed(); }
function previewOp() {
  const [a, b] = ed.hasSelection() ? [ed.selA, ed.selB] : [0, ed.audio.frames]; const c = ed.audio.slice(a, b); MC.applyOp(c, state.curOp, 0, c.frames, opP[state.curOp]); playAudio(c, a, false);
}
function buildOps() {
  const host = $('#oplist'), groups = [], by = {}; OPS.forEach((o, i) => { if (!by[o.group]) { by[o.group] = []; groups.push(o.group); } by[o.group].push(i); });
  groups.forEach(gn => { const d = document.createElement('div'); d.className = 'opgrp'; d.innerHTML = `<h4>${gn}</h4>`; by[gn].forEach(i => { const b = document.createElement('button'); b.textContent = OPS[i].name; b.dataset.op = i; b.dataset.tip = OPS[i].tip; b.addEventListener('click', () => selectOp(i)); b.addEventListener('dblclick', () => { selectOp(i); doOp(); }); d.appendChild(b); }); host.appendChild(d); });
}
function selectOp(i, open) {
  state.curOp = i; $$('#oplist button').forEach(b => b.classList.toggle('on', +b.dataset.op === i)); const o = OPS[i]; $('#opName').textContent = o.name; $('#opTip').textContent = o.tip + (o.changesLength ? ' This one can change the length.' : ''); const host = $('#opK'); host.innerHTML = '';
  o.params.forEach((d, k) => host.appendChild(d.choice ? makeChoice(d, () => opP[i][k], v => { opP[i][k] = v; }) : makeKnob(d, o.group, () => opP[i][k], v => { opP[i][k] = v; })));
  if (!o.params.length) host.innerHTML = '<div class="hint">No settings.</div>';
  if (open) { const pop = $('#oppop'), ctl = $('.ctl'); pop.hidden = false; pop.style.top = (ctl.offsetTop + ctl.offsetHeight + 6) + 'px'; }
}
function closeOp() { $('#oppop').hidden = true; }
$('#bOpClose').addEventListener('click', closeOp);
function buildOps() {
  const host = $('#oplist'), order = []; OPS.forEach((o, i) => order.push(i)); const byG = {}; OPS.forEach((o, i) => { (byG[o.group] = byG[o.group] || []).push(i); }); const seq = [].concat(...Object.keys(byG).map(g => byG[g]));
  seq.forEach(i => { const o = OPS[i], b = document.createElement('button'); b.className = 'ib ' + (GRPCLS[o.group] || ''); b.dataset.op = i; b.innerHTML = svgOf(OPICON[o.id] || o.id); setTip(b, o.name, o.tip, 'Shift+' + OPKEYS[o.id]);
    b.addEventListener('click', () => selectOp(i, true)); b.addEventListener('dblclick', () => { selectOp(i); doOp(); }); host.appendChild(b); });
}
function buildRec() {
  const h = $('#recK'); h.appendChild(makeKnob({ id: 'thresh', name: 'Threshold', lo: -80, hi: 0, def: -40, unit: 'dB', tip: 'Level that starts a recording when Start is on Threshold.', choice: false, choices: [] }, 'Record', () => recP.thresh, v => { recP.thresh = v; }));
  h.appendChild(makeKnob({ id: 'bpm', name: 'Tempo', lo: 40, hi: 240, def: 120, unit: 'bpm', tip: 'Tempo of the click for the count-in and for recording with the click on. 4/4.', choice: false, choices: [] }, 'Record', () => recP.bpm, v => { recP.bpm = v; }));
  h.appendChild(makeKnob({ id: 'pre', name: 'Pre-roll', lo: 0, hi: 3000, def: 200, unit: 'ms', tip: 'How much sound from before the start is included in the recording.', choice: false, choices: [] }, 'Record', () => recP.pre, v => { recP.pre = v; }));
}

// ---------- spectral view and tools ----------
const specC = $('#spec'), TOOLHINT = { mouse: 'Click for the cursor, drag to select a time range.', select: 'Drag a box over time and frequency. Use Cutter, Amp or Copy on the box from the Spectral menu.', scissors: 'Drag across time: that slice is cut out and the gap closes.', cutter: 'Drag a box: the sound inside it is removed, the length stays.', eraser: 'Paint over what should go. It is removed when you let go.', copy: 'Drag a box to copy that piece, then click with Paste where it should land.', paste: 'Click where the copied piece should go. Left edge is time, height is frequency.', amp: 'Drag a box: it gets the level change set next to the tools.' };
const TOOLS = [['mouse', 'Mouse'], ['select', 'Selector'], ['scissors', 'Scissors'], ['cutter', 'Cutter'], ['eraser', 'Eraser'], ['copy', 'Copy'], ['paste', 'Paste'], ['amp', 'Amp']];
function syncSpecView() { $('#spec').hidden = !showSpec; $('#spectools').hidden = !showSpec; if (typeof layoutScale === 'function') try { layoutScale(); } catch (e) {} }
function setTool(t) { if (!showSpec) { showSpec = true; saveSet(); state.ver++; } markTool(t); }
function markTool(t) { state.tool = t; syncSpecView(); $$('#toolSeg button').forEach(b => b.classList.toggle('on', b.dataset.tool === t)); $('#toolHint').textContent = TOOLHINT[t]; specC.style.cursor = t === 'mouse' ? 'default' : (t === 'paste' ? 'copy' : 'crosshair'); }
const TOOLICON = { mouse: 'mouse', select: 'select', scissors: 'cut', cutter: 'cutter', eraser: 'eraser', copy: 'copy', paste: 'paste', amp: 'amp' };
TOOLS.forEach(([t, n], i) => { const b = document.createElement('button'); b.dataset.tool = t; b.innerHTML = svgOf(TOOLICON[t]); setTip(b, n, TOOLHINT[t], 'Shift+' + (i + 1)); $('#toolSeg').appendChild(b); });
$$('#toolSeg button').forEach(b => b.addEventListener('click', () => setTool(b.dataset.tool)));
$('#ampdb').addEventListener('change', e => { state.amp = +e.target.value; }); $('#brush').addEventListener('change', e => { state.brush = +e.target.value; });
const fMaxOf = () => (ed.audio.sr || 48000) / 2;
const spPt = e => { const r = specC.getBoundingClientRect(); return { x: (e.clientX - r.left) / (r.width || 1) * specC.clientWidth, y: (e.clientY - r.top) / (r.height || 1) * specC.clientHeight }; };
const hzAtY = y => { const h = specC.clientHeight; return 20 * Math.pow(fMaxOf() / 20, 1 - clamp(y, 0, h - 1) / (h - 1)); };
const yAtHz = hz => (1 - Math.log(clamp(hz, 20, fMaxOf()) / 20) / Math.log(fMaxOf() / 20)) * (specC.clientHeight - 1);
const frAtX = x => clamp(xToFrame(x, specC.clientWidth), 0, ed.audio.frames);
function boxOf(a, b) { const f = [frAtX(a.x), frAtX(b.x)], hz = [hzAtY(a.y), hzAtY(b.y)]; return { t0: Math.min(...f), t1: Math.max(...f), f0: Math.min(...hz), f1: Math.max(...hz) }; }
function brushRect(p) { const w = specC.clientWidth, fpp = (state.viewB - state.viewA) / w, r = state.brush, c = xToFrame(p.x, w); return { t0: c - r * fpp, t1: c + r * fpp, f0: hzAtY(p.y + r), f1: hzAtY(p.y - r) }; }
function specDone() { state.sreg = null; changed(); }
specC.addEventListener('pointerdown', e => {
  if (!ed.audio.frames) return; specC.setPointerCapture(e.pointerId); hideTip(); const p = spPt(e);
  state.drag = { spec: true, tool: state.tool, p0: p, p, rects: state.tool === 'eraser' ? [brushRect(p)] : [], f0: frAtX(p.x), moved: false };
});
specC.addEventListener('pointermove', e => {
  const p = spPt(e), d = state.drag;
  if (state.tool === 'paste') state.pghost = p;
  if (!d || !d.spec) return; d.p = p;
  if (d.tool === 'mouse') { if (Math.abs(p.x - d.p0.x) > 3) { d.moved = true; ed.setSelection(d.f0, frAtX(p.x)); syncEdit(); } }
  else if (d.tool === 'eraser') { const l = d.last || d.p0; if (Math.hypot(p.x - l.x, p.y - l.y) >= state.brush / 2) { d.rects.push(brushRect(p)); d.last = p; } }
});
specC.addEventListener('pointerleave', () => { state.pghost = null; });
specC.addEventListener('pointerup', e => {
  const d = state.drag; if (!d || !d.spec) return; state.drag = null; const p = spPt(e), tiny = Math.abs(p.x - d.p0.x) < 4 && Math.abs(p.y - d.p0.y) < 4, box = boxOf(d.p0, p);
  switch (d.tool) {
    case 'mouse': if (!d.moved) { ed.setCursor(frAtX(p.x)); } syncEdit(); break;
    case 'select': if (tiny) { state.sreg = null; ed.setCursor(frAtX(p.x)); syncEdit(); } else { state.sreg = box; ed.setSelection(box.t0, box.t1); syncEdit(); } break;
    case 'scissors': if (!tiny) { stopPlay(); ed.setSelection(box.t0, box.t1); ed.deleteSelection(); specDone(); } break;
    case 'cutter': if (!tiny) { stopPlay(); ed.spectralEdit('cut', [box]); specDone(); } break;
    case 'amp': if (!tiny) { stopPlay(); ed.spectralEdit('amp', [box], state.amp); specDone(); } break;
    case 'eraser': stopPlay(); ed.spectralEdit('cut', d.rects); specDone(); break;
    case 'copy': if (!tiny && ed.spectralCopy(box)) { state.sreg = box; setTool('paste'); $('#toolHint').textContent = 'Copied. Click where it should land.'; syncEdit(); } break;
    case 'paste': stopPlay(); if (ed.spectralPaste(frAtX(p.x), hzAtY(p.y))) specDone(); else $('#toolHint').textContent = 'Nothing copied yet. Use Copy first.'; break;
  }
});
specC.addEventListener('wheel', e => { e.preventDefault(); if (e.altKey || e.shiftKey) { const len = state.viewB - state.viewA, d = (e.deltaY + e.deltaX) / 400 * len; state.viewA += d; state.viewB += d; clampView(); state.ver++; } else zoomAt(e.deltaY < 0 ? .8 : 1.25, frAtX(spPt(e).x)); }, { passive: false });
const fmtHz = f => f >= 1000 ? (f / 1000).toFixed(f >= 10000 ? 1 : 2) + 'k' : Math.round(f) + '';
function drawBox(g, r, col, fill, label) { const w = specC.clientWidth, x0 = frameToX(r.t0, w), x1 = frameToX(r.t1, w), y0 = yAtHz(r.f1), y1 = yAtHz(r.f0); g.fillStyle = fill; g.fillRect(x0, y0, x1 - x0, y1 - y0); g.strokeStyle = col; g.lineWidth = 1; g.setLineDash([4, 3]); g.strokeRect(x0 + .5, y0 + .5, x1 - x0, y1 - y0); g.setLineDash([]); if (label) { g.fillStyle = col; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; g.fillText(label, x0 + 3, Math.max(10, y0 - 3)); } }
function drawSpecOverlay(g, w, h) {
  const d = state.drag; if (state.sreg) drawBox(g, state.sreg, '#6CC9F5', '#6cc9f522', fmtHz(state.sreg.f0) + ' to ' + fmtHz(state.sreg.f1) + ' Hz');
  if (d && d.spec) {
    if (d.tool === 'eraser') d.rects.forEach(r => drawBox(g, r, '#FF1383', '#ff138355', null));
    else if (['select', 'scissors', 'cutter', 'copy', 'amp'].includes(d.tool)) { const b = boxOf(d.p0, d.p); if (d.tool === 'scissors') { b.f0 = 20; b.f1 = fMaxOf(); } drawBox(g, b, d.tool === 'cutter' || d.tool === 'scissors' ? '#FF1383' : (d.tool === 'amp' ? '#FFD100' : '#6CC9F5'), '#ffffff1c', null); }
  } else if (state.tool === 'paste' && state.pghost && ed.sclip) { const c = ed.sclip, p = state.pghost, fr = frAtX(p.x), dur = ((c.nf - 1) * MC.SH + MC.SN / 2); const half = (c.nb * c.sr / MC.SN) / 2, hz = hzAtY(p.y); drawBox(g, { t0: fr, t1: fr + dur, f0: Math.max(20, hz - half), f1: hz + half }, '#02EBBA', '#02ebba22', 'paste here'); }
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
$('#file').addEventListener('change', e => { openFile(e.target.files[0]); e.target.value = ''; });
const dw = $('#dispwrap'); ['dragenter', 'dragover'].forEach(t => dw.addEventListener(t, e => { e.preventDefault(); dw.classList.add('hot'); })); ['dragleave', 'drop'].forEach(t => dw.addEventListener(t, e => { e.preventDefault(); dw.classList.remove('hot'); }));
dw.addEventListener('drop', e => openFile(e.dataTransfer.files[0]));
function saveWav() { const bytes = MC.encodeWav(ed.audio, state.bits), url = URL.createObjectURL(new Blob([bytes], { type: 'audio/wav' })), a = document.createElement('a'); a.href = url; a.download = (state.fname.replace(/[^\w.-]+/g, '_').replace(/_?(not_saved|edited)/g, '') || 'meowcorder') + '.wav'; document.body.appendChild(a); a.click(); a.remove(); setTimeout(() => URL.revokeObjectURL(url), 2000); state.fname = state.fname.replace(/, not saved$/, ''); syncEdit(); }
function newSound() { stopPlay(); ed.pushUndo(); const keep = ed.undoStack.slice(), a = new MC.Audio(ed.audio.sr || 48000); ed.load(a); ed.undoStack = keep; state.fname = 'empty, not saved'; viewFit(); changed(); }

// ---------- buttons ----------
$('#bStart').addEventListener('click', powerOn); $('#power').addEventListener('click', () => { if (ctx && ctx.state === 'running') { ctx.suspend(); $('#power').classList.remove('on'); $('#overlay').hidden = false; } else powerOn(); });
$('#inSel').addEventListener('change', e => chooseInput(e.target.value)); $('#bRec').addEventListener('click', armOrRecord);
$$('#startSeg button').forEach(b => b.addEventListener('click', () => { state.startMode = +b.dataset.v; syncSegs(); })); $$('#takeSeg button').forEach(b => b.addEventListener('click', () => { state.takeMode = +b.dataset.v; syncSegs(); }));
function syncSegs() { syncIcons(); $$('#startSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === state.startMode)); $$('#takeSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === state.takeMode)); }
$('#bGrab').addEventListener('click', () => { if (!rec.ring) { powerOn(); return; } const a = rec.captureLast(Math.max(.2, recP.pre / 1000 * 1 + 0) || 3); if (a.frames) { ed.takeRecording(a, state.takeMode); state.fname = 'grabbed, not saved'; viewFit(); changed(); } });
$('#bPlay').addEventListener('click', togglePlay); $('#bLoop').addEventListener('click', () => { state.loop = !state.loop; $('#bLoop').classList.toggle('on', state.loop); if (state.playing) { const [a, b] = playRange(); playAudio(ed.audio.slice(a, b), a, state.loop); } });
const act = (id, fn) => $(id).addEventListener('click', () => { stopPlay(); fn(); changed(); });
act('#bUndo', () => ed.undo()); act('#bRedo', () => ed.redo()); act('#bCut', () => ed.cut()); act('#bCopy', () => ed.copy()); act('#bPaste', () => ed.paste()); act('#bDel', () => ed.deleteSelection()); act('#bCrop', () => { ed.cropToSelection(); viewFit(); });
act('#bAll', () => ed.setSelection(0, ed.audio.frames)); act('#bSnap', () => { if (!ed.hasSelection()) return; const a = ed.snapToZero(ed.selA, 2000), b = ed.snapToZero(ed.selB, 2000); ed.setSelection(a, b); });
act('#bRepeat', () => ed.repeatLast());
$('#bZin').addEventListener('click', () => zoomAt(.5, (state.viewA + state.viewB) / 2)); $('#bZout').addEventListener('click', () => zoomAt(2, (state.viewA + state.viewB) / 2)); $('#bFit').addEventListener('click', () => { viewFit(); state.ver++; });
$('#bSel').addEventListener('click', () => { if (ed.hasSelection()) { state.viewA = ed.selA; state.viewB = ed.selB; clampView(); state.ver++; } });
$('#bApply').addEventListener('click', doOp); $('#bPrev').addEventListener('click', previewOp);

// ---------- dialogs ----------
function openGuide() { modal('Guide', `<ol><li>Click <b>Start audio</b>. In the plugin, Meowcorder listens to the mixer channel it sits on. Here, a demo loop stands in for that channel.</li><li><b>Record</b> (Enter) records what comes in. The small arrow next to it chooses how it starts: <b>Now</b>, <b>On input</b> (waits for sound above the threshold, with pre-roll), or after a <b>Count-in</b> (1, 2 or 4 bars of 4/4 with a click you hear, at the tempo you set).</li><li>The same arrow sets what a new take does: <b>Replace</b>, <b>Append</b>, <b>Punch</b> over the selection or <b>Insert</b>. <b>Grab the last seconds</b> keeps what you already heard.</li><li>Drag on the waveform to select, click to place the cursor. Wheel zooms, Alt wheel scrolls, double click selects all.</li><li>All functions are in the menus on top, with their keys next to them. <b>Help, Keyboard shortcuts</b> (?) lists them all.</li><li><b>Spectral, Show the spectral view</b> (Alt+G) opens the picture of the sound with tools: Mouse, Selector, Scissors, Cutter, Eraser, Copy, Paste and Amp. Cut out a cough, a whistle or one note by looking at it.</li><li><b>Process</b> has 24 operations. Pick one, set it, <b>Preview</b>, then <b>Apply</b>. Shift plus a letter applies it straight away. <b>R</b> repeats the last one.</li><li><b>Undo</b> (Ctrl Z) always brings the old sound back.</li></ol>`, [['Close', null, 'on']]); }
function openSettings() { modal('Settings', `<label class="row"><input type="checkbox" id="sTip" ${tipOn ? 'checked' : ''}> Info bubbles (after 1 second)</label><label class="row"><input type="checkbox" id="sPow" ${powerSave ? 'checked' : ''}> Power saving (slower drawing)</label><label class="row"><input type="checkbox" id="sSpec" ${showSpec ? 'checked' : ''}> Show the spectral view</label><label class="row"><input type="checkbox" id="sHit" ${showHits ? 'checked' : ''}> Mark hits (orange ticks) on the waveform</label>`, [['Reset window size', () => { setZoom(1); return false; }], ['Close', m => { tipOn = $('#sTip', m).checked; powerSave = $('#sPow', m).checked; showSpec = $('#sSpec', m).checked; showHits = $('#sHit', m).checked; syncSpecView(); store.set('mc.set', JSON.stringify({ tipOn, powerSave, showSpec, showHits })); state.ver++; changed(); }, 'on']]); }
function openAbout() { modal('About & Support', `<p><b>Meowcorder</b> by Studio 87, version 0.1 design mockup.</p><p>Free and open source (AGPL-3.0-or-later). It collects nothing and makes no network calls. The microphone is only used after you choose it and press Record. The button below only opens a link when you click it.</p><p>Nothing leaves this page: recordings stay in memory until you save a file.</p><p><a class="btn" href="https://ko-fi.com/studio87" target="_blank" rel="noopener">Support on Ko-fi</a></p>`, [['Close', null, 'on']]); }

// ---------- menus (the single source for items, shortcuts and key handling) ----------
function saveSet() { store.set('mc.set', JSON.stringify({ tipOn, powerSave, showSpec, showHits })); }
function rebuildRec() { $('#recK').innerHTML = ''; buildRec(); }
function runOp(i) { state.curOp = i; selectOp(i); doOp(); }
const ED = fn => () => { stopPlay(); fn(); changed(); };
const OPKEYS = { gain: 'G', normalize: 'N', fadeIn: 'I', fadeOut: 'O', reverse: 'R', invert: 'P', dcRemove: 'D', silence: 'S', mono: 'M', pan: 'B', rate: 'Y', bitcrush: 'K', drive: 'V', eq: 'E', filter: 'F', delay: 'L', chorus: 'H', phaser: 'A', reverb: 'W', compressor: 'C', limiter: 'T', gate: 'U', trimSilence: 'X', loopXfade: 'J' };
const radio = (l, get, set, k) => ({ l, k, ck: () => get(), f: set });
const processSubs = () => { const by = {}, order = []; OPS.forEach((o, i) => { if (!by[o.group]) { by[o.group] = []; order.push(o.group); } by[o.group].push({ l: o.name, k: 'Shift+' + OPKEYS[o.id], tip: o.tip, f: () => runOp(i) }); }); return order.map(g => ({ l: g, sub: by[g] })); };
const MENUS = [
  { name: 'File', items: [
    { l: 'New empty sound', k: 'Alt+N', f: newSound },
    { l: 'Open file...', k: 'Ctrl+O', f: () => $('#file').click() },
    { l: 'Save as WAV', k: 'Ctrl+S', f: saveWav },
    { l: 'Save bit depth', sub: [16, 24, 32].map(b => radio(b === 32 ? '32 bit float' : b + ' bit', () => state.bits === b, () => { state.bits = b; })) },
    '-',
    { l: 'Audio on or off', k: 'Alt+A', f: () => $('#power').click() },
    { l: 'Reload the demo loop', f: () => { stopPlay(); ed.pushUndo(); const keep = ed.undoStack.slice(); ed.load(demo.clone()); ed.undoStack = keep; state.fname = 'demo loop, not saved'; viewFit(); changed(); } } ] },
  { name: 'Edit', items: [
    { l: 'Undo', k: 'Ctrl+Z', f: ED(() => ed.undo()), en: () => ed.canUndo() },
    { l: 'Redo', k: 'Ctrl+Y', also: ['Ctrl+Shift+Z'], f: ED(() => ed.redo()), en: () => ed.canRedo() },
    '-',
    { l: 'Cut', k: 'Ctrl+X', f: ED(() => ed.cut()), en: () => ed.hasSelection() },
    { l: 'Copy', k: 'Ctrl+C', f: () => { ed.copy(); syncEdit(); }, en: () => ed.hasSelection() },
    { l: 'Paste', k: 'Ctrl+V', f: ED(() => ed.paste()), en: () => ed.clip.frames > 0 },
    { l: 'Delete selection', k: 'Del', also: ['Backspace'], f: ED(() => ed.deleteSelection()), en: () => ed.hasSelection() },
    { l: 'Crop to selection', k: 'T', f: ED(() => { ed.cropToSelection(); viewFit(); }), en: () => ed.hasSelection() },
    '-',
    { l: 'Repeat last operation', k: 'R', f: ED(() => ed.repeatLast()), en: () => !!ed.last } ] },
  { name: 'Select', items: [
    { l: 'All', k: 'Ctrl+A', f: () => { ed.setSelection(0, ed.audio.frames); syncEdit(); } },
    { l: 'None', k: 'Esc', f: () => { if (!$('#oppop').hidden) { closeOp(); return; } state.sreg = null; ed.setCursor(ed.cursor); syncEdit(); } },
    { l: 'From cursor to the end', k: 'Shift+End', f: () => { ed.setSelection(ed.hasSelection() ? ed.selA : ed.cursor, ed.audio.frames); syncEdit(); } },
    { l: 'From the start to the cursor', k: 'Shift+Home', f: () => { ed.setSelection(0, ed.hasSelection() ? ed.selB : ed.cursor); syncEdit(); } },
    '-',
    { l: 'Snap edges to zero crossings', k: 'Z', f: ED(() => { const a = ed.snapToZero(ed.selA, 2000), b = ed.snapToZero(ed.selB, 2000); ed.setSelection(a, b); }), en: () => ed.hasSelection() } ] },
  { name: 'View', items: [
    { l: 'Zoom in', k: '+', also: ['='], f: () => zoomAt(.5, (state.viewA + state.viewB) / 2) },
    { l: 'Zoom out', k: '-', f: () => zoomAt(2, (state.viewA + state.viewB) / 2) },
    { l: 'Show the whole sound', k: 'F', f: () => { viewFit(); state.ver++; } },
    { l: 'Zoom to the selection', k: 'S', f: () => { if (ed.hasSelection()) { state.viewA = ed.selA; state.viewB = ed.selB; clampView(); state.ver++; } }, en: () => ed.hasSelection() },
    '-',
    { l: 'Hit marks', k: 'Alt+H', ck: () => showHits, f: () => { showHits = !showHits; saveSet(); changed(); } },
    { l: 'Info bubbles', k: 'Alt+I', ck: () => tipOn, f: () => { tipOn = !tipOn; saveSet(); } },
    { l: 'Power saving', k: 'Alt+P', ck: () => powerSave, f: () => { powerSave = !powerSave; saveSet(); } },
    '-',
    { l: 'Window size', sub: [[75, 1], [100, 2], [125, 3], [150, 4], [200, 5]].map(([pc, n]) => ({ l: pc + '%', k: 'Alt+' + n, ck: () => Math.round(zoom * 100) === pc, f: () => setZoom(pc / 100) })).concat([{ l: 'Back to 100%', k: 'Alt+0', f: () => setZoom(1) }]) } ] },
  { name: 'Spectral', items: [
    { l: 'Show the spectral view', k: 'Alt+G', ck: () => showSpec, f: toggleSpec },
    '-',
    { l: 'Tool', sub: TOOLS.map(([t, n], i) => ({ l: n, k: 'Shift+' + (i + 1), ck: () => state.tool === t, f: () => setTool(t), tip: TOOLHINT[t] })) },
    { l: 'Amp amount', sub: [-24, -12, -6, -3, 3, 6, 12].map(v => radio((v > 0 ? '+' : '') + v + ' dB', () => state.amp === v, () => { state.amp = v; $('#ampdb').value = String(v); })) },
    { l: 'Eraser brush', sub: [['Small', 6], ['Medium', 12], ['Large', 24]].map(([n, v]) => radio(n, () => state.brush === v, () => { state.brush = v; $('#brush').value = String(v); })) },
    '-',
    { l: 'Cut the box', k: 'Alt+X', f: () => { ed.spectralEdit('cut', [state.sreg]); specDone(); }, en: () => !!state.sreg },
    { l: 'Amp the box', k: 'Alt+U', f: () => { ed.spectralEdit('amp', [state.sreg], state.amp); specDone(); }, en: () => !!state.sreg },
    { l: 'Copy the box', k: 'Alt+C', f: () => { ed.spectralCopy(state.sreg); syncEdit(); }, en: () => !!state.sreg },
    { l: 'Paste at the cursor', k: 'Alt+V', f: () => { const c = ed.sclip; ed.spectralPaste(ed.cursor, c.fc); specDone(); }, en: () => !!ed.sclip },
    { l: 'Clear the box', f: () => { state.sreg = null; }, en: () => !!state.sreg } ] },
  { name: 'Play', items: [
    { l: 'Play or stop', k: 'Space', f: togglePlay },
    { l: 'Loop', k: 'L', ck: () => state.loop, f: () => $('#bLoop').click() },
    { l: 'Preview the chosen operation', k: 'P', f: previewOp },
    '-',
    { l: 'Cursor to the start', k: 'Home', f: () => { ed.setCursor(0); state.viewA = 0; clampView(); state.ver++; syncEdit(); } },
    { l: 'Cursor to the end', k: 'End', f: () => { ed.setCursor(ed.audio.frames); syncEdit(); } } ] },
  { name: 'Record', items: [
    { l: 'Record or stop', k: 'Enter', f: armOrRecord },
    { l: 'Grab the last seconds', k: 'G', f: () => $('#bGrab').click() },
    { l: 'Monitor the input', k: 'Alt+M', ck: () => state.monitor, f: () => setMonitor(!state.monitor) },
    { l: 'Record in mono', k: 'Alt+O', ck: () => state.recMono, f: () => { state.recMono = !state.recMono; syncIcons(); } },
    '-',
    { l: 'Start', sub: [radio('Now', () => state.startMode === 1, () => { state.startMode = 1; syncSegs(); }, '1'), radio('On input (when sound arrives)', () => state.startMode === 2, () => { state.startMode = 2; syncSegs(); }, '2'), radio('After a count-in', () => state.startMode === 3, () => { state.startMode = 3; syncSegs(); }, '3')] },
    { l: 'Count-in length', sub: [1, 2, 4].map(n => radio(n + (n === 1 ? ' bar' : ' bars') + ' of 4/4', () => state.bars === n, () => { state.bars = n; })) },
    { l: 'Tempo', sub: [90, 100, 110, 120, 128, 140, 150, 160, 174].map(v => radio(v + ' bpm', () => Math.round(recP.bpm) === v, () => { recP.bpm = v; rebuildRec(); })) },
    { l: 'Click', sub: [['Off', 0], ['During the count-in', 1], ['Count-in and while recording', 2]].map(([n, v]) => radio(n, () => state.click === v, () => { state.click = v; })) },
    '-',
    { l: 'A new take will', sub: ['Replace the sound', 'Append at the end', 'Punch over the selection', 'Insert at the selection'].map((t, i) => radio(t, () => state.takeMode === i, () => { state.takeMode = i; syncSegs(); }, String(i + 4))) },
    { l: 'Threshold level', sub: [-70, -60, -50, -40, -30, -20, -10].map(v => radio(v + ' dB', () => Math.round(recP.thresh) === v, () => { recP.thresh = v; rebuildRec(); })) },
    { l: 'Pre-roll (on input)', sub: [0, 100, 200, 500, 1000, 3000].map(v => radio(v >= 1000 ? v / 1000 + ' s' : v + ' ms', () => Math.round(recP.pre) === v, () => { recP.pre = v; rebuildRec(); })) },
    { l: 'Use the microphone instead of the mixer channel', k: 'M', f: () => { const v = state.input === 'demo' ? 'mic' : 'demo'; $('#inSel').value = v; chooseInput(v); }, ck: () => state.input === 'mic' }
  ] },
  { name: 'Process', items: [{ l: 'Apply the chosen operation', k: 'Ctrl+Enter', f: doOp }, '-'].concat(processSubs()) },
  { name: 'Help', items: [
    { l: 'Guide', k: 'F1', f: openGuide },
    { l: 'Keyboard shortcuts', k: '?', f: openKeys },
    { l: 'Settings', k: 'Ctrl+,', f: openSettings },
    '-',
    { l: 'About & Support', f: openAbout } ] }
];
// ---- key table built from the menus ----
const KEYS = [];
function parseKey(str) { if (str === '+') return { ctrl: false, alt: false, shift: false, key: '+', letter: false }; const parts = str.split('+'); let key = parts.pop(); if (str.endsWith('++')) { key = '+'; parts.pop(); } const o = { ctrl: parts.includes('Ctrl'), alt: parts.includes('Alt'), shift: parts.includes('Shift') }; const names = { Space: ' ', Del: 'delete', Esc: 'escape', Enter: 'enter', Home: 'home', End: 'end', Backspace: 'backspace', F1: 'f1' }; o.key = (names[key] || key).toLowerCase(); o.letter = /^[a-z]$/.test(o.key); return o; }
(function reg() { const walk = (items, menu) => items.forEach(it => { if (it === '-') return; if (it.sub) { walk(it.sub, menu); return; } if (it.k) { [it.k].concat(it.also || []).forEach(k => KEYS.push({ p: parseKey(k), it, menu })); } }); MENUS.forEach(m => walk(m.items, m.name)); })();
function keyMatch(p, e) {
  const ctrl = e.ctrlKey || e.metaKey; if (p.ctrl !== ctrl || p.alt !== e.altKey) return false;
  const code = e.code || '', k = (e.key || '').toLowerCase();
  if (p.letter) return e.shiftKey === p.shift && (code === 'Key' + p.key.toUpperCase() || (k === p.key && !e.altKey));
  if (/^\d$/.test(p.key)) return (code === 'Digit' + p.key || code === 'Numpad' + p.key || k === p.key) && p.shift === e.shiftKey;
  if (p.shift && !e.shiftKey) return false;
  if (p.key === '+') return k === '+' || (k === '=' && true);
  if (p.key === '=') return k === '=' || k === '+';
  if (p.key === '-') return k === '-' || k === '_';
  if (p.key === '?') return k === '?' || (code === 'Slash' && e.shiftKey);
  return k === p.key;
}
// ---- dropdown rendering ----
let openIdx = -1, activeDD = null, hoverOpened = false; const splitWrap = $('.splitrec'); const menuBar = $('#menubar');
function ddFor(items) {
  const dd = document.createElement('div'); dd.className = 'dd'; dd.setAttribute('role', 'menu');
  items.forEach(it => {
    if (it === '-') { const s = document.createElement('div'); s.className = 'msep'; dd.appendChild(s); return; }
    const row = document.createElement('div'); row.className = 'mi'; row.setAttribute('role', 'menuitem'); const en = it.en ? it.en() : true; if (!en) row.classList.add('dis');
    const ck = document.createElement('span'); ck.className = 'ck'; ck.textContent = it.ck && it.ck() ? '✓' : ''; const lb = document.createElement('span'); lb.textContent = it.l; const kk = document.createElement('span'); kk.className = 'k'; kk.textContent = it.sub ? '▸' : (it.k || '');
    row.append(ck, lb, kk); if (it.tip) row.dataset.tip = it.tip;
    if (it.sub) { row.classList.add('has'); row.appendChild(ddFor(it.sub)); }
    else row.addEventListener('click', e => { e.stopPropagation(); closeMenu(); it.f(); });
    row.addEventListener('mouseenter', () => { $$('.mi.hi', row.parentNode).forEach(x => x.classList.remove('hi')); row.classList.add('hi'); activeDD = row.parentNode; });
    dd.appendChild(row);
  });
  return dd;
}
function openMenu(i) {
  closeMenu(); openIdx = i; const w = $$('.mn', menuBar)[i]; w.classList.add('open'); const dd = ddFor(MENUS[i].items); w.appendChild(dd); activeDD = dd; hideTip();
}
function closeMenu() { $$(':scope > .dd', splitWrap).forEach(d => d.remove()); hoverOpened = false; $$('.mn', menuBar).forEach(w => { w.classList.remove('open'); const d = $('.dd', w); if (d && d.parentNode === w) d.remove(); }); openIdx = -1; activeDD = null; }
MENUS.forEach((m, i) => { const w = document.createElement('div'); w.className = 'mn'; const b = document.createElement('button'); b.className = 'mt'; b.textContent = m.name; b.setAttribute('aria-haspopup', 'true');
  b.addEventListener('click', e => { e.stopPropagation(); if (hoverOpened) { hoverOpened = false; return; } openIdx === i ? closeMenu() : openMenu(i); }); b.addEventListener('mouseenter', () => { if (openIdx >= 0 && openIdx !== i && openIdx < MENUS.length) { openMenu(i); hoverOpened = true; } }); w.appendChild(b); menuBar.appendChild(w); });
menuBar.addEventListener('pointerleave', () => { hoverOpened = false; });
document.addEventListener('pointerdown', e => { if (openIdx >= 0 && !e.target.closest('.mn') && !e.target.closest('.splitrec')) closeMenu(); });
$('#bRecOpt').addEventListener('click', e => { e.stopPropagation(); if (openIdx === 99) { closeMenu(); return; } closeMenu(); openIdx = 99; const dd = ddFor(MENUS.find(m => m.name === 'Record').items); splitWrap.appendChild(dd); activeDD = dd; hideTip(); });
function menuKey(e) {
  const rows = () => $$(':scope > .mi:not(.dis)', activeDD), cur = () => rows().findIndex(r => r.classList.contains('hi')), setHi = n => { const r = rows(); if (!r.length) return; $$('.mi.hi', activeDD).forEach(x => x.classList.remove('hi')); r[(n + r.length) % r.length].classList.add('hi'); };
  const k = e.key; if (k === 'Escape') { closeMenu(); return true; }
  if (k === 'ArrowDown') { setHi(cur() + 1); return true; } if (k === 'ArrowUp') { setHi(cur() < 0 ? -1 : cur() - 1); return true; }
  const hiRow = rows()[cur()];
  if (k === 'ArrowRight') { if (hiRow && hiRow.classList.contains('has')) { hiRow.classList.add('show'); activeDD = $(':scope > .dd', hiRow); setHi(0); } else if (openIdx < MENUS.length) openMenu((openIdx + 1) % MENUS.length); return true; }
  if (k === 'ArrowLeft') { const par = activeDD.parentNode; if (par && par.classList.contains('mi')) { par.classList.remove('show'); activeDD = par.parentNode; } else if (openIdx < MENUS.length) openMenu((openIdx + MENUS.length - 1) % MENUS.length); return true; }
  if (k === 'Enter' || k === ' ') { if (hiRow) { if (hiRow.classList.contains('has')) { hiRow.classList.add('show'); activeDD = $(':scope > .dd', hiRow); setHi(0); } else hiRow.click(); } return true; }
  return false;
}
document.addEventListener('keydown', e => {
  if (openIdx >= 0 && menuKey(e)) { e.preventDefault(); e.stopPropagation(); return; }
  if (e.target.matches && e.target.matches('input, select, textarea')) return;
  if (document.querySelector('.modalbg')) return;
  for (const k of KEYS) { if (!keyMatch(k.p, e)) continue; if (k.it.en && !k.it.en()) continue; e.preventDefault(); k.it.f(); return; }
}, true);
function openKeys() {
  let html = '<div class="keytbl">'; MENUS.forEach(m => { const rows = []; const walk = items => items.forEach(it => { if (it === '-') return; if (it.sub) walk(it.sub); else if (it.k) rows.push(`<div class="r"><span>${it.l}</span><span class="kk">${it.k}</span></div>`); }); walk(m.items); if (rows.length) html += `<section><h4>${m.name}</h4>${rows.join('')}</section>`; });
  html += '</div><div class="hint" style="margin-top:10px">Mouse: drag to select, click for the cursor, Shift click to extend, wheel to zoom, Alt wheel to scroll, double click selects all. Double click an operation to apply it.</div>';
  modal('Keyboard shortcuts', html, [['Close', null, 'on']], m => { m.style.width = 'min(980px, 100%)'; });
}

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
  if (rec.ring && state.recOn && !state.counting && !rec.isRecording() && !rec.isArmed()) { state.recOn = false; syncRec(); }
  if (rec.isRecording() && rec.recordedFrames() / rec.sr > 1200) stopRecording(); if (rec.ring && (rec.isRecording() || rec.isArmed() || state.counting) && n % 6 === 0) syncRec();
  if (state.recOn && rec.isRecording() === false && !rec.isArmed()) { /* stopped by the engine */ }
  drawLive(); drawWave(); drawOvw(); if (n % 2 === 0 || state.playing) drawSpec(); drawMeter(); if (n % 4 === 0) drawReadout();
}

// ---------- start ----------
try { const s = JSON.parse(store.get('mc.set') || '{}'); tipOn = s.tipOn !== false; powerSave = !!s.powerSave; showSpec = s.showSpec === true; showHits = s.showHits !== false; } catch (e) {}
// ---------- icon buttons: icons, tips with shortcuts, states ----------
function keyOfLabel(label) { let k = ''; const walk = items => items.forEach(it => { if (it === '-' || k) return; if (it.l === label && it.k) k = it.k; else if (it.sub) walk(it.sub); }); MENUS.forEach(m => walk(m.items)); return k; }
const TB = [['bLoop', 'loop', 'Loop', 'Repeats the playback again and again.', 'Loop'], ['bStop', 'stop', 'Stop', 'Stops playing and recording.', 'Play or stop'], ['bNew', 'new', 'New empty sound', 'Starts with an empty sound. Undo brings the old one back.', 'New empty sound'], ['bOpen', 'open', 'Open file', 'Opens an audio file. You can also drop a file on the display.', 'Open file...'], ['bSave', 'save', 'Save as WAV', 'Saves the sound as a WAV file. In this web mockup the browser may block the download.', 'Save as WAV'],
  ['bUndo', 'undo', 'Undo', 'Takes back the last change.', 'Undo'], ['bRedo', 'redo', 'Redo', 'Puts back what Undo took away.', 'Redo'], ['bCut', 'cut', 'Cut', 'Moves the selection to the clipboard and closes the gap.', 'Cut'], ['bCopy', 'copy', 'Copy', 'Copies the selection to the clipboard.', 'Copy'], ['bPaste', 'paste', 'Paste', 'Puts the clipboard at the cursor, or over the start of the selection.', 'Paste'], ['bDel', 'del', 'Delete', 'Deletes the selection and closes the gap.', 'Delete selection'], ['bCrop', 'crop', 'Crop', 'Keeps only the selection and throws the rest away.', 'Crop to selection'],
  ['bAll', 'all', 'Select all', 'Selects the whole sound.', 'All'], ['bSnap', 'snap', 'Snap to zero', 'Moves the selection edges to the nearest zero crossing, so cuts do not click.', 'Snap edges to zero crossings'], ['bZin', 'zin', 'Zoom in', 'Shows less of the sound, in more detail.', 'Zoom in'], ['bZout', 'zout', 'Zoom out', 'Shows more of the sound.', 'Zoom out'], ['bFit', 'fit', 'Show everything', 'Fits the whole sound in the display.', 'Show the whole sound'], ['bSel', 'tosel', 'Zoom to selection', 'Fills the display with the selection.', 'Zoom to the selection'],
  ['bGrab', 'grab', 'Grab the last seconds', 'Keeps what you already heard, even though you were not recording. Uses the pre-roll length.', 'Grab the last seconds']];
function decorateIcons() { $('#bRepeat').innerHTML = svgOf('repeat'); TB.forEach(([id, ic, name, desc, lab]) => { const b = $('#' + id); b.innerHTML = svgOf(ic); setTip(b, name, desc, keyOfLabel(lab)); }); syncIcons(); }
const TAKES = ['Replace', 'Append', 'Punch', 'Insert'];
function syncIcons() {
  const m = $('#bMono'), i = $('#bInput'), k = $('#bMon'), sp = $('#bSpec'); if (!m) return;
  m.innerHTML = svgOf(state.recMono ? 'mono' : 'stereo'); m.classList.toggle('on', state.recMono); setTip(m, state.recMono ? 'Recording in mono' : 'Recording in stereo', 'What a new take is made of. Click to switch. The display shows one lane for a mono sound and two for a stereo one. Operations like Make mono change a sound you already have.', keyOfLabel('Record in mono'));
  i.innerHTML = svgOf(state.input === 'mic' ? 'mic' : 'mixer'); setTip(i, state.input === 'mic' ? 'Input: microphone' : 'Input: mixer channel', 'Where the recording comes from. In the plugin: the mixer channel it sits on. Here: a demo loop stands in, or your microphone. Click to switch.', keyOfLabel('Use the microphone instead of the mixer channel'));
  k.innerHTML = svgOf(state.monitor ? 'spk' : 'spkoff'); k.classList.toggle('on', state.monitor); k.classList.toggle('g', state.monitor); setTip(k, state.monitor ? 'Monitoring the input' : 'Input muted', 'Lets you hear what comes in while you record. Mute it when the sound already plays somewhere else.', keyOfLabel('Monitor the input'));
  sp.innerHTML = svgOf('spec'); sp.classList.toggle('on', showSpec); sp.classList.toggle('g', showSpec); setTip(sp, 'Spectral view', 'Shows the sound as a picture of its frequencies, with tools to cut, erase, copy and amplify parts of it.', keyOfLabel('Show the spectral view'));
  const tl = $('#bTake'); if (tl) tl.textContent = TAKES[state.takeMode];
}
function toggleSpec() { showSpec = !showSpec; saveSet(); syncSpecView(); state.ver++; syncIcons(); }
$('#bNew').addEventListener('click', newSound); $('#bOpen').addEventListener('click', () => $('#file').click()); $('#bSave').addEventListener('click', saveWav);
$('#bStop').addEventListener('click', () => { stopPlay(); if (state.counting) cancelCount(); else if (rec.isRecording() || rec.isArmed()) stopRecording(); });
$('#bTake').addEventListener('click', () => { state.takeMode = (state.takeMode + 1) % 4; syncSegs(); });
$('#bMono').addEventListener('click', () => { state.recMono = !state.recMono; syncIcons(); });
$('#bInput').addEventListener('click', async () => { const v = state.input === 'demo' ? 'mic' : 'demo'; $('#inSel').value = v; await chooseInput(v); syncIcons(); });
$('#bMon').addEventListener('click', () => setMonitor(!state.monitor)); $('#bSpec').addEventListener('click', toggleSpec);
buildOps(); buildRec(); selectOp(0); decorateIcons(); syncSegs(); markTool('mouse'); viewFit(); changed(); syncPlay(); syncRec();
const z0 = parseFloat(store.get('mc.zoom')); zoom = z0 > 0 ? clamp(z0, .4, 3) : 1; layoutScale();
window.__mc = { ed, rec, state, opP, MC, OPS };
requestAnimationFrame(frame);
})();
