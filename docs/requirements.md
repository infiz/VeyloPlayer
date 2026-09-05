# VeyloPlayer Product Requirements

## 1. Purpose

VeyloPlayer is a desktop media player for Windows and macOS. It provides a simple way to open and play local audio files, video files, and JPEG images, including files opened directly from the operating system.

This document defines the minimum viable product (MVP). Technology choices and detailed visual design are intentionally left to the implementation phase.

## 2. Product goals

- Play common local audio and video files reliably.
- Display JPEG images and support keyboard-based browsing.
- Integrate with Windows and macOS as an available handler for supported file types.
- Let users choose embedded audio and subtitle tracks and load external subtitles.
- Continue through playable files in the current folder using human-friendly filename ordering.
- Provide installable Windows and macOS release packages with repeatable build scripts.
- Develop and distribute VeyloPlayer as an open-source product.

## 3. Supported platforms

### 3.1 Windows

- Support 64-bit Windows 10 and Windows 11.
- Provide a signed installer suitable for end-user installation.
- The installed application must appear in Windows' default-app and **Open with** choices for every supported file type.

### 3.2 macOS

- Support the current macOS release and the two previous major releases at the time of a VeyloPlayer release.
- Provide a signed and notarized application distributed in a DMG disk image.
- Support Apple silicon. Intel support is desirable but may be delivered as a separate build or universal binary.
- The installed application must appear as an available application for every supported file type in Finder's **Open With** and **Get Info** workflows.

### 3.3 Default application behavior

- Installation must register VeyloPlayer as capable of opening its supported file types.
- VeyloPlayer must provide a user-facing action that opens or guides the user to the operating system's default-app controls.
- The application and installers must follow operating-system consent rules. They must not silently override an existing default application where the operating system requires the user to confirm the change.
- Opening an associated file from File Explorer, Finder, the command line, or another application must reuse the running VeyloPlayer instance when practical and begin opening the requested file.

## 4. Supported media

The initial release must support these categories:

| Category | Required extensions | Expected behavior |
| --- | --- | --- |
| Audio | `.mp3`, `.m4a`, `.aac`, `.wav`, `.flac`, `.ogg` | Play audio and expose basic transport controls. |
| Video | `.mp4`, `.m4v`, `.mov`, `.mkv`, `.webm`, `.avi` | Play video and expose track, subtitle, and transport controls. |
| Image | `.jpg`, `.jpeg` | Display the image and enable previous/next image navigation. |
| External subtitle | `.srt`, `.vtt`, `.ass`, `.ssa` | Load as a subtitle track through the selected playback engine. |

Actual codec availability may depend on the bundled playback engine and platform licensing. Unsupported or corrupt content must produce a clear, non-destructive error instead of crashing or hanging the application.

## 5. Functional requirements

### 5.1 Opening files

- **FR-001:** The user can open a supported file through an in-app file picker.
- **FR-001A:** The play bar's Add media control offers distinct actions for adding one or more files and for adding a folder.
- **FR-001B:** Selecting multiple files creates a naturally ordered playback queue. Selecting a folder recursively discovers media using the rules in FR-039.
- **FR-002:** The user can open a supported file by passing its path when launching VeyloPlayer.
- **FR-003:** The user can open an associated file from Windows File Explorer or macOS Finder.
- **FR-004:** The open dialog filters files by supported media category and also offers an all-supported-files view.
- **FR-005:** Opening a new file stops the current item, replaces it, and begins playback for audio/video or displays it for an image.
- **FR-006:** If a file cannot be opened, VeyloPlayer shows the filename and a useful reason when available, while remaining usable.

### 5.2 Audio and video playback

- **FR-010:** Audio and video files support play, pause, seek, elapsed time, total duration, and volume control.
- **FR-011:** The user can mute and unmute audio.
- **FR-012:** Video preserves the source aspect ratio by default.
- **FR-013:** The application prevents the display from sleeping while video is actively playing and restores normal system behavior when playback stops, pauses, or the application exits.
- **FR-014:** Playback state must clearly distinguish playing, paused, loading, ended, and failed states.
- **FR-015:** Video can enter and leave true fullscreen mode from a visible control, the `F` or `F11` key, and a double-click on the video surface. `Escape` leaves fullscreen mode.
- **FR-016:** VeyloPlayer stores the current local video path, duration, and playback position locally at regular intervals and when playback pauses, stops, switches files, or the application exits.
- **FR-017:** On a normal launch without an explicitly requested file, VeyloPlayer offers to resume the last available video from its stored position without starting playback unexpectedly.
- **FR-018:** A resume position is offered only after at least five seconds of meaningful playback and is discarded when it is within ten seconds of the end. Missing, unreadable, or unsupported previous files do not prevent the application from opening normally.
- **FR-019:** A single primary-button click on the audio or video presentation surface toggles play and pause. A video double-click performs only the fullscreen action and must not also leave playback in an unintended state.
- **FR-019A:** After an audio or video file reaches its end and no next item replaces it, the timeline remains enabled. Seeking backward restarts the completed item at the selected position.
- **FR-019B:** For videos containing embedded chapters or position markers, the seek timeline displays each chapter boundary after the start as a light-grey divider. The Previous and Next transport buttons move to the adjacent marker in that direction before opening an adjacent media file. At the first or last marker, the corresponding button falls back to the previous or next file when one exists.

### 5.3 Audio tracks and subtitles

- **FR-020:** For a video containing multiple audio tracks, the user can view and select any available audio track during playback.
- **FR-021:** The audio-track menu identifies tracks using available metadata such as language, title, and channel layout; otherwise it uses a stable numbered label.
- **FR-022:** For a video containing embedded subtitle tracks, the user can view and select any available subtitle track during playback.
- **FR-023:** The user can turn subtitles off.
- **FR-024:** The user can select one external subtitle file through a file picker and use it with the current video.
- **FR-025:** Loading an external subtitle must not modify the source video or subtitle file.
- **FR-026:** The subtitle menu distinguishes external subtitles from embedded subtitles.
- **FR-026A:** VeyloPlayer stores the selected audio-track and embedded-subtitle preference locally for each media folder, including subtitles Off. When another file from that folder opens, it first matches the saved track label and then falls back to the saved track position when labels differ. External subtitle files remain specific to the file for which they were loaded.
- **FR-027:** Changing audio or subtitle tracks should preserve the current playback position and playing/paused state.
- **FR-028:** An invalid or unsupported external subtitle file produces a clear error and does not interrupt video playback.

### 5.4 Folder sequence and automatic continuation

- **FR-030:** When an audio or video item finishes normally, VeyloPlayer scans that item's folder for other supported audio and video files.
- **FR-031:** Files are ordered by natural, case-insensitive filename order so numeric portions compare by numeric value. For example, `1.mp4`, `2.mp4`, `3.mp4`, `10.mp4`, `11.mp4`, `12.mp4` is the required order.
- **FR-032:** If the current file has a next item in that order, VeyloPlayer opens it and starts playback automatically.
- **FR-033:** If the current file is the final playable item, playback stops in the ended state.
- **FR-034:** Automatic continuation stays within the current folder and does not search subfolders.
- **FR-035:** Folder sequencing ignores hidden files, unsupported extensions, directories, the current file, and files that disappeared after the folder was scanned.
- **FR-036:** If a candidate next file cannot be played, VeyloPlayer reports or records the failure and attempts the next playable candidate without entering an infinite loop.
- **FR-037:** Manually opening a file establishes that file's containing folder as the new sequence context.
- **FR-038:** Filename ordering is deterministic when names compare equally after case folding and numeric comparison.
- **FR-039:** Dropping a folder onto VeyloPlayer recursively discovers supported files in that folder and its visible subfolders. Audio and video files are played continuously in natural relative-path order. If no audio or video files are present, the first JPEG is displayed and Left/Right Arrow navigation traverses the recursively discovered JPEGs. Dropping a new file or folder replaces the recursive queue context.
- **FR-039A:** While audio or video is open, Previous and Next controls navigate the supported audio/video sequence using the same natural order and recursive queue rules as automatic continuation. Each control is disabled when its direction has no item.

### 5.5 JPEG viewing and navigation

- **FR-040:** Opening a `.jpg` or `.jpeg` file displays it fitted within the available window while preserving its aspect ratio.
- **FR-041:** Pressing the Right Arrow key displays the next JPEG in the same folder using the natural ordering defined in FR-031, regardless of the current zoom level.
- **FR-042:** Pressing the Left Arrow key displays the previous JPEG in the same folder using the natural ordering defined in FR-031, regardless of the current zoom level.
- **FR-043:** Image navigation does not wrap at the first or last image. The unavailable direction is disabled or has no effect.
- **FR-044:** Image navigation includes only `.jpg` and `.jpeg` files, is case-insensitive, and does not search subfolders.
- **FR-045:** Audio/video autoplay is not triggered while navigating images.
- **FR-046:** Keyboard image navigation works whenever the main player window has focus, except while a text-entry or menu control is consuming the key.
- **FR-047:** Scrolling the mouse wheel over a JPEG zooms smoothly in or out around the pointer location, from the fitted view up to 800%, without blanking the viewport while the zoom changes. The selected zoom level carries over when navigating to the previous or next image.
- **FR-048:** While a JPEG is zoomed, dragging it with the primary mouse button repositions it within the viewport without exposing space beyond its usable image bounds.
- **FR-049:** While a JPEG is zoomed, the Up and Down Arrow keys reposition it vertically. Left and Right always navigate to the previous or next image. Each newly displayed image starts centered at the retained zoom level, and the viewer displays its one-based index and the total number of images in the active folder or recursive image queue.
- **FR-049A:** While a video is open and seekable, pressing Left Arrow seeks backward five seconds and pressing Right Arrow seeks forward five seconds. Seeking is clamped at the beginning and end of the video and works in both windowed and fullscreen modes.

### 5.6 File-type registration

- **FR-050:** Windows installation registers application identity, icons, display names, supported extensions, and open commands for the media types listed in Section 4.
- **FR-051:** Windows uninstallation removes registrations owned by VeyloPlayer without changing or deleting user files.
- **FR-052:** The macOS app bundle declares supported document types and roles for the media types listed in Section 4.
- **FR-053:** Each registered category has an appropriate VeyloPlayer file icon or a consistent generic VeyloPlayer media icon.
- **FR-054:** File paths containing spaces, Unicode characters, or shell-sensitive characters open correctly.

## 6. User experience requirements

- **UX-001:** The primary window contains an unobstructed media area and a unified bottom playback bar with discoverable controls for open, previous/next media, play/pause, seek, volume, audio track, subtitles, and fullscreen video.
- **UX-002:** Controls that do not apply to the current media type are hidden or disabled.
- **UX-003:** During audio or video playback, the playback bar appears immediately when pointer activity occurs anywhere in the window, including over the native video surface, and fades after five seconds without pointer activity. Opening media or changing playback state also reveals it. For video, the bar is a translucent overlay; showing, hiding, or animating it never changes the video viewport size or aspect ratio.
- **UX-004:** The title area shows the current filename.
- **UX-005:** The application remembers window size and position between launches, while ensuring the restored window remains visible on the current displays.
- **UX-006:** Standard platform shortcuts are provided, including Space for play/pause, Left/Right Arrow for five-second video seeking or image navigation, Up/Down panning for zoomed images, and a documented shortcut for opening a file.
- **UX-007:** User-facing errors are concise, actionable, and do not expose stack traces.
- **UX-008:** The visual design is modern, calm, and media-first, with a dark neutral playback surface, restrained use of color, consistent spacing, rounded controls, and high-quality vector icons.
- **UX-009:** The default window presents a clear empty state with a prominent **Open file** action and drag-and-drop guidance. It must not resemble a developer tool or expose codec terminology.
- **UX-010:** Frequently used controls are available in one click. Audio-track, subtitle, and less common actions are grouped in plainly labeled menus rather than crowding the primary control bar.
- **UX-011:** Icon-only controls have accessible names and tooltips. Ambiguous actions must include a text label.
- **UX-012:** The interface automatically follows the operating system's light or dark appearance and updates when the system appearance changes. VeyloPlayer does not expose a separate appearance setting.
- **UX-013:** Layouts adapt gracefully from the minimum supported window size through fullscreen without clipped labels, overlapping controls, or unusably small targets.
- **UX-014:** Primary pointer targets are at least 32 by 32 logical pixels, with at least 40 by 40 logical pixels preferred for transport controls.
- **UX-015:** Motion is subtle and functional. Routine transitions complete within approximately 100–200 ms, do not delay input, and are disabled or reduced when the operating system requests reduced motion.
- **UX-016:** The application uses native file dialogs, menus where appropriate, window behavior, and familiar Windows/macOS keyboard conventions while retaining a consistent VeyloPlayer identity.
- **UX-017:** Loading and buffering states use a quiet progress treatment that does not obscure the media. The application must never present a permanently spinning indicator without an error or recovery path.
- **UX-018:** Track menus display human-readable language and channel labels such as `English — Stereo`; raw track IDs or codec identifiers appear only in optional diagnostics.
- **UX-019:** Dragging a supported local file onto the window opens it using the same behavior as the in-app file picker. Dragging a readable folder starts the recursive behavior in FR-039.
- **UX-020:** A first-time user can open a file, play or pause it, adjust volume, enter fullscreen, choose tracks, and leave fullscreen without reading documentation.
- **UX-021:** VeyloPlayer uses a distinctive, high-resolution application icon in the Windows taskbar, Start menu, installer, and application switcher. The same brand mark is available to the macOS bundle and Dock packaging flow.
- **UX-022:** Play, pause, enter-fullscreen, and exit-fullscreen actions use crisp vector controls that remain visually distinct at normal and high-DPI sizes; tooltips and accessible names describe every action.
- **UX-023:** The application menu provides File, Playback, and Help actions with platform-appropriate keyboard shortcuts. Help > About VeyloPlayer displays the version, copyright, warranty notice, project license, third-party notices, and source link without requiring network access for the legal text.
- **UX-024:** File > Set as default player provides a direct, platform-native route to make VeyloPlayer the default for supported media types. Windows opens VeyloPlayer's registered Default Apps settings and preserves required user confirmation; macOS requests the associations through the system API and displays any consent UI required by the OS.
- **UX-025:** Settings provides a human-readable list of available audio output devices. Selecting a device applies it to playback and remembers it for future launches; the list can be refreshed after devices are connected or removed.

## 7. Installation and build requirements

### 7.1 Build scripts

- **BLD-001:** All release and packaging scripts live under `scripts/`.
- **BLD-002:** `scripts/build-windows.*` produces the Windows application and installer from a clean checkout on a supported Windows build host.
- **BLD-003:** `scripts/build-macos.*` produces the macOS application bundle and DMG from a clean checkout on a supported macOS build host.
- **BLD-004:** Platform scripts support a non-interactive mode suitable for continuous integration.
- **BLD-005:** Build output is written to a documented distribution directory and includes predictable versioned filenames.
- **BLD-006:** Signing credentials and secrets are supplied through the build environment or secure CI secret storage and are never committed to the repository.
- **BLD-007:** Scripts fail with a non-zero status and a useful message when required tooling, signing configuration, or packaging steps fail.
- **BLD-008:** A developer build may omit signing/notarization, but release mode must enforce them.

### 7.2 Windows installer

- **INS-001:** The installer supports install, upgrade, repair when supported by the packaging technology, and uninstall.
- **INS-002:** Installation creates a Start menu entry and registers VeyloPlayer in the installed-apps list.
- **INS-003:** The installer supports per-user installation without requiring administrator access where the selected framework permits it.
- **INS-004:** Upgrading preserves user preferences.
- **INS-005:** Uninstalling removes installed application files and registrations but preserves user media.
- **INS-006:** The installer displays VeyloPlayer's open-source license notice and does not impose terms that restrict rights granted by GPL, LGPL, or a bundled component's license.
- **INS-007:** The installed application and portable ZIP contain the complete project license, applicable GNU license texts, third-party copyright notices, Qt SBOM, VLC upstream notices, and an exact source offer.

### 7.3 macOS DMG

- **INS-010:** The DMG contains the VeyloPlayer app and a clear affordance for copying it to Applications.
- **INS-011:** The app bundle uses a stable bundle identifier and version metadata.
- **INS-012:** Release builds pass code-signature verification and Apple notarization checks before the DMG is published.
- **INS-013:** Updating or replacing the application preserves user preferences.
- **INS-014:** The app bundle contains the same license and source payload as the Windows distribution. The DMG exposes the project license, third-party notices, and source offer without requiring the app to be launched.

## 8. Non-functional requirements

- **NFR-001:** A normal launch on a reference development machine should show the main window within 3 seconds.
- **NFR-002:** Basic playback controls should respond within 100 ms when the media engine is not blocked on I/O.
- **NFR-003:** The application must remain responsive while opening, probing, or switching large media files.
- **NFR-004:** The player must not upload, analyze remotely, rename, move, or modify user media unless a future feature explicitly requests and explains that behavior.
- **NFR-005:** Usage telemetry is off by default unless a later privacy requirement defines informed consent, collected fields, retention, and opt-out behavior.
- **NFR-006:** The application must release file handles after media is closed so files can be renamed, moved, or deleted by the user.
- **NFR-007:** Accessibility labels, keyboard focus, readable contrast, and platform screen-reader support are required for primary controls.
- **NFR-008:** The application must handle sleep/wake, display changes, audio-device changes, and removal of the current file without crashing.
- **NFR-009:** The application must not require network access for local playback.
- **NFR-010:** Release packaging preserves the user's ability to replace dynamically linked Qt and LibVLC components and does not prohibit reverse engineering for debugging those modifications.
- **NFR-011:** No binary release is published while any packaged file has an unknown license, missing attribution, unavailable corresponding source, or unresolved distribution restriction.

## 9. Acceptance scenarios

1. **Natural continuation:** Given `1.mp4`, `2.mp4`, and `10.mp4` in one folder, opening `1.mp4` and allowing it to finish starts `2.mp4`; when that finishes, `10.mp4` starts.
2. **Mixed folder:** Given videos, audio, images, documents, and subfolders, automatic audio/video continuation considers only supported audio/video files in the same folder.
3. **Track selection:** Given a video with two audio tracks and two embedded subtitle tracks, the user can switch between all tracks, disable subtitles, and keep the same playback position.
4. **External subtitles:** Given a playing video and a valid supported external subtitle file, the user can load and display that subtitle without changing the video file.
5. **Image browsing:** Given `photo1.jpg`, `photo2.jpeg`, and `photo10.JPG`, Right Arrow moves through them in that order and Left Arrow moves backward.
6. **Association launch:** After installation and user confirmation of the default app, double-clicking a supported file in File Explorer or Finder opens it in VeyloPlayer.
7. **Special-character path:** A supported file in a Unicode path containing spaces and shell-sensitive characters opens and plays successfully.
8. **End of folder:** Finishing the last audio/video item stops playback without restarting the first item.
9. **Packaging:** A clean Windows build produces an installable signed package, and a clean macOS build produces a signed and notarized DMG using only documented prerequisites and scripts under `scripts/`.
10. **Recursive folder drop:** Given nested folders containing `1.mp4`, `Season2/episode2.mp3`, `Season2/episode10.mkv`, and `Season10/episode1.mp4`, dropping the root folder plays them in that natural relative-path order and stops after the final item.
11. **Recursive photo folder:** Given a folder tree containing JPEGs but no audio or video, dropping the root folder displays the first JPEG and Left/Right Arrow navigates all nested JPEGs in natural relative-path order without wrapping.
12. **Continue watching:** Given a video stopped at 12:34, closing and reopening VeyloPlayer without another file request presents a Continue watching action; choosing it opens the same video near 12:34.
13. **Fullscreen:** While video is open, the fullscreen button, `F`, `F11`, and video double-click enter fullscreen, while `Escape` and the exit-fullscreen button restore the normal window.
14. **Playback-bar activity:** With audio or video open, moving the pointer over any part of the media reveals the bottom playback bar; leaving the pointer inactive for five seconds hides it.
15. **Surface play/pause:** A single click on audio artwork or video toggles play/pause, while a video double-click enters fullscreen without causing an extra single-click toggle.
16. **Zoomed image navigation:** After zooming `photo1.jpg`, Right Arrow opens `photo2.jpg` at the same zoom level and centered position, the counter changes from `1 / 2` to `2 / 2`, and wheel zooming does not show a blank frame.
17. **Overlay transport:** While a middle video in a sequence is open, the translucent bar shows enabled Previous and Next controls. Revealing and hiding it does not resize the video, and either control opens the adjacent item in natural order.
18. **Video keyboard seeking:** During seekable video playback, Left Arrow moves playback backward five seconds and Right Arrow moves it forward five seconds, clamping at the start and end and behaving the same way in fullscreen.
19. **Folder track preferences:** After selecting the second audio track and an embedded subtitle in one video, opening another video from the same folder selects tracks with matching labels, or the same ordinal positions when labels differ. Choosing subtitles Off is also restored, while an external subtitle is not carried to another file.
20. **Chapter-aware transport:** Given a video with three embedded chapters followed by another media file, the timeline shows light-grey dividers at the second and third chapter starts. Repeated presses of Next visit those chapters before opening the next file; Previous follows the same priority in reverse.
21. **Compact playback bar:** At the minimum supported window width with audio and subtitle selectors visible, the complete seek timeline and every playback action remain visible and usable. Loading an external subtitle uses a compact icon with an accessible name and tooltip.
22. **Remembered window placement:** After moving and resizing the main window, closing and reopening VeyloPlayer restores its last normal position and size. Maximized or fullscreen state is also restored, and a saved position from a disconnected display is moved fully onto an available display.
23. **Open-source notices:** Installing the MSI shows the open-source notice, and inspecting either installed platform package reveals GPL/LGPL texts, third-party notices, an exact source offer, Qt SBOM information, and VLC's supplied notices.
24. **In-app legal notice:** Help > About VeyloPlayer opens an accessible dialog containing the complete GPL-3.0 license and bundled third-party notices, with selectable text and a source-code link.
25. **Default-player action:** Choosing File > Set as default player opens VeyloPlayer's Default Apps page on Windows or requests the supported media associations on macOS. The operating system retains control of consent, and the app reports whether the request could be started or completed.

## 10. Out of scope for the MVP

- Streaming URLs and online media services.
- DRM-protected media.
- Media-library indexing or metadata management.
- User-created playlists, shuffle, repeat, or timed image slideshows.
- Image editing, slideshow timing, or non-JPEG image formats.
- Video editing, transcoding, downloading, or screen capture.
- Mobile, Linux, browser, or television versions.
- Automatic software updates, unless separately specified before implementation.

## 11. Technical baseline and remaining decisions

The language, UI framework, playback engine, decoding strategy, image and subtitle handling, build system, and packaging toolchain are defined in [Technical Stack](technical-stack.md). The baseline is C++20, Qt Quick/QML, LibVLC 3, CMake, and platform-native signed packaging.

The following product and distribution decisions remain:

- Intel macOS support versus Apple-silicon-only support.
- Whether an additional portable Windows build is desired.
- Product identity: application icon, bundle identifier, publisher name, and signing identities.
- Whether image formats beyond JPEG, playlist features, repeat behavior, or playback-speed controls belong in the first release.
- Minimum supported external subtitle encodings and styling behavior.
