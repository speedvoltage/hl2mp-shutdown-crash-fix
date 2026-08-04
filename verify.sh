#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")" && pwd)"
plugin="$root/package/addons/srcds_shutdown_fix/bin/srcds_shutdown_fix.so"

cd "$root"
sha256sum -c CHECKSUMS.sha256
file "$plugin" | grep -q 'ELF 64-bit.*x86-64'

if readelf -d "$plugin" | grep -qE 'libstdc\+\+|funchook|capstone'; then
    printf 'Unexpected runtime dependency.\n' >&2
    exit 1
fi

exports="$(nm -D --defined-only "$plugin" | awk '{print $3}' | sort)"
expected=$'CreateInterface_MMS\nUnloadInterface_MMS'

if [[ "$exports" != "$expected" ]]; then
    printf 'Unexpected exports:\n%s\n' "$exports" >&2
    exit 1
fi

max_glibc="$(readelf --version-info "$plugin" | grep -o 'GLIBC_[0-9.]*' | sort -V | tail -n 1)"

if [[ "$max_glibc" != "GLIBC_2.2.5" ]]; then
    printf 'Unexpected maximum GLIBC requirement: %s\n' "$max_glibc" >&2
    exit 1
fi

printf 'Verification PASS\n'
