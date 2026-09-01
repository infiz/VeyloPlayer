# VeyloPlayer

VeyloPlayer is a modern, open-source desktop player for local video, audio, and JPEG files. The first end-to-end implementation targets Windows while keeping the C++/Qt/LibVLC application core compatible with macOS.

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

Register a development build as an available Windows file handler:

```powershell
.\scripts\register-windows-file-types.ps1 `
  -ApplicationPath .\build\windows\Debug\VeyloPlayer.exe
```

Windows requires the user to confirm VeyloPlayer as the default through **Settings > Apps > Default apps**.

## Build on macOS

The initial macOS build entry point is `scripts/build-macos.sh`. It expects pinned Qt and LibVLC locations through `QT_ROOT` and `LIBVLC_ROOT`. Set `PACKAGE=1` to create the DMG. The app bundle declares its supported audio, video, and JPEG document types and handles Finder open events. Signing, notarization, and final DMG validation remain part of the macOS release milestone.

## Verification status

The Windows MVP has been built and exercised end to end with JPEG navigation,
video rendering, embedded track selection, external subtitles, automatic
same-folder continuation, and recursive folder discovery. Core natural-sort and folder-sequence tests run during
every build. macOS remains source-compatible but requires validation on macOS
hardware before a public release.

## License

VeyloPlayer will be open source. The exact project license is still being selected; GPL-3.0-or-later is the current recommendation. Dependency licenses and codec distribution obligations must be audited before publishing binaries.
