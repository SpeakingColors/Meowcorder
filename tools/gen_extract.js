const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: 1414, height: 896 } });
  await p.goto('file://' + require('path').resolve(__dirname, '../docs/mockups/Meowcorder.html') + '');
  await p.waitForTimeout(1200);
  const stage = await p.evaluate(() => { const r = document.querySelector('#stage').getBoundingClientRect(); return { x: r.x, y: r.y, w: r.width, h: r.height }; });
  const items = await p.evaluate(() => {
    const st = document.querySelector('#stage').getBoundingClientRect();
    const out = [];
    document.querySelectorAll('#stage button, #stage .knob, #stage canvas, #stage select, #stage [role=button]').forEach(e => {
      const r = e.getBoundingClientRect(); if (r.width < 2 || r.height < 2) return;
      const svg = e.querySelector('svg');
      out.push({ tag: e.tagName, id: e.id, cls: (e.className && e.className.baseVal === undefined) ? e.className : '', title: e.title || e.getAttribute('aria-label') || '', text: (e.textContent || '').trim().slice(0, 24),
        x: Math.round(r.x - st.x), y: Math.round(r.y - st.y), w: Math.round(r.width), h: Math.round(r.height), svg: svg ? svg.outerHTML : '', color: getComputedStyle(svg || e).color });
    });
    return out;
  });
  require('fs').writeFileSync(require('path').join(process.env.OUT || '.', 'mock_items.json'), JSON.stringify({ stage, items }, null, 1));
  console.log(JSON.stringify(stage), items.length, 'items;', items.filter(i => i.svg).length, 'with icons');
  console.log(items.slice(0, 12).map(i => [i.tag, i.id, i.title || i.text, i.x, i.y, i.w, i.h].join('|')).join('\n'));
  await b.close();
})();
