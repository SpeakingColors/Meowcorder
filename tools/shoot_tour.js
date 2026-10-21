// Renders the real mockup and writes the interface tour for the manual: docs/tour/*.png and docs/tour/tour.json.
// Run: node tools/shoot_tour.js   (needs playwright and a chromium; set CHROMIUM and PW if they are elsewhere)
const PW = process.env.PW || '/home/claude/.npm-global/lib/node_modules/playwright';
const { chromium } = require(PW);
const path = require('path'), fs = require('fs');
const OUT = path.join(__dirname, '..', 'docs', 'tour'); fs.mkdirSync(OUT, { recursive: true });
const URL = 'file://' + path.join(__dirname, '..', 'docs', 'mockups', 'Meowcorder.html');
(async () => {
  const b = await chromium.launch({ executablePath: process.env.CHROMIUM || '/opt/pw-browsers/chromium', args: ['--autoplay-policy=no-user-gesture-required'] });
  const p = await b.newPage({ viewport: { width: 1500, height: 1000 }, deviceScaleFactor: 2 });
  await p.goto(URL); await p.waitForTimeout(1200);
  await p.addStyleTag({ content: '#tip{display:none!important}' });
  const S = f => p.evaluate(f);
  const shot = async (name, sel, pad) => { const r = await p.evaluate(s => { const e = document.querySelector(s).getBoundingClientRect(); return { x: e.left, y: e.top, w: e.width, h: e.height }; }, sel); pad = pad || 0; await p.screenshot({ path: path.join(OUT, name + '.png'), clip: { x: Math.max(0, r.x - pad), y: Math.max(0, r.y - pad), width: r.w + 2 * pad, height: r.h + 2 * pad } }); };
  const wait = ms => p.waitForTimeout(ms || 250);
  // ---- state for the tour: the drum loop with a selection and a few steps in the history
  await S(() => { __mc.ed.setSelection(30000, 110000); });
  await p.click('#bStart'); await wait(900);
  // ---- controls per panel, read from the page (tips are the real info-bubble texts)
  const panels = [
    { key: 'header', title: 'Header', sel: '.top' },
    { key: 'menus', title: 'Menu bar', sel: '.menurow' },
    { key: 'transport', title: 'Transport and recording', sel: '.trow' },
    { key: 'ops', title: 'Operations', sel: '#oplist' },
    { key: 'toolbar', title: 'Tool bar', sel: '#itools' },
    { key: 'info', title: 'Readout and sound info', sel: '#infobar', extra: '#readout' },
    { key: 'meters', title: 'Level meters', sel: '.meterbox' },
    { key: 'display', title: 'Display, views and time line', sel: '#dispwrap' }
  ];
  const tour = { panels: [], shots: {} };
  const read = async (sel, extra) => p.evaluate(([sel, extra]) => {
    const out = []; const root = document.querySelector(sel); const els = [root, ...root.querySelectorAll('[data-tip]')].filter(e => e.dataset && e.dataset.tip);
    for (const e of els) { const t = e.dataset.tip; let name = '', desc = '', key = ''; if (t[0] === '@') { const [d, k, n] = t.slice(1).split('|'); desc = d; key = (k || '').replace(/^Shortcut: /, ''); name = n; } else { desc = t; const FN = { live: 'Live input', wave: 'Waveform display', spec: 'Spectral display', pitch: 'Pitch display', ruler: 'Time line', ovw: 'Whole sound', ampdb: 'Amp level', brush: 'Eraser brush', pdb: 'Pen level', pSnap: 'Drag mode', pSnapAll: 'Snap notes to semitones', fname: 'File name' }; name = FN[e.id] || (e.getAttribute('aria-label') || (e.tagName === 'SELECT' ? '' : e.textContent) || e.id || '').trim().slice(0, 40); } const r = e.getBoundingClientRect(); out.push({ name, desc, key, x: r.left, y: r.top, w: r.width, h: r.height, id: e.id || '' }); }
    return out;
  }, [sel, extra]);
  for (const pn of panels) { const items = await read(pn.sel);
    if (pn.key === 'meters') items.push({ name: 'Level meters and trigger arrow', desc: 'Input (left bar) and output (right bar) level in dBFS, with peak hold. The yellow arrow is the record trigger level, the same value as the Threshold knob: drag it up or down. It starts a recording in On input mode.', key: '' }); if (pn.extra) items.push(...await read(pn.extra)); tour.panels.push({ key: pn.key, title: pn.title, items: items.map(i => ({ name: i.name, desc: i.desc, key: i.key })) }); await shot(pn.key, pn.sel, 6); }
  // ---- full window with numbered badges (pink) on controls and lettered badges (teal) on displays
  const badges = [
    ['#menubar', '1', 0, 'tl'], ['.trow', '2', 0, 'tl'], ['#oplist', '3', 0, 'tl'], ['#itools', '4', 0, 'tl'], ['#cCh', '5'], ['#cSr', '6'], ['#cBits', '7'], ['#cBpm', '8'], ['#cSync', '9'], ['#viewbar', '10', 0, 'tl'], ['#vPen', '11'],
    ['#live', 'A', 1], ['#dv', 'B', 1], ['#ruler', 'C', 1], ['#ovw', 'D', 1], ['.meterbox', 'E', 1], ['#readout', 'F', 1]
  ];
  await p.mouse.move(5, 5); await p.evaluate(() => { document.querySelector('#tip').hidden = true; });
  await p.evaluate(bs => { for (const [sel, lab, teal, pos] of bs) { const e = document.querySelector(sel); if (!e) continue; const r = e.getBoundingClientRect(); const d = document.createElement('div'); d.className = 'tourbadge'; d.textContent = lab; const x = teal ? r.left + r.width - 30 : (pos === 'tl' ? r.left - 12 : r.left + r.width - 12), y = teal ? r.top + 2 : r.top - 10; d.style.cssText = `position:fixed;z-index:9999;left:${x}px;top:${y}px;min-width:22px;height:22px;border-radius:11px;padding:0 5px;box-sizing:border-box;background:${teal ? '#02D9C0' : '#FF1383'};color:#06060a;font:700 13px/22px sans-serif;text-align:center;box-shadow:0 0 0 2px #000a`; document.body.appendChild(d); } }, badges);
  await shot('window', '#unit', 0);
  tour.badges = badges.map(([sel, lab, teal]) => ({ sel, lab, teal: !!teal }));
  await p.evaluate(() => document.querySelectorAll('.tourbadge').forEach(e => e.remove()));
  // ---- the three views
  await shot('view_wave', '#dispwrap', 4); tour.shots.view_wave = 'Waveform view with a selection. The yellow line over the live strip and the arrow in the meter are the record trigger.';
  await p.keyboard.press('Shift+Q'); await wait(400);
  const wb = await p.locator('#wave').boundingBox();
  await p.mouse.move(wb.x + wb.width * .2, wb.y + wb.height * .3); await p.mouse.down(); await p.mouse.move(wb.x + wb.width * .45, wb.y + wb.height * .75, { steps: 6 }); await p.mouse.move(wb.x + wb.width * .7, wb.y + wb.height * .3, { steps: 6 }); await p.mouse.up(); await wait(400);
  await shot('view_pencil', '#dispwrap', 4); await p.keyboard.press('Control+z'); await p.keyboard.press('Shift+Q'); await p.click('#bFit'); await wait();
  await p.evaluate(() => document.querySelector('#vSeg button[data-l=spec]').click()); await wait(500);
  await p.evaluate(() => document.querySelector('#toolSeg button[data-tool=pencil]').click());
  const sb = await p.locator('#spec').boundingBox();
  await p.mouse.move(sb.x + sb.width * .25, sb.y + sb.height * .65); await p.mouse.down(); for (let i = 0; i <= 24; i++) await p.mouse.move(sb.x + sb.width * (.25 + i * .012), sb.y + sb.height * (.65 - i * .01 - (i % 6) * .01)); await p.mouse.up(); await wait(700);
  await shot('view_spec', '#dispwrap', 4);
  await p.keyboard.press('Control+z'); await wait();
  // pitch with the melody
  await p.evaluate(() => { document.querySelector('#menubar .mt').click(); }); await wait(100); await p.keyboard.press('Escape');
  await p.click('#menubar .mt:text-is("File")'); await p.hover('#popLayer .mi:has-text("Demo sounds")'); await p.click('#popLayer .dd .dd .mi:has-text("Melody")'); await wait(400);
  await p.evaluate(() => document.querySelector('#vSeg button[data-l=pitch]').click()); await wait(900); await shot('view_pitch', '#dispwrap', 4);
  // split with all three, back to the loop
  await p.click('#menubar .mt:text-is("File")'); await p.hover('#popLayer .mi:has-text("Demo sounds")'); await p.click('#popLayer .dd .dd .mi:has-text("Drum loop")'); await wait(400);
  await p.evaluate(() => { document.querySelector('#vComb').click(); document.querySelector('#vSeg button[data-l=wave]').click(); document.querySelector('#vSeg button[data-l=spec]').click(); }); await wait(300);
  await p.evaluate(() => document.querySelector('#vLay button[data-v=split]').click()); await wait(700); await shot('view_split', '#dispwrap', 4);
  await p.evaluate(() => { document.querySelector('#vLay button[data-v=overlay]').click(); }); await wait(600); await shot('view_overlay', '#dispwrap', 4);
  await p.evaluate(() => { document.querySelector('#vComb').click(); document.querySelector('#vSeg button[data-l=wave]').click(); }); await wait(300);
  // ---- popups and menus, drawn over the whole window
  await S(() => { __mc.ed.setSelection(30000, 110000); });
  await p.keyboard.press('Shift+G'); await wait(200); await p.keyboard.press('Shift+W'); await wait(200); await p.keyboard.press('Control+Shift+z'); await wait(200);
  await p.click('#oplist button[data-op="0"]'); await wait(300); await shot('pop_process', '#oppop', 14); await p.keyboard.press('Escape');
  await p.click('#bHist'); await wait(300); await shot('pop_history', '#histpop', 14); await p.keyboard.press('Escape');
  await p.click('#cWarp'); await wait(300); await shot('pop_warp', '#warppop', 14); await p.keyboard.press('Escape');
  await p.click('#cCh'); await wait(250); await shot('menu_channels', '#popLayer .dd', 14); await p.keyboard.press('Escape');
  await p.click('#cBits'); await wait(250); await shot('menu_bits', '#popLayer .dd', 14); await p.keyboard.press('Escape');
  await p.click('#menubar .mt:text-is("View")'); await wait(250); await shot('menu_view', '#popLayer .dd', 14); await p.keyboard.press('Escape');
  await p.click('#menubar .mt:text-is("Record")'); await wait(250); await shot('menu_record', '#popLayer .dd', 14); await p.keyboard.press('Escape');
  await p.click('#menubar .mt:text-is("Help")'); await p.click('#popLayer .mi:has-text("Settings")'); await wait(300); await shot('pop_settings', '.modal', 14); await p.keyboard.press('Escape');
  // ---- shortcut table straight from the menus
  tour.keys = await S(() => __mc.MENUS.map(m => { const rows = []; const walk = items => items.forEach(it => { if (it === '-' || it.hd) return; if (it.sub) walk(it.sub); else if (it.k) rows.push([it.l, it.k]); }); walk(m.items); return { menu: m.name, rows }; }).filter(m => m.rows.length));
  tour.menus = await S(() => __mc.MENUS.map(m => ({ menu: m.name, items: m.items.filter(i => i !== '-' && !i.hd).map(i => i.l) })));
  tour.play = await S(() => __mc.play);
  fs.writeFileSync(path.join(OUT, 'tour.json'), JSON.stringify(tour, null, 1));
  console.log('tour written:', fs.readdirSync(OUT).length, 'files');
  await b.close();
})();
