const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1414, height: 896 } });
  await p.goto('file://' + require('path').resolve(__dirname, '../docs/mockups/Meowcorder.html') + ''); await p.waitForTimeout(800);
  const r = await p.evaluate(() => { const st = document.querySelector('#stage').getBoundingClientRect(); const s = [...document.querySelectorAll('#stage svg')].find(e => { const r = e.getBoundingClientRect(); return r.y - st.y < 60 && r.x - st.x < 200 && r.width > 20; });
    if (!s) return null; const r2 = s.getBoundingClientRect(); const t = [...document.querySelectorAll('#stage *')].filter(e => e.children.length === 0 && /Meowcorder|STUDIO/.test(e.textContent || '') && e.getBoundingClientRect().y - st.y < 60).map(e => { const q = e.getBoundingClientRect(); return { t: e.textContent.trim(), x: Math.round(q.x - st.x), y: Math.round(q.y - st.y), w: Math.round(q.width), h: Math.round(q.height), fs: getComputedStyle(e).fontSize, ls: getComputedStyle(e).letterSpacing, c: getComputedStyle(e).color, ff: getComputedStyle(e).fontFamily.slice(0, 30), fw: getComputedStyle(e).fontWeight }; });
    return { svg: s.outerHTML, x: Math.round(r2.x - st.x), y: Math.round(r2.y - st.y), w: Math.round(r2.width), h: Math.round(r2.height), texts: t }; });
  require('fs').writeFileSync(require('path').join(process.env.OUT || '.', 'mock_logo.json'), JSON.stringify(r, null, 1)); console.log(JSON.stringify(r).slice(0, 900)); await b.close(); })();
