#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    printf 'Usage: %s /path/to/server/game [hl2dm|css|dods|tf2]\n' "$0" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")" && pwd)"
game="$(realpath -m "$1")"
source_plugin="$root/build/srcds_shutdown_fix.so"
source_vdf="$root/package/addons/metamod/srcds_shutdown_fix.vdf"
destination_plugin="$game/addons/srcds_shutdown_fix/bin/srcds_shutdown_fix.so"
destination_vdf="$game/addons/metamod/srcds_shutdown_fix.vdf"
timestamp="$(date +%Y%m%d-%H%M%S)"
backup="$game/.patch-backups/srcds_shutdown_fix-$timestamp"

if [[ ! -f "$game/gameinfo.txt" ]]; then
    printf 'Not a Source game directory: %s\n' "$game" >&2
    exit 1
fi

sdk="${2:-$(basename "$game")}"
case "$sdk" in
    hl2mp|hl2dm) sdk=hl2dm ;;
    cstrike|css) sdk=css ;;
    dod|dods) sdk=dods ;;
    tf|tf2) sdk=tf2 ;;
    *) printf 'Specify the game SDK: hl2dm, css, dods, or tf2.\n' >&2; exit 2 ;;
esac

source_metamod="$root/build/metamod/$sdk/package/addons/metamod/bin/linux64"
destination_metamod="$game/addons/metamod/bin/linux64"
if [[ ! -f "$destination_metamod/server.so" ]]; then
    printf 'Install Linux x64 Metamod in this game before running this installer.\n' >&2
    exit 1
fi

for binary in server.so "metamod.2.$sdk.so"; do
    if [[ ! -f "$source_metamod/$binary" ]]; then
        printf 'Build the fixed Metamod core first: ./build-metamod.sh /path/to/game-sdk %s\n' "$sdk" >&2
        exit 1
    fi
    if ! file "$source_metamod/$binary" | grep -q 'ELF 64-bit.*x86-64'; then
        printf 'Metamod binary is not x86-64 ELF: %s\n' "$source_metamod/$binary" >&2
        exit 1
    fi
done

if [[ ! -f "$source_plugin" || ! -f "$source_vdf" ]]; then
    printf 'Build the plugin first with ./build.sh /path/to/game-sdk %s.\n' "$sdk" >&2
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

for binary in server.so "metamod.2.$sdk.so"; do
    if [[ -f "$destination_metamod/$binary" ]]; then
        cp -a "$destination_metamod/$binary" "$backup/"
    fi
    install -m 0755 "$source_metamod/$binary" "$destination_metamod/$binary.new"
    mv -f "$destination_metamod/$binary.new" "$destination_metamod/$binary"
    printf 'Installed: %s\n' "$destination_metamod/$binary"
done

printf 'Installed: %s\n' "$destination_plugin"
printf 'Installed: %s\n' "$destination_vdf"
printf 'Backup:    %s\n' "$backup"
