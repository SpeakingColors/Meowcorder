#!/usr/bin/env python3
"""Builds docs/mockups/Meowcorder.html (full page) and Meowcorder.fragment.html (for the artifact tool)
from meowcorder.src.html + engine.js + ops.json + app.js. Run after changing any of them."""
import json, pathlib
d = pathlib.Path(__file__).resolve().parent.parent / "docs" / "mockups"
src = (d / "meowcorder.src.html").read_text()
fix = lambda s: s.replace("</script", "<\\/script")
eng = fix((d / "engine.js").read_text())
app = fix((d / "app.js").read_text())
ops = json.dumps(json.loads((d / "ops.json").read_text()), separators=(",", ":"))
frag = src.replace("/*ENGINE*/", eng, 1).replace("/*OPS*/", ops, 1).replace("/*APP*/", app, 1)
(d / "Meowcorder.fragment.html").write_text(frag)
head = '<!doctype html>\n<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"></head><body>\n'
(d / "Meowcorder.html").write_text(head + frag + "\n</body></html>\n")
print("built", len(frag) // 1024, "KB")
