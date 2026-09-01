# VeyloPlayer Technical Stack

## 1. Decision summary

VeyloPlayer will be an open-source native C++20 desktop application using Qt Quick/QML for its user interface and LibVLC 3 for audio/video playback. Qt will handle JPEG decoding, filesystem access, settings, localization, and platform integration. LibVLC will own the complete media pipeline, including demuxing, decoding, hardware acceleration, audio output, embedded tracks, and external subtitles.

| Area | Selected technology | Responsibility |
| --- | --- | --- |
| Language | C++20 | Application and platform-integration code |
| UI framework | Qt Quick, QML, and Qt Quick Controls | Fluid consumer UI, dialogs, controls, keyboard input, accessibility, and HiDPI |
| Media engine | LibVLC 3 stable | Audio/video demuxing, decoding, rendering, seeking, tracks, and subtitles |
| Video rendering | LibVLC native video output | Render into an `HWND` on Windows and an `NSView` on macOS |
| Audio output | LibVLC audio outputs | WASAPI on Windows and Core Audio on macOS |
| Image decoding | Qt GUI `QImageReader` | JPEG validation, EXIF orientation, scaled decoding, and error reporting |
| Subtitles | LibVLC subtitle pipeline | Embedded and external `.srt`, `.vtt`, `.ass`, and `.ssa` subtitles |
| Build system | CMake and Ninja | Configure, compile, test, install, and package |
| Unit/UI tests | Qt Test, Qt Quick Test, and CTest | Core logic, QML components, signals, keyboard behavior, and integration tests |
| Windows package | CPack with WiX Toolset 4 | Signed MSI, uninstall, shortcuts, and file-type registration |
| macOS package | Qt deployment tools, CPack DragNDrop, and Apple signing tools | Signed/notarized `.app` and DMG |
| Settings | `QSettings` | Platform-native per-user preferences |
| Logging | `QLoggingCategory` | Structured application and playback diagnostics |

Initial implementation baseline: Qt 6.10.3, C++20, CMake 3.28 or newer, MSVC 2022 on Windows, and Apple Clang from a supported Xcode release on macOS. LibVLC remains on the stable version 3 line until version 4 is officially stable and a deliberate migration is approved. Every dependency must be pinned to an exact version in the build configuration; release scripts must not download an unpinned `latest` artifact.

## 2. Why this stack

### 2.1 C++20

- Qt and LibVLC expose native C/C++ APIs, so C++ avoids a language bridge in the playback path.
- C++ provides deterministic resource ownership for media handles, windows, and file access.
- C++20 is fully capable for this product without requiring very recent C++23 compiler support.
- Application code will use RAII, smart pointers, value types, and standard-library algorithms. Raw ownership is not allowed outside narrowly scoped wrappers around C APIs.

### 2.2 Qt Quick and QML

Qt Quick is selected for the presentation layer because VeyloPlayer is a new consumer product that requires a fluid, responsive, brandable interface. QML defines presentation and short visual state transitions; application state and business logic remain in testable C++ controllers.

LibVLC still renders efficiently into a native child window. Qt Quick's `WindowContainer` embeds a dedicated `QWindow`; its platform handle maps to an `HWND` on Windows and an `NSView` on macOS. This preserves LibVLC hardware decoding and avoids copying every decoded frame through application memory.

Required Qt modules:

- `Qt6::Core` for the event loop, filesystem support, URLs, settings, command-line parsing, and inter-process communication.
- `Qt6::Gui` for images, icons, keyboard input, and display information.
- `Qt6::Quick` for the GPU-accelerated scene, windows, and QML integration.
- `Qt6::QuickControls2` for accessible desktop controls, menus, and styling.
- `QtQuick.Dialogs` for platform file dialogs.
- `Qt6::Svg` for scalable application and control icons.
- `Qt6::Test` for automated tests; it is a development-only dependency.
- `Qt Quick Test` for QML component and interaction tests; it is a development-only dependency.

Use a small VeyloPlayer Qt Quick Controls style based on the Basic style. Shared design tokens define color, typography, spacing, corner radii, focus rings, motion duration, and icon sizing. Native dialogs and platform keyboard conventions remain platform-specific. The style follows the system color scheme by default and supports explicit Light and Dark preferences.

QML must not contain filesystem, sorting, playback, packaging, or persistence logic. JavaScript is limited to local presentation calculations and transient animation state.
- `Qt6::Svg` only if the final icon set uses SVG assets at runtime.

Qt Multimedia is not part of the selected playback path. Running two media engines would add size and produce inconsistent codec, track, and subtitle behavior.

### 2.3 LibVLC 3

LibVLC is the embedded playback engine, not a separately launched VLC application. VeyloPlayer will call the LibVLC C API through a small internal C++ RAII adapter.

LibVLC is selected because it provides:

- broad container and codec coverage from one cross-platform engine;
- audio, video, and subtitle track discovery and selection;
- external subtitle attachment before or during playback as supported by the API;
- seeking, duration, volume, mute, and playback-state events;
- Windows and macOS hardware-accelerated decoding where supported;
- native Windows and macOS audio-device output;
- native video-window embedding without application-side frame copies; and
- an LGPL 2.1 library license, subject to the distribution obligations in Section 10.

The adapter must isolate all LibVLC types from the rest of the application. This makes a future LibVLC 4 migration or a media-engine replacement possible without rewriting the UI and folder-navigation logic.

## 3. Encoder, decoder, and codec policy

### 3.1 Encoding

No encoder is required for the MVP. VeyloPlayer does not record, transcode, convert, or modify media. Encoding libraries and LibVLC conversion/streaming modules must not be included merely because they are available.

### 3.2 Demuxing and decoding

LibVLC and its runtime plugin set are the only supported demux/decode path. VeyloPlayer must not link directly to FFmpeg's `libavcodec`, `libavformat`, `libavutil`, `libswresample`, or `libswscale` in the MVP.

LibVLC may use FFmpeg and other codec libraries internally. Keeping those dependencies behind LibVLC gives VeyloPlayer one playback state machine and one place to manage codec updates.

Release builds must bundle a tested, minimal LibVLC runtime containing every module needed by the required formats. Removing unused modules is allowed only after automated playback tests confirm that all required containers, codecs, subtitles, hardware-decoding paths, and audio outputs still work.

### 3.3 Hardware acceleration

- Hardware decoding is enabled using LibVLC's automatic/default policy.
- VeyloPlayer must fall back to software decoding when the selected hardware path fails.
- The application must not force a particular GPU API globally.
- Release testing must cover at least H.264 and HEVC where legal/test media are available, on representative Intel, AMD, Apple, and NVIDIA hardware as applicable.

### 3.4 Codec support contract

File extensions identify candidates; successful probing by LibVLC determines whether the content is playable. The UI must not claim that every codec possible inside a listed container is guaranteed.

At minimum, the release qualification suite should include:

- MP3, AAC/M4A, PCM/WAV, FLAC, and Vorbis/Ogg audio;
- H.264/AAC in MP4 and MOV;
- H.264 and HEVC content in MKV when the bundled codec build permits it;
- VP8/VP9 and Opus in WebM;
- multiple embedded audio tracks;
- embedded text subtitles; and
- external SRT, WebVTT, ASS, and SSA subtitles.

## 4. Audio architecture

LibVLC owns decoding, synchronization, mixing, volume, mute, and device output. VeyloPlayer must not insert a second audio library such as SDL, PortAudio, OpenAL, or Qt Multimedia.

- Windows output uses the LibVLC WASAPI output module.
- macOS output uses the LibVLC Core Audio output module.
- The UI communicates volume and mute changes to the playback adapter.
- Audio-device changes and device loss are observed through LibVLC events/logging and surfaced as recoverable playback errors.
- Custom decoded-audio callbacks are out of scope because they would make VeyloPlayer responsible for timing and output buffering.

## 5. Video architecture

The player contains a dedicated native `VideoSurfaceWindow` (`QWindow`) whose lifetime is longer than the active media player. QML embeds it using `WindowContainer`.

- On Windows, its native handle is passed to `libvlc_media_player_set_hwnd()`.
- On macOS, its native view is passed to `libvlc_media_player_set_nsobject()`.
- LibVLC renders video and subtitles into that surface.
- Qt Quick owns the surrounding controls, menus, fullscreen window state, keyboard handling, and visual transitions.
- The implementation must not use LibVLC decoded-frame callbacks for normal playback because that disables or weakens zero-copy hardware rendering and adds avoidable CPU copies.
- A native embedded video window is always above ordinary items in the Qt Quick scene. Controls normally occupy a dedicated control rail outside the video surface; fullscreen overlay controls use a separate child `Window` layered above the video, following the `WindowContainer` embedding constraints.
- Platform-specific code is confined to `src/platform/windows/` and `src/platform/macos/`.

## 6. Subtitle architecture

No separate subtitle parser is required for the MVP. LibVLC is responsible for parsing, timing, styling, character decoding, and rendering embedded and external subtitle tracks.

- Embedded track metadata is exposed through the playback adapter as stable application value objects.
- Selecting `Off` disables the active subtitle track.
- External subtitles are attached as LibVLC media slaves using file URLs created by Qt, not hand-built URI strings.
- The application accepts `.srt`, `.vtt`, `.ass`, and `.ssa` in its picker, while LibVLC performs final content validation.
- External subtitle failure does not stop audio/video playback.
- The application never edits or rewrites subtitle files.

Do not add `libass` directly in the MVP. LibVLC may use it internally for styled subtitles. A direct `libass` integration would create a second subtitle renderer and synchronization path without adding a current requirement.

## 7. Image architecture

JPEG display uses `QImageReader`, `QImage`, and `QPixmap`; no separate image library is needed.

- Enable `QImageReader::setAutoTransform(true)` so EXIF orientation is applied.
- Use scaled decoding when supported to avoid allocating unnecessarily large full-resolution images for small windows.
- Preserve aspect ratio and use smooth display transforms.
- Keep a bounded cache containing only the current image and, when useful, one neighboring image in each direction.
- Keep Qt's image allocation limit enabled and treat oversized or malformed images as recoverable errors.
- File navigation is based on `.jpg` and `.jpeg` suffixes case-insensitively, but the decoder validates actual content.

Qt's JPEG image plugin/runtime dependency must be included by the platform deployment step.

## 8. Application architecture

Use a layered structure with UI-independent core logic:

```text
src/
  app/                 Application startup and dependency composition
  core/                Media types, natural sorting, folder sequence, state
  playback/            LibVLC adapter and PlaybackController
  images/              Image loading, scaling, cache, and navigation
  ui/                  QML views, components, themes, and C++ view models
  platform/
    windows/            File activation and Windows-specific integration
    macos/              Finder activation and macOS-specific integration
tests/
  unit/
  integration/
scripts/
  build-windows.ps1
  build-macos.sh
  bootstrap-windows.ps1
  bootstrap-macos.sh
```

Key boundaries:

- `PlaybackController` is the only application component allowed to command LibVLC.
- `FolderSequence` owns filtering and deterministic natural ordering. It must use a tested digit-run comparator rather than locale-dependent collation, ensuring the same sequence on Windows and macOS.
- `MediaSession` owns the current path, media kind, folder context, selected tracks, and state.
- `ImageController` owns image decoding and previous/next navigation and does not depend on LibVLC.
- The UI observes controller signals and never infers playback state from button state.
- LibVLC callbacks are marshaled onto the Qt application thread before updating application state or QML-facing models.
- QML observes typed C++ properties and signals; it never calls LibVLC directly.

## 9. Build, dependencies, and packaging

### 9.1 Dependency policy

- Use shared/dynamic Qt and LibVLC libraries for release builds.
- Pin exact Qt, LibVLC, CMake, and packaging-tool versions in build documentation or machine-readable configuration.
- Acquire third-party binaries only from official sources or build them from pinned source revisions.
- Validate every downloaded artifact with a committed SHA-256 checksum.
- Never depend on a user's separate VLC installation.
- Package LibVLC's runtime plugins inside VeyloPlayer's private application directory/bundle.
- Generate a software bill of materials and third-party notice file for each release.

The bootstrap scripts install or locate build dependencies. The build scripts compile, test, stage, sign when release mode is enabled, and produce packages under `dist/`.

### 9.2 Windows

- Compiler: MSVC 2022, 64-bit.
- Generator: Ninja through CMake.
- Qt deployment: `windeployqt` or Qt's CMake deployment API.
- Installer: CPack WiX generator targeting WiX Toolset 4.
- Signing: Windows SDK `signtool` for executable, DLL, and MSI signing.
- File associations: WiX registration of application capabilities and ProgIDs; Windows retains user consent and control of defaults.
- Single-instance file activation: `QLocalServer`/`QLocalSocket` with authenticated same-user message handling and correctly encoded absolute paths.

### 9.3 macOS

- Compiler: Apple Clang from a supported Xcode release.
- Architectures: Apple silicon (`arm64`) is required; a universal `arm64;x86_64` build is preferred if the selected LibVLC artifact is universal.
- Qt deployment: `macdeployqt` or Qt's CMake deployment API.
- LibVLC deployment: copy its dynamic libraries and plugin directory into the app bundle, fix bundle-relative load paths, and verify with `otool`.
- File associations: declare document types and UTTypes in `Info.plist`; handle Finder opens through Qt file-open events.
- Signing/notarization: `codesign` with hardened runtime, `notarytool`, and `stapler`.
- DMG: CPack DragNDrop after the complete app bundle has been assembled and signed.

macOS packaging order is strict: assemble the complete bundle, rewrite/verify load paths, sign nested libraries and the app, verify the signature, create/sign the DMG, notarize it, staple the ticket, and run a Gatekeeper assessment.

## 10. Licensing and distribution constraints

VeyloPlayer will be developed and distributed as open-source software. GPL-3.0-or-later is the recommended license for VeyloPlayer's original source code because it is compatible with the selected open-source dependency strategy and ensures that distributed modifications to the application remain open. The final license choice must be confirmed before adding the repository's `LICENSE` file and release artifacts.

The dependency plan uses Qt under LGPL 3 and LibVLC under LGPL 2.1 or later. This is a technical distribution plan, not legal advice.

- Dynamically link Qt and LibVLC.
- Include the applicable license texts, copyright notices, third-party notices, and relinking/replacement information required by their licenses.
- Make the exact corresponding library source, modifications, and build information available as required by the selected licenses.
- Do not use GPL-only Qt modules or LibVLC plugins in a proprietary release without an explicit license decision.
- Do not use the VLC name or cone logo as VeyloPlayer branding.
- Review codec patent and royalty obligations for each distribution territory before publishing. Library licensing does not grant patent licenses for formats such as H.264 or HEVC.
- Run an automated license/SBOM audit on the actual packaged binaries and plugins, not only the declared top-level dependencies.
- Keep VeyloPlayer source, build scripts, dependency manifests, and release instructions publicly available from the canonical source repository.
- Add contributor guidance and require contributors to certify that they have the right to submit their changes; use the Developer Certificate of Origin unless the project later adopts a contributor license agreement.

## 11. Testing stack

- Qt Test for C++ unit tests, data-driven sorting cases, controller signals, and settings.
- Qt Quick Test for QML components, focus order, keyboard behavior, system-theme contrast, control visibility, and responsive layouts.
- CTest as the common test runner invoked by local and CI builds.
- Small redistributable media fixtures committed under `tests/fixtures/`, with documented provenance and licenses.
- Integration tests for LibVLC initialization, supported fixture playback, track enumeration, subtitle loading, end-of-media events, and file-handle release.
- Installer smoke tests in clean Windows and macOS virtual machines.
- Manual hardware matrix testing for GPU decoding, sleep/wake, display changes, audio-device changes, fullscreen, and multi-monitor behavior.

The natural-sort suite must include numeric runs, leading zeros, mixed case, Unicode, identical folded names, and deterministic tie-breaking.

## 12. Alternatives considered

### Qt Multimedia with its FFmpeg backend

This would remove the LibVLC dependency and Qt currently exposes embedded audio/subtitle track selection. It is not selected because external subtitle attachment is not a stable public `QMediaPlayer` feature, and implementing a separate parser/renderer would duplicate timing and rendering responsibilities. It remains the preferred fallback if a prototype exposes unacceptable LibVLC packaging or embedding issues.

### Direct FFmpeg plus libass and an audio library

This offers maximum control but requires VeyloPlayer to implement demuxing, clocks, A/V synchronization, seeking, buffering, hardware-frame handling, subtitle composition, and audio-device recovery. That is disproportionate for this MVP.

### C# with Avalonia and LibVLCSharp

This can shorten general UI development, but adds the .NET runtime, a managed/native binding layer, and more macOS bundle complexity. It is a reasonable team-driven alternative if the maintainers have substantially stronger C# experience than C++ experience.

### Electron or another browser shell

This is not selected because local codec support and hardware playback would still require a native media engine, while the browser runtime would add significant package size and memory use.

### libmpv

libmpv has a strong playback API and is technically capable. LibVLC is preferred because its stable library is explicitly LGPL and its native Windows/macOS embedding API matches the product. libmpv can be reconsidered only after a deliberate licensing and distribution review.

## 13. Prototype gate

Before building the full UI, create a short technical spike that proves all of the following on both Windows and macOS:

1. Package a minimal Qt application with a private LibVLC runtime and launch it on a clean machine.
2. Play MP4/H.264/AAC and MKV files with hardware acceleration enabled.
3. Render video into the Qt-owned native surface and enter/exit fullscreen cleanly.
4. Enumerate and switch two audio tracks and two embedded subtitle tracks without losing playback position.
5. Load SRT, WebVTT, ASS, and SSA external subtitles from Unicode paths.
6. Receive end-of-media and error events safely on the Qt thread.
7. Release file handles after the media is closed.
8. Produce an MSI and a signed/notarized DMG containing every runtime dependency.

If this spike fails because of LibVLC embedding, signing, or packaging—not an application bug—the fallback evaluation is Qt Multimedia with its FFmpeg backend plus a narrowly scoped external-subtitle solution.

## 14. Primary references

- [Qt supported platforms](https://doc.qt.io/qt-6/supported-platforms.html)
- [Qt user-interface guidance](https://doc.qt.io/qt-6/qt-intro.html)
- [Qt Quick Controls styling](https://doc.qt.io/qt-6/qtquickcontrols-styles.html)
- [Qt Quick WindowContainer](https://doc.qt.io/qt-6/qml-qtquick-windowcontainer.html)
- [Qt deployment with CMake](https://doc.qt.io/qt-6/cmake-deployment.html)
- [Qt macOS deployment](https://doc.qt.io/qt-6/macos-deployment.html)
- [Qt licensing](https://doc.qt.io/qt-6/licensing.html)
- [Qt QImageReader](https://doc.qt.io/qt-6/qimagereader.html)
- [Qt Test overview](https://doc.qt.io/qt-6/qtest-overview.html)
- [VideoLAN LibVLC overview](https://www.videolan.org/vlc/libvlc.html)
- [LibVLC media API](https://videolan.videolan.me/vlc-3.0/group__libvlc__media.html)
- [LibVLC media-player API](https://videolan.videolan.me/vlc-3.0/group__libvlc__media__player.html)
- [CMake CPack WiX generator](https://cmake.org/cmake/help/latest/cpack_gen/wix.html)
- [CMake packaging generators](https://cmake.org/cmake/help/latest/manual/cpack-generators.7.html)
- [Microsoft default-app registration](https://learn.microsoft.com/en-us/windows/win32/shell/default-programs)
