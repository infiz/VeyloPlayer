# Contributing to VeyloPlayer

Thank you for helping improve VeyloPlayer.

## License of contributions

VeyloPlayer is licensed under GPL-3.0-or-later. By submitting a contribution,
you agree that it may be distributed under GPL-3.0-or-later and that you have
the right to submit it. Do not copy code, media, fonts, icons, test fixtures, or
other material unless its license is compatible and its copyright/license
notices are included.

Contributions use Developer Certificate of Origin 1.1 sign-off. Add this line
to each commit, using your real name and an email address you control:

```text
Signed-off-by: Your Name <you@example.com>
```

Sign-off certifies the Developer Certificate of Origin available at
<https://developercertificate.org/>. Use `git commit -s` to add it.

## Before submitting

1. Keep changes compatible with Windows and macOS unless the code is clearly
   isolated to one platform.
2. Run `pre-commit run --all-files`.
3. Build the application and run its tests on every platform affected.
4. Document new runtime dependencies in `THIRD_PARTY_NOTICES.md` and
   `docs/technical-stack.md`.
5. Add the dependency's exact version, license expression, source location,
   copyright notice, and required license text or SBOM.
6. Never add a codec or VLC plugin to a release without reviewing its copyright,
   patent, trademark, and source-distribution requirements.

Contributors retain copyright in their contributions. No contributor license
agreement or copyright assignment is required unless the project adopts one in
the future through a clearly documented change.
