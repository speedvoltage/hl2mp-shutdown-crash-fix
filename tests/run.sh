#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/tests/build"
rm -rf "$build"
mkdir -p "$build"

cc -shared -fPIC "$root/tests/mock_bad_runtime.c" -Wl,-soname,libsteam_api.so -o "$build/libsteam_api.so"
cc -shared -fPIC "$root/tests/mock_dedicated.c" -Wl,-soname,dedicated_srv.so -o "$build/dedicated_srv.so"
g++ -std=c++17 -D_GNU_SOURCE -D_LINUX -DLINUX -DPOSIX -DGNUC -DCOMPILER_GCC -DHL2MP -DPLATFORM_64BITS -DX64BITS \
    -I"$root/src" -isystem "${HL2SDK:?}/public" -isystem "${HL2SDK}/public/tier0" \
    "$root/tests/test_loader.cpp" -ldl -o "$build/test_loader"

readelf -rW "$build/dedicated_srv.so" | grep -q '__gxx_personality_v0'
output="$("$build/test_loader" "$build/libsteam_api.so" "$build/dedicated_srv.so" "$root/build/hl2mp_shutdown_fix.so")"
expected=$'[META] Loaded HL2MP Shutdown Fix\n[META] HL2MP Shutdown Fix unloaded, but retaining shutdown fix!'
if [[ "$output" != "$expected" ]]; then
    printf 'Unexpected runtime output:\n%s\n' "$output" >&2
    exit 1
fi
rm -rf "$build"
printf 'All relocation-fix tests passed.\n'
