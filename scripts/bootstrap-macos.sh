#!/usr/bin/env bash
set -euo pipefail
repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dependencies="${repository_root}/.deps"
qt_version=6.12.0
vlc_version=3.0.24
fail() { printf 'error: %s\n' "$*" >&2; exit 1; }
[[ "$(uname -s)" == Darwin ]] || fail "Run this script on macOS."
for tool in python3 curl shasum hdiutil ditto tar 7zz; do
    command -v "${tool}" >/dev/null || fail "'${tool}' is required. Install build dependencies with 'brew install python sevenzip'."
done
case "$(uname -m)" in
    arm64)
        vlc_arch=arm64
        vlc_hash=64a89d93cdd30b0e97131743e246373db82d6826ea882d265d46d74b136da2b7 ;;
    x86_64)
        vlc_arch=intel64
        vlc_hash=1ef6c903e2dc026d4e58ddf7b2a9ed0eb6f8caf80a9a9f46f96490340b962707 ;;
    *) fail "Unsupported Mac architecture." ;;
esac
mkdir -p "${dependencies}/downloads"
qt_root="${dependencies}/Qt/${qt_version}/macos"
if [[ ! -x "${qt_root}/bin/qtpaths" || ! -f "${qt_root}/plugins/imageformats/libqwebp.dylib" ]]; then
    python_environment="${dependencies}/bootstrap-python-macos"
    [[ -x "${python_environment}/bin/python" ]] || python3 -m venv "${python_environment}"
    "${python_environment}/bin/python" -m pip install --disable-pip-version-check \
        'aqtinstall @ git+https://github.com/miurahr/aqtinstall.git@8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b'
    "${python_environment}/bin/python" -m aqt install-qt mac desktop \
        "${qt_version}" clang_64 --outputdir "${dependencies}/Qt" \
        --modules qtshadertools qtimageformats \
        --external "$(command -v 7zz)"
fi
verify_hash() {
    local actual
    actual="$(shasum -a 256 "$1" | awk '{print $1}')"
    [[ "${actual}" == "$2" ]] || fail "Checksum mismatch for '$1'."
}
vlc_parent="${dependencies}/vlc-${vlc_version}-${vlc_arch}"
vlc_app="${vlc_parent}/VLC.app"
if [[ ! -f "${vlc_app}/Contents/MacOS/lib/libvlc.dylib" ]]; then
    archive="${dependencies}/downloads/vlc-${vlc_version}-${vlc_arch}.dmg"
    [[ -f "${archive}" ]] || curl --fail --location --retry 3 \
        "https://download.videolan.org/pub/videolan/vlc/${vlc_version}/macosx/vlc-${vlc_version}-${vlc_arch}.dmg" \
        --output "${archive}"
    verify_hash "${archive}" "${vlc_hash}"
    mount_directory="$(mktemp -d "${dependencies}/vlc-mount.XXXXXX")"
    cleanup() {
        hdiutil detach -quiet "${mount_directory}" 2>/dev/null || true
        rmdir "${mount_directory}" 2>/dev/null || true
    }
    trap cleanup EXIT
    hdiutil attach -quiet -readonly -nobrowse -mountpoint "${mount_directory}" "${archive}"
    mkdir -p "${vlc_parent}"
    ditto "${mount_directory}/VLC.app" "${vlc_app}"
    cleanup
    trap - EXIT
fi
# Obtain matching SDK headers if the runtime distribution omits them.
if [[ ! -f "${vlc_app}/Contents/MacOS/include/vlc/vlc.h" ]]; then
    source_archive="${dependencies}/downloads/vlc-${vlc_version}.tar.xz"
    [[ -f "${source_archive}" ]] || curl --fail --location --retry 3 \
        "https://download.videolan.org/pub/videolan/vlc/${vlc_version}/vlc-${vlc_version}.tar.xz" \
        --output "${source_archive}"
    verify_hash "${source_archive}" e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf
    mkdir -p "${vlc_app}/Contents/MacOS/include"
    tar -xf "${source_archive}" -C "${vlc_app}/Contents/MacOS/include" \
        --strip-components=2 "vlc-${vlc_version}/include/vlc"
fi
printf 'Qt: %s\nLibVLC: %s\n' "${qt_root}" "${vlc_app}/Contents/MacOS"
