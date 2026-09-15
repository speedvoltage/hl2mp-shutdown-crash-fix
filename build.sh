#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    printf 'Usage: %s /path/to/game-sdk [hl2dm|css|dods|tf2]\n' "$0" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")" && pwd)"
make -C "$root" clean
make -C "$root" HL2SDK="$(realpath "$1")" METAMOD_SDK="${2:-hl2dm}"
