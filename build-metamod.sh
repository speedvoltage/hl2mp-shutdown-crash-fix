#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    printf 'Usage: %s /path/to/game-sdk [hl2dm|css|dods|tf2]\n' "$0" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")" && pwd)"
sdk="${2:-hl2dm}"
case "$sdk" in
    hl2dm|css|dods|tf2) ;;
    *) printf 'Unsupported Metamod SDK: %s\n' "$sdk" >&2; exit 2 ;;
esac

sdk_path="$(realpath "$1")"
if [[ ! -f "$sdk_path/public/tier1/interface.h" ]]; then
    printf 'Not a Source SDK directory: %s\n' "$sdk_path" >&2
    exit 1
fi

python="${PYTHON:-python3}"
if ! "$python" -c 'from ambuild2 import run; assert tuple(map(int, run.CURRENT_API.split("."))) >= (2, 2)' 2>/dev/null; then
    printf 'AMBuild 2.2 is required. Install requirements-build.txt with %s -m pip.\n' "$python" >&2
    exit 1
fi

export "HL2SDK${sdk^^}=$sdk_path"
export CC="${METAMOD_CC:-clang}"
export CXX="${METAMOD_CXX:-clang++}"
export CFLAGS="${CFLAGS:-} -Wno-unknown-warning-option"
export CXXFLAGS="${CXXFLAGS:-} -Wno-unknown-warning-option"

build="$root/build/metamod/$sdk"
mkdir -p "$build"
cd "$build"
"$python" "$root/third_party/metamod-source/configure.py" \
    --enable-debug --enable-optimize --targets=x86_64 --sdks="$sdk"
"$python" -c 'from ambuild2.run import cli_run; cli_run()' -j "${JOBS:-4}"
printf 'Built Metamod: %s/package/addons/metamod/bin/linux64/metamod.2.%s.so\n' "$build" "$sdk"
