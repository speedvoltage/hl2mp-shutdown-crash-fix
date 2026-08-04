#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf 'Usage: %s /path/to/server/game\n' "$0" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")" && pwd)"
game="$(realpath -m "$1")"
source_plugin="$root/package/addons/srcds_shutdown_fix/bin/srcds_shutdown_fix.so"
source_vdf="$root/package/addons/metamod/srcds_shutdown_fix.vdf"
destination_plugin="$game/addons/srcds_shutdown_fix/bin/srcds_shutdown_fix.so"
destination_vdf="$game/addons/metamod/srcds_shutdown_fix.vdf"
timestamp="$(date +%Y%m%d-%H%M%S)"
backup="$game/.patch-backups/srcds_shutdown_fix-$timestamp"

if [[ ! -f "$game/gameinfo.txt" ]]; then
    printf 'Not a Source game directory: %s\n' "$game" >&2
    exit 1
fi

if [[ ! -f "$source_plugin" || ! -f "$source_vdf" ]]; then
    printf 'Package files are missing.\n' >&2
    exit 1
fi

if ! file "$source_plugin" | grep -q 'ELF 64-bit.*x86-64'; then
    printf 'Plugin is not an x86-64 ELF shared object.\n' >&2
    exit 1
fi

mkdir -p "$backup" "$(dirname "$destination_plugin")" "$(dirname "$destination_vdf")"

if [[ -f "$destination_plugin" ]]; then
    cp -a "$destination_plugin" "$backup/"
fi

if [[ -f "$destination_vdf" ]]; then
    cp -a "$destination_vdf" "$backup/"
fi

install -m 0755 "$source_plugin" "$destination_plugin"
install -m 0644 "$source_vdf" "$destination_vdf"

printf 'Installed: %s\n' "$destination_plugin"
printf 'Installed: %s\n' "$destination_vdf"
printf 'Backup:    %s\n' "$backup"
