import json, re, sys
import os; S=os.environ.get('OUT','.')+'/'; HERE=os.path.dirname(os.path.abspath(__file__))
rows=json.load(open(S+'mock_icons.json'))['rows']
items=json.load(open(S+'mock_items.json'))['items']
def hexcol(c):
    m=re.match(r'rgba?\((\d+), (\d+), (\d+)',c); return '#%02X%02X%02X'%tuple(int(v) for v in m.groups())
def svg_for(x):
    col=hexcol(x['color'])
    inner=re.sub(r'^<svg[^>]*>|</svg>$','',x['svg']).replace('currentColor',col)
    inner=re.sub(r'></\w+>','/>',inner)
    return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20"><g fill="none" stroke="%s" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">%s</g></svg>'%(col,inner), col
out=['// Generated from docs/mockups/Meowcorder.html by tools/gen_layout.py. Do not edit by hand.','#pragma once','namespace icons {','struct Def { const char* key; const char* svg; unsigned colour; };','static const Def kIcons[] = {']
lay=['// Generated from docs/mockups/Meowcorder.html (positions in the 1382 x 876 design window). Do not edit by hand.','#pragma once','namespace layout {','struct R { int x, y, w, h; };','constexpr int kW = 1382, kH = 876;']
ops={}
for x in rows:
    if x['cls'].startswith('knob'): continue
    key=('op%s'%x['data']['op']) if 'op' in x['data'] else x['id']
    svg,col=svg_for(x)
    out.append('    { "%s", R"SVG(%s)SVG", 0x%s },'%(key,svg,col[1:]))
    if 'op' in x['data']:
        parts=x['data']['tip'].lstrip('@').split('|')
        ops[int(x['data']['op'])]=(x['x'],x['y'],x['w'],x['h'],parts[0],[p for p in parts if p.startswith('Shortcut')],parts[-1],col)
out+=['};','} // namespace icons','']
open(HERE+'/../src/plugin/Icons.h','w').write('\n'.join(out))
# layout
named={}
for it in items:
    k=it['id'] or ''
    if k: named[k]=(it['x'],it['y'],it['w'],it['h'])
for k,(x,y,w,h) in named.items(): lay.append('constexpr R %s{%d,%d,%d,%d};'%(k,x,y,w,h))
byText={}
for it in items:
    t=(it['title'] or it['text']).strip()
    if not it['id'] and t and it['tag']=='BUTTON': byText.setdefault(t,(it['x'],it['y'],it['w'],it['h']))
for name,t in [('mFile','File'),('mEdit','Edit'),('mSelect','Select'),('mView','View'),('mSpectral','Spectral'),('mWarp','Warp'),('mPlay','Play'),('mRecord','Record'),('mProcess','Process'),('mHelp','Help'),('tNow','Now'),('tOnInput','On input'),('tCountIn','Count-in'),('tWave','Waveform'),('tPitch','Pitch'),('tPitch2','Pitch')]:
    if t in byText: lay.append('constexpr R %s{%d,%d,%d,%d};'%((name,)+byText[t]))
# knobs
kn=[it for it in items if it['tag']=='DIV']
for name,it in zip(['kThreshold','kTempo','kPreroll'],kn): lay.append('constexpr R %s{%d,%d,%d,%d};'%(name,it['x'],it['y'],it['w'],it['h']))
vals=[it for it in items if it['tag']=='BUTTON' and re.search(r'(dB|bpm|ms)$',it['text']) and it['y']==148]
for name,it in zip(['vThreshold','vTempo','vPreroll'],vals): lay.append('constexpr R %s{%d,%d,%d,%d};'%(name,it['x'],it['y'],it['w'],it['h']))
lay.append('constexpr R bTakeBtn{242,125,192,19};')
lay.append('struct OpCell { int x, y, w, h; };')
lay.append('static const OpCell kOpCells[24] = {')
for i in range(24): x,y,w,h=ops[i][:4]; lay.append('    {%d,%d,%d,%d},'%(x,y,w,h))
lay.append('};')
lay.append('constexpr R opCell(int op) { return R{ kOpCells[op].x, kOpCells[op].y, kOpCells[op].w, kOpCells[op].h }; }')
lay.append('struct OpMeta { const char* tip; const char* shortcut; const char* name; unsigned colour; };')
lay.append('static const OpMeta kOpMeta[24] = {')
for i in range(24):
    _,_,_,_,tip,sc,nm,col=ops[i]; lay.append('    { "%s", "%s", "%s", 0x%s },'%(tip.replace('"','\\"'),(sc[0].replace('Shortcut: ','') if sc else ''),nm,col[1:]))
lay.append('};')
lay.append('struct Tip { const char* key; const char* text; const char* shortcut; const char* name; };')
lay.append('static const Tip kTips[] = {')
for x in rows:
    if 'op' in x['data'] or x['cls'].startswith('knob'): continue
    parts=x['data'].get('tip','').lstrip('@').split('|')
    sc=[q.replace('Shortcut: ','') for q in parts if q.startswith('Shortcut')]
    lay.append('    { "%s", "%s", "%s", "%s" },'%(x['id'],parts[0].replace('"','\\"'),sc[0] if sc else '',parts[-1].replace('"','\\"')))
lay.append('};')
logo=json.load(open(S+'mock_logo.json'))
lay.append('constexpr R logo{%d,%d,%d,%d};'%(logo['x'],logo['y'],logo['w'],logo['h']))
lay.append('constexpr R title{93,16,131,28};')
lay.append('constexpr const char* kLogoSvg = R"SVG(%s)SVG";'%logo['svg'].replace('<svg ','<svg xmlns="http://www.w3.org/2000/svg" ').replace('></path>','/>'))
lay+=['} // namespace layout','']
open(HERE+'/../src/plugin/Layout.h','w').write('\n'.join(lay))
print('icons',len([l for l in out if l.startswith('    { "')]),'ops',len(ops))
