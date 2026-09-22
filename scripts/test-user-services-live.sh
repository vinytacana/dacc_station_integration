#!/usr/bin/env bash
set -euo pipefail

if [[ ${1:-} != --live || $# -ne 1 ]]; then
    echo "Usage: $0 --live (runs and stops DACC user services)" >&2
    exit 2
fi

tmp_dir=$(mktemp -d)
catalog_before=$(systemctl --user show-environment | sed -n 's/^DACC_GAME_CATALOG=//p')
catalog_was_set=false
[[ -z "$catalog_before" ]] || catalog_was_set=true

cleanup() {
    systemctl --user stop dacc-process-manager.service dacc-log.service >/dev/null 2>&1 || true
    if $catalog_was_set; then
        systemctl --user set-environment "DACC_GAME_CATALOG=$catalog_before" || true
    else
        systemctl --user unset-environment DACC_GAME_CATALOG || true
    fi
    systemctl --user reset-failed dacc-process-manager.service dacc-log.service >/dev/null 2>&1 || true
    rm -f -- "$tmp_dir/game.sh" "$tmp_dir/games.json" "$tmp_dir/child.pid"
    rmdir -- "$tmp_dir"
}
trap cleanup EXIT

python3 - "$tmp_dir" <<'PY'
import json
import os
from pathlib import Path
import sys

root = Path(sys.argv[1])
script = root / "game.sh"
script.write_text("#!/bin/sh\ntrap '' TERM\nsetsid sh -c 'trap \"\" TERM; while :; do sleep 1; done' &\n"
                  "echo $! > child.pid\nwhile :; do sleep 1; done\n")
script.chmod(0o700)
catalog = root / "games.json"
catalog.write_text(json.dumps({"games": [{"id": "systemd-test", "argv": ["game.sh"], "cwd": "."}]}))
catalog.chmod(0o600)
PY

systemctl --user set-environment "DACC_GAME_CATALOG=$tmp_dir/games.json"
systemctl --user start dacc-log.service
systemctl --user restart dacc-process-manager.service

socket_path="${XDG_RUNTIME_DIR:?}/dacc-station/process-manager.sock"
launch_game() {
    python3 - "$socket_path" "$tmp_dir/child.pid" <<'PY'
import json
import socket
import sys
import time
from pathlib import Path

sock_path, child_path = sys.argv[1:]
deadline = time.monotonic() + 8
while True:
    try:
        connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        connection.connect(sock_path)
        break
    except OSError:
        connection.close()
        if time.monotonic() >= deadline:
            raise
        time.sleep(0.05)
connection.settimeout(5)
request = {"action": "start", "request_id": "systemd-live", "game_id": "systemd-test"}
connection.sendall((json.dumps(request) + "\n").encode())
buffer = b""
while b"\n" not in buffer:
    buffer += connection.recv(4096)
event = json.loads(buffer.split(b"\n", 1)[0])
assert event["event"] == "game_started", event
leader = event["pid"]
while not Path(child_path).exists():
    if time.monotonic() >= deadline:
        raise TimeoutError("game child did not start")
    time.sleep(0.05)
child = int(Path(child_path).read_text().strip())
print(leader, child)
PY
}
read -r game_pid child_pid < <(launch_game)
[[ "$game_pid" =~ ^[0-9]+$ && "$child_pid" =~ ^[0-9]+$ ]]

old_pm=$(systemctl --user show dacc-process-manager.service -p MainPID --value)
systemctl --user kill --signal=SIGKILL --kill-who=main dacc-process-manager.service
python3 - "$old_pm" "$game_pid" "$child_pid" <<'PY'
import os
import subprocess
import sys
import time

old_pm, leader, child = map(int, sys.argv[1:])
deadline = time.monotonic() + 12
while time.monotonic() < deadline:
    new_pm = int(subprocess.check_output([
        "systemctl", "--user", "show", "dacc-process-manager.service", "-p", "MainPID", "--value"
    ]).strip())
    if new_pm > 0 and new_pm != old_pm and all(not os.path.exists(f"/proc/{pid}") for pid in (leader, child)):
        break
    time.sleep(0.05)
else:
    raise AssertionError("PM did not restart or game processes survived its cgroup")
PY

rm -f -- "$tmp_dir/child.pid"
read -r game_pid child_pid < <(launch_game)
systemctl --user stop dacc-process-manager.service
python3 - "$game_pid" "$child_pid" <<'PY'
import os
import sys
import time

pids = [int(value) for value in sys.argv[1:]]
deadline = time.monotonic() + 7
while time.monotonic() < deadline:
    if all(not os.path.exists(f"/proc/{pid}") for pid in pids):
        break
    time.sleep(0.05)
else:
    raise AssertionError("game processes survived graceful PM stop")
PY
[[ ! -e "$socket_path" ]] || { echo "PM socket remained after stop" >&2; exit 1; }
echo 'Live PM crash/restart, escaped child cgroup cleanup and active-game stop passed.'
