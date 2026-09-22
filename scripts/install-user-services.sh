#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 --dry-run|--install|--uninstall|--render-dir DIRECTORY" >&2
    exit 2
}

[[ $# -eq 1 || ( $# -eq 2 && $1 == --render-dir ) ]] || usage
case "$1" in
    --dry-run|--install|--uninstall|--render-dir) action=$1 ;;
    *) usage ;;
esac

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)
unit_dir="${XDG_CONFIG_HOME:-${HOME}/.config}/systemd/user"
marker='# Managed by DACC Station user-service installer'
units=(dacc-log.service dacc-process-manager.service dacc-ui.service dacc-station.target)

# Unit-file ExecStart/WorkingDirectory syntax needs escaping for these characters.
# Refuse ambiguous paths instead of installing a different command.
[[ "$project_root" != *[[:space:]%\\\&]* ]] || {
    echo "Unsupported project path for systemd unit templates: $project_root" >&2
    exit 1
}

render_unit() {
    local name=$1 content
    if [[ "$name" == *.target ]]; then
        content=$(<"$project_root/systemd/user/$name")
    else
        content=$(<"$project_root/systemd/user/$name.in")
    fi
    content=${content//@DACC_ROOT@/$project_root}
    printf '%s\n%s\n' "$marker" "$content"
}

if [[ "$action" == --dry-run ]]; then
    for unit in "${units[@]}"; do
        printf 'Would install %s\n' "$unit_dir/$unit"
        render_unit "$unit"
    done
    echo 'No files installed, services enabled or services started.'
    exit 0
fi

if [[ "$action" == --render-dir ]]; then
    [[ -d "$2" && ! -L "$2" ]] || { echo "Not a real directory: $2" >&2; exit 1; }
    for unit in "${units[@]}"; do
        destination="$2/$unit"
        [[ ! -e "$destination" && ! -L "$destination" ]] || {
            echo "Refusing to overwrite: $destination" >&2
            exit 1
        }
        render_unit "$unit" > "$destination"
    done
    exit 0
fi

if [[ "$action" == --install ]]; then
    for executable in bin/log-server bin/process-manager bin/dacc-ui scripts/check-wayland-session.sh; do
        [[ -x "$project_root/$executable" ]] || {
            echo "Missing executable: $project_root/$executable (run make first)" >&2
            exit 1
        }
    done
    [[ -f "$project_root/process-manager/games.json" ]] || {
        echo 'Missing process-manager/games.json' >&2
        exit 1
    }
    mkdir -p "$unit_dir"
    for unit in "${units[@]}"; do
        destination="$unit_dir/$unit"
        [[ ! -L "$destination" ]] || { echo "Refusing symlink: $destination" >&2; exit 1; }
        if [[ -e "$destination" ]]; then
            if ! cmp -s "$destination" <(render_unit "$unit"); then
                echo "Refusing to overwrite existing unit: $destination" >&2
                exit 1
            fi
        fi
    done
    for unit in "${units[@]}"; do
        destination="$unit_dir/$unit"
        if [[ ! -e "$destination" ]]; then
            render_unit "$unit" > "$destination"
            chmod 0644 "$destination"
            echo "Installed $destination"
        fi
    done
    systemctl --user daemon-reload
    echo 'Installed without enabling or starting services.'
    exit 0
fi

for unit in "${units[@]}"; do
    destination="$unit_dir/$unit"
    [[ ! -L "$destination" ]] || { echo "Refusing symlink: $destination" >&2; exit 1; }
    if [[ -e "$destination" ]] && ! cmp -s "$destination" <(render_unit "$unit"); then
        echo "Refusing to remove modified or unmanaged unit: $destination" >&2
        exit 1
    fi
done
for unit in "${units[@]}"; do
    destination="$unit_dir/$unit"
    if [[ -e "$destination" ]]; then
        rm -- "$destination"
        echo "Removed $destination"
    fi
done
systemctl --user daemon-reload
echo 'Uninstalled units; no other user configuration was changed.'
