#!/usr/bin/env python3
"""Builds docs/Meowcorder_manual.pdf from docs/manual.txt and the interface tour (docs/tour, made by tools/shoot_tour.js)."""
import json, os, re, sys
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Image, Table, TableStyle, PageBreak, KeepTogether, ListFlowable, ListItem
from PIL import Image as PI

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
TOUR = os.path.join(ROOT, 'docs', 'tour')
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'docs', 'Meowcorder_manual.pdf')
PINK, TEAL, INK, DIM = colors.HexColor('#FF1383'), colors.HexColor('#0aa896'), colors.HexColor('#15161b'), colors.HexColor('#5d6270')
H1 = ParagraphStyle('h1', fontName='Helvetica-Bold', fontSize=20, leading=24, textColor=INK, spaceBefore=4, spaceAfter=8)
H2 = ParagraphStyle('h2', fontName='Helvetica-Bold', fontSize=13, leading=16, textColor=PINK, spaceBefore=8, spaceAfter=4)
B = ParagraphStyle('b', fontName='Helvetica', fontSize=10, leading=14, textColor=INK, spaceAfter=4)
SM = ParagraphStyle('sm', parent=B, fontSize=8.2, leading=10.4, spaceAfter=0)
SMB = ParagraphStyle('smb', parent=SM, fontName='Helvetica-Bold')
CAP = ParagraphStyle('cap', parent=B, fontSize=8.5, leading=11, textColor=DIM, spaceAfter=8)
esc = lambda t: t.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
tour = json.load(open(os.path.join(TOUR, 'tour.json')))
W = A4[0] - 36 * mm

def img(name, width=None, maxh=None):
    p = os.path.join(TOUR, name + '.png'); w, h = PI.open(p).size; width = width or W
    if maxh and width * h / w > maxh: width = maxh * w / h
    return Image(p, width=width, height=width * h / w)

def sections():
    out, cur = {}, None
    for line in open(os.path.join(ROOT, 'docs', 'manual.txt'), encoding='utf8'):
        line = line.rstrip('\n')
        if line.startswith('## '): cur = line[3:].strip(); out[cur] = []
        elif cur and line.strip(): out[cur].append(line)
    return out

def block(lines):
    fl, items = [], []
    def flush():
        nonlocal items
        if items: fl.append(ListFlowable([ListItem(Paragraph(esc(t), B), leftIndent=12) for t in items], bulletType='bullet', start='•', leftIndent=12, bulletFontSize=8)); items = []
    for l in lines:
        if l.startswith('- '): items.append(l[2:])
        else: flush(); fl.append(Paragraph(esc(l), B))
    flush(); return fl

def table(rows, widths, head):
    data = [[Paragraph(h, SMB) for h in head]] + [[Paragraph(esc(str(c)), SM) for c in r] for r in rows]
    t = Table(data, colWidths=widths, repeatRows=1)
    t.setStyle(TableStyle([('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#eceef3')), ('LINEBELOW', (0, 0), (-1, -1), .25, colors.HexColor('#c9ccd6')), ('VALIGN', (0, 0), (-1, -1), 'TOP'), ('TOPPADDING', (0, 0), (-1, -1), 2.5), ('BOTTOMPADDING', (0, 0), (-1, -1), 2.5)]))
    return t

def footer(c, d):
    c.saveState(); c.setFont('Helvetica', 8); c.setFillColor(DIM); c.drawString(18 * mm, 10 * mm, 'Meowcorder, Studio 87'); c.drawRightString(A4[0] - 18 * mm, 10 * mm, str(d.page)); c.restoreState()

S = sections(); story = []
# cover
story += [Spacer(1, 40 * mm), Paragraph('Meowcorder', ParagraphStyle('t', parent=H1, fontSize=40, leading=46)), Paragraph('Record, cut and reshape sound. Part of the Studio 87 cat plugin suite.', ParagraphStyle('s', parent=B, fontSize=13, leading=18, textColor=DIM)), Spacer(1, 12 * mm), img('window', W), Spacer(1, 6 * mm), Paragraph('Free and open source, AGPL-3.0-or-later. Collects nothing, makes no network calls.', CAP), PageBreak()]
for k in ['Intro', 'Quick start']:
    story += [Paragraph(k, H1)] + block(S[k])
story.append(PageBreak())
# interface tour
story += [Paragraph('Interface tour', H1), Paragraph('Pink numbers mark controls, teal letters mark displays. Every control has an info bubble: hover it for one second.', B), img('window', W)]
legend = [('1', 'Menu bar: every action with its shortcut'), ('2', 'Transport and recording: loop, play, stop, record, start mode, take mode, Threshold, Tempo, Pre-roll'), ('3', 'Operations: 24 effects and edits'), ('4', 'Tool bar: file, undo, redo, history, edit, zoom, repeat, grab, input'), ('5', 'Channels: what the sound is made of, click to change'), ('6', 'Sample rate: click to convert'), ('7', 'Bit depth: click to change'), ('8', 'Sample tempo: click to set'), ('9', 'Sync: free or synced to the host, warp mode'), ('10', 'View bar: Waveform, Spectral, Pitch, layers'), ('11', 'Waveform pencil')]
dis = [('A', 'Live input: the last 3 seconds, pink band is the pre-roll'), ('B', 'Main display: the view you are working in'), ('C', 'Time line'), ('D', 'Whole sound: drag the light box to scroll'), ('E', 'Level meters: input and output, with the trigger arrow'), ('F', 'Readout: length, selection, cursor, peak')]
story += [Spacer(1, 3 * mm), table(legend + dis, [14 * mm, W - 14 * mm], ['', 'What it is']), Spacer(1, 3 * mm), Paragraph('Signal flow: input (mixer channel or microphone) > always-listening ring buffer > recording > editor (selection, operations, views, warp) > playback or drag back into the project.', B), PageBreak()]
# panels
for pn in tour['panels']:
    items = [i for i in pn['items'] if i['name'] and i['desc']]; seen = set(); rows = []
    for i in items:
        k = (i['name'], i['desc'])
        if k in seen: continue
        seen.add(k); rows.append([i['name'], i['key'], i['desc']])
    if pn['key'] == 'menus': rows = [[m['menu'], '', ', '.join(m['items'])] for m in tour['menus']]
    story += [Paragraph(pn['title'], H1), img(pn['key'], W, 55 * mm), Spacer(1, 3 * mm), table(rows, [34 * mm, 28 * mm, W - 62 * mm], ['Control', 'Shortcut', 'What it does'])]
    story.append(Spacer(1, 6 * mm))
# features with screenshots
def feature(title, shots, key):
    f = [Paragraph(title, H1)] + block(S[key])
    for n, cap in shots: f += [Spacer(1, 2 * mm), img(n, W, 85 * mm), Paragraph(cap, CAP)]
    return f
story += feature('Recording', [('view_wave', 'Waveform view with a selection. The arrow in the meter on the right is the record trigger level.')], 'Recording') + [PageBreak()]
story += feature('Editing and history', [('pop_history', 'History: click any step to go back to it, or forward again. This list is Meowcorder\'s own; your DAW\'s undo is not touched.')], 'Editing and history') + [PageBreak()]
story += feature('Operations', [('pop_process', 'An operation popup: knobs, Mix, and Apply to Selection or Whole sound.')], 'Operations') + [PageBreak()]
story += [Paragraph('Views', H1)] + block(S['Views']) + [img('view_spec', W, 80 * mm), Paragraph('Spectral view with the Pencil: the drawn line is real sound added to the file.', CAP), img('view_pitch', W, 80 * mm), Paragraph('Pitch view: the notes found in a melody, ready to drag.', CAP), PageBreak(), img('view_split', W, 100 * mm), Paragraph('Split: Waveform, Spectral and Pitch stacked. The view In front gets your edits.', CAP), img('view_overlay', W, 100 * mm), Paragraph('Overlay: the views on top of each other.', CAP), PageBreak()]
story += feature('Pencils', [('view_pencil', 'Waveform pencil at sample level: the drawn wave replaces the samples under the pen.')], 'Pencils') + [PageBreak()]
story += feature('Timing and warp', [('pop_warp', 'The warp panel: sync, warp mode, transpose, fine tune, formants, and Bake in.')], 'Timing and warp') + [PageBreak()]
story += [Paragraph('Sound info', H1)] + block(S['Sound info']) + [img('menu_channels', W * .8, 80 * mm), Paragraph('The Channels button shows what was detected and offers the changes.', CAP), img('menu_bits', W * .8, 80 * mm), Paragraph('The bit depth button, with dither for going lower.', CAP), PageBreak()]
story += [Paragraph('Menus and settings', H1), img('menu_view', W * .8, 80 * mm), Paragraph('The View menu. Every menu shows its shortcuts.', CAP), img('pop_settings', W * .8, 80 * mm), Paragraph('Settings, including the Play keys.', CAP), PageBreak()]
story += [Paragraph('Play keys', H1)] + block(S['Play keys'])
pl = tour.get('play', {})
if pl: story.append(Paragraph('Defaults: Space = ' + ' then '.join(pl.get('space', [])) + '; Shift+Space = ' + ' then '.join(pl.get('shift', [])) + '; the Play button = ' + ' then '.join(pl.get('button', [])) + '.', B))
story.append(Paragraph('Keyboard shortcuts', H2))
for m in tour['keys']:
    story.append(KeepTogether([Paragraph(m['menu'], ParagraphStyle('mh', parent=B, fontName='Helvetica-Bold', spaceBefore=4, spaceAfter=2)), table(m['rows'], [W - 40 * mm, 40 * mm], ['Action', 'Shortcut'])]))
story.append(PageBreak())
for k in ['Install', 'Under the hood', 'Support']:
    story += [Paragraph(k, H1)] + block(S[k])
SimpleDocTemplate(OUT, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=16 * mm, bottomMargin=18 * mm, title='Meowcorder manual', author='Studio 87').build(story, onFirstPage=lambda c, d: None, onLaterPages=footer)
print('wrote', OUT)
