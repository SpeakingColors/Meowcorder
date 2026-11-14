const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: 1414, height: 896 } });
  await p.goto('file://' + require('path').resolve(__dirname, '../docs/mockups/Meowcorder.html') + '');
  await p.waitForTimeout(1200);
  const r = await p.evaluate(() => {
    const st = document.querySelector('#stage').getBoundingClientRect();
    const rows = [];
    document.querySelectorAll('#stage button, #stage .knob').forEach(e => {
      const r = e.getBoundingClientRect(); if (r.width < 2) return;
      const svg = e.querySelector('svg'); if (!svg) return;
      const cs = getComputedStyle(e);
      rows.push({ id: e.id, data: Object.assign({}, e.dataset), cls: String(e.className), x: Math.round(r.x - st.x), y: Math.round(r.y - st.y), w: Math.round(r.width), h: Math.round(r.height),
        svg: svg.outerHTML, color: getComputedStyle(svg).color, stroke: getComputedStyle(svg).stroke, bg: cs.backgroundColor, tip: e.getAttribute('data-tip') || e.title || '' });
    });
    const css = []; for (const s of document.styleSheets) { try { for (const rule of s.cssRules) css.push(rule.cssText); } catch (e) {} }
    return { rows, bodyBg: getComputedStyle(document.body).backgroundColor, stageCss: css.filter(c => /^#stage|\.stage|\.cheek|\.wood/.test(c)).slice(0, 8) };
  });
  require('fs').writeFileSync(require('path').join(process.env.OUT || '.', 'mock_icons.json'), JSON.stringify(r, null, 1));
  console.log(r.rows.length, 'icon rows');
  console.log(r.rows.slice(10, 16).map(x => JSON.stringify({ id: x.id, data: x.data, cls: x.cls, color: x.color, tip: x.tip.slice(0, 40) })).join('\n'));
  console.log(r.stageCss.join('\n').slice(0, 900));
  await b.close();
})();
