# VeyloPlayer

VeyloPlayer is a modern, open-source desktop player for local video, audio, and JPEG files. The first end-to-end implementation targets Windows while keeping the C++/Qt/LibVLC application core compatible with macOS.

VeyloPlayer's original source code is licensed under
[GPL-3.0-or-later](LICENSE). Contributions are welcome under the same license.

## Current MVP

- Audio and video playback through a private LibVLC runtime.
- Chapter-aware previous/next media, play/pause, seeking, five-second Left/Right
  video jumps, volume, mute, loading, and error states.
- Completed audio and video remain seekable; dragging backward restarts playback
  from the selected position.
- Fullscreen video through the control bar, F/F11, or double-click; Escape exits.
- A translucent playback bar overlays video without resizing it, appears on
  pointer activity, and hides after five seconds; clicking the audio or video
  surface toggles play/pause.
- Local continue-watching history remembers the last video and resume position.
- Embedded audio and subtitle track selection.
- A remembered audio-output device selector in Settings.
- Per-folder audio and embedded-subtitle preferences with label and position matching.
- External SRT, WebVTT, ASS, and SSA subtitle loading.
- Natural same-folder continuation (`1`, `2`, `10`, `11`).
- JPEG viewing with smooth pointer-centered mouse-wheel zoom, click-and-drag
  panning, Up/Down repositioning, persistent zoom between photos, an image
  counter, and Left/Right previous/next navigation at every zoom level.
- Add-media menu with multi-file and recursive-folder pickers, command-line
  opening, and multi-item file or folder drag-and-drop.
- Recursive folder playback with natural relative-path ordering; photo-only folders
  support Left/Right Arrow navigation through nested photos.
- Branded VeyloPlayer icon in the Windows executable, taskbar, Start menu, and installer.
- A modern interface that automatically follows the system light or dark appearance.
- File, Playback, and Help menus, including an in-app About dialog with the
  complete project license, third-party notices, version, and source link.
- A **Set as default player** menu action that opens VeyloPlayer's Windows
  Default Apps page or requests the corresponding macOS media associations.
- Fast startup: the window is shown before LibVLC is initialized, and packaged
  VLC plugins include a generated lookup cache.

See [product requirements](docs/requirements.md) and the [technical stack](docs/technical-stack.md).

## Build on Windows

Prerequisites:

- Windows 10 or 11, 64-bit.
- Visual Studio 2022 with **Desktop development with C++**.
- CMake 3.28 or newer.
- Python 3 available through the `py` launcher.

From PowerShell:

```powershell
.\scripts\bootstrap-windows.ps1
.\scripts\build-windows.ps1
```

The bootstrap script downloads pinned Qt and official VLC artifacts into the ignored `.deps/` directory and verifies the VLC SHA-256 checksum. It does not use a separately installed VLC application.

Run the debug build:

```powershell
.\build\windows\Debug\VeyloPlayer.exe
```

Build a release ZIP and MSI:

```powershell
.\scripts\build-windows.ps1 -Configuration Release -Package
```

Alternatively, run the one-command package builder from Command Prompt or by
double-clicking it in File Explorer:

```bat
scripts\build-windows.cmd
```

It bootstraps missing pinned dependencies, builds the Release application, runs
the tests, and creates the available Windows packages.

Packages are written to `dist/`. The bootstrap process downloads a pinned,
checksum-verified WiX Toolset 4 into `.deps/`, so a machine-wide WiX installation
is not required. Packaging fails if either the ZIP or MSI cannot be created. The
installer adds VeyloPlayer to the Start menu and Windows search. Its completion
screen also includes a checked **Launch VeyloPlayer** option.

The MSI presents the open-source license notice before installation. Both the
MSI and portable ZIP include the complete project and dependency license texts,
VLC notices, Qt SBOM files, and a source offer under `licenses/`.

Register a development build as an available Windows file handler:

```powershell
.\scripts\register-windows-file-types.ps1 `
  -ApplicationPath .\build\windows\Debug\VeyloPlayer.exe
```

Windows requires the user to confirm VeyloPlayer as the default through **Settings > Apps > Default apps**.

## Code quality

Install [pre-commit](https://pre-commit.com/) and enable the Git hook once per
checkout:

```powershell
pre-commit install
```

The hook checks changed files automatically before each commit. Run every check
against all tracked files with:

```powershell
pre-commit run --all-files
```

## Build on macOS

Prerequisites:

- macOS 12 or newer with Xcode command-line tools.
- CMake, Ninja, Qt 6.8 or newer, and VLC 3. Homebrew users can install them with:

```bash
brew install cmake ninja qt
brew install --cask vlc
```

Build the Release application, run its tests, deploy private Qt and LibVLC
runtimes, sign the bundle ad hoc, and create a verified DMG:

```bash
./scripts/build_mac.sh
```

The app is written to `build/macos/VeyloPlayer.app`, and the installer is
written to `dist/`. The script automatically detects Homebrew Qt and VLC in
`/Applications`. Set `QT_ROOT` or `LIBVLC_ROOT` to use another installation.
Set `CONFIGURATION=Debug PACKAGE=0` for a development build without a DMG, or
set `CODESIGN_IDENTITY` to sign with a Developer ID identity. Public releases
still require Apple notarization.

The app bundle contains legal notices under `Contents/Resources/licenses`; the
DMG also exposes the project license, third-party notices, and source offer at
its top level.

## Open-source distribution

- [Project license](LICENSE)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [Corresponding source information](SOURCE_CODE.md)
- [Contribution and DCO policy](CONTRIBUTING.md)
- [Binary-release compliance checklist](docs/distribution-compliance.md)

Do not publish a binary until the final package inventory, license/SBOM, source
availability, and codec/patent checks in the compliance checklist pass. A build
success alone is not a distribution approval.

## Verification status

The Windows MVP has been built and exercised end to end with JPEG navigation,
video rendering, embedded track selection, external subtitles, automatic
same-folder continuation, and recursive folder discovery. Core natural-sort and folder-sequence tests run during
every build. The self-contained Apple silicon macOS app and DMG build have been
validated on macOS hardware; signing with a distribution identity, notarization,
and broader compatibility testing remain release requirements.

## License

Copyright (C) 2026 VeyloPlayer contributors. VeyloPlayer is free software under
GPL-3.0-or-later and is provided without warranty. Qt, VLC/LibVLC, FFmpeg, and
other bundled components remain under their respective licenses; see the
third-party notices and installed `licenses/` directory. Open-source copyright
licenses do not by themselves grant every codec patent, trademark, decryption,
or media-content right.
