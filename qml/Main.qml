import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Veylo.Core

ApplicationWindow {
    id: root

    width: 1120
    height: 720
    minimumWidth: 720
    minimumHeight: 480
    visible: false
    color: Theme.window
    readonly property string applicationTitle: "VeyloPlayer " + Qt.application.version
    readonly property bool compactControls: width < 820
    title: Player.title.length > 0 ? Player.title + " — " + applicationTitle : applicationTitle

    property bool controlsVisible: true
    property bool fullScreen: visibility === Window.FullScreen
    property real imageZoom: 1.0
    property real imagePanX: 0.0
    property real imagePanY: 0.0
    readonly property bool imageIsZoomed: imageZoom > 1.001

    component AboutTabButton: TabButton {
        id: aboutTabButton
        implicitHeight: 38

        contentItem: Text {
            text: aboutTabButton.text
            color: aboutTabButton.checked ? "white" : Theme.text
            font.weight: aboutTabButton.checked ? Font.DemiBold : Font.Normal
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: Theme.radiusSmall
            color: aboutTabButton.checked
                ? Theme.accent
                : aboutTabButton.hovered ? Theme.elevated : "transparent"
        }
    }

    component AboutTextArea: TextArea {
        readOnly: true
        selectByMouse: true
        wrapMode: TextEdit.Wrap
        color: Theme.text
        selectionColor: Theme.accent
        selectedTextColor: "white"
        padding: 14

        background: Rectangle {
            color: Theme.window
            radius: Theme.radiusSmall
        }
    }

    component AppMenu: Menu {
        id: appMenu
        popupType: Popup.Window
        topPadding: 4
        bottomPadding: 4

        palette.window: Theme.surface
        palette.windowText: Theme.text
        palette.mid: Theme.dark ? "#3a404a" : "#c8ccd2"

        background: Rectangle {
            implicitWidth: 220
            implicitHeight: 32
            color: Theme.surface
            border.width: 1
            border.color: Theme.dark ? "#3a404a" : "#c8ccd2"
            radius: 6
        }
    }

    component AppMenuItem: MenuItem {
        id: appMenuItem
        implicitHeight: 30
        leftPadding: 10
        rightPadding: 10
        topPadding: 4
        bottomPadding: 4
        readonly property string shortcutText: action && action.shortcut
            ? action.shortcut.toString()
            : ""

        contentItem: RowLayout {
            spacing: 20

            Text {
                Layout.fillWidth: true
                text: appMenuItem.text
                color: appMenuItem.enabled ? Theme.text : Theme.secondaryText
                font: appMenuItem.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                visible: appMenuItem.shortcutText.length > 0
                text: appMenuItem.shortcutText
                color: Theme.secondaryText
                font: appMenuItem.font
                verticalAlignment: Text.AlignVCenter
            }
        }

        background: Rectangle {
            implicitWidth: 220
            implicitHeight: 30
            color: appMenuItem.highlighted
                ? (Theme.dark ? "#303641" : "#e4e7ec")
                : "transparent"
            radius: 4
        }
    }

    component AppMenuSeparator: MenuSeparator {
        implicitHeight: 9
        topPadding: 4
        bottomPadding: 4

        contentItem: Rectangle {
            implicitWidth: 200
            implicitHeight: 1
            color: Theme.dark ? "#3a404a" : "#c8ccd2"
        }
    }

    Action {
        id: openFilesAction
        text: "Open files…"
        shortcut: StandardKey.Open
        onTriggered: openDialog.open()
    }

    Action {
        id: openFolderAction
        text: "Open folder…"
        shortcut: "Ctrl+Shift+O"
        onTriggered: folderDialog.open()
    }

    Action {
        id: exitAction
        text: "Exit"
        shortcut: StandardKey.Quit
        onTriggered: Qt.quit()
    }

    Action {
        id: settingsAction
        text: "Settings…"
        shortcut: "Ctrl+,"
        onTriggered: Qt.callLater(root.showSettingsWindow)
    }

    Action {
        id: playPauseAction
        text: Player.playing ? "Pause" : "Play"
        shortcut: "Space"
        enabled: Player.isAudio || Player.isVideo
        onTriggered: Player.playPause()
    }

    Action {
        id: fullScreenAction
        text: root.fullScreen ? "Exit fullscreen" : "Enter fullscreen"
        shortcut: "F11"
        enabled: Player.isVideo
        onTriggered: root.toggleFullScreen()
    }

    Action {
        id: aboutAction
        text: "About VeyloPlayer"
        onTriggered: Qt.callLater(root.showAboutWindow)
    }

    menuBar: MenuBar {
        implicitHeight: 30

        delegate: MenuBarItem {
            implicitHeight: 30
            topPadding: 4
            bottomPadding: 4
        }

        AppMenu {
            title: "&File"

            AppMenuItem { action: openFilesAction }
            AppMenuItem { action: openFolderAction }
            AppMenuItem {
                text: "Load subtitle file…"
                enabled: Player.isVideo
                onTriggered: subtitleDialog.open()
            }
            AppMenuSeparator {}
            AppMenuItem {
                text: "Set as default player…"
                enabled: !SystemIntegration.defaultPlayerRequestInProgress
                onTriggered: SystemIntegration.requestDefaultPlayer()
            }
            AppMenuSeparator {}
            AppMenuItem { action: settingsAction }
            AppMenuSeparator {}
            AppMenuItem { action: exitAction }
        }

        AppMenu {
            title: "&Playback"

            AppMenuItem { action: playPauseAction }
            AppMenuItem {
                text: "Previous"
                enabled: Player.isImage ? Player.canPreviousImage : Player.canPreviousMedia
                onTriggered: Player.isImage ? Player.previousImage() : Player.previousMedia()
            }
            AppMenuItem {
                text: "Next"
                enabled: Player.isImage ? Player.canNextImage : Player.canNextMedia
                onTriggered: Player.isImage ? Player.nextImage() : Player.nextMedia()
            }
            AppMenuSeparator {}
            AppMenuItem { action: fullScreenAction }
        }

        AppMenu {
            title: "&Help"

            AppMenuItem { action: aboutAction }
        }
    }

    function formatTime(milliseconds) {
        const seconds = Math.max(0, Math.floor(milliseconds / 1000))
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        const remaining = seconds % 60
        if (hours > 0)
            return hours + ":" + String(minutes).padStart(2, "0") + ":" + String(remaining).padStart(2, "0")
        return minutes + ":" + String(remaining).padStart(2, "0")
    }

    function showAboutWindow() {
        aboutWindow.show()
        aboutWindow.raise()
        aboutWindow.requestActivate()
    }

    function showSettingsWindow() {
        Player.refreshAudioOutputDevices()
        settingsWindow.show()
        settingsWindow.raise()
        settingsWindow.requestActivate()
    }

    function showControls() {
        controlsVisible = true
        if (Player.isAudio || Player.isVideo)
            hideControlsTimer.restart()
        else
            hideControlsTimer.stop()
    }

    function toggleFullScreen() {
        if (fullScreen)
            showNormal()
        else
            showFullScreen()
        showControls()
    }

    function clampImagePan() {
        if (!Player.isImage || imageZoom <= 1.001) {
            imagePanX = 0
            imagePanY = 0
            return
        }

        const maximumX = Math.max(0, (currentImage.paintedWidth * imageZoom - imageViewport.width) / 2)
        const maximumY = Math.max(0, (currentImage.paintedHeight * imageZoom - imageViewport.height) / 2)
        imagePanX = Math.max(-maximumX, Math.min(maximumX, imagePanX))
        imagePanY = Math.max(-maximumY, Math.min(maximumY, imagePanY))
    }

    function resetImageTransform() {
        imageZoom = 1.0
        imagePanX = 0.0
        imagePanY = 0.0
    }

    function zoomImageAt(pointerX, pointerY, factor) {
        if (!Player.isImage || factor <= 0)
            return

        const oldZoom = imageZoom
        const newZoom = Math.max(1.0, Math.min(8.0, oldZoom * factor))
        if (Math.abs(newZoom - oldZoom) < 0.001)
            return

        const centerX = imageViewport.width / 2
        const centerY = imageViewport.height / 2
        const ratio = newZoom / oldZoom
        imagePanX += (1 - ratio) * (pointerX - centerX - imagePanX)
        imagePanY += (1 - ratio) * (pointerY - centerY - imagePanY)
        imageZoom = newZoom
        clampImagePan()
    }

    function panImage(deltaX, deltaY) {
        if (!Player.isImage || !imageIsZoomed)
            return
        imagePanX += deltaX
        imagePanY += deltaY
        clampImagePan()
    }

    Timer {
        id: hideControlsTimer
        interval: 5000
        onTriggered: if (Player.isAudio || Player.isVideo) root.controlsVisible = false
    }

    Connections {
        target: Player
        function onPlayingChanged() { root.showControls() }
        function onCurrentMediaChanged() {
            if (Player.isImage) {
                root.imagePanX = 0
                root.imagePanY = 0
                Qt.callLater(root.clampImagePan)
            } else {
                root.resetImageTransform()
            }
            root.showControls()
        }
        function onFullscreenToggleRequested() { root.toggleFullScreen() }
        function onMediaSurfaceActivity() { root.showControls() }
        function onMediaSurfaceClicked() {
            if (Player.isVideo) {
                Player.playPause()
                root.showControls()
            }
        }
    }

    FileDialog {
        id: openDialog
        title: "Add media files"
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            "All supported media (*.mp3 *.m4a *.aac *.wav *.flac *.ogg *.mp4 *.m4v *.mov *.mkv *.webm *.avi *.jpg *.jpeg)",
            "Video files (*.mp4 *.m4v *.mov *.mkv *.webm *.avi)",
            "Audio files (*.mp3 *.m4a *.aac *.wav *.flac *.ogg)",
            "JPEG images (*.jpg *.jpeg)"
        ]
        onAccepted: Player.openUrls(selectedFiles)
    }

    ApplicationWindow {
        id: aboutWindow
        transientParent: root
        flags: Qt.Dialog
        modality: Qt.WindowModal
        visible: false
        width: Math.max(560, Math.min(760, root.width - 40))
        height: Math.max(420, Math.min(620, root.height - 60))
        x: root.x + Math.round((root.width - width) / 2)
        y: root.y + Math.round((root.height - height) / 2)
        title: "About VeyloPlayer"
        color: Theme.surface
        palette.window: Theme.surface
        palette.windowText: Theme.text
        palette.base: Theme.window
        palette.text: Theme.text
        palette.button: Theme.elevated
        palette.buttonText: Theme.text
        palette.highlight: Theme.accent
        palette.highlightedText: "white"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Image {
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 54
                    source: "qrc:/branding/veylo-player.png"
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: root.applicationTitle
                        color: Theme.text
                        font.pixelSize: 21
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: "Copyright © 2026 VeyloPlayer contributors"
                        color: Theme.secondaryText
                    }
                    Label {
                        text: "Free software licensed under GPL-3.0-or-later"
                        color: Theme.secondaryText
                    }
                }
            }

            TabBar {
                id: aboutTabs
                Layout.fillWidth: true
                spacing: 6
                background: Rectangle { color: "transparent" }
                AboutTabButton { text: "About" }
                AboutTabButton { text: "License" }
                AboutTabButton { text: "Third-party notices" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: aboutTabs.currentIndex

                ScrollView {
                    clip: true
                    AboutTextArea {
                        textFormat: TextEdit.RichText
                        text: "<p><b>VeyloPlayer</b> is a modern open-source player for local video, audio, and JPEG files.</p>"
                            + "<p>This program comes with absolutely no warranty. You may redistribute and modify it under GPL-3.0-or-later.</p>"
                            + "<p>Qt, VLC/LibVLC, FFmpeg, and other bundled software remain under their own licenses. Open-source licenses do not necessarily grant codec patent, trademark, decryption, or media-content rights.</p>"
                            + "<p><a style=\"color:" + Theme.accent + "\" href=\"https://github.com/infiz/VeyloPlayer\">View source code and release information</a></p>"
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }
                }

                ScrollView {
                    clip: true
                    AboutTextArea {
                        font.family: "monospace"
                        font.pixelSize: 12
                        text: Legal.licenseText
                    }
                }

                ScrollView {
                    clip: true
                    AboutTextArea {
                        font.pixelSize: 12
                        text: Legal.thirdPartyNoticesText
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Item { Layout.fillWidth: true }

                Button {
                    text: "Close"
                    onClicked: aboutWindow.close()
                }
            }
        }
    }

    ApplicationWindow {
        id: settingsWindow
        transientParent: root
        flags: Qt.Dialog
        modality: Qt.WindowModal
        visible: false
        width: Math.max(460, Math.min(580, root.width - 40))
        height: 250
        x: root.x + Math.round((root.width - width) / 2)
        y: root.y + Math.round((root.height - height) / 2)
        title: "Settings"
        color: Theme.surface
        palette.window: Theme.surface
        palette.windowText: Theme.text
        palette.base: Theme.window
        palette.text: Theme.text
        palette.button: Theme.elevated
        palette.buttonText: Theme.text
        palette.highlight: Theme.accent
        palette.highlightedText: "white"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Label {
                text: "Audio output"
                color: Theme.text
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }
            Label {
                Layout.fillWidth: true
                text: "Choose where VeyloPlayer plays sound. Your choice is remembered."
                color: Theme.secondaryText
                wrapMode: Text.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ComboBox {
                    id: audioOutputDevices
                    Layout.fillWidth: true
                    model: Player.audioOutputDevices
                    textRole: "label"
                    valueRole: "key"
                    enabled: count > 0
                    currentIndex: {
                        Player.audioOutputDevices.length
                        return indexOfValue(Player.selectedAudioOutputDeviceKey)
                    }
                    Accessible.name: "Audio output device: " + currentText
                    onActivated: Player.selectAudioOutputDevice(currentValue)
                }
                Button {
                    text: "Refresh"
                    Accessible.name: "Refresh audio output devices"
                    onClicked: Player.refreshAudioOutputDevices()
                }
            }
            Label {
                Layout.fillWidth: true
                visible: Player.audioOutputDevices.length === 0
                text: "No audio output devices were found. Connect a device and choose Refresh."
                color: Theme.secondaryText
                wrapMode: Text.Wrap
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    text: "Close"
                    onClicked: settingsWindow.close()
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Add a media folder"
        onAccepted: Player.openUrl(selectedFolder)
    }

    FileDialog {
        id: subtitleDialog
        title: "Load external subtitles"
        nameFilters: ["Subtitle files (*.srt *.vtt *.ass *.ssa)"]
        onAccepted: Player.addExternalSubtitle(selectedFile)
    }

    MessageDialog {
        id: defaultPlayerResultDialog
        title: "Default player"
        buttons: MessageDialog.Ok
    }

    Connections {
        target: SystemIntegration
        function onDefaultPlayerRequestFinished(success, message) {
            defaultPlayerResultDialog.title = success
                ? "Default player"
                : "Could not change default player"
            defaultPlayerResultDialog.text = message
            defaultPlayerResultDialog.open()
        }
    }

    Shortcut {
        sequence: "Left"
        enabled: Player.isImage
        onActivated: Player.previousImage()
    }
    Shortcut {
        sequence: "Right"
        enabled: Player.isImage
        onActivated: Player.nextImage()
    }
    Shortcut {
        sequence: "Left"
        enabled: Player.isVideo && Player.seekable
        onActivated: Player.seek(Player.position - 5000)
    }
    Shortcut {
        sequence: "Right"
        enabled: Player.isVideo && Player.seekable
        onActivated: Player.seek(Player.position + 5000)
    }
    Shortcut { sequence: "Up"; enabled: Player.isImage && root.imageIsZoomed; onActivated: root.panImage(0, -48) }
    Shortcut { sequence: "Down"; enabled: Player.isImage && root.imageIsZoomed; onActivated: root.panImage(0, 48) }
    Shortcut { sequence: "F"; enabled: Player.isVideo; onActivated: root.toggleFullScreen() }
    Shortcut { sequence: "Escape"; enabled: root.fullScreen; onActivated: root.toggleFullScreen() }

    DropArea {
        anchors.fill: parent
        onEntered: function(drag) {
            if (drag.hasUrls) drag.acceptProposedAction()
        }
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                Player.openUrls(drop.urls)
                drop.acceptProposedAction()
            }
        }
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onPointChanged: root.showControls()
    }

    Item {
        anchors.fill: parent

        Rectangle {
            id: mediaArea
            anchors.fill: parent
            color: Theme.canvas

            TapHandler {
                enabled: Player.isAudio
                acceptedButtons: Qt.LeftButton
                onTapped: {
                    Player.playPause()
                    root.showControls()
                }
            }

            WindowContainer {
                id: videoContainer
                anchors.fill: parent
                window: Player.videoWindow
                visible: Player.isVideo
            }

            Item {
                id: imageViewport
                anchors.fill: parent
                anchors.margins: 16
                visible: Player.isImage
                clip: true

                onWidthChanged: Qt.callLater(root.clampImagePan)
                onHeightChanged: Qt.callLater(root.clampImagePan)

                Image {
                    id: currentImage
                    width: imageViewport.width
                    height: imageViewport.height
                    x: root.imagePanX
                    y: root.imagePanY
                    scale: root.imageZoom
                    transformOrigin: Item.Center
                    source: Player.imageSource
                    sourceSize.width: 4096
                    sourceSize.height: 4096
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    retainWhileLoading: true
                    cache: false
                    smooth: true
                    mipmap: true

                    onPaintedWidthChanged: Qt.callLater(root.clampImagePan)
                    onPaintedHeightChanged: Qt.callLater(root.clampImagePan)
                }

                MouseArea {
                    id: imagePointerArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    cursorShape: pressed && root.imageIsZoomed ? Qt.ClosedHandCursor
                        : root.imageIsZoomed ? Qt.OpenHandCursor
                        : Qt.ArrowCursor

                    property real previousX: 0
                    property real previousY: 0

                    onPressed: function(mouse) {
                        previousX = mouse.x
                        previousY = mouse.y
                    }
                    onDoubleClicked: function(mouse) {
                        root.resetImageTransform()
                        mouse.accepted = true
                    }
                    onPositionChanged: function(mouse) {
                        if (!pressed || !root.imageIsZoomed)
                            return
                        root.panImage(mouse.x - previousX, mouse.y - previousY)
                        previousX = mouse.x
                        previousY = mouse.y
                    }
                    onWheel: function(wheel) {
                        let steps = wheel.angleDelta.y / 120
                        if (steps === 0 && wheel.pixelDelta.y !== 0)
                            steps = wheel.pixelDelta.y / 120
                        if (steps !== 0)
                            root.zoomImageAt(wheel.x, wheel.y, Math.pow(1.15, steps))
                        wheel.accepted = true
                    }
                }
            }

            Button {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 20
                visible: Player.isImage && root.imageIsZoomed
                z: 3
                text: Math.round(root.imageZoom * 100) + "%  Reset"
                Accessible.name: "Reset image zoom"
                ToolTip.visible: hovered
                ToolTip.text: "Return image to fit"
                onClicked: root.resetImageTransform()

                contentItem: Text {
                    text: parent.text
                    color: "#f5f7fa"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 108
                    implicitHeight: 38
                    radius: 19
                    color: parent.hovered ? "#c01a1e25" : "#991a1e25"
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 20
                width: imageCounter.implicitWidth + 30
                height: 38
                radius: 19
                color: "#991a1e25"
                visible: Player.isImage && Player.imageCount > 0
                z: 3

                Text {
                    id: imageCounter
                    anchors.centerIn: parent
                    text: Player.imageIndex + " / " + Player.imageCount
                    color: "#f5f7fa"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 18
                visible: Player.isAudio

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 132
                    height: 132
                    radius: 38
                    color: Theme.elevated
                    Text {
                        anchors.centerIn: parent
                        text: "♫"
                        color: Theme.accent
                        font.pixelSize: 62
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(mediaArea.width - 80, 620)
                    text: Player.title
                    color: "#f5f7fa"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 18
                visible: !Player.hasMedia

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "V"
                    color: Theme.accent
                    font.pixelSize: 72
                    font.weight: Font.Bold
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Player.resumeAvailable ? "Continue watching" : "Play something you love"
                    color: "#f5f7fa"
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(mediaArea.width - 80, 600)
                    visible: Player.resumeAvailable
                    text: Player.resumeTitle
                    color: "#f5f7fa"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Player.resumeAvailable
                        ? "Pick up where you stopped last time."
                        : "Open a video, song, or JPEG — or drop a file or folder here."
                    color: "#a9b0ba"
                    font.pixelSize: 14
                }
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Player.resumeAvailable
                        ? "Resume at " + root.formatTime(Player.resumePosition)
                        : "Open file"
                    onClicked: {
                        if (Player.resumeAvailable)
                            Player.resumeLastVideo()
                        else
                            openDialog.open()
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: Player.resumeAvailable ? 180 : 132
                        implicitHeight: 44
                        radius: 22
                        color: parent.hovered ? Theme.accentHover : Theme.accent
                    }
                }
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: Player.resumeAvailable
                    text: "Open another file"
                    onClicked: openDialog.open()
                }
            }

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 20
                visible: Player.isImage && root.controlsVisible

                IconButton {
                    glyph: "‹"
                    accessibleName: "Previous image"
                    enabled: Player.canPreviousImage
                    onClicked: Player.previousImage()
                    background: Rectangle { radius: 21; color: "#990f1115" }
                }
                Item { Layout.fillWidth: true }
                IconButton {
                    glyph: "›"
                    accessibleName: "Next image"
                    enabled: Player.canNextImage
                    onClicked: Player.nextImage()
                    background: Rectangle { radius: 21; color: "#990f1115" }
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: Player.loading
                visible: running
                palette.accent: Theme.accent
            }
        }

        Rectangle {
            id: controlBar
            visible: !Player.isVideo
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: Player.isVideo ? 16 : 0
            anchors.rightMargin: Player.isVideo ? 16 : 0
            anchors.bottomMargin: Player.isVideo ? 16 : 0
            height: visible && root.controlsVisible
                ? ((Player.isAudio || Player.isVideo) ? 118 : 60)
                : 0
            radius: Player.isVideo ? 18 : 0
            color: Player.isVideo
                ? (Theme.dark ? "#dc111419" : "#e8ffffff")
                : Theme.surface
            border.width: Player.isVideo ? 1 : 0
            border.color: Theme.dark ? "#38ffffff" : "#26000000"
            opacity: root.controlsVisible ? 1 : 0
            clip: true
            z: 50

            Behavior on height { NumberAnimation { duration: Theme.motionFast } }
            Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 8
                anchors.bottomMargin: 10
                spacing: 7

                RowLayout {
                    Layout.fillWidth: true
                    visible: Player.isAudio || Player.isVideo
                    spacing: root.compactControls ? 6 : 10

                    Text { text: root.formatTime(seekSlider.pressed ? seekSlider.value : Player.position); color: Theme.secondaryText; font.pixelSize: 12 }
                    TimelineSlider {
                        id: seekSlider
                        Layout.fillWidth: true
                        Layout.minimumWidth: 80
                        from: 0
                        to: Math.max(1, Player.duration)
                        chapterPositions: Player.isVideo ? Player.chapterPositions : []
                        enabled: Player.seekable
                        onMoved: Player.seek(value)
                        Binding on value { value: Player.position; when: !seekSlider.pressed }
                    }
                    Text { text: root.formatTime(Player.duration); color: Theme.secondaryText; font.pixelSize: 12 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.compactControls ? 4 : 8

                    AddMediaButton {
                        onFilesRequested: openDialog.open()
                        onFolderRequested: folderDialog.open()
                    }
                    IconButton {
                        visible: Player.isAudio || Player.isVideo
                        symbol: "previous"
                        accessibleName: "Previous chapter or media"
                        enabled: Player.canPreviousMedia
                        onClicked: Player.previousMedia()
                    }
                    IconButton {
                        visible: Player.isAudio || Player.isVideo
                        symbol: Player.playing ? "pause" : "play"
                        accessibleName: Player.playing ? "Pause" : "Play"
                        prominent: true
                        onClicked: Player.playPause()
                    }
                    IconButton {
                        visible: Player.isAudio || Player.isVideo
                        symbol: "next"
                        accessibleName: "Next chapter or media"
                        enabled: Player.canNextMedia
                        onClicked: Player.nextMedia()
                    }
                    IconButton {
                        visible: Player.isAudio || Player.isVideo
                        glyph: Player.muted ? "×" : "●"
                        accessibleName: Player.muted ? "Unmute" : "Mute"
                        onClicked: Player.toggleMute()
                    }
                    Slider {
                        visible: Player.isAudio || Player.isVideo
                        Layout.minimumWidth: 64
                        Layout.preferredWidth: root.compactControls ? 76 : 110
                        from: 0
                        to: 100
                        value: Player.volume
                        onMoved: Player.volume = value
                        Accessible.name: "Volume"
                    }

                    Item { Layout.fillWidth: true }

                    ComboBox {
                        id: audioTracks
                        Layout.minimumWidth: root.compactControls ? 92 : 110
                        Layout.preferredWidth: root.compactControls ? 120 : 190
                        Layout.maximumWidth: 260
                        visible: Player.audioTracks.length > 1
                        model: Player.audioTracks
                        textRole: "label"
                        valueRole: "id"
                        currentIndex: {
                            Player.audioTracks.length
                            return indexOfValue(Player.activeAudioTrack)
                        }
                        Accessible.name: "Audio track: " + currentText
                        ToolTip.visible: hovered && currentText.length > 0
                        ToolTip.text: currentText
                        onActivated: Player.selectAudioTrack(currentValue)
                        Component.onCompleted: Player.refreshTracks()
                    }
                    ComboBox {
                        id: subtitleTracks
                        Layout.minimumWidth: root.compactControls ? 92 : 110
                        Layout.preferredWidth: root.compactControls ? 120 : 190
                        Layout.maximumWidth: 260
                        visible: Player.isVideo
                        model: Player.subtitleTracks
                        textRole: "label"
                        valueRole: "id"
                        currentIndex: {
                            Player.subtitleTracks.length
                            return indexOfValue(Player.activeSubtitleTrack)
                        }
                        Accessible.name: "Subtitle track: " + currentText
                        ToolTip.visible: hovered && currentText.length > 0
                        ToolTip.text: currentText
                        onActivated: Player.selectSubtitleTrack(currentValue)
                    }
                    IconButton {
                        visible: Player.isVideo
                        symbol: "subtitleFile"
                        accessibleName: "Load subtitle file"
                        onClicked: subtitleDialog.open()
                    }
                    IconButton {
                        visible: Player.isVideo
                        symbol: root.fullScreen ? "exitFullscreen" : "fullscreen"
                        accessibleName: root.fullScreen ? "Exit fullscreen" : "Fullscreen"
                        onClicked: root.toggleFullScreen()
                    }
                }
            }
        }
    }

    Window {
        id: videoControlOverlay

        transientParent: root
        screen: root.screen
        flags: Qt.FramelessWindowHint | Qt.Tool | Qt.NoDropShadowWindowHint
        color: "transparent"
        width: Math.max(1, mediaArea.width - 32)
        height: 118
        visible: root.visible
            && root.visibility !== Window.Minimized
            && Player.isVideo
            && (root.controlsVisible || opacity > 0.01)
        opacity: root.controlsVisible ? 1 : 0

        x: root.x + 16
        y: root.y + root.height - height - 16

        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }

        Rectangle {
            anchors.fill: parent
            radius: 18
            color: Theme.dark ? "#dc111419" : "#e8ffffff"
            border.width: 1
            border.color: Theme.dark ? "#38ffffff" : "#26000000"
        }

        PlaybackBar {
            anchors.fill: parent
            fullScreen: root.fullScreen
            onFilesRequested: openDialog.open()
            onFolderRequested: folderDialog.open()
            onSubtitleRequested: subtitleDialog.open()
            onFullscreenRequested: root.toggleFullScreen()
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        height: errorRow.implicitHeight + 22
        radius: Theme.radius
        color: Theme.dark ? "#402126" : "#fff0f0"
        border.color: Theme.danger
        visible: Player.errorMessage.length > 0
        z: 100

        RowLayout {
            id: errorRow
            anchors.fill: parent
            anchors.margins: 11
            Text {
                text: Player.errorMessage
                color: Theme.text
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            Button { text: "Dismiss"; onClicked: Player.clearError() }
        }
    }
}
