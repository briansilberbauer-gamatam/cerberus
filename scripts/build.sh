#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
CONFIG="${CONFIG:-Release}"

# If development headers are not installed system-wide, use the local sysroot
# populated by scripts/setup-linux.sh.
if [ -d "$ROOT/deps/sysroot/usr" ]; then
    export PKG_CONFIG_PATH="$ROOT/deps/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$ROOT/deps/sysroot/usr/share/pkgconfig:${PKG_CONFIG_PATH:-}"
    export PKG_CONFIG_SYSROOT_DIR="$ROOT/deps/sysroot"
    CMAKE_EXTRA_ARGS=(
        "-DCMAKE_INCLUDE_PATH=$ROOT/deps/sysroot/usr/include"
        "-DCMAKE_LIBRARY_PATH=$ROOT/deps/sysroot/usr/lib/x86_64-linux-gnu"
    )
else
    CMAKE_EXTRA_ARGS=()
fi

cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DCERBERUS_BUILD_TESTS=ON \
    -DCERBERUS_BUILD_SCREENSHOT=ON \
    "${CMAKE_EXTRA_ARGS[@]}"

if [ -z "${JOBS:-}" ]; then
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel "$JOBS"

VST3="$BUILD_DIR/Cerberus_artefacts/$CONFIG/VST3/Cerberus.vst3"
STANDALONE="$BUILD_DIR/Cerberus_artefacts/$CONFIG/Standalone/Cerberus"

mkdir -p "$ROOT/dist"
if [ -d "$VST3" ]; then
    rm -rf "$ROOT/dist/Cerberus.vst3"
    cp -a "$VST3" "$ROOT/dist/"
fi
if [ -x "$STANDALONE" ]; then
    cp -a "$STANDALONE" "$ROOT/dist/Cerberus"
fi

echo
echo "Built artefacts:"
[ -d "$ROOT/dist/Cerberus.vst3" ] && echo "  VST3        $ROOT/dist/Cerberus.vst3"
[ -x "$ROOT/dist/Cerberus" ] && echo "  Standalone  $ROOT/dist/Cerberus"
echo "  Tests       $BUILD_DIR/CerberusTests_artefacts/$CONFIG/CerberusTests"
if [ -x "$BUILD_DIR/CerberusHostSmokeTest_artefacts/$CONFIG/CerberusHostSmokeTest" ]; then
    echo "  Host test   $BUILD_DIR/CerberusHostSmokeTest_artefacts/$CONFIG/CerberusHostSmokeTest"
fi
if [ -x "$BUILD_DIR/CerberusScreenshot_artefacts/$CONFIG/Cerberus Screenshot" ]; then
    echo "  Screenshot  $BUILD_DIR/CerberusScreenshot_artefacts/$CONFIG/Cerberus Screenshot"
fi
