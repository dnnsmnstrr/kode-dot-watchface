#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT="${1:-$ROOT_DIR/banner.png}"
SCREENSHOT="$ROOT_DIR/screenshot_emery.png"
FONT="$ROOT_DIR/Outfit/static/Outfit-Bold.ttf"

if ! command -v magick >/dev/null 2>&1; then
  echo "ImageMagick is required (install it with: brew install imagemagick)" >&2
  exit 1
fi

for file in "$SCREENSHOT" "$FONT"; do
  if [[ ! -f "$file" ]]; then
    echo "Missing required asset: $file" >&2
    exit 1
  fi
done

mkdir -p "$(dirname "$OUTPUT")"

magick \
  -size 720x320 canvas:'#000000' \
  \( "$SCREENSHOT" -filter point -resize 246x280 \) \
  -gravity east -geometry +52+0 -composite \
  -font "$FONT" -fill '#F5F5F0' -gravity northwest \
  -pointsize 62 -annotate +54+112 'Kode Dot' \
  -fill '#45B2D8' -pointsize 21 -annotate +58+229 'A PEBBLE WATCHFACE' \
  -strip -define png:color-type=6 "$OUTPUT"

dimensions="$(magick identify -format '%wx%h' "$OUTPUT")"
if [[ "$dimensions" != '720x320' ]]; then
  echo "Banner has invalid dimensions: $dimensions" >&2
  exit 1
fi

echo "Generated $OUTPUT ($dimensions)"
