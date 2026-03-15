import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    anchors.fill: parent

    property var theme
    property int score: 0

    color: theme ? theme.bg : "#000000"

    property alias backButton: backBtn
    property alias overlayBackButton: overlayBackBtn
    property alias overlayExitButton: overlayExitBtn
    property alias pauseResumeButton: pauseResumeBtn
    property alias playField: play_field

    property string playTimeText: "00:00"

    readonly property bool isDark: (theme && theme.dark !== undefined) ? theme.dark : true
    property bool _stmOk: false
    readonly property bool stmOk: _stmOk

    property bool stmOverlaySticky: false
    property bool _stmOkPrev: false
    property bool _overlayPaused: false

    signal overlayPauseChanged(bool paused)
    property bool _lastOverlayActive: false

    Timer {
        id: stmOkPoll
        interval: 200
        running: true
        repeat: true
        onTriggered: {
            var nowOk = (backend && backend.stmConnected) ? true : false
            if (root._stmOkPrev && !nowOk) root.stmOverlaySticky = true
            root._stmOkPrev = nowOk
            root._stmOk = nowOk

            var overlayActive = (!nowOk) || root.stmOverlaySticky

            if (overlayActive && !root._overlayPaused) {
                root._overlayPaused = true
                if (backend && backend.pauseGame) backend.pauseGame()
            }

            if (!root._lastOverlayActive) root._lastOverlayActive = false

            if (overlayActive !== root._lastOverlayActive) {
                root._lastOverlayActive = overlayActive
                root.overlayPauseChanged(overlayActive)
            }

            if (!overlayActive) {
                root._overlayPaused = false
            }
        }
    }

    Component.onCompleted: {
        var nowOk = (backend && backend.stmConnected) ? true : false
        root._stmOk = nowOk
        root._stmOkPrev = nowOk
        root.stmOverlaySticky = !nowOk
        if (!nowOk) {
            root._overlayPaused = true
            if (backend && backend.pauseGame) backend.pauseGame()
        }
        var overlayActive = (!nowOk) || root.stmOverlaySticky
        root._lastOverlayActive = overlayActive
        root.overlayPauseChanged(overlayActive)
    }

    function clearStmOverlay() { stmOverlaySticky = false; _overlayPaused = false }

    readonly property int pad: 16
    readonly property int radiusCard: 18
    readonly property int radiusInner: 14
    readonly property int radiusBtn: 12

    readonly property int btnH: 40
    readonly property int btnW: 120

    readonly property color fg: theme ? theme.fg : "#ffffff"
    readonly property color muted: theme ? (theme.muted ? theme.muted : (isDark ? "#b6b6b6" : "#666666"))
                                       : (isDark ? "#b6b6b6" : "#666666")

    readonly property color panelBg: theme ? (theme.panelBg ? theme.panelBg : (isDark ? "#121212" : "#ffffff"))
                                          : (isDark ? "#121212" : "#ffffff")
    readonly property color panelBorder: theme ? (theme.panelBorder ? theme.panelBorder : (isDark ? "#2a2a2a" : "#e5e5e5"))
                                              : (isDark ? "#2a2a2a" : "#e5e5e5")

    readonly property color cardBg: isDark ? "#1a1a1a" : "#f7f7f7"
    readonly property color cardBorder: isDark ? "#3a3a3a" : "#d5d5d5"

    readonly property color btnBg: theme ? theme.btnBg : (isDark ? "#2a2a2a" : "#e9e9e9")
    readonly property color btnBorder: theme ? theme.btnBorder : (isDark ? "#3a3a3a" : "#c8c8c8")
    readonly property color btnText: fg

    component CardShadow: DropShadow {
        horizontalOffset: 0
        verticalOffset: 6
        radius: 18
        samples: 25
        color: root.isDark ? "#55000000" : "#22000000"
    }

    component ThemedButton: Button {
        id: b
        property color textColor: root.btnText

        background: Rectangle {
            radius: root.radiusBtn
            color: b.down ? Qt.darker(root.btnBg, 1.06)
                          : (b.hovered ? Qt.lighter(root.btnBg, 1.06) : root.btnBg)
            border.width: 1
            border.color: root.btnBorder
        }
        contentItem: Text {
            text: b.text
            color: b.enabled ? b.textColor : Qt.rgba(b.textColor.r, b.textColor.g, b.textColor.b, 0.45)
            font.pixelSize: 14
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: pad
        anchors.rightMargin: pad
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 118
            radius: radiusCard
            color: panelBg
            border.width: 1
            border.color: panelBorder

            layer.enabled: true
            layer.effect: CardShadow { }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: btnH
                    spacing: 10

                    ThemedButton {
                        id: backBtn
                        text: qsTr("Back")
                        Layout.preferredWidth: btnW
                        Layout.preferredHeight: btnH
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        id: timerCenterTxt
                        text: playTimeText
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: fg
                        font.pixelSize: 20
                        font.weight: Font.Bold

                    }

                    Item { Layout.fillWidth: true }

                    ThemedButton {
                        id: pauseResumeBtn
                        text: qsTr("Pause")
                        Layout.preferredWidth: btnW
                        Layout.preferredHeight: btnH

                        property bool paused: false

                        onClicked: {
                            paused = !paused
                            text = paused ? qsTr("Resume") : qsTr("Pause")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: qsTr("Current score:")
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: muted
                    }

                    Text {
                        id: scoreValueTxt
                        text: String(score)
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        color: fg
                    }

                    Item { Layout.fillWidth: true }
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
            layer.effect: CardShadow { }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 10
                radius: radiusInner
                color: cardBg
                border.width: 1
                border.color: cardBorder

                Frame {
                    id: play_field
                    anchors.fill: parent
                    padding: 0
                    background: null
                }
            }
        }
    }

    Item {
        id: stmDisconnectOverlay
        anchors.fill: parent
        visible: (!root.stmOk) || root.stmOverlaySticky
        z: 9999

        Rectangle {
            anchors.fill: parent
            color: "#80000000"
        }

        MouseArea {
            anchors.fill: parent
            enabled: stmDisconnectOverlay.visible
            onClicked: {}
        }

        Rectangle {
            width: Math.min(parent.width - 40, 360)
            height: 180
            radius: root.radiusCard
            anchors.centerIn: parent
            color: root.panelBg
            border.width: 1
            border.color: root.panelBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: root.stmOk ? qsTr("STM connected") : qsTr("STM not connected")
                    color: root.stmOk ? (isDark ? "#6CFF9A" : "#1b7a3a")
                                      : (isDark ? "#ff8080" : "#b00020")
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Text {
                    text: root.stmOk ? qsTr("Connection restored.")
                         : qsTr("Lost connection through game.\nPlease reconnect your STM32.")
                    color: root.muted
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ThemedButton {
                        id: overlayBackBtn
                        text: qsTr("Back to Menu")
                        visible: root.stmOk
                        enabled: root.stmOk
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.btnH
                    }

                    ThemedButton {
                        id: overlayExitBtn
                        text: qsTr("Exit")
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.btnH
                    }
                }
            }
        }
    }
}