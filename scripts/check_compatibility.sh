#!/usr/bin/env bash
set -u

missing_required=0
missing_optional=0

ok() { echo "[OK] $*"; }
warn() { echo "[!] $*"; missing_optional=$((missing_optional + 1)); }
fail() { echo "[FAIL] $*"; missing_required=$((missing_required + 1)); }

has_cmd() {
    command -v "$1" >/dev/null 2>&1
}

check_cmd() {
    local cmd="$1"
    local label="${2:-$1}"
    if has_cmd "$cmd"; then
        ok "$label: $(command -v "$cmd")"
        return 0
    fi
    warn "$label ausente"
    return 1
}

check_required_cmd() {
    local cmd="$1"
    local label="${2:-$1}"
    if has_cmd "$cmd"; then
        ok "$label: $(command -v "$cmd")"
        return 0
    fi
    fail "$label ausente"
    return 1
}

check_group() {
    local group="$1"
    if id -nG | tr ' ' '\n' | grep -qx "$group"; then
        ok "usuario pertence ao grupo $group"
    else
        warn "usuario nao pertence ao grupo $group"
    fi
}

echo "--- DACC Station Compatibility Check ---"
echo "OS: $(uname -srm)"
echo "User: $(id -un)"
echo "Session: ${XDG_SESSION_TYPE:-unknown}"

if [[ -r /proc/device-tree/model ]]; then
    echo "Host: $(tr -d '\0' < /proc/device-tree/model)"
else
    echo "Host: PC/Generic"
fi

echo
echo "[1/5] Build basico"
check_required_cmd "g++" "g++"
check_required_cmd "make" "make"

echo
echo "[2/5] Capacidades de configuracao"
check_cmd "nmcli" "NetworkManager/nmcli"
check_cmd "bluetoothctl" "BlueZ/bluetoothctl"

if has_cmd "wpctl" || has_cmd "pactl" || has_cmd "aplay" || has_cmd "amixer"; then
    ok "audio: $(for c in wpctl pactl aplay amixer; do has_cmd "$c" && printf '%s ' "$c"; done)"
else
    warn "audio indisponivel: wpctl/pactl/aplay/amixer ausentes"
fi

if has_cmd "xrandr" || has_cmd "wlr-randr"; then
    ok "display config: $(for c in xrandr wlr-randr; do has_cmd "$c" && printf '%s ' "$c"; done)"
else
    warn "display config indisponivel: xrandr/wlr-randr ausentes"
fi

if has_cmd "brightnessctl"; then
    ok "brightnessctl: $(command -v brightnessctl)"
elif compgen -G "/sys/class/backlight/*/brightness" >/dev/null; then
    ok "backlight via sysfs disponivel"
else
    warn "brilho indisponivel: brightnessctl e /sys/class/backlight ausentes"
fi

check_cmd "mpv" "intro/mpv"

echo
echo "[3/5] Permissoes de usuario"
check_group "video"
check_group "audio"
check_group "netdev"

echo
echo "[4/5] Hardware exposto"
if compgen -G "/sys/class/backlight/*/brightness" >/dev/null; then
    ok "backlights encontrados:"
    for path in /sys/class/backlight/*; do
        [[ -d "$path" ]] && echo "     - $(basename "$path")"
    done
else
    warn "nenhum backlight em /sys/class/backlight"
fi

if [[ -r /dev/vchiq || -w /dev/vchiq ]]; then
    ok "VCHIQ acessivel"
else
    echo "[ ] VCHIQ nao encontrado ou sem permissao; normal fora de Raspberry"
fi

echo
echo "[5/5] Sockets IPC"
for sock in /tmp/dacc-station.sock /tmp/gameman.sock; do
    if [[ -S "$sock" ]]; then
        warn "socket existente: $sock"
    else
        ok "socket livre: $sock"
    fi
done

echo
echo "--- Resumo ---"
echo "Obrigatorios faltando: $missing_required"
echo "Opcionais faltando: $missing_optional"

if (( missing_required > 0 )); then
    exit 1
fi
exit 0
