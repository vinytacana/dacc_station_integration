#!/usr/bin/env bash

set -euo pipefail

APT_BUILD_PACKAGES=(
    gcc
    g++
    make
    libsdl2-dev
    libsdl2-image-dev
    libsdl2-ttf-dev
    libsdl2-gfx-dev
    libsdl2-mixer-dev
)

APT_RUNTIME_PACKAGES=(
    network-manager
    bluez
    rfkill
    pipewire
    wireplumber
    pulseaudio-utils
    alsa-utils
    x11-xserver-utils
    wlr-randr
    brightnessctl
)

DNF_BUILD_PACKAGES=(
    gcc
    gcc-c++
    make
    SDL2-devel
    SDL2_image-devel
    SDL2_ttf-devel
    SDL2_gfx-devel
    SDL2_mixer-devel
)

DNF_RUNTIME_PACKAGES=(
    NetworkManager
    bluez
    bluez-tools
    rfkill
    pipewire
    wireplumber
    pulseaudio-utils
    alsa-utils
    xorg-x11-server-utils
    wlr-randr
    brightnessctl
)

run_as_root() {
    if [[ "${EUID}" -eq 0 ]]; then
        "$@"
        return
    fi

    if ! command -v sudo >/dev/null 2>&1; then
        echo "Erro: este script precisa de root ou sudo para instalar pacotes." >&2
        exit 1
    fi

    sudo "$@"
}

install_with_apt() {
    echo "Gerenciador detectado: apt-get"
    echo "Atualizando indice de pacotes..."
    run_as_root apt-get update

    echo "Instalando dependencias de compilacao..."
    run_as_root apt-get install -y "${APT_BUILD_PACKAGES[@]}"

    echo "Instalando dependencias de runtime..."
    run_as_root apt-get install -y "${APT_RUNTIME_PACKAGES[@]}"
}

install_with_dnf() {
    echo "Gerenciador detectado: dnf"

    echo "Instalando dependencias de compilacao..."
    run_as_root dnf install -y "${DNF_BUILD_PACKAGES[@]}"

    echo "Instalando dependencias de runtime..."
    run_as_root dnf install -y "${DNF_RUNTIME_PACKAGES[@]}"
}

main() {
    echo "Preparando ambiente do DACC Station..."

    if command -v apt-get >/dev/null 2>&1; then
        install_with_apt
    elif command -v dnf >/dev/null 2>&1; then
        install_with_dnf
    else
        echo "Erro: nenhum gerenciador de pacotes suportado foi encontrado." >&2
        echo "Instale manualmente as dependencias listadas no README.md." >&2
        exit 1
    fi

    echo "Dependencias instaladas com sucesso."
}

main "$@"
