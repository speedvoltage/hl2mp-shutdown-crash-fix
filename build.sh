#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf 'Usage: %s /path/to/hl2dm-src\n' "$0" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")" && pwd)"
make -C "$root" clean
make -C "$root" HL2SDK="$(realpath "$1")"
