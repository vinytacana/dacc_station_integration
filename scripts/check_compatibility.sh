#!/usr/bin/env bash
set -euo pipefail

echo "--- DACC Station Compatibility Check ---"
echo "OS: $(uname -srm)"

if [[ -r /proc/device-tree/model ]]; then
    echo "Host: $(tr -d '\0' < /proc/device-tree/model)"
else
    echo "Host: PC/Generic"
fi

echo
echo "[1/4] Bibliotecas de desenvolvimento"
check_pkg() {
    if command -v dpkg >/dev/null 2>&1; then
        dpkg -l "$1" >/dev/null 2>&1 && echo "[OK] $1" || echo "[!] FALTANDO: $1"
    else
        echo "[?] $1: verifique manualmente neste sistema"
    fi
}

check_pkg "libsdl2-dev"
check_pkg "libsdl2-image-dev"
check_pkg "libsdl2-ttf-dev"
check_pkg "libsdl2-mixer-dev"
check_pkg "libsdl2-gfx-dev"

echo
echo "[2/4] Ferramentas de sistema"
check_cmd() {
    command -v "$1" >/dev/null 2>&1 && echo "[OK] $1" || echo "[!] FALTANDO: $1"
}

check_cmd "g++"
check_cmd "make"
check_cmd "java"
check_cmd "wpctl"
check_cmd "pactl"
check_cmd "amixer"
check_cmd "nmcli"
check_cmd "bluetoothctl"
check_cmd "xrandr"
check_cmd "wlr-randr"

echo
echo "[3/4] Hardware e permissoes"
if [[ -w /dev/vchiq ]]; then
    echo "[OK] VCHIQ acessivel"
else
    echo "[ ] VCHIQ nao encontrado ou sem escrita"
fi

groups | grep -qw "video" && echo "[OK] Usuario no grupo video" || echo "[!] Usuario nao esta no grupo video"

echo
echo "[4/4] Sockets IPC"
if compgen -G "/tmp/*.sock" >/dev/null; then
    echo "[!] Sockets encontrados em /tmp:"
    ls /tmp/*.sock
else
    echo "[OK] Nenhum socket antigo encontrado em /tmp"
fi

echo "--- Fim do diagnostico ---"
