import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Veylo.Core

Item {
    id: bar

    property bool fullScreen: false
    signal filesRequested()
    signal folderRequested()
    signal subtitleRequested()
    signal fullscreenRequested()

    function formatTime(milliseconds) {
        const seconds = Math.max(0, Math.floor(milliseconds / 1000))
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        const remaining = seconds % 60
        if (hours > 0)
            return hours + ":" + String(minutes).padStart(2, "0") + ":" + String(remaining).padStart(2, "0")
        return minutes + ":" + String(remaining).padStart(2, "0")
    }

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

            Text {
                text: bar.formatTime(seekSlider.pressed ? seekSlider.value : Player.position)
                color: Theme.secondaryText
                font.pixelSize: 12
            }
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
            Text {
                text: bar.formatTime(Player.duration)
                color: Theme.secondaryText
                font.pixelSize: 12
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AddMediaButton {
                onFilesRequested: bar.filesRequested()
                onFolderRequested: bar.folderRequested()
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
                onClicked: bar.subtitleRequested()
            }
            IconButton {
                visible: Player.isVideo
                symbol: bar.fullScreen ? "exitFullscreen" : "fullscreen"
                accessibleName: bar.fullScreen ? "Exit fullscreen" : "Fullscreen"
                onClicked: bar.fullscreenRequested()
            }
        }
    }
}
