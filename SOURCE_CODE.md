# Source code availability

Every official VeyloPlayer binary release must provide equivalent network
access to the complete corresponding source used to build that release.

## VeyloPlayer

The source, CMake files, QML, installer definitions, and build scripts are at:

<https://github.com/infiz/VeyloPlayer>

The installed `SOURCE_OFFER.txt` records the exact Git commit for a binary.
Release automation must verify that commit is publicly accessible before the
binary is published. Modified distributors must publish their modifications
and the scripts/configuration needed to reproduce their binaries.

## Qt

Windows builds use the version recorded in `SOURCE_OFFER.txt`. Official Qt
source archives are available from:

<https://download.qt.io/official_releases/qt/>

At minimum, publish the exact sources corresponding to every deployed Qt DLL,
framework, QML module, and plugin. For the Windows 6.10.3 baseline this includes
`qtbase`, `qtdeclarative`, `qtsvg`, and any other module named by the packaged
Qt SBOM. Preserve Qt's source-side license files and third-party notices.

## VLC and contributed libraries

VLC source archives and required-library archives are available from:

<https://www.videolan.org/vlc/download-sources.html>

For the Windows 3.0.23 baseline, use the exact source archive and SHA-256 value
listed in `THIRD_PARTY_NOTICES.md`. Because the deployed VLC plugins contain or
link additional codec and utility libraries, the release source set must also
contain the exact contributed-library sources and build recipes corresponding
to the official VLC binary archive. A VLC core tarball alone is not necessarily
the complete corresponding source for the deployed runtime.

## Release rule

Do not publish a binary merely because an upstream URL exists. Copy the required
source archives to the same release/download location as the binary, or provide
another method that satisfies the applicable license for the full required
period. Record checksums and retain a durable copy under the distributor's
control. See `docs/distribution-compliance.md` for the release checklist.
