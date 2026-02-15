#!/bin/bash
echo "--- DACC Station Compatibility Check ---"
echo "OS: $(uname -srm)"
echo "Host: $(cat /proc/device-tree/model 2>/dev/null || echo 'PC/Generic')"

echo -e "
[1] Verificando Compilador e Bibliotecas:"
check_lib() { dpkg -l $1 >/dev/null 2>&1 && echo "[OK] $1" || echo "[!] FALTANDO: $1"; }
check_lib "libsdl2-dev"
check_lib "libsdl2-image-dev"
check_lib "nlohmann-json3-dev"

echo -e "
[2] Verificando Ferramentas de Sistema:"
check_cmd() { which $1 >/dev/null 2>&1 && echo "[OK] $1" || echo "[!] FALTANDO: $1"; }
check_cmd "mpv"
check_cmd "wpctl"
check_cmd "wlr-randr"
check_cmd "gamescope"

echo -e "
[3] Verificando Acesso ao Hardware:"
[ -w /dev/vchiq ] && echo "[OK] VCHIQ (Raspberry GPU) acessível" || echo "[ ] VCHIQ não encontrado (Normal em PC)"
groups | grep -q "video" && echo "[OK] Usuário no grupo 'video'" || echo "[!] Usuário NÃO está no grupo 'video'"

echo -e "
[4] Verificando Sockets IPC:"
ls /tmp/*.sock 2>/dev/null && echo "[!] Aviso: Sockets antigos encontrados em /tmp" || echo "[OK] /tmp limpo"

echo "--- Fim do Diagnóstico ---"
