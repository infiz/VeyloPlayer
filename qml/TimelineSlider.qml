import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: timeline

    property var chapterPositions: []

    Repeater {
        parent: timeline.background
        model: timeline.chapterPositions

        delegate: Rectangle {
            readonly property real chapterPosition: Number(modelData)

            visible: timeline.to > timeline.from
                && chapterPosition > timeline.from
                && chapterPosition < timeline.to
            x: Math.round(((chapterPosition - timeline.from)
                / (timeline.to - timeline.from)) * timeline.background.width - width / 2)
            y: Math.round((timeline.background.height - height) / 2)
            width: 2
            height: 10
            radius: 1
            color: "#c1c5cc"
            opacity: timeline.enabled ? 0.9 : 0.55
            z: 2
        }
    }
}
