#!/usr/bin/env bash
set -euo pipefail

# Installs the Linux build dependencies and JUCE. Run as a normal user; packages
# are downloaded and unpacked into deps/sysroot, so root access is not required.
# If your machine already has the -dev packages, you can skip this script.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! pkg-config --exists x11 freetype2 fontconfig alsa; then
    mkdir -p deps/packages deps/sysroot
    cd deps/packages

    packages=(
        libasound2-dev
        libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev
        libxcomposite-dev libxrender-dev libxfixes-dev libxi-dev libsm-dev libice-dev
        libxcb1-dev libxau-dev libxdmcp-dev
        libfreetype-dev libfontconfig-dev libexpat1-dev
        libpng-dev libbrotli-dev libbz2-dev
        x11proto-dev
    )

    for package in "${packages[@]}"; do
        echo "Downloading $package"
        apt-get download "$package" >/dev/null
    done

    for archive in *.deb; do
        dpkg-deb -x "$archive" ../sysroot
    done

    # The dev packages contain linker symlinks such as libX11.so -> libX11.so.6.
    # Their versioned targets live in the runtime packages, which are already
    # installed system-wide. Point every broken symlink at its system copy.
    LIBDIR="../sysroot/usr/lib/x86_64-linux-gnu"

    for link in "$LIBDIR"/*.so; do
        [ -L "$link" ] || continue

        if [ ! -e "$link" ]; then
            name="$(basename "$link")"
            target="$(ldconfig -p | awk -v name="$name" 'index($1, name ".") == 1 { print $NF; exit }')"

            if [ -n "$target" ]; then
                ln -sfn "$target" "$link"
            fi
        fi
    done

    cd "$ROOT"
fi

if [ ! -f external/JUCE/CMakeLists.txt ]; then
    mkdir -p external
    git clone --depth 1 --branch 8.0.8 https://github.com/juce-framework/JUCE.git external/JUCE
fi

echo "Dependencies are ready. Run scripts/build.sh next."
