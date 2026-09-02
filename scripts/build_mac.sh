#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_directory="${repository_root}/build/macos"
distribution_directory="${repository_root}/dist"
configuration="${CONFIGURATION:-Release}"
package="${PACKAGE:-1}"
lock_directory="${repository_root}/build/.build_mac.lock"
mounted_device=""

fail() {
    printf 'error: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "'$1' is required. $2"
}

if [[ "$(uname -s)" != "Darwin" ]]; then
    fail "scripts/build_mac.sh must be run on macOS."
fi
if [[ "${configuration}" != "Debug" && "${configuration}" != "Release" ]]; then
    fail "CONFIGURATION must be either Debug or Release."
fi
if [[ "${package}" != "0" && "${package}" != "1" ]]; then
    fail "PACKAGE must be either 0 or 1."
fi

require_command cmake "Install it with 'brew install cmake'."
require_command ninja "Install it with 'brew install ninja'."
require_command codesign "Install the Xcode command-line tools."
require_command diskutil "It is included with macOS."
require_command file "It is included with macOS."
require_command hdiutil "It is included with macOS."
require_command install_name_tool "Install the Xcode command-line tools."
require_command lipo "Install the Xcode command-line tools."
require_command otool "Install the Xcode command-line tools."
require_command plutil "It is included with macOS."
if [[ "${package}" == "1" ]]; then
    require_command osascript "It is included with macOS."
    require_command sips "It is included with macOS."
fi

detach_installer_image() {
    if [[ -n "${mounted_device}" ]]; then
        hdiutil detach -quiet "${mounted_device}" 2>/dev/null || true
        mounted_device=""
    fi
}

cleanup() {
    detach_installer_image
    rmdir "${lock_directory}" 2>/dev/null || true
}

# CMake cleaning and macdeployqt both replace files inside the application
# bundle. Two builds sharing this directory can therefore make deployment
# files disappear while install_name_tool is rewriting them. Acquire an
# atomic lock before the first build-directory mutation.
mkdir -p "${repository_root}/build"
if ! mkdir "${lock_directory}" 2>/dev/null; then
    fail "Another macOS build is already using '${build_directory}'. Wait for it to finish, then retry."
fi
trap cleanup EXIT

qt_root="${QT_ROOT:-}"
if [[ -z "${qt_root}" ]] && command -v brew >/dev/null 2>&1; then
    qt_root="$(brew --prefix qt 2>/dev/null || true)"
fi
if [[ -z "${qt_root}" ]] && command -v qtpaths6 >/dev/null 2>&1; then
    qt_root="$(qtpaths6 --query QT_INSTALL_PREFIX 2>/dev/null || true)"
fi
[[ -n "${qt_root}" ]] || fail "Qt 6.8 or newer was not found. Set QT_ROOT or run 'brew install qt'."

macdeployqt="${qt_root}/bin/macdeployqt"
qt_qml_root="${qt_root}/qml"
qt_plugins_root="${qt_root}/plugins"
if [[ -d "${qt_root}/share/qt/qml" ]]; then
    qt_qml_root="${qt_root}/share/qt/qml"
fi
if [[ -d "${qt_root}/share/qt/plugins" ]]; then
    qt_plugins_root="${qt_root}/share/qt/plugins"
fi
[[ -x "${macdeployqt}" ]] || fail "macdeployqt was not found below QT_ROOT ('${qt_root}')."
[[ -d "${qt_qml_root}" ]] || fail "The Qt QML modules were not found below QT_ROOT ('${qt_root}')."
[[ -f "${qt_plugins_root}/platforms/libqcocoa.dylib" ]] || fail "The Qt Cocoa platform plug-in was not found."

libvlc_root="${LIBVLC_ROOT:-}"
if [[ -z "${libvlc_root}" && -d /Applications/VLC.app/Contents/MacOS ]]; then
    libvlc_root=/Applications/VLC.app/Contents/MacOS
fi
[[ -n "${libvlc_root}" ]] || fail "VLC was not found. Set LIBVLC_ROOT or run 'brew install --cask vlc'."
[[ -f "${libvlc_root}/include/vlc/vlc.h" ]] || fail "LibVLC headers were not found below LIBVLC_ROOT ('${libvlc_root}')."
[[ -f "${libvlc_root}/lib/libvlc.dylib" ]] || fail "libvlc.dylib was not found below LIBVLC_ROOT ('${libvlc_root}')."
[[ -f "${libvlc_root}/lib/libvlccore.dylib" ]] || fail "libvlccore.dylib was not found below LIBVLC_ROOT ('${libvlc_root}')."
[[ -d "${libvlc_root}/plugins" ]] || fail "VLC plug-ins were not found below LIBVLC_ROOT ('${libvlc_root}')."

vlc_version="unknown"
if [[ -f "${libvlc_root}/../Info.plist" ]]; then
    vlc_version="$(plutil -extract CFBundleShortVersionString raw \
        "${libvlc_root}/../Info.plist" 2>/dev/null || printf 'unknown')"
fi

host_architecture="$(uname -m)"
if ! lipo -archs "${libvlc_root}/lib/libvlc.dylib" | tr ' ' '\n' | grep -Fxq "${host_architecture}"; then
    fail "The VLC runtime does not contain the host architecture '${host_architecture}'."
fi

# Link through an isolated SDK copy. If CMake points into VLC.app directly,
# macdeployqt also discovers VLC's standalone GUI plug-ins and tries to deploy
# their optional updater frameworks (such as Sparkle), which this app does not
# use. The complete playback runtime is copied explicitly after Qt deployment.
vlc_sdk_root="${build_directory}/vlc-sdk"
cmake -E remove_directory "${vlc_sdk_root}"
mkdir -p "${vlc_sdk_root}"
cp -RLp "${libvlc_root}/include" "${vlc_sdk_root}/include"
cp -RLp "${libvlc_root}/lib" "${vlc_sdk_root}/lib"

# Remove a previously deployed bundle before configuration so stale frameworks
# cannot leak into the package. CMake recreates Info.plist while generating.
cmake -E remove_directory "${build_directory}/VeyloPlayer.app"

cmake -S "${repository_root}" -B "${build_directory}" -G Ninja \
    -DCMAKE_BUILD_TYPE="${configuration}" \
    -DCMAKE_PREFIX_PATH="${qt_root}" \
    -DLIBVLC_ROOT="${vlc_sdk_root}" \
    -DLIBVLC_INCLUDE_DIR="${vlc_sdk_root}/include" \
    -DLIBVLC_LIBRARY="${vlc_sdk_root}/lib/libvlc.dylib" \
    -DLIBVLC_RUNTIME_DIR="${vlc_sdk_root}/lib" \
    -DVEYLO_VLC_VERSION="${vlc_version}" \
    -DVEYLO_BUILD_TESTS=ON

cmake --build "${build_directory}" --parallel
ctest --test-dir "${build_directory}" --output-on-failure

application="${build_directory}/VeyloPlayer.app"
executable="${application}/Contents/MacOS/VeyloPlayer"
[[ -x "${executable}" ]] || fail "The application bundle was not created."

# Deploy linked Qt frameworks first. QML modules are copied explicitly below;
# this avoids macdeployqt pulling every optional Qt Quick Controls style into
# the application when Qt is installed as split Homebrew formulae.
"${macdeployqt}" "${application}" \
    -no-plugins -no-codesign -always-overwrite -verbose=1

qml_destination="${application}/Contents/Resources/qml"
mkdir -p "${qml_destination}/QtQml" \
    "${qml_destination}/QtQuick/Controls" \
    "${qml_destination}/Qt/labs"

copy_module_files() {
    local module="$1"
    local source="${qt_qml_root}/${module}"
    local destination="${qml_destination}/${module}"
    [[ -d "${source}" ]] || fail "Required Qt QML module '${module}' was not found."
    mkdir -p "${destination}"
    find -L "${source}" -maxdepth 1 -type f -exec cp -Lp {} "${destination}/" \;
}

copy_module_tree() {
    local module="$1"
    local source="${qt_qml_root}/${module}"
    local destination="${qml_destination}/${module}"
    [[ -d "${source}" ]] || fail "Required Qt QML module '${module}' was not found."
    mkdir -p "$(dirname "${destination}")"
    cp -RLp "${source}" "${destination}"
}

copy_module_files QML
copy_module_files QtQml
copy_module_files QtQuick
copy_module_files QtQuick/Controls
copy_module_tree QtQml/Models
copy_module_tree QtQml/WorkerScript
copy_module_tree QtQuick/Controls/Basic
copy_module_tree QtQuick/Controls/impl
copy_module_tree QtQuick/Templates
copy_module_files QtQuick/Shapes
copy_module_tree QtQuick/Layouts
copy_module_tree QtQuick/Dialogs
copy_module_tree QtQuick/Window
copy_module_tree Qt/labs/folderlistmodel

mkdir -p "${application}/Contents/PlugIns/platforms" \
    "${application}/Contents/PlugIns/imageformats"
cp -Lp "${qt_plugins_root}/platforms/libqcocoa.dylib" \
    "${application}/Contents/PlugIns/platforms/"
cp -Lp "${qt_plugins_root}/imageformats/libqjpeg.dylib" \
    "${application}/Contents/PlugIns/imageformats/"

deployment_arguments=()
while IFS= read -r plugin; do
    deployment_arguments+=("-executable=${plugin}")
done < <(find "${qml_destination}" "${application}/Contents/PlugIns" \
    -type f -name '*.dylib' -print)

"${macdeployqt}" "${application}" \
    -no-plugins -no-codesign -always-overwrite -verbose=1 \
    "${deployment_arguments[@]}"

# Keep the complete VLC runtime in VLC's standard macOS bundle layout.
mkdir -p "${application}/Contents/MacOS/lib"
cp -RLp "${libvlc_root}/lib/." "${application}/Contents/MacOS/lib/"
cp -RLp "${libvlc_root}/plugins" "${application}/Contents/MacOS/plugins"
# These belong to VLC's own Cocoa interface and desktop notifier, not to
# embedded LibVLC playback, and depend on updater/UI frameworks outside the
# private runtime copied above.
cmake -E rm -f \
    "${application}/Contents/MacOS/plugins/libmacosx_plugin.dylib" \
    "${application}/Contents/MacOS/plugins/libosx_notifications_plugin.dylib"
if ! otool -l "${executable}" | grep -Fq '@executable_path/lib'; then
    install_name_tool -add_rpath '@executable_path/lib' "${executable}"
fi

# Preserve the license texts, dependency notices, exact source offer, and Qt
# SBOM inside the signed bundle. These files must be present before codesign.
license_destination="${application}/Contents/Resources/licenses"
mkdir -p "${license_destination}"
cp -Lp "${repository_root}/LICENSE" \
    "${repository_root}/INSTALLER_LICENSE.txt" \
    "${repository_root}/SOURCE_CODE.md" \
    "${repository_root}/THIRD_PARTY_NOTICES.md" \
    "${build_directory}/SOURCE_OFFER.txt" \
    "${license_destination}/"
cp -RLp "${repository_root}/LICENSES" "${license_destination}/"

qt_sbom_root="${qt_root}/sbom"
if [[ ! -d "${qt_sbom_root}" && -d "${qt_root}/share/qt/sbom" ]]; then
    qt_sbom_root="${qt_root}/share/qt/sbom"
fi
if [[ -d "${qt_sbom_root}" ]]; then
    mkdir -p "${license_destination}/qt-sbom"
    find "${qt_sbom_root}" -maxdepth 1 -type f \
        \( -name 'qtbase-*.spdx' -o -name 'qtdeclarative-*.spdx' \
           -o -name 'qtsvg-*.spdx' -o -name 'qtshadertools-*.spdx' \) \
        -exec cp -Lp {} "${license_destination}/qt-sbom/" \;
fi

# macdeployqt and install_name_tool rewrite Mach-O files. Sign each nested
# binary before sealing the outer bundle so ad-hoc builds launch on macOS.
codesign_identity="${CODESIGN_IDENTITY:--}"
while IFS= read -r binary; do
    if file -b "${binary}" | grep -q 'Mach-O'; then
        codesign --force --sign "${codesign_identity}" "${binary}" 2>/dev/null
    fi
done < <(find "${application}/Contents" -type f -print)
codesign --force --sign "${codesign_identity}" "${application}"
codesign --verify --deep --strict "${application}"

if [[ "${package}" == "1" ]]; then
    version="$(plutil -extract CFBundleShortVersionString raw "${application}/Contents/Info.plist")"
    package_name="VeyloPlayer-${version}-macOS-${host_architecture}.dmg"
    package_path="${distribution_directory}/${package_name}"
    staging_directory="${build_directory}/dmg"
    mount_directory="${build_directory}/dmg-mount"
    read_write_image="${build_directory}/VeyloPlayer-layout.dmg"
    layout_volume_name="VeyloPlayer Layout $$"

    cmake -E remove_directory "${staging_directory}"
    cmake -E remove_directory "${mount_directory}"
    cmake -E rm -f "${read_write_image}" "${package_path}"
    mkdir -p "${staging_directory}/.background" \
        "${mount_directory}" "${distribution_directory}"
    /usr/bin/ditto "${application}" "${staging_directory}/VeyloPlayer.app"
    cp -Lp "${repository_root}/LICENSE" "${staging_directory}/LICENSE.txt"
    cp -Lp "${repository_root}/THIRD_PARTY_NOTICES.md" \
        "${staging_directory}/THIRD_PARTY_NOTICES.txt"
    cp -Lp "${build_directory}/SOURCE_OFFER.txt" \
        "${staging_directory}/SOURCE_OFFER.txt"
    codesign --verify --deep --strict "${staging_directory}/VeyloPlayer.app"
    ln -s /Applications "${staging_directory}/Applications"
    sips -s format png "${repository_root}/assets/branding/dmg-background.svg" \
        --out "${staging_directory}/.background/background.png" >/dev/null

    # Use a unique name while Finder writes the layout. This prevents an older
    # mounted VeyloPlayer DMG from receiving the new window metadata instead.
    hdiutil create -quiet -volname "${layout_volume_name}" \
        -srcfolder "${staging_directory}" \
        -format UDRW "${read_write_image}"

    attach_output="$(hdiutil attach -readwrite -noverify -noautoopen -nobrowse \
        -mountpoint "${mount_directory}" "${read_write_image}")"
    mounted_device="$(printf '%s\n' "${attach_output}" | awk '/^\/dev\// { print $1; exit }')"
    [[ -n "${mounted_device}" ]] || fail "The writable installer image could not be mounted."

    osascript "${repository_root}/scripts/configure-dmg.applescript" "${mount_directory}"
    [[ -f "${mount_directory}/.DS_Store" ]] || \
        fail "Finder did not save the installer window layout."
    diskutil renameVolume "${mount_directory}" VeyloPlayer >/dev/null
    sync
    detach_installer_image
    rmdir "${mount_directory}"

    hdiutil convert -quiet "${read_write_image}" -format UDZO \
        -imagekey zlib-level=9 -o "${package_path}"
    cmake -E rm -f "${read_write_image}"
    hdiutil verify -quiet "${package_path}"
    printf 'Installer: %s\n' "${package_path}"
fi

printf 'Application: %s\n' "${application}"
