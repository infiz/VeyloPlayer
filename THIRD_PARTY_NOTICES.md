# Third-party notices

VeyloPlayer is free and open-source software. Its original source code is
licensed under **GPL-3.0-or-later**. The complete GPL version 3 text is in
[`LICENSE`](LICENSE).

Copyright in VeyloPlayer remains with the individual contributors. Copyright
in third-party software remains with its respective authors. Nothing in the
VeyloPlayer license changes a third party's license.

## Qt

VeyloPlayer dynamically links and deploys parts of the Qt framework, including
Qt Core, GUI, Network, OpenGL, QML, Quick, Quick Controls, Quick Dialogs,
Quick Layouts, Quick Shapes, SVG, platform and image-format plugins, and their
runtime dependencies.

- Copyright: The Qt Company Ltd. and other Qt contributors.
- Windows release baseline: Qt 6.12.0.
- License used by VeyloPlayer distributions: LGPL-3.0-only, or GPL-3.0-only
  where a shipped Qt component is not offered under LGPL-3.0-only.
- License texts: `LICENSE` and `LICENSES/LGPL-3.0.txt`.
- Licensing information: <https://doc.qt.io/qt-6/licensing.html>
- Exact Qt 6.12.0 sources: <https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/>
- Qt source repositories: <https://code.qt.io/cgit/qt/>

Qt contains third-party code under additional permissive and open-source
licenses. Windows packages include Qt's SPDX software bills of materials in
the installed `licenses/qt-sbom` directory when supplied by the pinned Qt
distribution. A macOS release must include the SBOM or corresponding notices
for the exact Qt build used to create that release.

VeyloPlayer uses shared Qt libraries. Recipients may replace those libraries
with ABI-compatible modified builds. VeyloPlayer imposes no restriction on
reverse engineering performed to debug such modifications.

## VLC, LibVLC, and codec libraries

VeyloPlayer dynamically links LibVLC and privately deploys VLC's runtime plugin
directory. Those plugins include multimedia libraries such as FFmpeg and other
codec, demuxer, subtitle, networking, and audio libraries. The precise set and
license selection depend on the VLC binary archive used for a release.

- Copyright: VideoLAN and VLC contributors; individual bundled libraries are
  copyright their respective authors.
- Windows release baseline: official VLC 3.0.24 64-bit archive.
- The Direct3D 11 output plugin is rebuilt from that version with VeyloPlayer's
  planar 10-bit 4:2:2 patch. Its original LGPL notices remain in the source.
  The patch and build recipe are in `scripts/vlc-d3d11/` and are included in
  Windows packages under `licenses/vlc/build/`.
- The rebuilt plugin statically links MinGW-w64 and GCC runtime components.
  Their copyright/license notices and the compiler version used are included
  under `licenses/vlc/` alongside the build manifest.
- `libvlc` and `libvlccore`: LGPL-2.1-or-later according to the VLC source.
- VLC runtime and plugins: licenses vary by file. The official Windows archive
  is distributed with the GPL-2.0 text, so VeyloPlayer release engineering
  treats the complete deployed VLC runtime as GPL-covered unless an audit of
  the exact files establishes a different license.
- License texts: `LICENSES/GPL-2.0.txt` and `LICENSES/LGPL-2.1.txt`.
- VLC source and required-library archives: <https://www.videolan.org/vlc/download-sources.html>
- Exact VLC 3.0.24 source: <https://download.videolan.org/videolan/vlc/3.0.24/vlc-3.0.24.tar.xz>
- Source checksum (SHA-256):
  `e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf`

Windows packages also include VLC's original `COPYING.txt`, `AUTHORS.txt`, and
`README.txt` when available from the pinned archive. Binary releases must make
the exact corresponding VLC and contributed-library sources available next to
the downloadable VeyloPlayer package as described in `SOURCE_CODE.md`.

Recipients may replace `libvlc`, `libvlccore`, and the private VLC plugins with
compatible modified builds. VeyloPlayer imposes no restriction on reverse
engineering performed to debug such modifications.

"VLC", "VideoLAN", and related marks belong to the VideoLAN organization.
VeyloPlayer is independent of and is not endorsed by VideoLAN. The VLC cone
and VideoLAN branding are not used by VeyloPlayer.

## Build-only and system components

CMake, Ninja, WiX Toolset, aqtinstall, the standalone 7-Zip extractor, Python,
Visual Studio build tools, Apple
developer tools, and code-signing/notarization tools are used to build or
package VeyloPlayer but are not intentionally redistributed as part of the
application. Their licenses still govern use of those tools by developers.
Operating-system libraries supplied by Windows or macOS are system components
and are not part of the VeyloPlayer source distribution.

## No patent or media-content license

Open-source copyright licenses do not grant every patent, trademark, media,
DVD/Blu-ray decryption, broadcast, or content-distribution right that may be
required in a particular country. Distributors must review the codecs and VLC
plugins they ship and obtain or remove any functionality requiring additional
rights. Users remain responsible for rights to the media they play.

## Corrections

If a required attribution or license is missing, open an issue at
<https://github.com/infiz/VeyloPlayer/issues> before redistributing the affected
binary. Release artifacts must not be published until the actual packaged-file
audit and source-availability checks in `docs/distribution-compliance.md` pass.
