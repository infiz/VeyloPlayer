#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_directory="${repository_root}/build/macos"
configuration="${CONFIGURATION:-Debug}"

: "${QT_ROOT:?Set QT_ROOT to the Qt macOS installation directory.}"
: "${LIBVLC_ROOT:?Set LIBVLC_ROOT to the pinned macOS LibVLC SDK/runtime directory.}"

cmake -S "${repository_root}" -B "${build_directory}" -G Ninja \
    -DCMAKE_BUILD_TYPE="${configuration}" \
    -DCMAKE_PREFIX_PATH="${QT_ROOT}" \
    -DLIBVLC_ROOT="${LIBVLC_ROOT}" \
    -DVEYLO_BUILD_TESTS=ON
cmake --build "${build_directory}" --parallel
ctest --test-dir "${build_directory}" --output-on-failure

if [[ "${PACKAGE:-0}" == "1" ]]; then
    cpack --config "${build_directory}/CPackConfig.cmake" -G DragNDrop
fi
