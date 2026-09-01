import QtQuick
import QtQuick.Controls.Basic

IconButton {
    id: button

    signal filesRequested()
    signal folderRequested()

    glyph: "+"
    accessibleName: "Add media"
    toolTipEnabled: false
    onClicked: addMediaMenu.open()

    Menu {
        id: addMediaMenu
        y: -implicitHeight

        MenuItem {
            text: "Add file(s)…"
            onTriggered: button.filesRequested()
        }
        MenuItem {
            text: "Add folder…"
            onTriggered: button.folderRequested()
        }
    }
}
