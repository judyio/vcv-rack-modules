#!/bin/bash
# Re-render panel-preview.png (repo root): rasterizes both panel SVGs and
# composites VCV Rack's component graphics at the positions used in
# src/RangeExpander.cpp. Keep the coordinates below in sync with the widget.
#
# Requires: brew install librsvg imagemagick   (and VCV Rack 2 installed)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/design/build"
mkdir -p "$BUILD"
cd "$BUILD"
CL="/Applications/VCV Rack 2 Pro.app/Contents/Resources/res/ComponentLibrary"
RES="$ROOT/res"
S=20  # px per mm

mm2px() { python3 -c "print(round($1 * $S))"; }

# component sizes in mm (svg px * 25.4/75)
KNOB_MM=9.6009   # RoundBlackKnob 28.34759px
PORT_MM=8.0264   # PJ301M 23.7px
LIGHT_MM=3.0
SCREW_MM=5.08    # 15px

rsvg-convert -w $(mm2px $KNOB_MM) "$CL/RoundBlackKnob_bg.svg" -o knob_bg.png
rsvg-convert -w $(mm2px $KNOB_MM) "$CL/RoundBlackKnob.svg" -o knob_fg.png
magick knob_bg.png knob_fg.png -gravity center -composite knob.png
rsvg-convert -w $(mm2px $PORT_MM) "$CL/PJ301M.svg" -o port_light.png
rsvg-convert -w $(mm2px $PORT_MM) "$CL/PJ301M-dark.svg" -o port_dark.png
rsvg-convert -w $(mm2px $LIGHT_MM) "$CL/MediumLight.svg" -o light.png
rsvg-convert -w $(mm2px $SCREW_MM) "$CL/ScrewSilver.svg" -o screw_light.png
rsvg-convert -w $(mm2px $SCREW_MM) "$CL/ScrewBlack.svg" -o screw_dark.png

# centered geometry helper: prints "+X+Y" for top-left of a w_mm-wide part centered at (cx,cy)
geo() { python3 -c "print(f'+{round(($1 - $3/2) * $S)}+{round(($2 - $3/2) * $S)}')"; }

compose() { # $1 panel svg, $2 port png, $3 screw png, $4 out png
  local panel=$1 port=$2 screw=$3 out=$4
  rsvg-convert -w $(mm2px 30.48) "$panel" -o panel_base.png
  cmd=(magick panel_base.png)
  # screws (top-left positioned in code: x=5.08 & 20.32 mm, y=0 & 123.42)
  for pos in "5.08 0" "20.32 0" "5.08 123.42" "20.32 123.42"; do
    read -r sx sy <<< "$pos"
    cmd+=("$screw" -geometry "+$(mm2px $sx)+$(mm2px $sy)" -composite)
  done
  # input jack
  cmd+=("$port" -geometry $(geo 15.0 20.0 $PORT_MM) -composite)
  # overlap + smooth rows
  cmd+=(knob.png -geometry $(geo 8.0 30.0 $KNOB_MM) -composite)
  cmd+=("$port" -geometry $(geo 24.0 30.0 $PORT_MM) -composite)
  cmd+=(knob.png -geometry $(geo 8.0 44.0 $KNOB_MM) -composite)
  cmd+=("$port" -geometry $(geo 24.0 44.0 $PORT_MM) -composite)
  # outputs + lights
  for y in 57.0 68.4 79.8 91.2 102.6 114.0; do
    cmd+=("$port" -geometry $(geo 20.0 $y $PORT_MM) -composite)
    cmd+=(light.png -geometry $(geo 10.0 $y $LIGHT_MM) -composite)
  done
  cmd+=("$out")
  "${cmd[@]}"
}

compose "$RES/RangeExpander.svg" port_light.png screw_light.png preview_light.png
compose "$RES/RangeExpanderDark.svg" port_dark.png screw_dark.png preview_dark.png
magick preview_light.png preview_dark.png +append -background gray -splice 4x0 "$ROOT/panel-preview.png"
echo "wrote $ROOT/panel-preview.png"
