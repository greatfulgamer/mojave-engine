#!/usr/bin/env bash
# scripts/progress-card.sh — the round reporting ritual.
# Usage: scripts/progress-card.sh <round-number> <title> [body-file]
# Produces: /tmp/opencode/progress/mojave-round<N>.png + .txt
# Deliver = copy the two files into the Commander Inbox of 10-of-spades, raul, randall-clark.
set -uo pipefail
ROUND="${1:?round number}"; TITLE="${2:-MOJAVE ENGINE — LOOP PROGRESS CARD}"
BODY="${3:-}"
OUT="/tmp/opencode/progress"
mkdir -p "$OUT"
TXT="$OUT/mojave-round${ROUND}.txt"
{
  printf '  %s\n' "$TITLE"
  printf '  Round %s · %s · build farm = general-lee-oliver (gcc 16, 56 cores)\n\n' "$ROUND" "$(date -I)"
  [ -n "$BODY" ] && cat "$BODY"
} > "$TXT"
FONT=$(magick -list font 2>/dev/null | grep -oE "Font: [A-Za-z0-9_-]*Mono[A-Za-z0-9_-]*" | head -1 | cut -d' ' -f2)
magick -background '#0b0b0b' -fill '#8ef58e' -font "${FONT:-monospace}" -pointsize 19 \
  -size 1500x caption:@"$TXT" -bordercolor '#0b0b0b' -border 30 \
  "$OUT/mojave-round${ROUND}.png"
echo "card: $OUT/mojave-round${ROUND}.png ($(stat -c%s "$OUT/mojave-round${ROUND}.png") bytes)"
echo "text: $TXT"
