#!/usr/bin/env python3
"""Regenerate the LABELS layer in both panel SVGs (res/RangeExpander*.svg).

Replaces the existing layer in place; everything else in the SVGs
(background, logo, title, hidden components layer) is left untouched.
Layout constants live in gen_labels.py.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_labels

ROOT = Path(__file__).resolve().parent.parent

ANCHOR = '''  <g
     inkscape:groupmode="layer"
     id="layer2"
     inkscape:label="components"'''

TARGETS = [
    (ROOT / "res/RangeExpander.svg", "#1f1f1f", "layerLabels"),
    (ROOT / "res/RangeExpanderDark.svg", "#ffffff", "layerLabelsDark"),
]

LAYER_RE = re.compile(
    r'  <g\n     inkscape:groupmode="layer"\n     id="layerLabels[^"]*"\n'
    r'     inkscape:label="LABELS">.*?\n  </g>\n', re.S)

for svg, ink, layer_id in TARGETS:
    src = svg.read_text()
    src, n = LAYER_RE.subn("", src)
    assert src.count(ANCHOR) == 1, f"components-layer anchor not found in {svg}"
    src = src.replace(ANCHOR, gen_labels.emit(ink, layer_id) + "\n" + ANCHOR)
    svg.write_text(src)
    print(f"{svg.name}: replaced {n} layer(s)")
