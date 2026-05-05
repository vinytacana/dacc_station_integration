#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ASSETS_DIR="$PROJECT_ROOT/ui/assets"

if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "ffmpeg nao encontrado. Instale ffmpeg antes de otimizar assets." >&2
    exit 1
fi

echo "--- DACC Asset Optimizer ---"

echo "[1/2] Otimizando audio..."
find "$ASSETS_DIR/sounds" -name "*.wav" -print0 | while IFS= read -r -d '' sound; do
    tmp="${sound%.wav}_tmp.wav"
    echo "  Processando: ${sound#$PROJECT_ROOT/}"
    ffmpeg -y -i "$sound" -acodec pcm_s16le -ar 44100 -ac 2 "$tmp" >/dev/null 2>&1
    mv "$tmp" "$sound"
done

echo "[2/2] Otimizando imagens..."
find "$ASSETS_DIR/images" -type f \( -name "*.jpg" -o -name "*.png" \) -print0 | while IFS= read -r -d '' img; do
    ext="${img##*.}"
    tmp="${img%.*}_tmp.${ext}"
    echo "  Comprimindo: ${img#$PROJECT_ROOT/}"
    ffmpeg -y -i "$img" -q:v 5 "$tmp" >/dev/null 2>&1
    mv "$tmp" "$img"
done

echo "--- Otimizacao concluida ---"
