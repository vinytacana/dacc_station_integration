#!/bin/bash
# DACC Station Asset Optimizer
# Requer: ffmpeg

ASSETS_DIR="ui/assets"

echo "--- DACC Asset Optimizer ---"

# 1. Otimizar Sons (.wav)
# SDL2 Mixer prefere PCM 16-bit, 44100Hz, Stereo
echo "[1/2] Otimizando áudio..."
find "$ASSETS_DIR/sounds" -name "*.wav" | while read -r sound; do
    echo "  Processando: $sound"
    ffmpeg -y -i "$sound" -acodec pcm_s16le -ar 44100 -ac 2 "${sound%.wav}_tmp.wav" >/dev/null 2>&1
    mv "${sound%.wav}_tmp.wav" "$sound"
done

# 2. Otimizar Imagens (.jpg, .png)
# Reduz qualidade para 85% e remove metadados para economizar RAM
echo "[2/2] Otimizando imagens..."
find "$ASSETS_DIR/images" -type f \( -name "*.jpg" -o -name "*.png" \) | while read -r img; do
    echo "  Comprimindo: $img"
    ffmpeg -y -i "$img" -q:v 5 "${img%.*}_tmp${img: -4}" >/dev/null 2>&1
    mv "${img%.*}_tmp${img: -4}" "$img"
done

echo "--- Otimização concluída! ---"
