#!/usr/bin/env python3
"""Writes docs/mockups/ops.json from src/core/MeowOps.h, so the mockup, the manual and the engine share one table. Run: python3 tools/gen_ops_json.py"""
import json, pathlib, re
root = pathlib.Path(__file__).resolve().parent.parent
src = (root / "src/core/MeowOps.h").read_text()
body = src[src.index("kOps[opCount] = {"):]
S = r'"((?:[^"\\]|\\.)*)"'; N = r'(-?[\d.]+)f?'
ops = []
for m in re.finditer(r'\{\s*' + S + r',\s*' + S + r',\s*' + S + r',\s*' + S + r',\s*(true|false),\s*(\d+),\s*\{(.*?)\}\s*\}(?=,\n|\n\};)', body, re.S):
    id_, name, group, tip, chg, n, ps = m.groups()
    params = [dict(id=a, name=b, lo=float(lo), hi=float(hi), **{"def": float(de)}, unit=u, tip=t, choice=(c == "true"), choices=(ch.split("|") if c == "true" else []))
              for a, b, lo, hi, de, u, t, c, ch in re.findall(r'\{\s*' + S + r',\s*' + S + r',\s*' + N + r',\s*' + N + r',\s*' + N + r',\s*' + S + r',\s*' + S + r',\s*(true|false),\s*' + S + r'\s*\}', ps)]
    assert len(params) == int(n), (id_, len(params), n)
    ops.append(dict(id=id_, name=name, group=group, tip=tip, changesLength=(chg == "true"), params=params))
assert len(ops) == 24, len(ops)
(root / "docs/mockups/ops.json").write_text(json.dumps(ops, indent=1))
print("wrote ops.json", len(ops), "operations")
