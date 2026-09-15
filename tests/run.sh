#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/tests/build"
rm -rf "$build"
mkdir -p "$build"

cc -shared -fPIC "$root/tests/mock_bad_runtime.c" -Wl,-soname,libsteam_api.so -o "$build/libsteam_api.so"
cc -shared -fPIC "$root/tests/mock_dedicated.c" -Wl,-soname,dedicated_srv.so -o "$build/dedicated_srv.so"
g++ -std=c++17 -D_GNU_SOURCE -D_LINUX -DLINUX -DPOSIX -DGNUC -DCOMPILER_GCC -DPLATFORM_64BITS -DX64BITS \
    -I"$root/src" -isystem "${HL2SDK:?}/public" -isystem "${HL2SDK}/public/tier0" \
    "$root/tests/test_loader.cpp" -ldl -o "$build/test_loader"

readelf -rW "$build/dedicated_srv.so" | grep -q '__gxx_personality_v0'
for mode in classic legacy16 legacy17 modern18 unsupported; do
    if ! "$build/test_loader" "$build/libsteam_api.so" "$build/dedicated_srv.so" "${PLUGIN_BINARY:-$root/build/srcds_shutdown_fix.so}" "$mode" > "$build/$mode.log" 2>&1; then
        cat "$build/$mode.log" >&2
        printf 'Metamod loader test failed: %s\n' "$mode" >&2
        exit 1
    fi
done
rm -rf "$build"
printf 'All Metamod loader and relocation tests passed.\n'
"$root/tests/run-metamod.sh" "${METAMOD_SDK:-hl2dm}"
