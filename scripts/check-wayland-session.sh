#!/usr/bin/env bash
set -eu

printf 'DACC UI environment: WAYLAND_DISPLAY=%s XDG_RUNTIME_DIR=%s XDG_SESSION_TYPE=%s\n' \
    "${WAYLAND_DISPLAY:-<unset>}" "${XDG_RUNTIME_DIR:-<unset>}" "${XDG_SESSION_TYPE:-<unset>}"

[[ -n "${WAYLAND_DISPLAY:-}" && -n "${XDG_RUNTIME_DIR:-}" ]] || exit 1
[[ "${XDG_SESSION_TYPE:-}" == wayland ]] || exit 1
[[ "${WAYLAND_DISPLAY}" != */* ]] || exit 1
[[ -S "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY}" ]] || exit 1
