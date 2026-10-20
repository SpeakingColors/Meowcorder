// Checks that the JavaScript engine (docs/mockups/engine.js) gives the same result as the C++ core on every operation.
// Run: g++ -std=c++17 -O2 -o /tmp/pdump tools/parity_dump.cpp && /tmp/pdump > /tmp/cpp_par.txt && node tools/parity.js /tmp/cpp_par.txt
const fs = require('fs'), path = require('path'), MC = require('../docs/mockups/engine.js'), OPS = require('../docs/mockups/ops.json');
const a = new MC.Audio(48000, 24000);
for (let i = 0; i < 24000; ++i) { const t = i / 48000; a.l[i] = 0.4 * Math.sin(2 * Math.PI * 440 * t) + 0.2 * Math.sin(2 * Math.PI * 3100 * t + 1) + (i % 6000 < 300 ? 0.3 * Math.exp(-(i % 6000) / 80) : 0); a.r[i] = 0.35 * Math.sin(2 * Math.PI * 440 * t + 0.4) + 0.25 * Math.sin(2 * Math.PI * 2300 * t); }
const cpp = fs.readFileSync(process.argv[2], 'utf8').trim().split('\n').map(l => l.split(' '));
let bad = 0, checks = 0;
OPS.forEach((o, op) => {
  const b = a.clone(), e = MC.applyOp(b, op, 2000, 20000, MC.defaultParams(OPS, op));
  let s0 = 0, s1 = 0, q = 0; for (let i = 0; i < b.frames; ++i) { s0 += b.l[i]; s1 += b.r[i]; q += b.l[i] * b.l[i] + b.r[i] * b.r[i]; }
  const js = [o.id, b.frames, e, s0, s1, q, b.l[b.frames >> 1], b.r[Math.floor(b.frames / 3)]], c = cpp[op];
  const ok = c[0] === js[0] && +c[1] === js[1] && +c[2] === js[2] && [3, 4, 5, 6, 7].every(k => Math.abs(+c[k] - js[k]) <= 2e-3 * Math.max(1, Math.abs(+c[k])) + 5e-3);
  ++checks; if (!ok) { ++bad; console.log('MISMATCH', o.id, '\n  cpp', c.join(' '), '\n  js ', js.map(x => typeof x === 'number' ? x.toFixed(6) : x).join(' ')); }
});
console.log(checks + ' operations compared, ' + bad + ' differ'); process.exit(bad ? 1 : 0);
