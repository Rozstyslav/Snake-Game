import QtQuick 2.15

QtObject {
    id: t

    property bool dark: true

    readonly property color bg: dark ? "#000000" : "#ffffff"
    readonly property color fg: dark ? "#ffffff" : "#000000"

    readonly property color btnBg: dark ? "#2a2a2a" : "#e9e9e9"
    readonly property color btnBorder: dark ? "#3a3a3a" : "#bcbcbc"

    readonly property color fieldBorder: fg
}
