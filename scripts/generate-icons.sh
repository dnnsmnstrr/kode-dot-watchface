#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCREENSHOT="$ROOT_DIR/screenshot_emery.png"
FONT="$ROOT_DIR/Outfit/static/Outfit-Bold.ttf"
TEMP_SCREENSHOT="$(mktemp "${TMPDIR:-/tmp}/kode-dot-screenshot.XXXXXX.png")"
TEMP_ARTWORK="$(mktemp "${TMPDIR:-/tmp}/kode-dot-artwork.XXXXXX.png")"
trap 'rm -f "$TEMP_SCREENSHOT" "$TEMP_ARTWORK"' EXIT

if ! command -v magick >/dev/null 2>&1; then
  echo "ImageMagick is required (install it with: brew install imagemagick)" >&2
  exit 1
fi

# Replace the old system-font time with the Outfit time used by the watchface.
magick "$SCREENSHOT" \
  -fill black -draw 'rectangle 0,20 199,78' \
  -font "$FONT" -fill '#F5F5F0' -gravity north \
  -pointsize 42 -annotate +0+25 '21:40' \
  -strip "$TEMP_SCREENSHOT"
mv "$TEMP_SCREENSHOT" "$SCREENSHOT"

# Basalt scales the 200x228 illustration to 144x164 and bottom-aligns it on
# its 144x168 display. Its text layer keeps the native 42px Outfit font.
magick "$SCREENSHOT" \
  -fill black -draw 'rectangle 0,0 199,78' \
  -filter point -resize 144x164! -strip "$TEMP_ARTWORK"
magick -size 144x168 canvas:black \
  "$TEMP_ARTWORK" -gravity south -composite \
  -font "$FONT" -fill '#F5F5F0' -gravity north \
  -pointsize 42 -annotate +0+8 '21:40' \
  -strip "$ROOT_DIR/screenshot_basalt.png"

# Preserve the original composition: scale by width, then crop the bottom.
magick "$SCREENSHOT" -filter point -resize 144x164! \
  -gravity north -crop 144x144+0+0 +repage -strip "$ROOT_DIR/icon_144x144.png"
magick "$SCREENSHOT" -filter point -resize 80x91! \
  -gravity north -crop 80x80+0+0 +repage -strip "$ROOT_DIR/icon_80x80.png"

for icon in "$ROOT_DIR/icon_144x144.png" "$ROOT_DIR/icon_80x80.png"; do
  echo "Generated $icon ($(magick identify -format '%wx%h' "$icon"))"
done
echo "Generated $ROOT_DIR/screenshot_basalt.png (144x168)"
