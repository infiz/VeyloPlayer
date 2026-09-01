pragma Singleton

import QtQuick
import Veylo.Core

QtObject {
    readonly property SystemPalette systemPalette: SystemPalette {
        colorGroup: SystemPalette.Active
    }
    readonly property bool dark: systemPalette.window.hslLightness < 0.5
    readonly property color window: dark ? "#0b0d10" : "#f4f5f7"
    readonly property color surface: dark ? "#14171c" : "#ffffff"
    readonly property color elevated: dark ? "#1c2027" : "#e9ebef"
    readonly property color canvas: "#050608"
    readonly property color text: dark ? "#f5f7fa" : "#17191d"
    readonly property color secondaryText: dark ? "#a9b0ba" : "#626975"
    readonly property color accent: "#7567ff"
    readonly property color accentHover: "#897dff"
    readonly property color danger: dark ? "#ff8b8b" : "#b42318"
    readonly property color focus: "#9e95ff"
    readonly property int radiusSmall: 8
    readonly property int radius: 12
    readonly property int radiusLarge: 18
    readonly property int spacing: 12
    readonly property int motionFast: 140
}
