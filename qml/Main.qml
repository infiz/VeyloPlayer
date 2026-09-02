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
    visible: true
    color: Theme.window
    readonly property string applicationTitle: "VeyloPlayer " + Qt.application.version
    title: Player.title.length > 0 ? Player.title + " — " + applicationTitle : applicationTitle

    property bool controlsVisible: true
    property bool fullScreen: visibility === Window.FullScreen
    property real imageZoom: 1.0
    property real imagePanX: 0.0
    property real imagePanY: 0.0
    readonly property bool imageIsZoomed: imageZoom > 1.001

    function formatTime(milliseconds) {
        const seconds = Math.max(0, Math.floor(milliseconds / 1000))
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        const remaining = seconds % 60
        if (hours > 0)
            return hours + ":" + String(minutes).padStart(2, "0") + ":" + String(remaining).padStart(2, "0")
        return minutes + ":" + String(remaining).padStart(2, "0")
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

    Shortcut { sequences: [StandardKey.Open]; onActivated: openDialog.open() }
    Shortcut { sequence: "Space"; enabled: Player.isAudio || Player.isVideo; onActivated: Player.playPause() }
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
    Shortcut { sequence: "F11"; enabled: Player.isVideo; onActivated: root.toggleFullScreen() }
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
                    spacing: 10

                    Text { text: root.formatTime(seekSlider.pressed ? seekSlider.value : Player.position); color: Theme.secondaryText; font.pixelSize: 12 }
                    TimelineSlider {
                        id: seekSlider
                        Layout.fillWidth: true
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
                    spacing: 8

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
                        Layout.preferredWidth: 110
                        from: 0
                        to: 100
                        value: Player.volume
                        onMoved: Player.volume = value
                        Accessible.name: "Volume"
                    }

                    Item { Layout.fillWidth: true }

                    ComboBox {
                        id: audioTracks
                        Layout.maximumWidth: 190
                        visible: Player.audioTracks.length > 1
                        model: Player.audioTracks
                        textRole: "label"
                        valueRole: "id"
                        currentIndex: {
                            Player.audioTracks.length
                            return indexOfValue(Player.activeAudioTrack)
                        }
                        Accessible.name: "Audio track"
                        onActivated: Player.selectAudioTrack(currentValue)
                        Component.onCompleted: Player.refreshTracks()
                    }
                    ComboBox {
                        id: subtitleTracks
                        Layout.maximumWidth: 190
                        visible: Player.isVideo
                        model: Player.subtitleTracks
                        textRole: "label"
                        valueRole: "id"
                        currentIndex: {
                            Player.subtitleTracks.length
                            return indexOfValue(Player.activeSubtitleTrack)
                        }
                        Accessible.name: "Subtitle track"
                        onActivated: Player.selectSubtitleTrack(currentValue)
                    }
                    Button {
                        visible: Player.isVideo
                        text: "Load subtitles"
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
