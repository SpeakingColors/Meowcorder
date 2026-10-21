(function () {
'use strict';
const $ = (s, r) => (r || document).querySelector(s), $$ = (s, r) => Array.from((r || document).querySelectorAll(s));
const store = { get(k) { try { return localStorage.getItem(k); } catch (e) { return null; } }, set(k, v) { try { localStorage.setItem(k, v); } catch (e) {} } };
const clamp = (v, a, b) => v < a ? a : (v > b ? b : v);
const OPS = window.MC_OPS, MC = window.MC;
OPS.forEach(o => o.params.forEach(d => { d.choices = d.choices || []; d.log = !d.choice && d.lo > 0 && d.hi / d.lo >= 40; }));

// ---------- icons (20 px, stroke) ----------
const IC = { hist: '<circle cx="10" cy="10" r="6.500"/><path d="M10 6v4l3 2"/>', layers: '<path d="M10 3l7 4-7 4-7-4z"/><path d="M3 11l7 4 7-4"/>', pencil: '<path d="M4 16l1-4 8-8 3 3-8 8z"/><path d="M12 5l3 3"/>',
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
let tipOn = true, powerSave = false, showHits = true;
const ed = new MC.Editor(OPS), rec = new MC.Recorder();
const state = { monitor: true, recMono: false, tool: 'mouse', sreg: null, amp: 6, brush: 12, bars: 1, click: 1, counting: false, bits: 24, scale: 1, ver: 0, viewA: 0, viewB: 1, playing: false, playA: 0, playN: 0, playT0: 0, loop: false, curOp: 0, takeMode: 1, startMode: 2, recOn: false, input: 'demo', lvl: 0, lvlHold: 0, lvlT: 0, drag: null, fname: 'demo loop, not saved' };
const opP = OPS.map(o => o.params.map(d => d.def)), recP = { thresh: -40, pre: 200, bpm: 120 };

Object.assign(state, { layers: { wave: true, spec: false, pitch: false }, combine: false, layout: 'overlay', front: 'wave', unit: 'time', scope: 'sel', mix: 100, ms: false, dither: true, aver: 0, bpmAuto: 0, stopPos: 0, pausePos: -1, playRatio: 1, demoKind: 'loop', rep: null, hits: [], pHover: null });
const warp = { sync: false, mode: 0, formant: 1, bpm: 0, semis: 0, cents: 0, fshift: 0, scope: 'sel' };
const sampleBpm = () => warp.bpm || state.bpmAuto || 0;
const toast = (msg, ms) => { let t = $('#toast'); if (!t) { t = document.createElement('div'); t.id = 'toast'; t.className = 'toast'; document.body.appendChild(t); } t.textContent = msg; t.hidden = false; clearTimeout(toast.t); toast.t = setTimeout(() => { t.hidden = true; }, ms || 3500); };

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
// a sung-like melody, so the pitch view has something clean to show
function makeMelody(sr) {
  const len = Math.floor(4 * sr), a = new MC.Audio(sr, len), seq = [57, 60, 64, 62, 60, 59, 57, 64]; let ph = 0;
  for (let n = 0; n < 8; ++n) {
    const t0 = Math.floor(n * .5 * sr), L = Math.floor(.46 * sr), f0 = 440 * Math.pow(2, (seq[n] - 69) / 12);
    for (let i = 0; i < L && t0 + i < len; ++i) {
      const t = i / sr, f = f0 * (1 + .006 * Math.sin(2 * Math.PI * 5.5 * t) * Math.min(1, t * 4)); ph += 2 * Math.PI * f / sr; let s = 0;
      for (let h = 1; h <= 16; ++h) { const hf = f * h; if (hf > sr * .45) break; s += Math.sin(h * ph) / h * (Math.exp(-Math.pow((hf - 800) / 500, 2)) + .6 * Math.exp(-Math.pow((hf - 2400) / 700, 2)) + .06); }
      a.l[t0 + i] = a.r[t0 + i] = s * Math.min(1, t / .02) * Math.min(1, (L - i) / (.05 * sr));
    }
  }
  let pk = 0; for (let i = 0; i < len; ++i) pk = Math.max(pk, Math.abs(a.l[i])); const g = pk > 0 ? .8 / pk : 1; for (let i = 0; i < len; ++i) { a.l[i] *= g; a.r[i] *= g; } return a;
}

let melody = null;
const demoFor = kind => kind === 'melody' ? (melody || (melody = makeMelody(ctx ? ctx.sampleRate : SR))) : demo;
function startInput() {
  if (!ctx) return; try { if (demoSrc) demoSrc.stop(); } catch (e) {} demoSrc = null; if (micNode) { try { micNode.disconnect(); } catch (e) {} micNode = null; }
  if (state.input === 'demo') {
    const dm = demoFor(state.demoKind), b = ctx.createBuffer(2, dm.frames, dm.sr); b.copyToChannel(dm.l, 0); b.copyToChannel(dm.r, 1); demoSrc = ctx.createBufferSource(); demoSrc.buffer = b; demoSrc.loop = true; demoSrc.connect(inBus);
    demoSrc.connect(monNode); demoSrc.start();
  } else if (micStream) { micNode = ctx.createMediaStreamSource(micStream); micNode.connect(inBus); micNode.connect(monNode); }
}
async function chooseInput(v) {
  if (v === 'mic') { state.input = 'mic'; if (!micStream) { try { micStream = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } }); } catch (e) { state.input = 'demo'; toast('Microphone not available in this viewer. Using the demo loop. In your own browser tab it works.', 5000); } } }
  else { state.input = 'demo'; state.demoKind = v === 'melody' ? 'melody' : 'loop'; }
  $('#inSel').value = state.input; setMonitor(state.input === 'demo'); startInput(); syncIcons();
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
const PLAYSTART = [['cursor', 'From the cursor, or the start of the selection'], ['start', 'From the very start'], ['resume', 'From where it stopped last']], PLAYAGAIN = [['stop', 'Stops'], ['pause', 'Pauses, the next press continues'], ['restart', 'Starts over']];
const play = { button: ['cursor', 'stop'], space: ['resume', 'pause'], shift: ['start', 'restart'] };
function playRange() { const a = ed.hasSelection() ? ed.selA : Math.min(ed.cursor, Math.max(0, ed.audio.frames - 1)), b = ed.hasSelection() ? ed.selB : ed.audio.frames; return [a, b]; }
function playAudio(au, from, loop, ratio) {
  stopPlay(); if (!ctx || ctx.state !== 'running') { powerOn().then(() => playAudio(au, from, loop, ratio)); return; } if (!au.frames) return;
  const b = ctx.createBuffer(2, au.frames, au.sr); b.copyToChannel(au.l, 0); b.copyToChannel(au.r, 1); playSrc = ctx.createBufferSource(); playSrc.buffer = b; playSrc.loop = loop; playSrc.connect(bus);
  const s = playSrc; playSrc.onended = () => { if (playSrc === s) { state.playing = false; state.stopPos = 0; syncPlay(); } }; playSrc.start(); state.playing = true; state.playA = from; state.playN = au.frames; state.playT0 = ctx.currentTime; state.playSr = au.sr; state.playRatio = ratio || 1; syncPlay();
}
const warpActive = () => (warp.sync && sampleBpm() > 0) || !!warp.semis || !!warp.cents;
function warpOpts(a, b) { const sb = sampleBpm(); return { ratio: warp.sync && sb > 0 ? recP.bpm / sb : 1, mode: warp.mode, semis: warp.semis, cents: warp.cents, formant: warp.formant, fshift: warp.fshift, hits: (state.hits || []).filter(h => h > a && h < b).map(h => h - a) }; }
let warpCache = { key: '', au: null };
function warpedSlice(a, b) { const seg = ed.audio.slice(a, b), o = warpOpts(a, b), key = state.aver + ':' + a + ':' + b + ':' + JSON.stringify(o); if (warpCache.key !== key) warpCache = { key, au: MC.warpAudio(seg, o), ratio: o.ratio }; return warpCache; }
function startPlayRange(a, b, loop, force) {
  if (b <= a) return; if (force || warpActive()) { const w = warpedSlice(a, b); playAudio(w.au, a, loop, w.ratio); } else playAudio(ed.audio.slice(a, b), a, loop, 1);
}
function startFrom(rule) {
  const n = ed.audio.frames; if (!n) return; let a, b = n;
  if (rule === 'start') a = 0; else if (rule === 'resume' && state.stopPos > 0 && state.stopPos < n) a = state.stopPos; else if (ed.hasSelection()) { a = ed.selA; b = ed.selB; } else a = Math.min(ed.cursor, n - 1);
  startPlayRange(a, b, state.loop);
}
function playTrigger(which) {
  const [rule, again] = play[which];
  if (state.playing) { state.stopPos = Math.max(0, playheadFrame()); if (again === 'restart') { startFrom(rule); return; } stopPlay(); state.pausePos = again === 'pause' ? state.stopPos : -1; return; }
  if (again === 'pause' && state.pausePos >= 0 && state.pausePos < ed.audio.frames) { const p = state.pausePos; state.pausePos = -1; startPlayRange(p, ed.audio.frames, state.loop); return; }
  state.pausePos = -1; startFrom(rule);
}
function stopPlay() { if (playSrc) { try { playSrc.onended = null; playSrc.stop(); } catch (e) {} playSrc = null; } state.playing = false; syncPlay(); }
const ruleText = which => { const [s, ag] = play[which]; return PLAYSTART.find(x => x[0] === s)[1].toLowerCase() + '. Pressed again while playing: ' + PLAYAGAIN.find(x => x[0] === ag)[1].toLowerCase() + '.'; };
function syncPlay() { const b = $('#bPlay'); b.classList.toggle('on', state.playing); b.innerHTML = svgOf(state.playing ? 'stop' : 'play'); setTip(b, state.playing ? 'Stop' : 'Play', 'Plays ' + ruleText('button') + ' Change this in Settings.', ''); }
const playheadFrame = () => { if (!state.playing || !ctx) return -1; const el = (ctx.currentTime - state.playT0) * state.playSr, pos = state.loop ? el % state.playN : Math.min(el, state.playN); return state.playA + pos * state.playRatio; };

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
// ---------- timing: units, grid, ruler ----------
const gridBpmV = () => sampleBpm() || recP.bpm, gridBeat = () => { const b = gridBpmV(); return b > 0 ? (ed.audio.sr || 48000) * 60 / b : 0; };
const UNITS = [['time', 'Time (minutes:seconds)'], ['sec', 'Seconds'], ['smp', 'Samples'], ['bars', 'Bars and beats (4/4)']];
function fmtClock(t, dec) { const m = Math.floor(t / 60), s = t - m * 60; return m + ':' + (s < 10 ? '0' : '') + s.toFixed(dec); }
function niceStep(x) { const p = Math.pow(10, Math.floor(Math.log10(Math.max(1e-9, x)))); for (const m of [1, 2, 5, 10]) if (m * p >= x) return m * p; return 10 * p; }
function fmtPos(f) {
  const sr = ed.audio.sr || 48000; switch (state.unit) {
    case 'smp': return String(Math.round(f)); case 'sec': return (f / sr).toFixed(3) + ' s';
    case 'bars': { const bt = gridBeat(); if (!bt) return fmtClock(f / sr, 3); const b = f / bt, bar = Math.floor(b / 4) + 1, rem = b % 4; return bar + '.' + (Math.floor(rem) + 1) + '.' + String(Math.floor((rem % 1) * 960)).padStart(3, '0'); }
    default: return fmtClock(f / sr, 3);
  }
}
function fmtLen(f) { const sr = ed.audio.sr || 48000; switch (state.unit) { case 'smp': return Math.round(f) + ' smp'; case 'sec': return (f / sr).toFixed(3) + ' s'; case 'bars': { const bt = gridBeat(); return bt ? (f / bt / 4).toFixed(2) + ' bars' : fmtClock(f / sr, 3); } default: return fmtClock(f / sr, 3); } }
function rulerTicks(w) {
  const a = ed.audio, sr = a.sr || 48000, A = state.viewA, B = state.viewB, out = [], u = state.unit;
  if (u === 'smp') { const st = Math.max(1, niceStep((B - A) / (w / 80))); for (let f = Math.ceil(A / st) * st; f < B; f += st) out.push({ f, label: String(Math.round(f)), major: true }); }
  else if (u === 'bars' && gridBeat() > 0) {
    const bt = gridBeat(), want = (B - A) / bt / (w / 64), steps = [.0625, .125, .25, .5, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024], st = steps.find(s => s >= want) || 1024;
    for (let b = Math.ceil(A / bt / st) * st; b * bt < B; b += st) { const bar = Math.floor(b / 4 + 1e-9) + 1, rem = b % 4, whole = Math.abs(rem - Math.round(rem)) < 1e-6; out.push({ f: b * bt, label: rem < 1e-6 ? String(bar) : (whole ? bar + '.' + (Math.round(rem) + 1) : bar + '.' + (Math.floor(rem) + 1) + '.' + Math.round((rem % 1) * 960)), major: rem < 1e-6 }); }
  } else { const st = fmtTicks(B - A, sr, w), dec = st < .01 ? 3 : (st < 1 ? 2 : 0), t0 = Math.ceil(A / sr / st) * st; for (let t = t0; t * sr < B; t += st) out.push({ f: t * sr, label: u === 'sec' ? t.toFixed(dec) + 's' : fmtClock(t, dec), major: true }); }
  return out;
}
function drawRuler() {
  const c = $('#ruler'), { g, w, h } = prep(c); g.fillStyle = '#101216'; g.fillRect(0, 0, w, h); if (!ed.audio.frames) return; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left';
  for (const t of rulerTicks(w)) { const x = Math.round(frameToX(t.f, w)); g.fillStyle = t.major ? '#8d92a0' : '#555a66'; g.fillRect(x, 0, 1, t.major ? 8 : 5); g.fillStyle = dim; g.fillText(t.label, x + 3, 17); }
  const ph = playheadFrame(); if (ph >= 0) { g.fillStyle = '#FF1383'; g.fillRect(Math.round(frameToX(ph, w)), 0, 2, h); }
}
function drawGrid(g, w, h) {
  if (state.unit !== 'bars') return; const bt = gridBeat(); if (!bt) return; const px = bt / (state.viewB - state.viewA) * w; if (px < 5) return;
  for (let b = Math.ceil(state.viewA / bt); b * bt < state.viewB; ++b) { const bar = b % 4 === 0; if (!bar && px < 12) continue; g.fillStyle = bar ? '#ffffff30' : '#ffffff12'; g.fillRect(Math.round(frameToX(b * bt, w)), 0, 1, h); }
}

// ---------- display layers: waveform, spectral, pitch ----------
const LAYERS = ['wave', 'spec', 'pitch'], LCV = { wave: $('#wave'), spec: $('#spec'), pitch: $('#pitch') }, VNAMES = { wave: 'Waveform', spec: 'Spectral', pitch: 'Pitch' };
const visLayers = () => LAYERS.filter(l => state.layers[l]);
function frontLayer() { const v = visLayers(); return v.length === 1 || !v.includes(state.front) ? v[0] : state.front; }
function isBottom(l) { const v = visLayers(), f = frontLayer(); if (state.layout === 'split' || v.length === 1) return true; return v.filter(x => x !== f).concat([f])[0] === l; }
function layoutDV() {
  const dv = $('#dv'), H = dv.clientHeight || 470, v = visLayers(), f = frontLayer(), split = state.layout === 'split' && v.length > 1; state.splitNow = split;
  LAYERS.forEach(l => { LCV[l].hidden = !state.layers[l]; });
  v.forEach((l, i) => { const c = LCV[l]; if (split) { c.style.top = Math.round(H / v.length * i) + 'px'; c.style.height = Math.round(H / v.length) + 'px'; c.style.zIndex = 1; c.classList.add('act'); } else { c.style.top = '0px'; c.style.height = H + 'px'; c.style.zIndex = l === f ? 5 : 1 + LAYERS.indexOf(l); c.classList.toggle('act', l === f); } });
  $('#spectools').hidden = !(state.layers.spec && (split || f === 'spec')); $('#pitchtools').hidden = !(state.layers.pitch && (split || f === 'pitch')); $('#viewtools').hidden = $('#spectools').hidden && $('#pitchtools').hidden;
  syncViewbar(); try { layoutScale(); } catch (e) {}
}
function afterView() { layoutDV(); syncIcons(); saveSet(); state.ver++; }
function setLayers(on, front) { LAYERS.forEach(l => { state.layers[l] = !!on[l]; }); state.front = front || visLayers()[0] || 'wave'; if (!visLayers().length) { state.layers.wave = true; state.front = 'wave'; } afterView(); }
function toggleLayer(l) {
  if (state.combine) { state.layers[l] = !state.layers[l]; if (!visLayers().length) state.layers[l] = true; if (state.layers[l]) state.front = l; else if (state.front === l) state.front = visLayers()[0]; afterView(); }
  else setLayers({ [l]: true }, l);
}
function showLayer(l) { if (state.layers[l] && frontLayer() === l) return; if (state.combine) { state.layers[l] = true; state.front = l; afterView(); } else setLayers({ [l]: true }, l); }
function setCombine(v) { state.combine = v; if (!v) { const f = frontLayer(); setLayers({ [f]: true }, f); } else afterView(); }
function buildViewbar() {
  const vb = $('#viewbar'); vb.innerHTML = '<label>View</label><div class="seg" id="vSeg"></div><button class="ib" id="vComb"></button><span id="vCombo" style="display:contents"><label>Layout</label><div class="seg" id="vLay"><button data-v="overlay">Overlay</button><button data-v="split">Split</button></div><label>In front</label><div class="seg" id="vFront"></div></span><button class="ib" id="vPen"></button><span class="hint" id="vHint2"></span><span class="sp"></span><span class="hint" id="vHint"></span>';
  LAYERS.forEach(l => { const b = document.createElement('button'); b.dataset.l = l; b.textContent = VNAMES[l]; b.addEventListener('click', () => toggleLayer(l)); $('#vSeg').appendChild(b); const f = document.createElement('button'); f.dataset.l = l; f.textContent = VNAMES[l]; f.addEventListener('click', () => { state.front = l; afterView(); }); $('#vFront').appendChild(f); });
  $('#vComb').innerHTML = svgOf('layers'); $('#vComb').addEventListener('click', () => setCombine(!state.combine));
  $$('#vLay button').forEach(b => b.addEventListener('click', () => { state.layout = b.dataset.v; afterView(); }));
}
function syncViewbar() {
  const f = frontLayer(), tips = { wave: ['Waveform', 'Shows the sound as a wave. Drag to select a time range. The usual way to cut, copy and process.', 'Alt+W'], spec: ['Spectral view', 'Shows the sound as a picture of its frequencies, with tools to cut, erase, copy and amplify parts of it.', 'Alt+G'], pitch: ['Pitch view', 'A piano roll of the notes found in the sound. Drag a note up or down to change its pitch, and the sound follows.', 'Alt+K'] };
  $$('#vSeg button').forEach(b => { const l = b.dataset.l; b.classList.toggle('on', !!state.layers[l]); setTip(b, tips[l][0], tips[l][1] + (state.combine ? ' Combine is on: click to add or remove it.' : ' Click to show it alone.'), tips[l][2]); });
  const cb = $('#vComb'); cb.classList.toggle('on', state.combine); setTip(cb, 'Show views together', 'Turn on to see more than one view at once. Overlay puts them on top of each other, Split stacks them. The view In front is the one your tools work on.', keyOfLabel('Show views together'));
  $('#vCombo').style.display = state.combine ? 'contents' : 'none'; $$('#vLay button').forEach(b => b.classList.toggle('on', b.dataset.v === state.layout)); $$('#vFront button').forEach(b => { b.hidden = !state.layers[b.dataset.l]; b.classList.toggle('on', b.dataset.l === f); });
  $('#vHint').textContent = state.combine ? 'Tools work on: ' + VNAMES[f] : '';
}
// shared marks: selection or cursor, play head
function drawMarks(g, w, h) {
  if (ed.hasSelection()) { const x0 = frameToX(ed.selA, w), x1 = frameToX(ed.selB, w); g.fillStyle = '#6cc9f533'; g.fillRect(x0, 0, x1 - x0, h); g.fillStyle = '#6cc9f5'; g.fillRect(x0, 0, 1, h); g.fillRect(x1 - 1, 0, 1, h); }
  else { const x = frameToX(ed.cursor, w); g.fillStyle = '#f4f5f7'; g.fillRect(Math.round(x), 0, 1, h); }
  const ph = playheadFrame(); if (ph >= 0) { g.fillStyle = '#FF1383'; g.fillRect(Math.round(frameToX(ph, w)), 0, 2, h); }
}
function drawWave(opaque) {
  const c = LCV.wave, { g, w, h } = prep(c), a = ed.audio; if (opaque) { g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); } else { g.clearRect(0, 0, w, h); g.fillStyle = 'rgba(8,8,10,.45)'; g.fillRect(0, 0, w, h); }
  if (!a.frames) { g.strokeStyle = gridCol; g.beginPath(); g.moveTo(0, h / 2 + .5); g.lineTo(w, h / 2 + .5); g.stroke(); g.fillStyle = dim; g.font = '14px Barlow Condensed, sans-serif'; g.textAlign = 'center'; g.fillText('Nothing here yet. Record, open a file or drop one here.', w / 2, h / 2 - 8); return; }
  drawGrid(g, w, h); const mono = isMono(a), lanes = mono ? 1 : 2, lh = h / lanes, cols = Math.floor(w), names = state.ms ? ['M', 'S'] : (mono ? ['M'] : ['L', 'R']);
  for (let ch = 0; ch < lanes; ++ch) {
    const mid = lh * (ch + .5), top = lh * ch; g.strokeStyle = gridCol; g.lineWidth = 1; g.beginPath(); g.moveTo(0, mid + .5); g.lineTo(w, mid + .5); g.stroke();
    if (ch) { g.strokeStyle = '#3a3d46'; g.beginPath(); g.moveTo(0, top + .5); g.lineTo(w, top + .5); g.stroke(); }
    const pk = MC.peakOverviewCh(a, ch, Math.floor(state.viewA), Math.ceil(state.viewB), cols); g.fillStyle = ch ? '#02D9C0' : '#02EBBA'; g.globalAlpha = .92;
    for (let x = 0; x < pk.mx.length; ++x) { const y0 = mid - pk.mx[x] * lh / 2 * .96, y1 = mid - pk.mn[x] * lh / 2 * .96; g.fillRect(x, y0, 1, Math.max(1, y1 - y0)); } g.globalAlpha = 1;
    if ((state.viewB - state.viewA) * 4 <= w) { const arr = ch ? a.r : a.l, f0 = Math.max(0, Math.floor(state.viewA)), f1 = Math.min(a.frames - 1, Math.ceil(state.viewB)); g.strokeStyle = '#ffffff'; g.fillStyle = '#ffffff'; g.lineWidth = 1; g.beginPath(); for (let f = f0; f <= f1; ++f) { const x = frameToX(f + .5, w), y = mid - arr[f] * lh / 2 * .96; if (f === f0) g.moveTo(x, y); else g.lineTo(x, y); } g.stroke(); for (let f = f0; f <= f1; ++f) { const x = frameToX(f + .5, w), y = mid - arr[f] * lh / 2 * .96; g.fillRect(x - 1.5, y - 1.5, 3, 3); } }
    g.fillStyle = '#ffffff55'; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; g.fillText(names[ch], 4, top + 12);
  }
  if (showHits) { g.fillStyle = '#FF9000'; for (const f of state.hits || []) { if (f < state.viewA || f > state.viewB) continue; g.fillRect(Math.round(frameToX(f, w)), 0, 1, 12); } }
  drawMarks(g, w, h);
}
let specKey = '', specCv = null;
function drawSpec(opaque) {
  const c = LCV.spec, { g, w, h } = prep(c), a = ed.audio; g.clearRect(0, 0, w, h); if (opaque) { g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); } if (!a.frames) return;
  const key = state.ver + '|' + Math.round(state.viewA) + '|' + Math.round(state.viewB) + '|' + Math.round(w) + '|' + Math.round(h);
  if (key !== specKey || !specCv) {
    specKey = key; const W = Math.max(1, Math.floor(w)), H = Math.max(1, Math.floor(h)), from = Math.floor(state.viewA), to = Math.ceil(state.viewB), hop = Math.max(64, Math.floor((to - from) / W)), sp = MC.spectrogram(a, from, to, 1024, hop);
    const img = new ImageData(W, H), fMax = a.sr / 2, row = new Int32Array(H); for (let y = 0; y < H; ++y) { const f = 20 * Math.pow(fMax / 20, 1 - y / (H - 1 || 1)); row[y] = clamp(Math.round(f / fMax * (sp.bins - 1)), 0, sp.bins - 1); }
    for (let x = 0; x < W; ++x) { const fr = clamp(Math.floor(x / W * sp.frames), 0, sp.frames - 1); for (let y = 0; y < H; ++y) { const db = sp.db[fr * sp.bins + row[y]], p = PAL[clamp(Math.round((db + 90) / 90 * 255), 0, 255)], o = (y * W + x) * 4; img.data[o] = p[0]; img.data[o + 1] = p[1]; img.data[o + 2] = p[2]; img.data[o + 3] = 255; } }
    if (!specCv) specCv = document.createElement('canvas'); specCv.width = W; specCv.height = H; specCv.getContext('2d').putImageData(img, 0, 0);
  }
  g.save(); g.globalAlpha = opaque ? 1 : .62; g.drawImage(specCv, 0, 0, w, h); g.restore();
  g.fillStyle = dim; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; const fMax = a.sr / 2; for (const f of [100, 1000, 10000]) { if (f > fMax) continue; const y = (1 - Math.log(f / 20) / Math.log(fMax / 20)) * (h - 1); g.fillStyle = '#ffffff22'; g.fillRect(0, Math.round(y), w, 1); g.fillStyle = '#c8ccd8'; g.fillText(f >= 1000 ? f / 1000 + 'k' : f, 3, y - 2); }
  drawMarks(g, w, h); drawSpecOverlay(g, w, h);
}
// ---------- pitch view (piano roll) ----------
const NN = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'], noteName = m => NN[((Math.round(m) % 12) + 12) % 12] + (Math.floor(Math.round(m) / 12) - 1);
let pitchKey = '', pitchData = { notes: [], track: null }, pitchLo = 48, pitchHi = 72;
function pitchInfo() {
  const a = ed.audio, key = state.aver + ':' + a.frames; if (key !== pitchKey) {
    pitchKey = key; pitchData = a.frames > 4096 ? MC.detectNotes(a) : { notes: [], track: null }; let lo = 1e9, hi = -1e9; pitchData.notes.forEach(n => { lo = Math.min(lo, n.m); hi = Math.max(hi, n.m); }); if (lo > hi) { lo = 55; hi = 67; }
    lo = Math.floor(lo) - 3; hi = Math.ceil(hi) + 3; while (hi - lo < 23) { lo--; hi++; } pitchLo = clamp(lo, 12, 100); pitchHi = clamp(hi, pitchLo + 12, 120);
  } return pitchData;
}
const pitchGeom = h => { const rows = pitchHi - pitchLo + 1, rh = h / rows; return { rows, rh, y: m => h - (m - pitchLo + .5) * rh }; };
function drawPitch(opaque) {
  const c = LCV.pitch, { g, w, h } = prep(c), a = ed.audio; if (opaque) { g.fillStyle = '#0a0a0c'; g.fillRect(0, 0, w, h); } else g.clearRect(0, 0, w, h); if (!a.frames) return;
  const P = pitchInfo(), G = pitchGeom(h);
  for (let m = pitchLo; m <= pitchHi; ++m) { const pc = m % 12, black = [1, 3, 6, 8, 10].includes(pc), y = G.y(m) - G.rh / 2; g.fillStyle = black ? (opaque ? '#0d0e12' : 'rgba(0,0,0,.28)') : (opaque ? '#13151a' : 'rgba(255,255,255,.035)'); g.fillRect(0, y, w, G.rh); if (pc === 0) { g.fillStyle = '#ffffff2a'; g.fillRect(0, y + G.rh - 1, w, 1); } }
  drawGrid(g, w, h); g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; for (let m = pitchLo; m <= pitchHi; ++m) if (m % 12 === 0 || G.rh >= 13) { g.fillStyle = m % 12 === 0 ? '#c8ccd8' : '#6b6f7c'; g.fillText(noteName(m), 4, G.y(m) + 3); }
  const tr = P.track; if (tr) { g.strokeStyle = '#FFD100cc'; g.lineWidth = 1.2; g.beginPath(); let pen = false, pt = -1e9; for (let i = 0; i < tr.midi.length; ++i) { const m = tr.midi[i]; if (m !== m || m < pitchLo - 2 || m > pitchHi + 2) { pen = false; continue; } const x = frameToX(tr.t[i], w), y = G.y(m); if (!pen || tr.t[i] - pt > tr.hop * 2.5) g.moveTo(x, y); else g.lineTo(x, y); pen = true; pt = tr.t[i]; } g.stroke(); }
  const d = state.drag && state.drag.pitch ? state.drag : null;
  P.notes.forEach(n => { const x0 = frameToX(n.t0, w), x1 = frameToX(n.t1, w); if (x1 < 0 || x0 > w) return; const y = G.y(n.m), bw = Math.max(3, x1 - x0), hot = state.pHover === n || (d && d.note === n), bh = G.rh * .78;
    g.fillStyle = hot ? 'rgba(255,90,168,.85)' : 'rgba(255,19,131,.66)'; g.fillRect(x0, y - bh / 2, bw, bh); g.strokeStyle = '#ff7ab8'; g.lineWidth = 1; g.strokeRect(x0 + .5, y - bh / 2 + .5, bw - 1, bh - 1);
    if (bw > 38 && bh > 9) { const ct = Math.round((n.m - Math.round(n.m)) * 100); g.fillStyle = '#fff'; g.fillText(noteName(n.m) + (ct ? ' ' + (ct > 0 ? '+' : '') + ct : ''), x0 + 4, y + 3); } });
  if (d && d.moved) { const n = d.note, x0 = frameToX(n.t0, w), x1 = frameToX(n.t1, w), y = G.y(n.m + d.dm), bh = G.rh * .78; g.setLineDash([4, 3]); g.strokeStyle = '#02EBBA'; g.lineWidth = 1.5; g.strokeRect(x0 + .5, y - bh / 2, Math.max(3, x1 - x0), bh); g.setLineDash([]); g.fillStyle = '#02EBBA'; g.fillText(noteName(n.m) + ' to ' + noteName(n.m + d.dm) + '  (' + (d.dm > 0 ? '+' : '') + d.dm.toFixed(2) + ' st)', x0 + 4, y - bh / 2 - 4); }
  drawMarks(g, w, h);
}
function drawReadout() {
  const a = ed.audio, sel = ed.hasSelection(); $('#readout').innerHTML = `<span>Length <b>${fmtLen(a.frames)}</b></span><span>Selection <b>${sel ? fmtPos(ed.selA) + ' to ' + fmtPos(ed.selB) + ' (' + fmtLen(ed.selB - ed.selA) + ')' : 'none, operations use everything'}</b></span><span>Cursor <b>${fmtPos(ed.cursor)}</b></span><span>Peak <b>${a.frames ? MC.linToDb(a.peak(sel ? ed.selA : 0, sel ? ed.selB : a.frames)).toFixed(1) + ' dBFS' : '-'}</b></span><span>${rec.isRecording() ? '<b style="color:var(--pink)">REC ' + fmtTime(rec.recordedFrames(), rec.sr) + '</b>' : ''}</span>`;
}

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

const specC = $('#spec'), TOOLHINT = { mouse: 'Click for the cursor, drag to select a time range.', select: 'Drag a box over time and frequency. Use Cutter, Amp or Copy on the box from the Spectral menu.', scissors: 'Drag across time: that slice is cut out and the gap closes.', cutter: 'Drag a box: the sound inside it is removed, the length stays.', eraser: 'Paint over what should go. It is removed when you let go.', copy: 'Drag a box to copy that piece, then click with Paste where it should land.', paste: 'Click where the copied piece should go. Left edge is time, height is frequency.', amp: 'Drag a box: it gets the level change set next to the tools.', pencil: 'Draw a line on the picture: a pure tone follows your pen, so you can draw new sounds. Drag forward in time. The Pen level is set next to the tools. Click for a short beep.' };
const TOOLS = [['mouse', 'Mouse'], ['select', 'Selector'], ['scissors', 'Scissors'], ['cutter', 'Cutter'], ['eraser', 'Eraser'], ['copy', 'Copy'], ['paste', 'Paste'], ['amp', 'Amp'], ['pencil', 'Pencil']];
const TOOLICON = { mouse: 'mouse', select: 'select', scissors: 'cut', cutter: 'cutter', eraser: 'eraser', copy: 'copy', paste: 'paste', amp: 'amp', pencil: 'pencil' };
TOOLS.forEach(([t, n], i) => { const b = document.createElement('button'); b.dataset.tool = t; b.innerHTML = svgOf(TOOLICON[t]); setTip(b, n, TOOLHINT[t], 'Shift+' + (i + 1)); $('#toolSeg').appendChild(b); });
$$('#toolSeg button').forEach(b => b.addEventListener('click', () => setTool(b.dataset.tool)));
const fMaxOf = () => (ed.audio.sr || 48000) / 2;
const spPt = e => { const r = specC.getBoundingClientRect(); return { x: (e.clientX - r.left) / (r.width || 1) * specC.clientWidth, y: (e.clientY - r.top) / (r.height || 1) * specC.clientHeight }; };
const hzAtY = y => { const h = specC.clientHeight; return 20 * Math.pow(fMaxOf() / 20, 1 - clamp(y, 0, h - 1) / (h - 1)); };
const yAtHz = hz => (1 - Math.log(clamp(hz, 20, fMaxOf()) / 20) / Math.log(fMaxOf() / 20)) * (specC.clientHeight - 1);
const frAtX = x => clamp(xToFrame(x, specC.clientWidth), 0, ed.audio.frames);
function boxOf(a, b) { const f = [frAtX(a.x), frAtX(b.x)], hz = [hzAtY(a.y), hzAtY(b.y)]; return { t0: Math.min(...f), t1: Math.max(...f), f0: Math.min(...hz), f1: Math.max(...hz) }; }
function brushRect(p) { const w = specC.clientWidth, fpp = (state.viewB - state.viewA) / w, r = state.brush, c = xToFrame(p.x, w); return { t0: c - r * fpp, t1: c + r * fpp, f0: hzAtY(p.y + r), f1: hzAtY(p.y - r) }; }
// ---------- editing UI ----------
function changed() { state.ver++; state.aver++; state.hits = MC.detectTransients(ed.audio, .6); state.bpmAuto = MC.detectBpm(ed.audio) || 0; clampView(); syncEdit(); if (!$('#histpop').hidden) buildHist(); if (!$('#warppop').hidden) syncWarp(); }
const markEdited = () => { state.fname = state.fname.replace(/(, edited)?(, not saved)?$/, '') + ', edited, not saved'; };
function syncEdit() {
  const sel = ed.hasSelection(), ul = ed.undoStack.length ? ed.undoStack[ed.undoStack.length - 1].label : '', rl = ed.redoStack.length ? ed.redoStack[ed.redoStack.length - 1].label : '';
  $('#bUndo').disabled = !ed.canUndo(); $('#bRedo').disabled = !ed.canRedo(); $('#bCut').disabled = !sel; $('#bCopy').disabled = !sel; $('#bDel').disabled = !sel; $('#bCrop').disabled = !sel; $('#bSnap').disabled = !sel; $('#bSel').disabled = !sel;
  setTip($('#bUndo'), 'Undo', ul ? 'Takes back: ' + ul + '.' : 'Nothing to undo.', 'Ctrl+Z'); setTip($('#bRedo'), 'Redo', rl ? 'Puts back: ' + rl + '. Ctrl+Shift+Z puts it back, or repeats the last operation when there is nothing to put back.' : 'Nothing to redo. Ctrl+Shift+Z repeats the last operation.', 'Ctrl+Y');
  $('#bPaste').disabled = !ed.clip.frames; $('#bRepeat').disabled = !state.rep; setTip($('#bRepeat'), 'Repeat last operation', state.rep ? 'Does ' + state.rep.label + ' again with the same settings, on the current selection.' : 'Nothing to repeat yet. Apply an operation first.', 'R'); $('#fname').textContent = state.fname;
  $('#opHint').textContent = (sel ? 'Operations work on the selection. ' : 'Operations work on everything (nothing selected). ') + 'Click one for its settings, double click or Shift+letter to apply at once.'; if (typeof syncScope === 'function') syncScope();
}
function runProcess(i, p, scope, mix) { stopPlay(); if (!ed.apply(i, p, { all: scope === 'all', mix: OPS[i].changesLength ? 1 : mix / 100 })) return false; markEdited(); changed(); return true; }
function doOp() { const i = state.curOp, p = opP[i].slice(), mx = state.mix; if (!runProcess(i, p, state.scope, mx)) return; state.rep = { label: OPS[i].name, fn: () => runProcess(i, p, state.scope, mx) }; syncEdit(); }
function repeat() { if (state.rep) state.rep.fn(); }
function redoOrRepeat() { if (ed.canRedo()) { stopPlay(); ed.redo(); changed(); } else repeat(); }
const opRange = () => state.scope === 'all' || !ed.hasSelection() ? [0, ed.audio.frames] : [ed.selA, ed.selB];
function previewOp() {
  const [a, b] = opRange(), c = ed.audio.slice(a, b), mix = OPS[state.curOp].changesLength ? 1 : state.mix / 100, dry = mix < 1 ? c.clone() : null; MC.applyOp(c, state.curOp, 0, c.frames, opP[state.curOp]);
  if (dry && dry.frames === c.frames) for (let i = 0; i < c.frames; ++i) { c.l[i] = dry.l[i] * (1 - mix) + c.l[i] * mix; c.r[i] = dry.r[i] * (1 - mix) + c.r[i] * mix; } playAudio(c, a, false, 1);
}
// popovers: operation settings, history, warp
const POPS = ['#oppop', '#histpop', '#warppop'];
function closePops() { POPS.forEach(s => { $(s).hidden = true; }); }
const anyPop = () => POPS.some(s => !$(s).hidden);
function showPop(sel) { const pop = $(sel), ctl = $('.ctl'); POPS.forEach(s => { if (s !== sel) $(s).hidden = true; }); pop.hidden = false; pop.style.top = (ctl.offsetTop + ctl.offsetHeight + 6) + 'px'; }
function buildScope() {
  const slot = $('#mixSlot'); slot.innerHTML = ''; slot.appendChild(makeKnob({ id: 'mix', name: 'Mix', lo: 0, hi: 100, def: 100, unit: '%', tip: 'How much of the processed sound is mixed in. 100% is fully processed, 0% leaves the sound as it was. Not used by operations that change the length.', choice: false, choices: [] }, 'Level', () => state.mix, v => { state.mix = Math.round(v); }));
  $$('#scopeSeg button').forEach(b => b.addEventListener('click', () => { state.scope = b.dataset.v; syncScope(); }));
}
function syncScope() { $$('#scopeSeg button').forEach(b => b.classList.toggle('on', b.dataset.v === state.scope)); const s = $('#scopeSeg button'); if (s) s.textContent = ed.hasSelection() ? 'Selection' : 'Selection (none)'; }
function selectOp(i, open) {
  state.curOp = i; $$('#oplist button').forEach(b => b.classList.toggle('on', +b.dataset.op === i)); const o = OPS[i]; $('#opName').textContent = o.name; $('#opTip').textContent = o.tip + (o.changesLength ? ' This one can change the length.' : ''); const host = $('#opK'); host.innerHTML = '';
  o.params.forEach((d, k) => host.appendChild(d.choice ? makeChoice(d, () => opP[i][k], v => { opP[i][k] = v; }) : makeKnob(d, o.group, () => opP[i][k], v => { opP[i][k] = v; })));
  if (!o.params.length) host.innerHTML = '<div class="hint">No settings.</div>'; $('#mixSlot').style.visibility = o.changesLength ? 'hidden' : 'visible'; if (open) showPop('#oppop');
}
function closeOp() { $('#oppop').hidden = true; }
$('#bOpClose').addEventListener('click', closeOp);
// history
function buildHist() {
  const h = ed.history(), host = $('#hlist'); host.innerHTML = '';
  const add = (label, k, cls) => { const b = document.createElement('button'); b.className = cls; b.innerHTML = '<i>' + k + '</i><span></span>'; $('span', b).textContent = label; b.addEventListener('click', () => { stopPlay(); ed.jumpTo(k); changed(); }); host.appendChild(b); };
  add('Start (as loaded)', 0, h.done.length === 0 ? 'cur' : ''); h.done.forEach((l, i) => add(l, i + 1, i + 1 === h.done.length ? 'cur' : '')); h.undone.forEach((l, i) => add(l, h.done.length + i + 1, 'und'));
  const cur = $('.cur', host); if (cur) cur.scrollIntoView({ block: 'nearest' });
}
function openHist() { if (!$('#histpop').hidden) { closePops(); return; } buildHist(); showPop('#histpop'); }
$('#bHistClose').addEventListener('click', closePops);
// warp, tempo and pitch
const WMODES = ['Stretch', 'Repitch', 'Beats', 'Texture']; const wk = [];
function buildWarp() {
  const host = $('#warpK'); host.innerHTML = ''; wk.length = 0; $('#warpTip').textContent = 'Plays the sound at the tempo of your project and with another pitch, without touching the sound until you press Bake in. Free means no warping at all.';
  const K = (d, get, set) => { const w = makeKnob(Object.assign({ choice: false, choices: [] }, d), 'Time', get, v => { set(v); syncWarp(); }); host.appendChild(w); wk.push(w); };
  K({ id: 'sbpm', name: 'Sample tempo', lo: 40, hi: 240, def: 120, unit: 'bpm', dec: 1, tip: 'The tempo of this sound. Meowcorder listens for it. Type a value if it guessed wrong.' }, () => sampleBpm() || 120, v => { warp.bpm = Math.round(v * 10) / 10; state.ver++; });
  K({ id: 'hbpm', name: 'Host tempo', lo: 40, hi: 240, def: 120, unit: 'bpm', dec: 1, tip: 'The tempo of your project. In the plugin this follows your DAW. Here it is the same as the Tempo knob.' }, () => recP.bpm, v => { recP.bpm = Math.round(v * 10) / 10; rebuildRec(); });
  K({ id: 'semi', name: 'Transpose', lo: -24, hi: 24, def: 0, unit: 'st', dec: 0, tip: 'Changes the pitch in semitones. 12 is an octave.' }, () => warp.semis, v => { warp.semis = Math.round(v); });
  K({ id: 'cent', name: 'Fine tune', lo: -100, hi: 100, def: 0, unit: 'ct', tip: 'Changes the pitch in hundredths of a semitone.' }, () => warp.cents, v => { warp.cents = Math.round(v); });
  K({ id: 'fsh', name: 'Formant', lo: -12, hi: 12, def: 0, unit: 'st', dec: 1, tip: 'Moves the voice character (the formants) up or down by itself. Works when Formants is on Keep.' }, () => warp.fshift, v => { warp.fshift = v; });
  const seg = (id, key, conv) => $$(id + ' button').forEach(b => b.addEventListener('click', () => { warp[key] = conv(b.dataset.v); syncWarp(); }));
  seg('#wSync', 'sync', v => v === '1'); seg('#wMode', 'mode', v => +v); seg('#wForm', 'formant', v => +v); seg('#wScope', 'scope', v => v);
  $$('#pFormSeg button').forEach(b => b.addEventListener('click', () => { warp.formant = +b.dataset.v; syncWarp(); }));
  $('#bWPrev').addEventListener('click', () => { const [a, b] = warp.scope === 'all' || !ed.hasSelection() ? [0, ed.audio.frames] : [ed.selA, ed.selB]; startPlayRange(a, b, false, true); });
  $('#bWBake').addEventListener('click', bakeWarp); $('#bWReset').addEventListener('click', () => { Object.assign(warp, { sync: false, semis: 0, cents: 0, fshift: 0 }); syncWarp(); }); $('#bWarpClose').addEventListener('click', closePops);
}
function syncWarp() {
  $$('#wSync button').forEach(b => b.classList.toggle('on', (b.dataset.v === '1') === warp.sync)); $$('#wMode button').forEach(b => b.classList.toggle('on', +b.dataset.v === warp.mode)); $$('#wForm button, #pFormSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === warp.formant)); $$('#wScope button').forEach(b => b.classList.toggle('on', b.dataset.v === warp.scope));
  wk.forEach(w => w._draw && w._draw()); if (wk[4]) wk[4].style.opacity = warp.formant ? 1 : .35;
  const sb = sampleBpm(), r = warp.sync && sb > 0 ? recP.bpm / sb : 1, st = warp.semis + warp.cents / 100;
  $('#warpInfo').textContent = (warp.sync ? (sb > 0 ? 'Sync: the sample (' + sb.toFixed(1) + ' bpm) plays at ' + recP.bpm.toFixed(1) + ' bpm, speed x' + r.toFixed(3) + ', ' + WMODES[warp.mode] + '. ' : 'No tempo found in this sound: set the sample tempo. ') : 'Free: the sound plays at its own speed. ') + (st ? 'Pitch ' + (st > 0 ? '+' : '') + st.toFixed(2) + ' st. ' : '') + (warpActive() ? 'Play uses the warp. Bake in keeps it in the sound.' : 'Nothing is warped now.');
  syncInfobar(); syncPlay(); state.ver++;
}
function bakeWith(o, scope, label) {
  stopPlay(); const all = scope === 'all' || !ed.hasSelection(), s = all ? 0 : ed.selA, e = all ? ed.audio.frames : ed.selB; if (e <= s) return false; const oo = Object.assign({}, o, { hits: (state.hits || []).filter(h => h > s && h < e).map(h => h - s) });
  ed.transform(label, a => { const w = MC.warpAudio(a.slice(s, e), oo); a.splice(s, e, w.l, w.r); if (!all) ed.selB = s + w.frames; else { ed.selA = ed.selB = 0; ed.cursor = 0; } }); markEdited(); viewFit(); changed(); return true;
}
function bakeWarp() {
  if (!warpActive() && !(warp.formant && warp.fshift)) { toast('Nothing to bake in yet. Turn on Sync, or change the pitch.'); return; } const st = warp.semis + warp.cents / 100, o = warpOpts(0, 0), label = 'Warp' + (warp.sync ? ' to ' + recP.bpm.toFixed(1) + ' bpm' : '') + (st ? ' ' + (st > 0 ? '+' : '') + st.toFixed(2) + ' st' : ''), sc = warp.scope;
  if (!bakeWith(o, sc, label)) return; state.rep = { label: label, fn: () => bakeWith(o, warp.scope, label) }; if (warp.sync) warp.bpm = 0; Object.assign(warp, { sync: false, semis: 0, cents: 0, fshift: 0 }); syncWarp(); syncEdit();
}
// ---------- pitch tools ----------
function shiftNote(n, dm) { stopPlay(); ed.transform('Pitch ' + noteName(n.m) + (dm > 0 ? ' +' : ' ') + dm.toFixed(2) + ' st', a => MC.pitchShiftRange(a, n.t0, n.t1, dm, warp.formant, warp.fshift)); markEdited(); changed(); }
function snapNotes() {
  const P = pitchInfo(); let notes = P.notes; if (ed.hasSelection()) notes = notes.filter(n => n.t1 > ed.selA && n.t0 < ed.selB);
  const todo = notes.map(n => ({ n, dm: Math.round(n.m) - n.m })).filter(x => Math.abs(x.dm) > .02); if (!todo.length) { toast(notes.length ? 'All notes are already on a semitone.' : 'No notes found. The pitch view needs a clear single voice or instrument.'); return; }
  stopPlay(); ed.transform('Snap ' + todo.length + ' notes to semitones', a => todo.forEach(x => MC.pitchShiftRange(a, x.n.t0, x.n.t1, x.dm, warp.formant, warp.fshift))); markEdited(); changed();
}
$('#pSnapAll').addEventListener('click', snapNotes);

function setTool(t) { showLayer('spec'); markTool(t); }
function markTool(t) { state.tool = t; $$('#toolSeg button').forEach(b => b.classList.toggle('on', b.dataset.tool === t)); $('#toolHint').textContent = TOOLHINT[t]; specC.style.cursor = t === 'mouse' ? 'default' : (t === 'paste' ? 'copy' : 'crosshair'); }

// ---------- pointer: time selection shared by the waveform, the pitch view and the ruler ----------
const tdPx = (e, c) => { const r = c.getBoundingClientRect(); return (e.clientX - r.left) / (r.width || 1) * (c.clientWidth || 1000); };
function tdStart(e, c) { if (!ed.audio.frames) return; c.setPointerCapture(e.pointerId); const w = c.clientWidth, f = clamp(xToFrame(tdPx(e, c), w), 0, ed.audio.frames); state.drag = { f0: e.shiftKey && ed.hasSelection() ? (Math.abs(f - ed.selA) < Math.abs(f - ed.selB) ? ed.selB : ed.selA) : f, moved: false, x0: tdPx(e, c) }; if (e.shiftKey && ed.hasSelection()) ed.setSelection(state.drag.f0, f); hideTip(); }
function tdMove(e, c) { const d = state.drag; if (!d || d.spec || d.pitch || d.wp) return; const f = clamp(xToFrame(tdPx(e, c), c.clientWidth), 0, ed.audio.frames); if (Math.abs(tdPx(e, c) - d.x0) > 3) d.moved = true; if (d.moved) { ed.setSelection(d.f0, f); syncEdit(); } }
function tdEnd(e, c) { const d = state.drag; if (!d || d.spec || d.pitch || d.wp) return; state.drag = null; if (!d.moved && !e.shiftKey) ed.setCursor(clamp(xToFrame(tdPx(e, c), c.clientWidth), 0, ed.audio.frames)); syncEdit(); }
const wheelOn = c => c.addEventListener('wheel', e => { e.preventDefault(); if (e.altKey || e.shiftKey) { const len = state.viewB - state.viewA, d = (e.deltaY + e.deltaX) / 400 * len; state.viewA += d; state.viewB += d; clampView(); state.ver++; } else zoomAt(e.deltaY < 0 ? .8 : 1.25, xToFrame(tdPx(e, c), c.clientWidth)); }, { passive: false });
const wave = LCV.wave, rulerC = $('#ruler'), pitchC = LCV.pitch;
[wave, rulerC].forEach(c => { c.addEventListener('pointerdown', e => { if (c === wave && wpOn()) wpStart(e); else tdStart(e, c); }); c.addEventListener('pointermove', e => tdMove(e, c)); c.addEventListener('pointerup', e => tdEnd(e, c)); c.addEventListener('dblclick', () => { ed.setSelection(0, ed.audio.frames); syncEdit(); }); wheelOn(c); });
const pPt = e => { const r = pitchC.getBoundingClientRect(); return { x: (e.clientX - r.left) / (r.width || 1) * pitchC.clientWidth, y: (e.clientY - r.top) / (r.height || 1) * pitchC.clientHeight }; };
function noteAt(p) { const G = pitchGeom(pitchC.clientHeight), P = pitchInfo(), f = xToFrame(p.x, pitchC.clientWidth); for (const n of P.notes) if (f >= n.t0 && f <= n.t1 && Math.abs(p.y - G.y(n.m)) <= G.rh * .6 + 2) return n; return null; }
pitchC.addEventListener('pointerdown', e => { if (!ed.audio.frames) return; const p = pPt(e), n = noteAt(p); if (n) { pitchC.setPointerCapture(e.pointerId); hideTip(); state.drag = { pitch: true, note: n, y0: p.y, dm: 0, moved: false }; } else tdStart(e, pitchC); });
pitchC.addEventListener('pointermove', e => { const d = state.drag, p = pPt(e); if (d && d.pitch) { const G = pitchGeom(pitchC.clientHeight); let dm = -(p.y - d.y0) / G.rh; if (Math.abs(p.y - d.y0) > 3) d.moved = true; if (+$('#pSnap').value) dm = Math.round(d.note.m + dm) - d.note.m; d.dm = dm; } else if (d) tdMove(e, pitchC); else { state.pHover = noteAt(p); pitchC.style.cursor = state.pHover ? 'ns-resize' : 'text'; } });
pitchC.addEventListener('pointerup', e => { const d = state.drag; if (d && d.pitch) { state.drag = null; if (d.moved && Math.abs(d.dm) > .02) shiftNote(d.note, d.dm); return; } tdEnd(e, pitchC); });
pitchC.addEventListener('pointerleave', () => { state.pHover = null; });
pitchC.addEventListener('dblclick', e => { const n = noteAt(pPt(e)); if (n) { const dm = Math.round(n.m) - n.m; if (Math.abs(dm) > .02) shiftNote(n, dm); } else { ed.setSelection(0, ed.audio.frames); syncEdit(); } });
wheelOn(pitchC);

// ---------- pencils: draw a tone in the spectral view, draw samples in the waveform view ----------
const penAmp = () => Math.pow(10, state.pdb / 20);
function penFade() { return Math.round(.003 * ed.audio.sr); }
function penTail(d) { const c = d.pc, a = ed.audio; if (c.n) { MC.pencilTone(a, c.f, c.f + penFade(), c.hz, c.hz, penAmp(), c.ph, 0, penFade()); d.did = true; } }
function penBump(d, force) { const now = performance.now(); if (force || now - d.lastFlush > 70) { d.lastFlush = now; state.ver++; state.aver++; } }
function penMove(d, p) {
  const a = ed.audio, c = d.pc, f = frAtX(p.x), hz = hzAtY(p.y), fpp = (state.viewB - state.viewA) / specC.clientWidth;
  if (f > c.f) { c.ph = MC.pencilTone(a, c.f, f, c.hz, hz, penAmp(), c.ph, c.n ? 0 : Math.min(penFade(), f - c.f), 0); c.n += f - c.f; c.f = f; c.hz = hz; d.did = true; penBump(d); }
  else if (f < c.f - 3 * fpp) { penTail(d); c.f = f; c.hz = hz; c.n = 0; c.ph = 0; penBump(d); }
}
function penEnd(d, p) {
  const a = ed.audio, c = d.pc;
  if (!c.n) { const n = Math.round(.06 * a.sr); c.ph = MC.pencilTone(a, c.f, c.f + n, c.hz, c.hz, penAmp(), 0, penFade(), penFade()); d.did = true; } else penTail(d);
  if (d.did) { penBump(d, true); specDone(); } else ed.undoStack.pop();
}
// waveform pencil: set the samples under the pen, only when single samples can be seen
const wpOn = () => state.wpen && frontLayer() === 'wave';
const wpSpp = () => (state.viewB - state.viewA) / (wave.clientWidth || 1000);
function syncPen() { const b = $('#vPen'); if (b) { b.classList.toggle('on', state.wpen); b.innerHTML = svgOf('pencil'); setTip(b, 'Waveform pencil', 'Draw the wave by hand. Zoom in until you can see single samples, then drag over the wave to set them. Shift+Q turns it on or off.', 'Shift+Q'); } wave.style.cursor = state.wpen ? 'crosshair' : 'text'; $('#vHint2').textContent = state.wpen ? (wpSpp() <= 4 ? 'Pencil on: drag over the wave to draw it.' : 'Pencil on: zoom in until you see single samples.') : ''; }
function toggleWPen() { state.wpen = !state.wpen; if (state.wpen && !state.layers.wave) showLayer('wave'); if (state.wpen && wpSpp() > 4 && ed.audio.frames) { const c = (ed.hasSelection() ? (ed.selA + ed.selB) / 2 : ed.cursor); const len = Math.max(64, wave.clientWidth / 8); state.viewA = c - len / 2; state.viewB = c + len / 2; clampView(); state.ver++; } syncPen(); }
function wpVal(e) { const r = wave.getBoundingClientRect(), w = wave.clientWidth, h = wave.clientHeight, x = (e.clientX - r.left) / (r.width || 1) * w, y = (e.clientY - r.top) / (r.height || 1) * h, mono = isMono(ed.audio), lanes = mono ? 1 : 2, lh = h / lanes, lane = mono ? -1 : clamp(Math.floor(y / lh), 0, 1), mid = lh * ((mono ? 0 : lane) + .5); return { f: state.viewA + x / w * (state.viewB - state.viewA), v: clamp((mid - y) / (lh / 2 * .96), -1, 1), lane }; }
function wpPut(d, e) { const q = wpVal(e), l = d.last; if (l && l.lane === q.lane) MC.pencilWave(ed.audio, l.f, l.v, q.f, q.v, q.lane); else MC.pencilWave(ed.audio, q.f, q.v, q.f, q.v, q.lane); d.last = q; d.did = true; const now = performance.now(); if (now - d.lastFlush > 50) { d.lastFlush = now; state.ver++; state.aver++; } }
function wpStart(e) { if (!ed.audio.frames) return; if (wpSpp() > 4) { toast('Zoom in until you see single samples, then draw.'); return; } wave.setPointerCapture(e.pointerId); hideTip(); stopPlay(); ed.pushUndo('Pencil (waveform)'); state.drag = { wp: true, last: null, did: false, lastFlush: 0 }; wpPut(state.drag, e); }
wave.addEventListener('pointermove', e => { const d = state.drag; if (d && d.wp) wpPut(d, e); });
wave.addEventListener('pointerup', e => { const d = state.drag; if (!d || !d.wp) return; state.drag = null; if (d.did) { markEdited(); changed(); } else ed.undoStack.pop(); });

function specDone() { state.sreg = null; markEdited(); changed(); }
// the eraser works while you paint: every 70 ms the new strokes are cut out and the display follows
function flushEraser(d, force) { const now = performance.now(); if (!d.pending.length || (!force && now - d.lastFlush < 70)) return; MC.spectralEdit(ed.audio, d.pending, 'cut'); d.pending = []; d.did = true; d.lastFlush = now; state.ver++; state.aver++; }
specC.addEventListener('pointerdown', e => {
  if (!ed.audio.frames) return; specC.setPointerCapture(e.pointerId); hideTip(); const p = spPt(e);
  state.drag = { spec: true, tool: state.tool, p0: p, p, rects: [], pending: [], f0: frAtX(p.x), moved: false, last: null, lastFlush: 0, did: false };
  if (state.tool === 'eraser') { stopPlay(); ed.pushUndo('Spectral eraser'); const r = brushRect(p); state.drag.rects = [r]; state.drag.pending = [r]; flushEraser(state.drag, true); }
  if (state.tool === 'pencil') { stopPlay(); ed.pushUndo('Pencil (spectral)'); state.drag.pc = { f: frAtX(p.x), hz: hzAtY(p.y), ph: 0, n: 0 }; }
});
specC.addEventListener('pointermove', e => {
  const p = spPt(e), d = state.drag;
  if (state.tool === 'paste') state.pghost = p;
  if (!d || !d.spec) return; d.p = p;
  if (d.tool === 'mouse') { if (Math.abs(p.x - d.p0.x) > 3) { d.moved = true; ed.setSelection(d.f0, frAtX(p.x)); syncEdit(); } }
  else if (d.tool === 'pencil') penMove(d, p);
  else if (d.tool === 'eraser') { const l = d.last || d.p0; if (Math.hypot(p.x - l.x, p.y - l.y) >= state.brush / 2) { const r = brushRect(p); d.rects = [r]; d.pending.push(r); d.last = p; flushEraser(d); } }
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
    case 'eraser': flushEraser(d, true); if (d.did) specDone(); else ed.undoStack.pop(); break;
    case 'pencil': penEnd(d, p); break;
    case 'copy': if (!tiny && ed.spectralCopy(box)) { state.sreg = box; setTool('paste'); $('#toolHint').textContent = 'Copied. Click where it should land.'; syncEdit(); } break;
    case 'paste': stopPlay(); if (ed.spectralPaste(frAtX(p.x), hzAtY(p.y))) specDone(); else $('#toolHint').textContent = 'Nothing copied yet. Use Copy first.'; break;
  }
});

wheelOn(specC);
const fmtHz = f => f >= 1000 ? (f / 1000).toFixed(f >= 10000 ? 1 : 2) + 'k' : Math.round(f) + '';
function drawBox(g, r, col, fill, label) { const w = specC.clientWidth, x0 = frameToX(r.t0, w), x1 = frameToX(r.t1, w), y0 = yAtHz(r.f1), y1 = yAtHz(r.f0); g.fillStyle = fill; g.fillRect(x0, y0, x1 - x0, y1 - y0); g.strokeStyle = col; g.lineWidth = 1; g.setLineDash([4, 3]); g.strokeRect(x0 + .5, y0 + .5, x1 - x0, y1 - y0); g.setLineDash([]); if (label) { g.fillStyle = col; g.font = '10px JetBrains Mono, monospace'; g.textAlign = 'left'; g.fillText(label, x0 + 3, Math.max(10, y0 - 3)); } }
function drawSpecOverlay(g, w, h) {
  const d = state.drag; if (state.sreg) drawBox(g, state.sreg, '#6CC9F5', '#6cc9f522', fmtHz(state.sreg.f0) + ' to ' + fmtHz(state.sreg.f1) + ' Hz');
  if (d && d.spec) {
    if (d.tool === 'eraser') d.rects.forEach(r => drawBox(g, r, '#FF1383', '#ff138355', null));
    else if (['select', 'scissors', 'cutter', 'copy', 'amp'].includes(d.tool)) { const b = boxOf(d.p0, d.p); if (d.tool === 'scissors') { b.f0 = 20; b.f1 = fMaxOf(); } drawBox(g, b, d.tool === 'cutter' || d.tool === 'scissors' ? '#FF1383' : (d.tool === 'amp' ? '#FFD100' : '#6CC9F5'), '#ffffff1c', null); }
  } else if (state.tool === 'paste' && state.pghost && ed.sclip) { const c = ed.sclip, p = state.pghost, fr = frAtX(p.x), dur = ((c.nf - 1) * MC.SH + MC.SN / 2); const half = (c.nb * c.sr / MC.SN) / 2, hz = hzAtY(p.y); drawBox(g, { t0: fr, t1: fr + dur, f0: Math.max(20, hz - half), f1: hz + half }, '#02EBBA', '#02ebba22', 'paste here'); }
}

// ---------- files ----------
async function openFile(f) {
  if (!f) return; const buf = new Uint8Array(await f.arrayBuffer()); let a = MC.decodeWav(buf);
  if (!a) { try { const c = ctx || new AC(); const d = await c.decodeAudioData(buf.buffer.slice(0)); a = new MC.Audio(d.sampleRate, d.length); d.copyFromChannel(a.l, 0); a.r = d.numberOfChannels > 1 ? (d.copyFromChannel(a.r, 1), a.r) : a.l.slice(); } catch (e) { modal('Open', 'This browser cannot read that file. Try a WAV file.', [['OK', null, 'on']]); return; } }
  stopPlay(); ed.pushUndo('Open ' + f.name); const keep = ed.undoStack.slice(); ed.load(a); ed.undoStack = keep; state.fname = f.name; state.bitsFile = a.bits || null; state.bits = a.bits || 32; state.ms = false; warp.bpm = 0; state.stopPos = 0; state.pausePos = -1; viewFit(); changed();
}
$('#file').addEventListener('change', e => { openFile(e.target.files[0]); e.target.value = ''; });
const dw = $('#dispwrap'); ['dragenter', 'dragover'].forEach(t => dw.addEventListener(t, e => { e.preventDefault(); dw.classList.add('hot'); })); ['dragleave', 'drop'].forEach(t => dw.addEventListener(t, e => { e.preventDefault(); dw.classList.remove('hot'); }));
dw.addEventListener('drop', e => openFile(e.dataTransfer.files[0]));
const CRCT = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; ++n) { let c = n; for (let k = 0; k < 8; ++k) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
const crc32 = d => { let c = 0xFFFFFFFF; for (let i = 0; i < d.length; ++i) c = CRCT[(c ^ d[i]) & 255] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; };
// a zip with one file, stored without compression (the artifact viewer only lets some file types be saved, WAV is not one of them)
function makeZip(name, data) {
  const nm = new TextEncoder().encode(name), crc = crc32(data), lh = new Uint8Array(30 + nm.length), v = new DataView(lh.buffer); v.setUint32(0, 0x04034b50, true); v.setUint16(4, 20, true); v.setUint16(6, 0x0800, true); v.setUint32(14, crc, true); v.setUint32(18, data.length, true); v.setUint32(22, data.length, true); v.setUint16(26, nm.length, true); lh.set(nm, 30);
  const cd = new Uint8Array(46 + nm.length), c = new DataView(cd.buffer); c.setUint32(0, 0x02014b50, true); c.setUint16(4, 20, true); c.setUint16(6, 20, true); c.setUint16(8, 0x0800, true); c.setUint16(14, 33, true); c.setUint32(16, crc, true); c.setUint32(20, data.length, true); c.setUint32(24, data.length, true); c.setUint16(28, nm.length, true); cd.set(nm, 46);
  const en = new Uint8Array(22), e = new DataView(en.buffer); e.setUint32(0, 0x06054b50, true); e.setUint16(8, 1, true); e.setUint16(10, 1, true); e.setUint32(12, cd.length, true); e.setUint32(16, lh.length + data.length, true);
  return new Blob([lh, data, cd, en], { type: 'application/zip' });
}
const fileBase = () => state.fname.replace(/(, edited)?(, not saved)?$/, '').replace(/[^\w.-]+/g, '_').replace(/\.(wav|aiff?|flac|mp3|ogg)$/i, '').replace(/^_+|_+$/g, '') || 'meowcorder';
async function saveWav() {
  const base = fileBase(), bytes = MC.encodeWav(ed.audio, state.bits); let dl = null; try { if (window.claude && window.claude.use) dl = await window.claude.use('downloads'); } catch (e) {}
  if (dl) { try { await dl.save({ filename: base + '.zip', data: makeZip(base + '.wav', bytes) }); state.fname = state.fname.replace(/, not saved$/, ''); syncEdit(); toast('Saved ' + base + '.zip with ' + base + '.wav inside. This viewer only allows some file types, so the WAV travels in a zip. The plugin saves a plain WAV.', 7000); } catch (e) { if (!e || e.code !== 'declined') toast('Could not save here: ' + ((e && e.message) || e), 6000); } return; }
  const url = URL.createObjectURL(new Blob([bytes], { type: 'audio/wav' })), a = document.createElement('a'); a.href = url; a.download = base + '.wav'; document.body.appendChild(a); a.click(); a.remove(); setTimeout(() => URL.revokeObjectURL(url), 2000); state.fname = state.fname.replace(/, not saved$/, ''); syncEdit();
}
function loadSound(a, name, label) { stopPlay(); ed.pushUndo(label); const keep = ed.undoStack.slice(); ed.load(a); ed.undoStack = keep; state.fname = name; state.ms = false; state.bitsFile = null; warp.bpm = 0; state.stopPos = 0; state.pausePos = -1; viewFit(); changed(); }
const newSound = () => loadSound(new MC.Audio(ed.audio.sr || 48000), 'empty, not saved', 'New empty sound');
const loadDemo = kind => loadSound(kind === 'melody' ? makeMelody(demo.sr) : demo.clone(), 'demo ' + (kind === 'melody' ? 'melody' : 'loop') + ', not saved', 'Load demo ' + (kind === 'melody' ? 'melody' : 'loop'));

// ---------- buttons ----------
$('#bStart').addEventListener('click', powerOn); $('#power').addEventListener('click', () => { if (ctx && ctx.state === 'running') { ctx.suspend(); $('#power').classList.remove('on'); $('#overlay').hidden = false; } else powerOn(); });
$('#bRec').addEventListener('click', armOrRecord);
$$('#startSeg button').forEach(b => b.addEventListener('click', () => { state.startMode = +b.dataset.v; syncSegs(); })); $$('#takeSeg button').forEach(b => b.addEventListener('click', () => { state.takeMode = +b.dataset.v; syncSegs(); }));
function syncSegs() { syncIcons(); $$('#startSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === state.startMode)); $$('#takeSeg button').forEach(b => b.classList.toggle('on', +b.dataset.v === state.takeMode)); }
$('#bGrab').addEventListener('click', () => { if (!rec.ring) { powerOn(); return; } const a = rec.captureLast(Math.max(.2, recP.pre / 1000 * 1 + 0) || 3); if (a.frames) { ed.takeRecording(a, state.takeMode); state.fname = 'grabbed, not saved'; viewFit(); changed(); } });
$('#bPlay').addEventListener('click', () => playTrigger('button')); $('#bLoop').addEventListener('click', () => { state.loop = !state.loop; $('#bLoop').classList.toggle('on', state.loop); if (state.playing) { const [a, b] = playRange(); startPlayRange(a, b, state.loop); } });
const act = (id, fn, mark) => $(id).addEventListener('click', () => { stopPlay(); fn(); if (mark) markEdited(); changed(); });
act('#bUndo', () => ed.undo()); act('#bRedo', () => ed.redo()); act('#bCut', () => ed.cut(), 1); act('#bCopy', () => ed.copy()); act('#bPaste', () => ed.paste(), 1); act('#bDel', () => ed.deleteSelection(), 1); act('#bCrop', () => { ed.cropToSelection(); viewFit(); }, 1);
act('#bAll', () => ed.setSelection(0, ed.audio.frames)); act('#bSnap', () => { if (!ed.hasSelection()) return; const a = ed.snapToZero(ed.selA, 2000), b = ed.snapToZero(ed.selB, 2000); ed.setSelection(a, b); });
$('#bRepeat').addEventListener('click', repeat); $('#bHist').addEventListener('click', openHist);
$('#bZin').addEventListener('click', () => zoomAt(.5, (state.viewA + state.viewB) / 2)); $('#bZout').addEventListener('click', () => zoomAt(2, (state.viewA + state.viewB) / 2)); $('#bFit').addEventListener('click', () => { viewFit(); state.ver++; });
$('#bSel').addEventListener('click', () => { if (ed.hasSelection()) { state.viewA = ed.selA; state.viewB = ed.selB; clampView(); state.ver++; } });
$('#bApply').addEventListener('click', doOp); $('#bPrev').addEventListener('click', previewOp);
$('#ampdb').addEventListener('change', e => { state.amp = +e.target.value; }); $('#brush').addEventListener('change', e => { state.brush = +e.target.value; });

// ---------- dialogs ----------
function openGuide() { modal('Guide', `<ol><li>Click <b>Start audio</b>. In the plugin, Meowcorder listens to the mixer channel it sits on. Here, a demo loop or a demo melody stands in for that channel (the input button picks which).</li><li><b>Record</b> (Enter) records what comes in. The small arrow next to it chooses how it starts: <b>Now</b>, <b>On input</b> (waits for sound above the threshold, with pre-roll) or after a <b>Count-in</b> (1, 2 or 4 bars of 4/4 with a click you hear).</li><li>Drag on the waveform to select, click to place the cursor. Wheel zooms, Alt wheel scrolls, double click selects all.</li><li><b>Views</b>: Waveform, Spectral and Pitch. One at a time by default. Turn on <b>Show views together</b> to overlay them or split the screen. The view <b>In front</b> is the one your tools work on. Cutting a band in the spectral view changes the waveform right away, and dragging a note in the pitch view changes the pitch of the sound.</li><li>The bar above the display shows what Meowcorder found: <b>channels</b>, <b>sample rate</b>, <b>bit depth</b>, <b>tempo</b>. Each one is a menu: make it mono, convert the rate, reduce the bit depth, set the tempo, change the time unit.</li><li><b>Warp</b>: Sync plays the sound at the host tempo, Free leaves it alone. Transpose and formants change the pitch. <b>Bake in</b> writes it into the sound.</li><li><b>Process</b> has 24 operations. Click one, set it, choose Selection or Whole sound, <b>Preview</b>, then <b>Apply</b>. Shift plus a letter applies it straight away.</li><li><b>R</b> repeats the last operation. <b>Ctrl+Shift+Z</b> puts back what you undid, or repeats the last operation when there is nothing to put back. <b>History</b> (Alt+Y) lists every step and lets you jump to any of them.</li><li>Settings has the <b>Play keys</b>: where the Play button, Space and Shift+Space start, and what they do when pressed again.</li></ol>`, [['Close', null, 'on']]); }
function openSettings() {
  const sel = (id, opts, cur) => `<select id="${id}">${opts.map(([v, t]) => `<option value="${v}"${v === cur ? ' selected' : ''}>${t}</option>`).join('')}</select>`, row = (id, name, which) => `<div class="row2"><b>${name}</b>${sel(id + 'S', PLAYSTART, play[which][0])}${sel(id + 'A', PLAYAGAIN, play[which][1])}</div>`;
  modal('Settings', `<label class="row"><input type="checkbox" id="sTip" ${tipOn ? 'checked' : ''}> Info bubbles (after 1 second)</label><label class="row"><input type="checkbox" id="sPow" ${powerSave ? 'checked' : ''}> Power saving (slower drawing)</label><label class="row"><input type="checkbox" id="sHit" ${showHits ? 'checked' : ''}> Mark hits (orange ticks) on the waveform</label><label class="row"><input type="checkbox" id="sDit" ${state.dither ? 'checked' : ''}> Dither when the bit depth is reduced</label><h4>Play keys</h4><div class="hint">Each way of starting playback has its own rule: where it starts, and what it does when you press it again while playing.</div><div class="row2"><span></span><span class="hint">Starts</span><span class="hint">Pressed again</span></div>${row('pb', 'Play button', 'button')}${row('sp', 'Space bar', 'space')}${row('ss', 'Shift + Space', 'shift')}<div class="kbdnote">Keys work while the Meowcorder window has the focus. In a DAW, the host may keep some keys for itself.</div>`,
    [['Reset window size', () => { setZoom(1); return false; }], ['Close', m => { tipOn = $('#sTip', m).checked; powerSave = $('#sPow', m).checked; showHits = $('#sHit', m).checked; state.dither = $('#sDit', m).checked; [['pb', 'button'], ['sp', 'space'], ['ss', 'shift']].forEach(([id, w]) => { play[w] = [$('#' + id + 'S', m).value, $('#' + id + 'A', m).value]; }); saveSet(); syncPlay(); state.ver++; changed(); }, 'on']]);
}
function openAbout() { modal('About & Support', `<p><b>Meowcorder</b> by Studio 87, version 0.1 design mockup.</p><p>Free and open source (AGPL-3.0-or-later). It collects nothing and makes no network calls. The microphone is only used after you choose it and press Record. The button below only opens a link when you click it.</p><p>Nothing leaves this page: recordings stay in memory until you save a file.</p><p><a class="btn" href="https://ko-fi.com/studio87" target="_blank" rel="noopener">Support on Ko-fi</a></p>`, [['Close', null, 'on']]); }
function askBpm() {
  modal('Sample tempo', `<p>Tempo of this sound in beats per minute (4/4).</p><input type="text" id="bpmIn" value="${sampleBpm() ? sampleBpm().toFixed(1) : '120'}">`, [['Cancel', null], ['Set', m => { const v = parseFloat($('#bpmIn', m).value); if (!(v >= 20 && v <= 400)) return false; setBpm(v); }, 'on']], m => { const i = $('#bpmIn', m); i.focus(); i.select(); i.addEventListener('keydown', e => { if (e.key === 'Enter') $('.btns button.on', m).click(); }); });
}

// ---------- menus (the single source for items, shortcuts and key handling) ----------
function saveSet() { store.set('mc.set', JSON.stringify({ tipOn, powerSave, showHits, layers: state.layers, combine: state.combine, layout: state.layout, front: state.front, unit: state.unit, dither: state.dither, play, formant: warp.formant })); }
function rebuildRec() { $('#recK').innerHTML = ''; buildRec(); }
function runOp(i) { state.curOp = i; selectOp(i); doOp(); }
const ED = fn => () => { stopPlay(); fn(); markEdited(); changed(); };
const OPKEYS = { gain: 'G', normalize: 'N', fadeIn: 'I', fadeOut: 'O', reverse: 'R', invert: 'P', dcRemove: 'D', silence: 'S', mono: 'M', pan: 'B', rate: 'Y', bitcrush: 'K', drive: 'V', eq: 'E', filter: 'F', delay: 'L', chorus: 'H', phaser: 'A', reverb: 'W', compressor: 'C', limiter: 'T', gate: 'U', trimSilence: 'X', loopXfade: 'J' };
const radio = (l, get, set, k) => ({ l, k, ck: () => get(), f: set });
const processSubs = () => { const by = {}, order = []; OPS.forEach((o, i) => { if (!by[o.group]) { by[o.group] = []; order.push(o.group); } by[o.group].push({ l: o.name, k: 'Shift+' + OPKEYS[o.id], tip: o.tip, f: () => runOp(i) }); }); return order.map(g => ({ l: g, sub: by[g] })); };
const fmtSr = r => String(+(r / 1000).toFixed(3)) + ' kHz', bitsName = b => b === 32 ? '32 bit float' : b + ' bit';
function afterBpm() { state.ver++; syncInfobar(); if (!$('#warppop').hidden) syncWarp(); }
function setBpm(v) { warp.bpm = Math.round(clamp(v, 20, 400) * 10) / 10; afterBpm(); }
function chanOp(label, kind) { stopPlay(); ed.transform(label, a => MC.channelOp(a, kind)); if (kind === 'enc') state.ms = true; else if (kind !== 'swap') state.ms = false; markEdited(); changed(); }
function convertRate(r) { if (r === ed.audio.sr) return; stopPlay(); ed.transform('Convert to ' + fmtSr(r), a => MC.resample(a, r)); ed.selA = ed.selB = 0; ed.cursor = 0; state.stopPos = 0; markEdited(); viewFit(); changed(); }
function reinterpret(r) { if (r === ed.audio.sr) return; stopPlay(); ed.transform('Set rate to ' + fmtSr(r), a => { a.sr = r; }); markEdited(); changed(); }
function setBits(b) { if (b === state.bits) return; if (b < state.bits && b < 32) { stopPlay(); ed.transform('Reduce to ' + bitsName(b) + (state.dither ? ' (dither)' : ''), a => MC.quantize(a, b, state.dither)); markEdited(); state.bits = b; changed(); } else state.bits = b; syncInfobar(); }
const curChan = () => state.ms ? 'ms' : (isMono(ed.audio) ? 'mono' : 'stereo');
const chItems = () => { const cur = curChan(); return [{ hd: 'Detected: ' + (cur === 'ms' ? 'mid and side (made here)' : cur === 'mono' ? 'mono, both sides the same' : 'stereo'), det: true },
  { l: 'Mono: mix left and right', ck: () => cur === 'mono', f: () => chanOp('Mono (mix)', 'mix') }, { l: 'Keep only the left side', f: () => chanOp('Keep left', 'left') }, { l: 'Keep only the right side', f: () => chanOp('Keep right', 'right') }, '-',
  { l: 'Swap left and right', f: () => chanOp('Swap sides', 'swap') }, { l: 'Split into mid and side', ck: () => cur === 'ms', f: () => chanOp('Left/right to mid/side', 'enc'), en: () => cur !== 'ms' }, { l: 'Mid and side back to left and right', f: () => chanOp('Mid/side to left/right', 'dec'), en: () => cur === 'ms' }, '-',
  { hd: 'New recordings are' }, radio('Stereo', () => !state.recMono, () => { state.recMono = false; syncIcons(); }), radio('Mono', () => state.recMono, () => { state.recMono = true; syncIcons(); })]; };
const srItems = () => { const cur = ed.audio.sr; return [{ hd: 'Detected: ' + fmtSr(cur), det: true }, { hd: 'Convert to (same sound, new rate)' }].concat([22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000].map(r => ({ l: fmtSr(r), ck: () => r === cur, f: () => convertRate(r) })), ['-', { hd: 'Set the rate only (pitch and speed change)' }], [44100, 48000, 88200, 96000].map(r => ({ l: fmtSr(r), f: () => reinterpret(r) }))); };
const bitItems = () => [{ hd: 'This sound: ' + bitsName(state.bits) + (state.bitsFile ? ' (the file was ' + bitsName(state.bitsFile) + ')' : ''), det: true }, { hd: 'Used when you save. Going down changes the sound.' }].concat([8, 16, 24, 32].map(b => radio(bitsName(b), () => state.bits === b, () => setBits(b))), ['-', { l: 'Dither when reducing', ck: () => state.dither, f: () => { state.dither = !state.dither; saveSet(); } }]);
const unitItems = () => UNITS.map(([u, n]) => radio(n, () => state.unit === u, () => { state.unit = u; saveSet(); syncInfobar(); }));
const bpmItems = () => { const sb = sampleBpm(), det = state.bpmAuto; return [{ hd: det ? 'Detected: ' + det.toFixed(1) + ' bpm' + (warp.bpm ? ' (you set ' + warp.bpm.toFixed(1) + ')' : '') : 'Detected: no clear beat found', det: true },
  { l: 'Set the tempo...', f: askBpm }, { l: 'Use the detected tempo', f: () => { warp.bpm = 0; afterBpm(); }, en: () => !!warp.bpm }, { l: 'Double (x2)', f: () => setBpm(sb * 2), en: () => sb > 0 && sb * 2 <= 400 }, { l: 'Half (÷2)', f: () => setBpm(sb / 2), en: () => sb >= 40 },
  { l: 'The sound is exactly...', sub: [1, 2, 4, 8].map(n => ({ l: n + (n === 1 ? ' bar' : ' bars') + ' of 4/4', f: () => setBpm(n * 4 * 60 / (ed.audio.frames / ed.audio.sr)), en: () => ed.audio.frames > 0 })) }, '-',
  { hd: 'Host tempo: ' + recP.bpm.toFixed(1) + ' bpm (the Tempo knob)' }, { l: 'Set the host tempo to this sound', f: () => { recP.bpm = clamp(Math.round(sb * 10) / 10, 40, 240); rebuildRec(); afterBpm(); }, en: () => sb >= 40 && sb <= 240 }]; };
const syncItems = () => [{ hd: 'Tempo' }, radio('Free: not warped', () => !warp.sync, () => { warp.sync = false; syncWarp(); }), radio('Sync to the host tempo', () => warp.sync, () => { warp.sync = true; syncWarp(); }), '-', { hd: 'Warp mode' }].concat(WMODES.map((n, i) => radio(n, () => warp.mode === i, () => { warp.mode = i; syncWarp(); })), ['-', { l: 'Warp, tempo and pitch settings...', k: 'Alt+Q', f: openWarp }]);
function openWarp() { if (!$('#warppop').hidden) { closePops(); return; } syncWarp(); showPop('#warppop'); }
const cycle = (arr, cur) => arr[(arr.indexOf(cur) + 1) % arr.length];
const MENUS = [
  { name: 'File', items: [
    { l: 'New empty sound', k: 'Alt+N', f: newSound },
    { l: 'Open file...', k: 'Ctrl+O', f: () => $('#file').click() },
    { l: 'Save as WAV', k: 'Ctrl+S', f: saveWav },
    { l: 'Save bit depth', sub: [8, 16, 24, 32].map(b => radio(bitsName(b), () => state.bits === b, () => setBits(b))) },
    '-',
    { l: 'Audio on or off', k: 'Alt+A', f: () => $('#power').click() },
    { l: 'Demo sounds', sub: [{ l: 'Drum loop', f: () => loadDemo('loop') }, { l: 'Melody (for the pitch view)', f: () => loadDemo('melody') }] } ] },
  { name: 'Edit', items: [
    { l: 'Undo', k: 'Ctrl+Z', f: ED(() => ed.undo()), en: () => ed.canUndo() },
    { l: 'Redo', k: 'Ctrl+Y', f: ED(() => ed.redo()), en: () => ed.canRedo() },
    { l: 'Redo, or repeat the last operation', k: 'Ctrl+Shift+Z', f: redoOrRepeat, en: () => ed.canRedo() || !!state.rep },
    '-',
    { l: 'Cut', k: 'Ctrl+X', f: ED(() => ed.cut()), en: () => ed.hasSelection() },
    { l: 'Copy', k: 'Ctrl+C', f: () => { ed.copy(); syncEdit(); }, en: () => ed.hasSelection() },
    { l: 'Paste', k: 'Ctrl+V', f: ED(() => ed.paste()), en: () => ed.clip.frames > 0 },
    { l: 'Delete selection', k: 'Del', also: ['Backspace'], f: ED(() => ed.deleteSelection()), en: () => ed.hasSelection() },
    { l: 'Crop to selection', k: 'T', f: ED(() => { ed.cropToSelection(); viewFit(); }), en: () => ed.hasSelection() },
    '-',
    { l: 'Repeat last operation', k: 'R', f: repeat, en: () => !!state.rep },
    { l: 'History...', k: 'Alt+Y', f: openHist } ] },
  { name: 'Select', items: [
    { l: 'All', k: 'Ctrl+A', f: () => { ed.setSelection(0, ed.audio.frames); syncEdit(); } },
    { l: 'None', k: 'Esc', f: () => { if (anyPop()) { closePops(); return; } state.sreg = null; ed.setCursor(ed.cursor); syncEdit(); } },
    { l: 'From cursor to the end', k: 'Shift+End', f: () => { ed.setSelection(ed.hasSelection() ? ed.selA : ed.cursor, ed.audio.frames); syncEdit(); } },
    { l: 'From the start to the cursor', k: 'Shift+Home', f: () => { ed.setSelection(0, ed.hasSelection() ? ed.selB : ed.cursor); syncEdit(); } },
    '-',
    { l: 'Snap edges to zero crossings', k: 'Z', f: ED(() => { const a = ed.snapToZero(ed.selA, 2000), b = ed.snapToZero(ed.selB, 2000); ed.setSelection(a, b); }), en: () => ed.hasSelection() } ] },
  { name: 'View', items: [
    { l: 'Waveform view', k: 'Alt+W', ck: () => state.layers.wave, f: () => toggleLayer('wave') },
    { l: 'Spectral view', k: 'Alt+G', ck: () => state.layers.spec, f: () => toggleLayer('spec') },
    { l: 'Pitch view (piano roll)', k: 'Alt+K', ck: () => state.layers.pitch, f: () => toggleLayer('pitch') },
    { l: 'Waveform pencil (draw samples)', k: 'Shift+Q', ck: () => state.wpen, f: toggleWPen },
    { l: 'Show views together', k: 'Alt+B', ck: () => state.combine, f: () => setCombine(!state.combine) },
    { l: 'Together as', sub: [radio('Overlay (on top of each other)', () => state.layout === 'overlay', () => { state.layout = 'overlay'; afterView(); }), radio('Split (one above the other)', () => state.layout === 'split', () => { state.layout = 'split'; afterView(); })] },
    { l: 'Switch overlay and split', k: 'Alt+L', f: () => { state.layout = state.layout === 'overlay' ? 'split' : 'overlay'; afterView(); }, en: () => state.combine },
    { l: 'Bring the next view to the front', k: 'Alt+F', f: () => { const v = visLayers(); state.front = v[(v.indexOf(frontLayer()) + 1) % v.length]; afterView(); }, en: () => state.combine && visLayers().length > 1 },
    '-',
    { l: 'Zoom in', k: '+', also: ['='], f: () => zoomAt(.5, (state.viewA + state.viewB) / 2) },
    { l: 'Zoom out', k: '-', f: () => zoomAt(2, (state.viewA + state.viewB) / 2) },
    { l: 'Show the whole sound', k: 'F', f: () => { viewFit(); state.ver++; } },
    { l: 'Zoom to the selection', k: 'S', f: () => { if (ed.hasSelection()) { state.viewA = ed.selA; state.viewB = ed.selB; clampView(); state.ver++; } }, en: () => ed.hasSelection() },
    '-',
    { l: 'Time line unit', sub: UNITS.map(([u, n]) => radio(n, () => state.unit === u, () => { state.unit = u; saveSet(); syncInfobar(); })) },
    { l: 'Next time line unit', k: 'Alt+T', f: () => { state.unit = cycle(UNITS.map(u => u[0]), state.unit); saveSet(); syncInfobar(); } },
    '-',
    { l: 'Hit marks', k: 'Alt+H', ck: () => showHits, f: () => { showHits = !showHits; saveSet(); changed(); } },
    { l: 'Info bubbles', k: 'Alt+I', ck: () => tipOn, f: () => { tipOn = !tipOn; saveSet(); } },
    { l: 'Power saving', k: 'Alt+P', ck: () => powerSave, f: () => { powerSave = !powerSave; saveSet(); } },
    '-',
    { l: 'Window size', sub: [[75, 1], [100, 2], [125, 3], [150, 4], [200, 5]].map(([pc, n]) => ({ l: pc + '%', k: 'Alt+' + n, ck: () => Math.round(zoom * 100) === pc, f: () => setZoom(pc / 100) })).concat([{ l: 'Back to 100%', k: 'Alt+0', f: () => setZoom(1) }]) } ] },
  { name: 'Spectral', items: [
    { l: 'Show the spectral view', ck: () => state.layers.spec, f: () => toggleLayer('spec') },
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
  { name: 'Warp', items: [
    { l: 'Warp, tempo and pitch...', k: 'Alt+Q', f: openWarp },
    '-',
    { l: 'Sync to the host tempo', k: 'Alt+S', ck: () => warp.sync, f: () => { warp.sync = !warp.sync; syncWarp(); } },
    { l: 'Warp mode', sub: WMODES.map((n, i) => radio(n, () => warp.mode === i, () => { warp.mode = i; syncWarp(); })) },
    { l: 'Formants', sub: [radio('Follow the pitch', () => warp.formant === 0, () => { warp.formant = 0; syncWarp(); }), radio('Keep', () => warp.formant === 1, () => { warp.formant = 1; syncWarp(); })] },
    { l: 'Transpose', sub: [-12, -7, -5, -2, -1, 0, 1, 2, 5, 7, 12].map(v => radio((v > 0 ? '+' : '') + v + ' st', () => warp.semis === v, () => { warp.semis = v; syncWarp(); })) },
    '-',
    { l: 'Bake the warp into the sound', k: 'Alt+E', f: bakeWarp, en: () => warpActive() },
    { l: 'Snap the notes to semitones', k: 'Alt+D', f: snapNotes },
    '-',
    { l: 'Set the tempo of this sound...', f: askBpm },
    { l: 'Detect the tempo again', f: () => { warp.bpm = 0; state.bpmAuto = MC.detectBpm(ed.audio) || 0; afterBpm(); } } ] },
  { name: 'Play', items: [
    { l: 'Play, Space bar rule', k: 'Space', f: () => playTrigger('space') },
    { l: 'Play, Shift+Space rule', k: 'Shift+Space', f: () => playTrigger('shift') },
    { l: 'Play, Play button rule', f: () => playTrigger('button') },
    { l: 'Loop', k: 'L', ck: () => state.loop, f: () => $('#bLoop').click() },
    { l: 'Preview the chosen operation', k: 'P', f: previewOp },
    '-',
    { l: 'Cursor to the start', k: 'Home', f: () => { ed.setCursor(0); state.viewA = 0; clampView(); state.ver++; syncEdit(); } },
    { l: 'Cursor to the end', k: 'End', f: () => { ed.setCursor(ed.audio.frames); syncEdit(); } },
    '-',
    { l: 'Play keys settings...', f: openSettings } ] },
  { name: 'Record', items: [
    { l: 'Record or stop', k: 'Enter', f: armOrRecord },
    { l: 'Grab the last seconds', k: 'G', f: () => $('#bGrab').click() },
    { l: 'Monitor the input', k: 'Alt+M', ck: () => state.monitor, f: () => setMonitor(!state.monitor) },
    { l: 'Record in mono', k: 'Alt+O', ck: () => state.recMono, f: () => { state.recMono = !state.recMono; syncIcons(); } },
    '-',
    { l: 'Start', sub: [radio('Now', () => state.startMode === 1, () => { state.startMode = 1; syncSegs(); }, '1'), radio('On input (when sound arrives)', () => state.startMode === 2, () => { state.startMode = 2; syncSegs(); }, '2'), radio('After a count-in', () => state.startMode === 3, () => { state.startMode = 3; syncSegs(); }, '3')] },
    { l: 'Count-in length', sub: [1, 2, 4].map(n => radio(n + (n === 1 ? ' bar' : ' bars') + ' of 4/4', () => state.bars === n, () => { state.bars = n; })) },
    { l: 'Tempo', sub: [90, 100, 110, 120, 128, 140, 150, 160, 174].map(v => radio(v + ' bpm', () => Math.round(recP.bpm) === v, () => { recP.bpm = v; rebuildRec(); afterBpm(); })) },
    { l: 'Click', sub: [['Off', 0], ['During the count-in', 1], ['Count-in and while recording', 2]].map(([n, v]) => radio(n, () => state.click === v, () => { state.click = v; })) },
    '-',
    { l: 'A new take will', sub: ['Replace the sound', 'Append at the end', 'Punch over the selection', 'Insert at the selection'].map((t, i) => radio(t, () => state.takeMode === i, () => { state.takeMode = i; syncSegs(); }, String(i + 4))) },
    { l: 'Threshold level', sub: [-70, -60, -50, -40, -30, -20, -10].map(v => radio(v + ' dB', () => Math.round(recP.thresh) === v, () => { recP.thresh = v; rebuildRec(); })) },
    { l: 'Pre-roll (on input)', sub: [0, 100, 200, 500, 1000, 3000].map(v => radio(v >= 1000 ? v / 1000 + ' s' : v + ' ms', () => Math.round(recP.pre) === v, () => { recP.pre = v; rebuildRec(); })) },
    { l: 'Input', sub: [radio('Mixer channel (demo drum loop here)', () => state.input === 'demo' && state.demoKind === 'loop', () => chooseInput('loop')), radio('Mixer channel (demo melody here)', () => state.input === 'demo' && state.demoKind === 'melody', () => chooseInput('melody')), radio('Microphone', () => state.input === 'mic', () => chooseInput('mic'))] },
    { l: 'Use the microphone instead of the mixer channel', k: 'M', f: () => chooseInput(state.input === 'demo' ? 'mic' : state.demoKind), ck: () => state.input === 'mic' }
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
(function reg() { const walk = (items, menu) => items.forEach(it => { if (it === '-' || it.hd) return; if (it.sub) { walk(it.sub, menu); return; } if (it.k) { [it.k].concat(it.also || []).forEach(k => KEYS.push({ p: parseKey(k), it, menu })); } }); MENUS.forEach(m => walk(m.items, m.name)); })();
function keyMatch(p, e) {
  const ctrl = e.ctrlKey || e.metaKey; if (p.ctrl !== ctrl || p.alt !== e.altKey) return false;
  const code = e.code || '', k = (e.key || '').toLowerCase();
  if (p.letter) return e.shiftKey === p.shift && (code === 'Key' + p.key.toUpperCase() || (k === p.key && !e.altKey));
  if (/^\d$/.test(p.key)) return (code === 'Digit' + p.key || code === 'Numpad' + p.key || k === p.key) && p.shift === e.shiftKey;
  if (p.shift && !e.shiftKey) return false;
  if (!p.shift && e.shiftKey && [' ', 'enter', 'home', 'end', 'delete', 'backspace', 'escape', 'f1'].includes(p.key)) return false;
  if (p.key === '+') return k === '+' || (k === '=' && true);
  if (p.key === '=') return k === '=' || k === '+';
  if (p.key === '-') return k === '-' || k === '_';
  if (p.key === '?') return k === '?' || (code === 'Slash' && e.shiftKey);
  return k === p.key;
}
// ---- dropdowns: drawn in one layer above everything, kept inside the window ----
let openIdx = -1, activeDD = null, hoverOpened = false; const splitWrap = $('.splitrec'), menuBar = $('#menubar'), popLayer = $('#popLayer');
function ddFor(items) {
  const dd = document.createElement('div'); dd.className = 'dd'; dd.setAttribute('role', 'menu');
  items.forEach(it => {
    if (it === '-') { const s = document.createElement('div'); s.className = 'msep'; dd.appendChild(s); return; }
    if (it.hd) { const h = document.createElement('div'); h.className = 'mh' + (it.det ? ' det' : ''); h.textContent = it.hd; dd.appendChild(h); return; }
    const row = document.createElement('div'); row.className = 'mi'; row.setAttribute('role', 'menuitem'); const en = it.en ? it.en() : true; if (!en) row.classList.add('dis');
    const ck = document.createElement('span'); ck.className = 'ck'; ck.textContent = it.ck && it.ck() ? '✓' : ''; const lb = document.createElement('span'); lb.textContent = it.l; const kk = document.createElement('span'); kk.className = 'k'; kk.textContent = it.sub ? '▸' : (it.k || '');
    row.append(ck, lb, kk); if (it.tip) row.dataset.tip = it.tip;
    if (it.sub) { row.classList.add('has'); row.appendChild(ddFor(it.sub)); }
    else row.addEventListener('click', e => { e.stopPropagation(); closeMenu(); it.f(); });
    row.addEventListener('mouseenter', () => { $$('.mi.hi', row.parentNode).forEach(x => x.classList.remove('hi')); row.classList.add('hi'); activeDD = row.parentNode; if (it.sub) fitSub(row); });
    dd.appendChild(row);
  });
  return dd;
}
function fitSub(row) {
  const sub = $(':scope > .dd', row); if (!sub) return; sub.classList.remove('flip'); sub.style.top = ''; const ur = $('#unit').getBoundingClientRect(), s = state.scale || 1; let r = sub.getBoundingClientRect();
  if (r.right > ur.right - 4) { sub.classList.add('flip'); r = sub.getBoundingClientRect(); } const over = r.bottom - (ur.bottom - 4); if (over > 0) sub.style.top = (-5 - over / s) + 'px';
}
function placeDD(dd, anchor, align) {
  const u = $('#unit'), ur = u.getBoundingClientRect(), s = state.scale || 1, ar = anchor.getBoundingClientRect(); dd.style.left = '0px'; dd.style.top = '0px'; const w = dd.offsetWidth, h = dd.offsetHeight, UW = u.offsetWidth, UH = u.offsetHeight;
  let x = (ar.left - ur.left) / s, y = (ar.bottom - ur.top) / s + 2; if (align === 'right') x = (ar.right - ur.left) / s - w; x = clamp(x, 4, Math.max(4, UW - w - 4));
  if (y + h > UH - 4) { const up = (ar.top - ur.top) / s - h - 2; y = up >= 4 ? up : Math.max(4, UH - h - 4); } dd.style.left = Math.round(x) + 'px'; dd.style.top = Math.round(y) + 'px';
}
function openMenu(i) { closeMenu(); openIdx = i; const w = $$('.mn', menuBar)[i]; w.classList.add('open'); const dd = ddFor(MENUS[i].items); popLayer.appendChild(dd); placeDD(dd, $('.mt', w)); activeDD = dd; hideTip(); }
function closeMenu() { popLayer.innerHTML = ''; hoverOpened = false; $$('.mn', menuBar).forEach(w => w.classList.remove('open')); openIdx = -1; activeDD = null; state.popFor = null; }
function openPop(anchor, items, align) { closeMenu(); openIdx = 98; state.popFor = anchor; const dd = ddFor(items); popLayer.appendChild(dd); placeDD(dd, anchor, align); activeDD = dd; hideTip(); }
MENUS.forEach((m, i) => { const w = document.createElement('div'); w.className = 'mn'; const b = document.createElement('button'); b.className = 'mt'; b.textContent = m.name; b.setAttribute('aria-haspopup', 'true');
  b.addEventListener('click', e => { e.stopPropagation(); if (hoverOpened) { hoverOpened = false; return; } openIdx === i ? closeMenu() : openMenu(i); }); b.addEventListener('mouseenter', () => { if (openIdx >= 0 && openIdx !== i && openIdx < MENUS.length) { openMenu(i); hoverOpened = true; } }); w.appendChild(b); menuBar.appendChild(w); });
menuBar.addEventListener('pointerleave', () => { hoverOpened = false; });
document.addEventListener('pointerdown', e => { if (openIdx >= 0 && !e.target.closest('.mn, .splitrec, .dd, .chip, .popbtn')) closeMenu(); });
$('#bRecOpt').addEventListener('click', e => { e.stopPropagation(); if (openIdx === 99) { closeMenu(); return; } closeMenu(); openIdx = 99; const dd = ddFor(MENUS.find(m => m.name === 'Record').items); popLayer.appendChild(dd); placeDD(dd, $('#bRecOpt'), 'right'); activeDD = dd; hideTip(); });
function menuKey(e) {
  const rows = () => $$(':scope > .mi:not(.dis)', activeDD), cur = () => rows().findIndex(r => r.classList.contains('hi')), setHi = n => { const r = rows(); if (!r.length) return; $$('.mi.hi', activeDD).forEach(x => x.classList.remove('hi')); r[(n + r.length) % r.length].classList.add('hi'); };
  const k = e.key; if (k === 'Escape') { closeMenu(); return true; }
  if (k === 'ArrowDown') { setHi(cur() + 1); return true; } if (k === 'ArrowUp') { setHi(cur() < 0 ? -1 : cur() - 1); return true; }
  const hiRow = rows()[cur()];
  if (k === 'ArrowRight') { if (hiRow && hiRow.classList.contains('has')) { hiRow.classList.add('show'); fitSub(hiRow); activeDD = $(':scope > .dd', hiRow); setHi(0); } else if (openIdx < MENUS.length) openMenu((openIdx + 1) % MENUS.length); return true; }
  if (k === 'ArrowLeft') { const par = activeDD.parentNode; if (par && par.classList.contains('mi')) { par.classList.remove('show'); activeDD = par.parentNode; } else if (openIdx < MENUS.length) openMenu((openIdx + MENUS.length - 1) % MENUS.length); return true; }
  if (k === 'Enter' || k === ' ') { if (hiRow) { if (hiRow.classList.contains('has')) { hiRow.classList.add('show'); fitSub(hiRow); activeDD = $(':scope > .dd', hiRow); setHi(0); } else hiRow.click(); } return true; }
  return false;
}
document.addEventListener('keydown', e => {
  if (openIdx >= 0 && menuKey(e)) { e.preventDefault(); e.stopPropagation(); return; }
  if (e.target.matches && e.target.matches('input, select, textarea')) return;
  if (document.querySelector('.modalbg')) return;
  for (const k of KEYS) { if (!keyMatch(k.p, e)) continue; if (k.it.en && !k.it.en()) continue; e.preventDefault(); k.it.f(); return; }
}, true);
function openKeys() {
  let html = '<div class="keytbl">'; MENUS.forEach(m => { const rows = []; const walk = items => items.forEach(it => { if (it === '-' || it.hd) return; if (it.sub) walk(it.sub); else if (it.k) rows.push(`<div class="r"><span>${it.l}</span><span class="kk">${it.k}</span></div>`); }); walk(m.items); if (rows.length) html += `<section><h4>${m.name}</h4>${rows.join('')}</section>`; });
  html += '</div><div class="hint" style="margin-top:10px">Mouse: drag to select, click for the cursor, Shift click to extend, wheel to zoom, Alt wheel to scroll, double click selects all. Double click an operation to apply it. In the pitch view: drag a note up or down, double click a note to snap it.</div>';
  modal('Keyboard shortcuts', html, [['Close', null, 'on']], m => { m.style.width = 'min(980px, 100%)'; });
}
// ---------- info bar: what Meowcorder found, each one a menu ----------
const chips = {};
function buildInfobar() {
  const ib = $('#infobar'); ib.innerHTML = '';
  const mk = (id, name, desc, items, cls) => { const b = document.createElement('button'); b.className = 'chip ' + (cls || ''); b.id = id; setTip(b, name, desc, ''); b.addEventListener('click', e => { e.stopPropagation(); if (openIdx === 98 && state.popFor === b) { closeMenu(); return; } items(b); }); ib.appendChild(b); chips[id] = b; };
  const sp = () => { const s = document.createElement('span'); s.className = 'sp'; ib.appendChild(s); };
  mk('cCh', 'Channels', 'What the sound is made of, as detected. Click to make it mono (mix, left only or right only), swap the sides or split it into mid and side.', b => openPop(b, chItems()));
  mk('cSr', 'Sample rate', 'The sample rate of this sound, as detected. Click to convert it to another rate, or to change the rate without converting.', b => openPop(b, srItems()));
  mk('cBits', 'Bit depth', 'The bit depth of this sound and of the file you save. Click to change it. Going lower changes the sound, with dither if you want it.', b => openPop(b, bitItems()));
  sp();
  mk('cUnit', 'Time line unit', 'How positions and the time line are shown: time, seconds, samples, or bars and beats.', b => openPop(b, unitItems()));
  mk('cBpm', 'Sample tempo', 'The tempo Meowcorder found in this sound. Click to set it yourself, double it, halve it, or say how many bars the sound is.', b => openPop(b, bpmItems()));
  mk('cSync', 'Sync', 'Free plays the sound at its own speed. Sync plays it at the host tempo. Click to choose, and to pick the warp mode.', b => openPop(b, syncItems()));
  mk('cWarp', 'Warp settings', 'Opens the warp panel: tempo sync, warp mode, transpose, fine tune and formants.', b => openWarp(), 'plain');
}
let infoKey = '';
function syncInfobar() {
  const a = ed.audio, sb = sampleBpm(), r = warp.sync && sb > 0 ? recP.bpm / sb : 1, st = warp.semis + warp.cents / 100, u = UNITS.find(x => x[0] === state.unit)[1].split(' (')[0].replace('Bars and beats', 'Bars:beats');
  const t = { cCh: curChan() === 'ms' ? 'M/S stereo' : (curChan() === 'mono' ? 'Mono' : 'Stereo'), cSr: fmtSr(a.sr || 48000), cBits: bitsName(state.bits), cUnit: u, cBpm: (sb ? sb.toFixed(1) : '--') + ' bpm' + (warp.bpm ? ' (set)' : ''), cSync: warp.sync ? 'Sync x' + r.toFixed(2) : 'Free', cWarp: 'Warp' + (st ? ' ' + (st > 0 ? '+' : '') + +st.toFixed(2) + ' st' : '') + ' >' };
  const key = JSON.stringify(t) + warpActive(); if (key === infoKey) return; infoKey = key; for (const id in t) chips[id].textContent = t[id]; chips.cSync.classList.toggle('hot', warp.sync); chips.cWarp.classList.toggle('hot', warpActive());
}

// ---------- resize ----------
let zoom = 1, base = 1; const ZSTEPS = [.75, 1, 1.25, 1.5, 2], DW = 1400; let DH = 800;
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
const DRAWERS = { wave: drawWave, spec: drawSpec, pitch: drawPitch };
function frame(t) {
  requestAnimationFrame(frame); if (document.hidden || !visible || t - last < (powerSave ? 50 : 16)) return; last = t; ++n;
  if (rec.ring && state.recOn && !state.counting && !rec.isRecording() && !rec.isArmed()) { state.recOn = false; syncRec(); }
  if (rec.isRecording() && rec.recordedFrames() / rec.sr > 1200) stopRecording(); if (rec.ring && (rec.isRecording() || rec.isArmed() || state.counting) && n % 6 === 0) syncRec();
  drawLive(); drawRuler(); visLayers().forEach(l => DRAWERS[l](isBottom(l))); drawOvw(); drawMeter(); if (n % 4 === 0) { drawReadout(); syncInfobar(); if (state.wpen) syncPen(); }
}
// ---------- start ----------
try {
  const s = JSON.parse(store.get('mc.set') || '{}'); tipOn = s.tipOn !== false; powerSave = !!s.powerSave; showHits = s.showHits !== false;
  if (s.layers) LAYERS.forEach(l => { state.layers[l] = !!s.layers[l]; }); if (!visLayers().length) state.layers.wave = true;
  state.combine = !!s.combine; if (s.layout) state.layout = s.layout; if (s.front) state.front = s.front; if (s.unit) state.unit = s.unit; if (s.dither === false) state.dither = false;
  if (s.play) Object.assign(play, s.play); if (s.formant !== undefined) warp.formant = s.formant;
} catch (e) {}
state.pdb = -18; state.wpen = false;

// ---------- icon buttons: icons, tips with shortcuts, states ----------
function keyOfLabel(label) { let k = ''; const walk = items => items.forEach(it => { if (it === '-' || k) return; if (it.l === label && it.k) k = it.k; else if (it.sub) walk(it.sub); }); MENUS.forEach(m => walk(m.items)); return k; }
const TB = [['bLoop', 'loop', 'Loop', 'Repeats the playback again and again.', 'Loop'], ['bStop', 'stop', 'Stop', 'Stops playing and recording.', 'Play or stop'], ['bNew', 'new', 'New empty sound', 'Starts with an empty sound. Undo brings the old one back.', 'New empty sound'], ['bOpen', 'open', 'Open file', 'Opens an audio file. You can also drop a file on the display.', 'Open file...'], ['bSave', 'save', 'Save as WAV', 'Saves the sound as a WAV file. In this web mockup the browser may block the download.', 'Save as WAV'],
  ['bUndo', 'undo', 'Undo', 'Takes back the last change.', 'Undo'], ['bRedo', 'redo', 'Redo', 'Puts back what Undo took away.', 'Redo'], ['bCut', 'cut', 'Cut', 'Moves the selection to the clipboard and closes the gap.', 'Cut'], ['bCopy', 'copy', 'Copy', 'Copies the selection to the clipboard.', 'Copy'], ['bPaste', 'paste', 'Paste', 'Puts the clipboard at the cursor, or over the start of the selection.', 'Paste'], ['bDel', 'del', 'Delete', 'Deletes the selection and closes the gap.', 'Delete selection'], ['bCrop', 'crop', 'Crop', 'Keeps only the selection and throws the rest away.', 'Crop to selection'],
  ['bAll', 'all', 'Select all', 'Selects the whole sound.', 'All'], ['bSnap', 'snap', 'Snap to zero', 'Moves the selection edges to the nearest zero crossing, so cuts do not click.', 'Snap edges to zero crossings'], ['bZin', 'zin', 'Zoom in', 'Shows less of the sound, in more detail.', 'Zoom in'], ['bZout', 'zout', 'Zoom out', 'Shows more of the sound.', 'Zoom out'], ['bFit', 'fit', 'Show everything', 'Fits the whole sound in the display.', 'Show the whole sound'], ['bSel', 'tosel', 'Zoom to selection', 'Fills the display with the selection.', 'Zoom to the selection'],
  ['bHist', 'hist', 'History', 'Every step of this sound, with its own undo that is separate from the host. Click to jump back to any step.', 'History...'], ['bGrab', 'grab', 'Grab the last seconds', 'Keeps what you already heard, even though you were not recording. Uses the pre-roll length.', 'Grab the last seconds']];
function decorateIcons() { $('#bRepeat').innerHTML = svgOf('repeat'); TB.forEach(([id, ic, name, desc, lab]) => { const b = $('#' + id); b.innerHTML = svgOf(ic); setTip(b, name, desc, keyOfLabel(lab)); }); syncIcons(); }
const TAKES = ['Replace', 'Append', 'Punch', 'Insert'];
function syncIcons() {
  const m = $('#bMono'), i = $('#bInput'), k = $('#bMon'), sp = $('#bSpec'); if (!m) return;
  m.innerHTML = svgOf(state.recMono ? 'mono' : 'stereo'); m.classList.toggle('on', state.recMono); setTip(m, state.recMono ? 'Recording in mono' : 'Recording in stereo', 'What a new take is made of. Click to switch. The display shows one lane for a mono sound and two for a stereo one. Operations like Make mono change a sound you already have.', keyOfLabel('Record in mono'));
  i.innerHTML = svgOf(state.input === 'mic' ? 'mic' : 'mixer'); setTip(i, state.input === 'mic' ? 'Input: microphone' : 'Input: mixer channel', 'Where the recording comes from. In the plugin: the mixer channel it sits on. Here: a demo loop stands in, or your microphone. Click to switch.', keyOfLabel('Use the microphone instead of the mixer channel'));
  k.innerHTML = svgOf(state.monitor ? 'spk' : 'spkoff'); k.classList.toggle('on', state.monitor); k.classList.toggle('g', state.monitor); setTip(k, state.monitor ? 'Monitoring the input' : 'Input muted', 'Lets you hear what comes in while you record. Mute it when the sound already plays somewhere else.', keyOfLabel('Monitor the input'));
  sp.innerHTML = svgOf('spec'); sp.classList.toggle('on', state.layers.spec); sp.classList.toggle('g', state.layers.spec); setTip(sp, 'Spectral view', 'Shows the sound as a picture of its frequencies, with tools to cut, erase, copy and amplify parts of it.', keyOfLabel('Show the spectral view'));
  const tl = $('#bTake'); if (tl) tl.textContent = TAKES[state.takeMode];
}
$('#bNew').addEventListener('click', newSound); $('#bOpen').addEventListener('click', () => $('#file').click()); $('#bSave').addEventListener('click', saveWav);
$('#bStop').addEventListener('click', () => { stopPlay(); if (state.counting) cancelCount(); else if (rec.isRecording() || rec.isArmed()) stopRecording(); });
$('#bTake').addEventListener('click', () => { state.takeMode = (state.takeMode + 1) % 4; syncSegs(); });
$('#bMono').addEventListener('click', () => { state.recMono = !state.recMono; syncIcons(); });
$('#bMon').addEventListener('click', () => setMonitor(!state.monitor)); $('#bSpec').addEventListener('click', () => toggleLayer('spec')); $('#pdb').addEventListener('change', e => { state.pdb = +e.target.value; }); $('#inSel').addEventListener('change', e => chooseInput(e.target.value));
buildOps(); buildRec(); selectOp(0); buildViewbar(); $('#vPen').addEventListener('click', toggleWPen); buildInfobar(); buildScope(); buildWarp(); decorateIcons(); syncSegs(); markTool('mouse'); viewFit(); changed(); syncPlay(); syncRec(); syncPen();
const z0 = parseFloat(store.get('mc.zoom')); zoom = z0 > 0 ? clamp(z0, .4, 3) : 1; layoutScale(); layoutDV(); syncInfobar();
window.__mc = { ed, rec, state, opP, MC, OPS, warp, play, MENUS, KEYS };
requestAnimationFrame(frame);
})();