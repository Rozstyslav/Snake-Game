import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    width: 300
    height: 400

    implicitWidth: 300
    implicitHeight: 420

    property var theme

    color: theme ? theme.bg : "#000000"

    property alias startButton: startBtn
    property alias settingsButton: settingsBtn
    property alias skinButton: skinBtn
    property alias exitButton: exitBtn

    readonly property bool stmOk: (backend && backend.stmConnected)

    readonly property real disabledOpacity: 0.45

    readonly property int pad: 22
    readonly property int btnH: 46
    readonly property int btnW: 190
    readonly property int iconS: 18
    readonly property int radiusCard: 18
    readonly property int radiusBtn: 12

    readonly property int titleSize: 24
    readonly property int subSize: 13
    readonly property int btnTextSize: 15

    readonly property color fg: theme ? theme.fg : "#ffffff"
    readonly property color btnText: fg
    readonly property color btnBg: theme ? theme.btnBg : "#2a2a2a"
    readonly property color btnBorder: theme ? theme.btnBorder : "#3a3a3a"

    readonly property color panelBg: theme ? (theme.panelBg ? theme.panelBg : (theme.dark ? "#121212" : "#ffffff"))
                                          : "#121212"
    readonly property color panelBorder: theme ? (theme.panelBorder ? theme.panelBorder : (theme.dark ? "#2a2a2a" : "#e5e5e5"))
                                              : "#2a2a2a"

    readonly property bool isDark: (theme && theme.dark !== undefined) ? theme.dark : true
    readonly property color iconColor: isDark ? "#FFFFFF" : "#111111"

    component IconText: Item {
        id: it
        property alias label: t.text
        property url iconSource
        property color textColor: "#fff"
        property int textSize: 15
        property int iconSize: 18
        property color iconTint: "#fff"
        property bool enabledState: true

        anchors.fill: parent

        Row {
            spacing: 10
            anchors.centerIn: parent

            Text {
                id: t
                color: it.textColor
                font.pixelSize: it.textSize
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }

            Image {
                id: ic
                source: it.iconSource
                width: it.iconSize
                height: it.iconSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                visible: false
            }

            ColorOverlay {
                source: ic
                width: ic.width
                height: ic.height
                color: it.iconTint
                opacity: it.enabledState ? 1.0 : 0.5
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        anchors.leftMargin: pad
        anchors.rightMargin: pad

        anchors.topMargin: 12
        anchors.bottomMargin: 12

        spacing: 14

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            radius: radiusCard
            color: panelBg
            border.width: 1
            border.color: panelBorder

            layer.enabled: true
            layer.effect: DropShadow {
                horizontalOffset: 0
                verticalOffset: 6
                radius: 18
                samples: 25
                color: isDark ? "#55000000" : "#22000000"
            }

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 6

                Text {
                    text: "Snake"
                    color: fg
                    font.pixelSize: titleSize
                    font.weight: Font.Bold
                }

                Text {
                    text: stmOk ? "STM32 connected — ready to play" : "Connect STM32 to start"
                    color: stmOk ? (isDark ? "#6CFF9A" : "#1b7a3a")
                                 : (isDark ? "#ff8080" : "#b00020")
                    font.pixelSize: subSize
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: radiusCard
            color: panelBg
            border.width: 1
            border.color: panelBorder

            layer.enabled: true
            layer.effect: DropShadow {
                horizontalOffset: 0
                verticalOffset: 6
                radius: 18
                samples: 25
                color: isDark ? "#55000000" : "#22000000"
            }

            ColumnLayout {
                anchors.fill: parent

                anchors.margins: 12
                spacing: 12

                Item { Layout.fillHeight: true }

                Button {
                    id: startBtn
                    text: qsTr("Start")
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: btnW
                    Layout.preferredHeight: btnH

                    enabled: stmOk
                    opacity: enabled ? 1.0 : disabledOpacity

                    background: Rectangle {
                        radius: radiusBtn
                        color: startBtn.down ? Qt.darker(btnBg, 1.08)
                                             : (startBtn.hovered ? Qt.lighter(btnBg, 1.08) : btnBg)
                        border.color: btnBorder
                        border.width: 1
                    }

                    contentItem: IconText {
                        label: startBtn.text
                        iconSource: "../assets/start.svg"
                        textColor: btnText
                        textSize: btnTextSize
                        iconSize: iconS
                        iconTint: iconColor
                        enabledState: startBtn.enabled
                    }
                }

                Button {
                    id: settingsBtn
                    text: qsTr("Settings")
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: btnW
                    Layout.preferredHeight: btnH

                    enabled: stmOk
                    opacity: enabled ? 1.0 : disabledOpacity

                    background: Rectangle {
                        radius: radiusBtn
                        color: settingsBtn.down ? Qt.darker(btnBg, 1.08)
                                                : (settingsBtn.hovered ? Qt.lighter(btnBg, 1.08) : btnBg)
                        border.color: btnBorder
                        border.width: 1
                    }

                    contentItem: IconText {
                        label: settingsBtn.text
                        iconSource: "../assets/settings.svg"
                        textColor: btnText
                        textSize: btnTextSize
                        iconSize: iconS
                        iconTint: iconColor
                        enabledState: settingsBtn.enabled
                    }
                }

                Button {
                    id: skinBtn
                    text: qsTr("Skins")
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: btnW
                    Layout.preferredHeight: btnH

                    enabled: stmOk
                    opacity: enabled ? 1.0 : disabledOpacity

                    background: Rectangle {
                        radius: radiusBtn
                        color: skinBtn.down ? Qt.darker(btnBg, 1.08)
                                            : (skinBtn.hovered ? Qt.lighter(btnBg, 1.08) : btnBg)
                        border.color: btnBorder
                        border.width: 1
                    }

                    contentItem: IconText {
                        label: skinBtn.text
                        iconSource: "../assets/skins.svg"
                        textColor: btnText
                        textSize: btnTextSize
                        iconSize: iconS
                        iconTint: iconColor
                        enabledState: skinBtn.enabled
                    }
                }

                Item { Layout.preferredHeight: 0 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        id: exitBtn
                        text: qsTr("Exit")
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: btnH

                        enabled: true
                        opacity: 1

                        background: Rectangle {
                            radius: radiusBtn
                            color: exitBtn.down ? Qt.darker(btnBg, 1.08)
                                                : (exitBtn.hovered ? Qt.lighter(btnBg, 1.08) : btnBg)
                            border.color: btnBorder
                            border.width: 1
                        }

                        contentItem: IconText {
                            label: exitBtn.text
                            iconSource: "../assets/exit.svg"
                            textColor: btnText
                            textSize: btnTextSize
                            iconSize: iconS
                            iconTint: iconColor
                            enabledState: exitBtn.enabled
                        }
                    }
                }
            }
        }
    }
}