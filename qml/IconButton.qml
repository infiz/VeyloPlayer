import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Button {
    id: control

    property string glyph: ""
    property string symbol: ""
    property string accessibleName: text
    property bool toolTipEnabled: true
    property bool prominent: false
    readonly property color glyphColor: enabled
        ? (prominent ? "white" : Theme.text)
        : Theme.secondaryText

    implicitWidth: 42
    implicitHeight: 42
    padding: 0
    text: accessibleName
    Accessible.name: accessibleName

    ToolTip.visible: toolTipEnabled && hovered && accessibleName.length > 0
    ToolTip.text: accessibleName
    ToolTip.delay: 500

    contentItem: Item {
        Text {
            anchors.fill: parent
            visible: control.symbol.length === 0
            text: control.glyph
            color: control.glyphColor
            font.pixelSize: control.prominent ? 20 : 18
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Shape {
            anchors.centerIn: parent
            width: 20
            height: 22
            visible: control.symbol === "play"
            antialiasing: true
            ShapePath {
                fillColor: control.glyphColor
                strokeColor: "transparent"
                startX: 4
                startY: 2
                PathLine { x: 18; y: 11 }
                PathLine { x: 4; y: 20 }
                PathLine { x: 4; y: 2 }
            }
        }

        Item {
            anchors.centerIn: parent
            width: 18
            height: 20
            visible: control.symbol === "pause"
            Rectangle {
                x: 2
                width: 5
                height: parent.height
                radius: 2
                color: control.glyphColor
            }
            Rectangle {
                x: 11
                width: 5
                height: parent.height
                radius: 2
                color: control.glyphColor
            }
        }

        Shape {
            anchors.centerIn: parent
            width: 22
            height: 22
            visible: control.symbol === "previous"
            antialiasing: true
            ShapePath {
                fillColor: control.glyphColor
                strokeColor: "transparent"
                startX: 18
                startY: 3
                PathLine { x: 7; y: 11 }
                PathLine { x: 18; y: 19 }
                PathLine { x: 18; y: 3 }
            }
            ShapePath {
                fillColor: control.glyphColor
                strokeColor: "transparent"
                startX: 4
                startY: 3
                PathLine { x: 7; y: 3 }
                PathLine { x: 7; y: 19 }
                PathLine { x: 4; y: 19 }
                PathLine { x: 4; y: 3 }
            }
        }

        Shape {
            anchors.centerIn: parent
            width: 22
            height: 22
            visible: control.symbol === "next"
            antialiasing: true
            ShapePath {
                fillColor: control.glyphColor
                strokeColor: "transparent"
                startX: 4
                startY: 3
                PathLine { x: 15; y: 11 }
                PathLine { x: 4; y: 19 }
                PathLine { x: 4; y: 3 }
            }
            ShapePath {
                fillColor: control.glyphColor
                strokeColor: "transparent"
                startX: 15
                startY: 3
                PathLine { x: 18; y: 3 }
                PathLine { x: 18; y: 19 }
                PathLine { x: 15; y: 19 }
                PathLine { x: 15; y: 3 }
            }
        }

        Shape {
            anchors.centerIn: parent
            width: 22
            height: 22
            visible: control.symbol === "fullscreen"
            antialiasing: true
            ShapePath {
                fillColor: "transparent"
                strokeColor: control.glyphColor
                strokeWidth: 2.2
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                startX: 8; startY: 2
                PathLine { x: 2; y: 2 }
                PathLine { x: 2; y: 8 }
                PathMove { x: 14; y: 2 }
                PathLine { x: 20; y: 2 }
                PathLine { x: 20; y: 8 }
                PathMove { x: 2; y: 14 }
                PathLine { x: 2; y: 20 }
                PathLine { x: 8; y: 20 }
                PathMove { x: 20; y: 14 }
                PathLine { x: 20; y: 20 }
                PathLine { x: 14; y: 20 }
            }
        }

        Shape {
            anchors.centerIn: parent
            width: 22
            height: 22
            visible: control.symbol === "exitFullscreen"
            antialiasing: true
            ShapePath {
                fillColor: "transparent"
                strokeColor: control.glyphColor
                strokeWidth: 2.2
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                startX: 2; startY: 8
                PathLine { x: 8; y: 8 }
                PathLine { x: 8; y: 2 }
                PathMove { x: 20; y: 8 }
                PathLine { x: 14; y: 8 }
                PathLine { x: 14; y: 2 }
                PathMove { x: 2; y: 14 }
                PathLine { x: 8; y: 14 }
                PathLine { x: 8; y: 20 }
                PathMove { x: 20; y: 14 }
                PathLine { x: 14; y: 14 }
                PathLine { x: 14; y: 20 }
            }
        }
    }

    background: Rectangle {
        radius: width / 2
        color: {
            if (!control.enabled) return "transparent"
            if (control.prominent) return control.hovered ? Theme.accentHover : Theme.accent
            return control.hovered || control.visualFocus ? Theme.elevated : "transparent"
        }
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focus

        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }
}
