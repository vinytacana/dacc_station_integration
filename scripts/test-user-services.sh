#!/usr/bin/env bash
set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)
test_dir=$(mktemp -d)
unit_names=(dacc-log.service dacc-process-manager.service dacc-ui.service dacc-station.target)
cleanup() {
    for unit in "${unit_names[@]}"; do rm -f -- "$test_dir/$unit"; done
    rmdir -- "$test_dir"
}
trap cleanup EXIT

"$project_root/scripts/install-user-services.sh" --render-dir "$test_dir"
systemd-analyze --user verify "${unit_names[@]/#/$test_dir/}"

for service in dacc-log.service dacc-process-manager.service dacc-ui.service; do
    file="$test_dir/$service"
    rg -q '^Type=exec$' "$file"
    rg -q '^Restart=on-failure$' "$file"
    rg -q '^RestartSec=2s$' "$file"
    rg -q '^StartLimitIntervalSec=30s$' "$file"
    rg -q '^StartLimitBurst=5$' "$file"
    rg -q '^TimeoutStopSec=5s$' "$file"
    rg -q '^NoNewPrivileges=yes$' "$file"
    rg -q "^WorkingDirectory=$project_root$" "$file"
    rg -q "^ExecStart=$project_root/bin/" "$file"
done
rg -q '^KillMode=mixed$' "$test_dir/dacc-process-manager.service"
rg -q '^ExecCondition=' "$test_dir/dacc-ui.service"
rg -q '^Wants=dacc-log.service$' "$test_dir/dacc-process-manager.service"
rg -q '^Wants=dacc-process-manager.service$' "$test_dir/dacc-ui.service"
! rg -q '^(Requires=|User=|Group=|ExecStartPre=.*sleep|ExecCondition=.*sleep)' "$test_dir"
! rg -q '(/tmp/|/run/user/[0-9]+|/home/viny/|WAYLAND_DISPLAY=wayland-|DISPLAY=:)' "$project_root/systemd"

env -u WAYLAND_DISPLAY "$project_root/scripts/check-wayland-session.sh" >/dev/null && {
    echo 'UI session guard accepted missing Wayland display' >&2
    exit 1
}
echo 'Systemd unit structure and Wayland guard tests passed.'
