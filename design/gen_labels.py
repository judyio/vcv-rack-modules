#!/usr/bin/env python3
"""Panel artwork generator for Range Expander.

Outlines label text to SVG paths using Eurostile Next LT Pro (so the panels
need no fonts at runtime — Rack can't render live <text> anyway) and emits
the full LABELS layer: labels, knob glyphs, in/out arrows, and the slice
ladder. All layout constants live here. Run via update_panels.py.

Requires: pip3 install fonttools
"""
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.misc.transform import Transform

FONT = "/Users/judy/Library/Fonts/EurostileNextLTPro-Regular.otf"

OUT_ROWS = [57.0, 68.4, 79.8, 91.2, 102.6, 114.0]

# (text, font_size_mm, center_x, baseline_y)
# output numbers sit between the light (x=10) and the jack (x=20)
LABELS = [
    ("OVERLAP", 2.6, 15.24, 37.2),
    ("SMOOTH",  2.6, 15.24, 51.2),
] + [(str(i + 1), 2.6, 13.75, y + 0.93) for i, y in enumerate(OUT_ROWS)]

TRACKING = 0.06  # em, between letters

# slice ladder geometry
LAD_X, LAD_W = 3.0, 3.2
LAD_TOP = OUT_ROWS[0] - 5.7
CELL_H = 11.4
STEP_OPACITY = 0.35

# ramp-glyph icons, centered between knob (x=8, edge 12.8) and CV jack (x=24, edge 20)
ICON_X0, ICON_X1 = 14.2, 18.7          # icon box, horizontal
ICON_H = 3.5                            # icon box, vertical
ICON_STROKE = 0.25
OVERLAP_ROW_Y, SMOOTH_ROW_Y = 30.0, 44.0

# arrows: line-art shafts with open chevron heads
ARROW_STROKE = 0.3
ARROW_HEAD_BACK = 1.3                   # chevron depth behind the tip
ARROW_HALF_H = 1.2                      # chevron half-height
IN_Y = 20.0
IN_ARROW_TAIL_X, IN_ARROW_TIP_X = 2.8, 10.7    # left edge -> IN jack
OUT_ARROW_TAIL_X, OUT_ARROW_TIP_X = 25.0, 29.6  # output jack -> right edge


def fmt(v):
    return format(round(v, 4), "g")


def text_path(font, text, size, cx, baseline):
    glyphset = font.getGlyphSet()
    cmap = font.getBestCmap()
    upm = font["head"].unitsPerEm
    scale = size / upm
    xs, x = [], 0
    for i, ch in enumerate(text):
        g = glyphset[cmap[ord(ch)]]
        xs.append(x)
        x += g.width
        if i < len(text) - 1:
            x += TRACKING * upm
    width = x * scale
    x0 = cx - width / 2
    pen = SVGPathPen(glyphset, ntos=fmt)
    for ch, gx in zip(text, xs):
        tpen = TransformPen(pen, Transform(scale, 0, 0, -scale, x0 + gx * scale, baseline))
        glyphset[cmap[ord(ch)]].draw(tpen)
    return pen.getCommands(), width


def icon_paths():
    """Return (id, label, d) tuples for the Overlap and Smooth glyphs."""
    x0, x1 = ICON_X0, ICON_X1
    # overlap: crossfade X — neighboring slices hand off gradually
    yb, yt = OVERLAP_ROW_Y + ICON_H / 2, OVERLAP_ROW_Y - ICON_H / 2
    overlap_a = f"M {fmt(x0)},{fmt(yb)} H {fmt(x0 + 0.5)} L {fmt(x1 - 0.5)},{fmt(yt)} H {fmt(x1)}"
    overlap_b = f"M {fmt(x0)},{fmt(yt)} H {fmt(x0 + 0.5)} L {fmt(x1 - 0.5)},{fmt(yb)} H {fmt(x1)}"
    # smooth: smootherstep S-curve, flat entry and exit
    yb, yt = SMOOTH_ROW_Y + ICON_H / 2, SMOOTH_ROW_Y - ICON_H / 2
    xm = (x0 + x1) / 2
    smooth = f"M {fmt(x0)},{fmt(yb)} C {fmt(xm)},{fmt(yb)} {fmt(xm)},{fmt(yt)} {fmt(x1)},{fmt(yt)}"
    return [
        ("icon-overlap-a", "overlap icon ramp a", overlap_a),
        ("icon-overlap-b", "overlap icon ramp b", overlap_b),
        ("icon-smooth", "smooth icon curve", smooth),
    ]


def arrow_paths():
    """Return (id, label, d) tuples for the input and output flow arrows."""
    arrows = [("input-arrow", "input arrow", IN_ARROW_TAIL_X, IN_ARROW_TIP_X, IN_Y)]
    for i, y in enumerate(OUT_ROWS):
        arrows.append((f"out-arrow{i + 1}", f"output arrow {i + 1}",
                       OUT_ARROW_TAIL_X, OUT_ARROW_TIP_X, y))
    out = []
    for pid, plabel, tail, tip, y in arrows:
        back = tip - ARROW_HEAD_BACK
        d = (f"M {fmt(tail)},{fmt(y)} H {fmt(tip)} "
             f"M {fmt(back)},{fmt(y - ARROW_HALF_H)} L {fmt(tip)},{fmt(y)} "
             f"L {fmt(back)},{fmt(y + ARROW_HALF_H)}")
        out.append((pid, plabel, d))
    return out


def emit(ink, layer_id):
    font = TTFont(FONT)
    out = []
    out.append(f'''  <g
     inkscape:groupmode="layer"
     id="{layer_id}"
     inkscape:label="LABELS">''')
    for text, size, cx, baseline in LABELS:
        d, width = text_path(font, text, size, cx, baseline)
        out.append(f'''    <path
       style="fill:{ink};stroke:none"
       d="{d}"
       id="label-{text}-{fmt(baseline)}"
       inkscape:label="{text}" />''')
    # slice ladder: outline + stepped fills, one cell per output row
    out.append(f'''    <rect
       style="fill:none;stroke:{ink};stroke-width:0.15"
       x="{fmt(LAD_X)}" y="{fmt(LAD_TOP)}" width="{fmt(LAD_W)}" height="{fmt(CELL_H * 6)}"
       id="ladder-frame"
       inkscape:label="ladder frame" />''')
    for i in range(6):
        w = LAD_W * (i + 1) / 6
        y = LAD_TOP + CELL_H * i
        out.append(f'''    <rect
       style="fill:{ink};fill-opacity:{STEP_OPACITY};stroke:none"
       x="{fmt(LAD_X)}" y="{fmt(y)}" width="{fmt(w)}" height="{fmt(CELL_H)}"
       id="ladder-step{i + 1}"
       inkscape:label="ladder step {i + 1}" />''')
        if i > 0:
            out.append(f'''    <path
       style="fill:none;stroke:{ink};stroke-width:0.15"
       d="M {fmt(LAD_X)},{fmt(y)} h {fmt(LAD_W)}"
       id="ladder-tick{i + 1}"
       inkscape:label="ladder tick {i + 1}" />''')
    for pid, plabel, d in icon_paths():
        out.append(f'''    <path
       style="fill:none;stroke:{ink};stroke-width:{ICON_STROKE};stroke-linecap:round;stroke-linejoin:round"
       d="{d}"
       id="{pid}"
       inkscape:label="{plabel}" />''')
    for pid, plabel, d in arrow_paths():
        out.append(f'''    <path
       style="fill:none;stroke:{ink};stroke-width:{ARROW_STROKE};stroke-linecap:round;stroke-linejoin:round"
       d="{d}"
       id="{pid}"
       inkscape:label="{plabel}" />''')
    out.append("  </g>")
    return "\n".join(out)


if __name__ == "__main__":
    # fit report: label widths and horizontal spans
    font = TTFont(FONT)
    for text, size, cx, baseline in LABELS:
        _, w = text_path(font, text, size, cx, baseline)
        print(f"{text!r}: width {w:.2f}mm, spans x {cx - w / 2:.2f}..{cx + w / 2:.2f}")
