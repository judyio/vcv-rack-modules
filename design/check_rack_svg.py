#!/usr/bin/env python3
"""Check panel SVGs against the SVG subset that VCV Rack can draw.

Rack parses panels with nanosvg and draws them with NanoVG (src/window/Svg.cpp).
That renderer supports much less than a real SVG renderer such as librsvg, which
is what design/render_preview.sh uses. A panel can therefore look correct in
panel-preview.png and wrong in Rack.

The most dangerous case is a gradient stroke. Rack's stroke code has a case for
a flat colour and a commented-out stub for a linear gradient, and no case at all
for a radial gradient. It then calls nvgStroke() regardless. Rack never sets a
stroke colour, so NanoVG uses its default: opaque black. A hairline you cannot
see in Inkscape becomes a hard black outline in Rack.

Usage:
    python3 design/check_rack_svg.py res/*.svg

Exits 1 if any error is found, 0 otherwise.
"""

import sys
import xml.etree.ElementTree as ET
from pathlib import Path

SVG_NS = "http://www.w3.org/2000/svg"


def tag(elem):
    """Return the local tag name, without the namespace."""
    t = elem.tag
    if isinstance(t, str) and t.startswith("{"):
        return t.split("}", 1)[1]
    return str(t)


def style_of(elem):
    """Return the effective presentation properties of one element.

    Inkscape writes most properties into a style attribute, but presentation
    attributes are also valid. The style attribute wins.
    """
    props = {}
    for name, value in elem.attrib.items():
        if name.startswith("{"):
            continue
        props[name] = value.strip()
    raw = elem.attrib.get("style", "")
    for part in raw.split(";"):
        if ":" not in part:
            continue
        name, _, value = part.partition(":")
        props[name.strip()] = value.strip()
    return props


def is_paint_server(value):
    """Return True if a paint value points at a gradient or a pattern."""
    return value.startswith("url(")


class Report:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.notes = []

    def error(self, path, message):
        self.errors.append((path, message))

    def warn(self, path, message):
        self.warnings.append((path, message))

    def note(self, path, message):
        self.notes.append((path, message))


def describe(elem):
    """Return a short identifier for an element, for the report."""
    label = elem.attrib.get("{http://www.inkscape.org/namespaces/inkscape}label")
    ident = elem.attrib.get("id")
    name = tag(elem)
    if label:
        return f"<{name} inkscape:label=\"{label}\">"
    if ident:
        return f"<{name} id=\"{ident}\">"
    return f"<{name}>"


def check_file(path, report):
    try:
        tree = ET.parse(path)
    except ET.ParseError as exc:
        report.error(path, f"cannot parse the file: {exc}")
        return
    root = tree.getroot()

    gradient_fills = 0
    gradient_stops = {}

    for elem in root.iter():
        name = tag(elem)
        props = style_of(elem)
        where = describe(elem)

        # Rack cannot draw text. Convert every label to a path.
        if name in ("text", "tspan", "flowRoot", "flowPara", "textPath"):
            report.error(path, f"{where} is a text element. Rack cannot render text. Convert it to a path.")
            continue

        # Rack ignores filters, masks, and clipping paths.
        for attr, feature in (("filter", "a filter"), ("mask", "a mask"), ("clip-path", "a clipping path")):
            value = props.get(attr, "none")
            if value not in ("none", ""):
                report.error(path, f"{where} uses {feature} ({attr}=\"{value}\"). Rack ignores it.")

        # CSS is not supported by nanosvg.
        if name == "style":
            report.warn(path, "the file contains a <style> element. nanosvg ignores CSS. Put the properties in a style attribute on each element.")

        # Count the stops of every gradient, to check the two-stop limit later.
        if name in ("linearGradient", "radialGradient"):
            stops = [c for c in elem if tag(c) == "stop"]
            if stops:
                gradient_stops[elem.attrib.get("id", "")] = len(stops)

        # The main check: a gradient or pattern stroke renders as black in Rack.
        stroke = props.get("stroke", "none")
        if is_paint_server(stroke):
            report.error(
                path,
                f"{where} has stroke=\"{stroke}\". Rack draws a gradient stroke as OPAQUE BLACK. "
                "Use a flat colour, or set stroke:none.",
            )

        # A gradient fill works, but Rack uses only the first and the last stop.
        fill = props.get("fill", "")
        if is_paint_server(fill):
            gradient_fills += 1

        # Dashes are parsed but never applied.
        dash = props.get("stroke-dasharray", "none")
        if dash not in ("none", ""):
            report.warn(path, f"{where} uses stroke-dasharray=\"{dash}\". Rack ignores dashes.")

        # paint-order changes which of fill and stroke goes first. Rack always fills first.
        order = props.get("paint-order", "normal")
        if order not in ("normal", ""):
            report.warn(path, f"{where} uses paint-order=\"{order}\". Rack always fills first, then strokes.")

    for ident, count in sorted(gradient_stops.items()):
        if count > 2:
            report.warn(path, f"gradient \"{ident}\" has {count} stops. Rack uses the first and the last stop only.")

    if gradient_fills:
        report.note(path, f"{gradient_fills} shapes use a gradient fill. Rack supports these, but check the panel in Rack, not only in the preview.")


def main(argv):
    paths = [Path(p) for p in argv[1:]]
    if not paths:
        paths = sorted(Path(__file__).resolve().parent.parent.glob("res/*.svg"))
    if not paths:
        print("check_rack_svg: no SVG files to check", file=sys.stderr)
        return 1

    report = Report()
    for path in paths:
        check_file(path, report)

    for path, message in report.notes:
        print(f"note:  {path}: {message}")
    for path, message in report.warnings:
        print(f"WARN:  {path}: {message}")
    for path, message in report.errors:
        print(f"ERROR: {path}: {message}")

    checked = ", ".join(str(p) for p in paths)
    if report.errors:
        print(f"\ncheck_rack_svg: {len(report.errors)} errors in {checked}")
        return 1
    print(f"\ncheck_rack_svg: no errors in {checked}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
