# Distribution compliance checklist

This checklist is a release-engineering control, not legal advice. Copyright
and patent rules vary by component and distribution territory. Obtain qualified
legal review before a public binary release when required.

## Required for every binary release

1. Build from a clean, public VeyloPlayer commit and record that commit in the
   generated `SOURCE_OFFER.txt`.
2. Generate a complete file inventory and cryptographic hashes from the final
   MSI, ZIP, `.app`, and DMG—not from the dependency declaration alone.
3. Identify every Qt framework/DLL, Qt plugin, QML module, VLC library, VLC
   plugin, statically linked library, font, icon, and media asset in that
   inventory.
4. Resolve each file to an SPDX license expression, copyright notice, exact
   source revision/archive, build recipe, and any attribution requirements.
5. Include `LICENSE`, `LICENSES/`, `THIRD_PARTY_NOTICES.md`, and the generated
   `SOURCE_OFFER.txt` in the installed application and portable archive.
6. Include Qt's supplied SPDX SBOM for the exact distributed Qt build. If the
   build supplier omits an SBOM, generate an equivalent notice set from the Qt
   sources before release.
7. Include VLC's original COPYING, AUTHORS, and README files when supplied with
   the chosen runtime.
8. Publish the complete corresponding VeyloPlayer, Qt, VLC, and VLC contributed
   library sources next to the binaries, with checksums and build instructions.
9. Verify users can replace the shared Qt and LibVLC files. Do not sign, encrypt,
   contractually prohibit, or technically block user-modified libraries beyond
   platform security that users can reapply themselves.
10. Confirm the Windows installer presents the open-source notice and that the
    DMG exposes the same license, notices, and source offer at its top level.
11. Preserve all existing copyright, attribution, disclaimer, and trademark
    notices. Do not use Qt, VLC, FFmpeg, codec, or contributor names to imply
    endorsement.
12. Review codec patent pools, DVD/Blu-ray/decryption functionality, export
    controls, and media-format royalties for every intended territory. Remove
    plugins that cannot be distributed lawfully there.
13. Sign and notarize only after the license/source payload is final; verify the
    signed packages still contain it.

## Windows baseline audit

The current Windows process downloads Qt 6.10.3 and the official VLC 3.0.23
64-bit archive. The entire VLC plugin directory is currently packaged. That
directory contains substantially more functionality than the MVP uses, so it
must receive a full binary/license/patent audit. A future hardening change should
replace the full copy with an allowlist proven by playback tests, but pruning
does not remove the obligation to audit the remaining plugins.

Before release, inspect the generated ZIP or MSI staging tree and verify at
least the following paths:

- `bin/Qt6*.dll`, `plugins/`, and `qml/` against the included Qt SBOM;
- `bin/libvlc.dll`, `bin/libvlccore.dll`, and every file in `bin/plugins/`;
- `licenses/` for all required texts, upstream notices, SBOMs, and the generated
  source offer.

## macOS baseline audit

The macOS script uses Qt and VLC from the build host. Therefore a release cannot
reuse a notice generated for a different machine. Record the versions and audit:

- `Contents/Frameworks`, `Contents/PlugIns`, and `Contents/Resources/qml`;
- `Contents/MacOS/lib` and every file in `Contents/MacOS/plugins`;
- `Contents/Resources/licenses` and the DMG's top-level notice files.

Replacing libraries invalidates the app's signature. This is not a prohibition:
users can apply an ad-hoc signature to their modified local copy with Apple's
`codesign` tool. Document any additional steps introduced by future platform
security changes.

## Source retention

Retain the exact source archives, hashes, build logs, compiler/linker flags,
dependency SBOM, and final package inventory for the full period required by
each applicable license. GPL-2.0 written offers can require at least three years;
prefer equivalent source downloads kept available for as long as binaries are
offered and for the longest applicable downstream requirement.

## Release blocker

A passing build is not a license audit. If any packaged file has an unknown
license, missing notice, unavailable corresponding source, incompatible term,
or unresolved patent/decryption restriction, do not publish that binary.
Packages built from an uncommitted tree are marked as development builds in
`SOURCE_OFFER.txt` and must never be redistributed.
