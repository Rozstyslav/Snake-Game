import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import "LevelsData.js" as LevelsData

Rectangle {
    id: root
    anchors.fill: parent
    property var theme

    readonly property bool darkApplied: (theme && theme.dark !== undefined) ? theme.dark : false
    readonly property bool isDark: (theme && theme.dark !== undefined) ? theme.dark : true

    color: theme ? theme.bg : "#000000"

    property int pendingSpeedLevel: 1
    property int pendingBarriersLevel: 0
    property bool uiReady: false

    property alias backButton: backBtn
    property alias saveButton: saveBtn
    property alias themeSwitch: themeSw
    property alias restrictionSwitch: restrictionSw
    property alias speed1Btn: speed1
    property alias speed2Btn: speed2
    property alias speed3Btn: speed3
    property alias barriers0Btn: barriers0
    property alias barriers1Btn: barriers1
    property alias barriers2Btn: barriers2
    property alias barriers3Btn: barriers3
    property alias barriers4Btn: barriers4
    property alias barriers5Btn: barriers5

    property bool _stmOk: false
    readonly property bool stmOk: _stmOk
    property bool stmOverlaySticky: false
    property bool _stmOkPrev: false
    property bool _overlayPaused: false
    property bool _lastOverlayActive: false

    signal overlayPauseChanged(bool paused)

    property alias overlayBackButton: overlayBackBtn
    property alias overlayExitButton: overlayExitBtn

    readonly property int btnH: 40
    readonly property int btnW: 120

    readonly property int pad: 16
    readonly property int radiusCard: 18
    readonly property int radiusInner: 14
    readonly property int radiusBtn: 12

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
        background: Rectangle {
            radius: root.radiusBtn
            color: b.down ? Qt.darker(root.btnBg, 1.06)
                          : (b.hovered ? Qt.lighter(root.btnBg, 1.06) : root.btnBg)
            border.width: 1
            border.color: root.btnBorder
        }
        contentItem: Text {
            text: b.text
            color: b.enabled ? root.btnText : Qt.rgba(root.btnText.r, root.btnText.g, root.btnText.b, 0.45)
            font.pixelSize: 14
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
        }
    }

    function updateMapPreview() { mapCanvas.requestPaint() }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: pad
        anchors.rightMargin: pad
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            radius: radiusCard
            color: panelBg
            border.width: 1
            border.color: panelBorder
            layer.enabled: true
            layer.effect: CardShadow { }

            Column {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 4
                Text { text: "Settings"; color: fg; font.pixelSize: 20; font.weight: Font.Bold }
                Text { text: "Apply changes with Save"; color: muted; font.pixelSize: 12; font.weight: Font.Medium }
            }
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            contentWidth: width
            contentHeight: contentCol.implicitHeight

            ScrollBar.vertical: ScrollBar { }

            Column {
                id: contentCol
                width: flick.width
                spacing: 12

                Rectangle {
                    width: parent.width
                    implicitHeight: optCol.implicitHeight + 24
                    radius: radiusCard
                    color: panelBg
                    border.width: 1
                    border.color: panelBorder
                    layer.enabled: true
                    layer.effect: CardShadow { }

                    Column {
                        id: optCol
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Rectangle {
                            width: parent.width
                            height: 56
                            radius: radiusInner
                            color: cardBg
                            border.width: 1
                            border.color: cardBorder

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                Text { text: "Theme"; color: fg; font.pixelSize: 16; font.weight: Font.DemiBold; Layout.fillWidth: true }
                                Switch { id: themeSw; text: checked ? "Dark" : "Light" }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 56
                            radius: radiusInner
                            color: cardBg
                            border.width: 1
                            border.color: cardBorder

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                Text { text: "Restriction"; color: fg; font.pixelSize: 16; font.weight: Font.DemiBold; Layout.fillWidth: true }
                                Switch { id: restrictionSw; checked: false; text: checked ? "ON" : "OFF" }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            implicitHeight: speedCol.implicitHeight + 24
                            radius: radiusInner
                            color: cardBg
                            border.width: 1
                            border.color: cardBorder

                            Column {
                                id: speedCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text { text: "Snake speed"; color: fg; font.pixelSize: 16; font.weight: Font.DemiBold }

                                ButtonGroup {
                                    id: speedGroup
                                    onCheckedButtonChanged: {
                                        if (!root.uiReady) return
                                        if (!checkedButton) return
                                        root.pendingSpeedLevel = checkedButton.speedLevel
                                    }
                                }

                                Row {
                                    width: parent.width
                                    height: 62
                                    spacing: 0

                                    Item { width: parent.width/3; height: parent.height
                                        Item { width: 70; height: 62; anchors.centerIn: parent
                                            RadioButton { id: speed1; property int speedLevel: 0; text: ""; ButtonGroup.group: speedGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                            Text { text: "1x"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: speed1.bottom; anchors.topMargin: 6 }
                                        }
                                    }
                                    Item { width: parent.width/3; height: parent.height
                                        Item { width: 70; height: 62; anchors.centerIn: parent
                                            RadioButton { id: speed2; property int speedLevel: 1; text: ""; checked: true; ButtonGroup.group: speedGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                            Text { text: "2x"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: speed2.bottom; anchors.topMargin: 6 }
                                        }
                                    }
                                    Item { width: parent.width/3; height: parent.height
                                        Item { width: 70; height: 62; anchors.centerIn: parent
                                            RadioButton { id: speed3; property int speedLevel: 2; text: ""; ButtonGroup.group: speedGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                            Text { text: "3x"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: speed3.bottom; anchors.topMargin: 6 }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    implicitHeight: lvlCol.implicitHeight + 24
                    radius: radiusCard
                    color: panelBg
                    border.width: 1
                    border.color: panelBorder
                    layer.enabled: true
                    layer.effect: CardShadow { }

                    Column {
                        id: lvlCol
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Text { text: "Levels"; color: fg; font.pixelSize: 16; font.weight: Font.DemiBold }

                        ButtonGroup {
                            id: barriersGroup
                            onCheckedButtonChanged: {
                                if (!root.uiReady) return
                                if (!checkedButton) return
                                root.pendingBarriersLevel = checkedButton.wallLevel
                                root.updateMapPreview()
                            }
                        }

                        Row {
                            width: parent.width
                            height: 62
                            spacing: 0

                            Item { width: parent.width/6; height: parent.height
                                Item { width: 70; height: 62; anchors.centerIn: parent
                                    RadioButton { id: barriers0; property int wallLevel: 0; text: ""; checked: true; ButtonGroup.group: barriersGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                    Text { text: "0"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: barriers0.bottom; anchors.topMargin: 6 }
                                }
                            }
                            Item { width: parent.width/6; height: parent.height
                                Item { width: 70; height: 62; anchors.centerIn: parent
                                    RadioButton { id: barriers1; property int wallLevel: 1; text: ""; ButtonGroup.group: barriersGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                    Text { text: "1"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: barriers1.bottom; anchors.topMargin: 6 }
                                }
                            }
                            Item { width: parent.width/6; height: parent.height
                                Item { width: 70; height: 62; anchors.centerIn: parent
                                    RadioButton { id: barriers2; property int wallLevel: 2; text: ""; ButtonGroup.group: barriersGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                    Text { text: "2"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: barriers2.bottom; anchors.topMargin: 6 }
                                }
                            }
                            Item { width: parent.width/6; height: parent.height
                                Item { width: 70; height: 62; anchors.centerIn: parent
                                    RadioButton { id: barriers3; property int wallLevel: 3; text: ""; ButtonGroup.group: barriersGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                    Text { text: "3"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: barriers3.bottom; anchors.topMargin: 6 }
                                }
                            }
                            Item { width: parent.width/6; height: parent.height
                                Item { width: 70; height: 62; anchors.centerIn: parent
                                    RadioButton { id: barriers4; property int wallLevel: 4; text: ""; ButtonGroup.group: barriersGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                    Text { text: "4"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: barriers4.bottom; anchors.topMargin: 6 }
                                }
                            }
                            Item { width: parent.width/6; height: parent.height
                                Item { width: 70; height: 62; anchors.centerIn: parent
                                    RadioButton { id: barriers5; property int wallLevel: 5; text: ""; ButtonGroup.group: barriersGroup; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                                    Text { text: "5"; color: fg; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: barriers5.bottom; anchors.topMargin: 6 }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 170
                            radius: radiusInner
                            color: root.darkApplied ? "#101010" : "#f2f2f2"
                            border.width: 1
                            border.color: root.darkApplied ? "#303030" : "#cfcfcf"

                            Canvas {
                                id: mapCanvas
                                anchors.fill: parent
                                anchors.margins: 10
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()

                                    var GW = 50
                                    var dark = root.darkApplied
                                    var bg     = dark ? "#0b0b0b" : "#ffffff"
                                    var grid   = dark ? "#1a1a1a" : "#e3e3e3"
                                    var wallC  = dark ? "#ffffff" : "#000000"
                                    var frame  = dark ? "#303030" : "#bdbdbd"

                                    ctx.fillStyle = bg
                                    ctx.fillRect(0, 0, width, height)

                                    var cell = Math.min(width/GW, height/GW)
                                    var gw = GW * cell
                                    var gh = GW * cell
                                    var ox = Math.round((width - gw) / 2)
                                    var oy = Math.round((height - gh) / 2)

                                    ctx.fillStyle = bg
                                    ctx.fillRect(ox, oy, gw, gh)

                                    ctx.save()
                                    ctx.beginPath()
                                    ctx.rect(ox, oy, gw, gh)
                                    ctx.clip()

                                    ctx.strokeStyle = grid
                                    ctx.lineWidth = 1
                                    for (var x = 0; x <= GW; x++) {
                                        var px = ox + x * cell
                                        ctx.beginPath()
                                        ctx.moveTo(px, oy)
                                        ctx.lineTo(px, oy + gh)
                                        ctx.stroke()
                                    }
                                    for (var y = 0; y <= GW; y++) {
                                        var py = oy + y * cell
                                        ctx.beginPath()
                                        ctx.moveTo(ox, py)
                                        ctx.lineTo(ox + gw, py)
                                        ctx.stroke()
                                    }

                                    var walls = LevelsData.wallsForLevel(root.pendingBarriersLevel)
                                    ctx.fillStyle = wallC
                                    for (var i = 0; i < walls.length; i++) {
                                        ctx.fillRect(ox + walls[i].x * cell, oy + walls[i].y * cell, cell, cell)
                                    }

                                    ctx.restore()
                                    ctx.strokeStyle = frame
                                    ctx.lineWidth = 1
                                    ctx.strokeRect(ox, oy, gw, gh)
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ThemedButton { id: backBtn; text: qsTr("Back"); Layout.preferredWidth: 120; Layout.preferredHeight: 40 }
            Item { Layout.fillWidth: true }
            ThemedButton { id: saveBtn; text: qsTr("Save"); Layout.preferredWidth: 120; Layout.preferredHeight: 40 }
        }
    }

    Component.onCompleted: {
        root.uiReady = false
        root.pendingSpeedLevel = 1
        speed2.checked = true
        root.pendingBarriersLevel = 0
        barriers0.checked = true
        root.updateMapPreview()
        root.uiReady = true

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