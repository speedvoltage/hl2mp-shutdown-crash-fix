#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="${1:-hl2dm}"
khook="$root/third_party/metamod-source/third_party/khook"
libraries="$root/build/metamod/$sdk/third_party/khook"
if [[ ! -f "$libraries/khook/linux-x86_64/libkhook.a" ]]; then
    printf 'Build Metamod for %s before running this test.\n' "$sdk" >&2
    exit 1
fi

build="$(mktemp -d "$root/build/khook-test.XXXXXX")"
trap 'rm -rf "$build"' EXIT
"${CXX:-g++}" -std=c++17 -m64 -g -DKHOOK_STANDALONE -I"$khook/include" \
    "$root/tests/test_virtual_shutdown.cpp" \
    "$libraries/khook/linux-x86_64/libkhook.a" \
    "$libraries/third_party/safetyhook/safetyhook/linux-x86_64/libsafetyhook.a" \
    -Wl,--wrap=_ZN5KHook10RemoveHookEjbPFvjPvES0_ -pthread -ldl -o "$build/test_virtual_shutdown"
"$build/test_virtual_shutdown"
