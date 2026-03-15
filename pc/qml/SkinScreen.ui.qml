import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    width: 300
    height: 460

    property var theme
    color: theme ? theme.bg : "#000000"

    property string pendingSnakeSkinId: "classic"
    property string pendingFoodId: "apple"
    property bool uiReady: true

    property alias backButton: backBtn
    property alias saveButton: saveBtn

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

    property alias skinsModel: skinsModel
    property alias foodModel: foodModel
    property alias snakeSwipe: snakeSwipe
    property alias foodSwipe: foodSwipe

    readonly property bool isDark: (theme && theme.dark !== undefined) ? theme.dark : true

    readonly property int pad: 16
    readonly property int radiusCard: 18
    readonly property int radiusInner: 14
    readonly property int radiusBtn: 12

    readonly property int titleSize: 20
    readonly property int sectionSize: 12
    readonly property int labelSize: 14

    readonly property color fg: theme ? theme.fg : "#ffffff"
    readonly property color muted: theme ? (theme.muted ? theme.muted : (isDark ? "#b6b6b6" : "#666666"))
                                       : (isDark ? "#b6b6b6" : "#666666")

    readonly property color panelBg: theme ? (theme.panelBg ? theme.panelBg : (isDark ? "#121212" : "#ffffff"))
                                          : (isDark ? "#121212" : "#ffffff")
    readonly property color panelBorder: theme ? (theme.panelBorder ? theme.panelBorder : (isDark ? "#2a2a2a" : "#e5e5e5"))
                                              : (isDark ? "#2a2a2a" : "#e5e5e5")

    readonly property color cardBg: isDark ? "#1a1a1a" : "#f7f7f7"
    readonly property color cardBorder: isDark ? "#3a3a3a" : "#d5d5d5"
    readonly property color activeBorder: isDark ? "#ffffff" : "#111111"

    readonly property color btnBg: theme ? theme.btnBg : (isDark ? "#2a2a2a" : "#e9e9e9")
    readonly property color btnBorder: theme ? theme.btnBorder : (isDark ? "#3a3a3a" : "#c8c8c8")
    readonly property color btnText: fg

    readonly property color dotOn: isDark ? "#dcdcdc" : "#333333"
    readonly property color dotOff: isDark ? "#5a5a5a" : "#c0c0c0"

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

    ListModel {
        id: skinsModel
        ListElement { skinId: "classic"; label: "Classic"; head: "#00FF66"; body: "#00AA44"; stroke: "#0a7a34" }
        ListElement { skinId: "neon";    label: "Neon";    head: "#00D9FF"; body: "#006BFF"; stroke: "#0a4a9a" }
        ListElement { skinId: "lava";    label: "Lava";    head: "#FF3B30"; body: "#FF9500"; stroke: "#a33b00" }
        ListElement { skinId: "mono";    label: "Mono";    head: "#FFFFFF"; body: "#BBBBBB"; stroke: "#444444" }
    }

    ListModel {
        id: foodModel
        ListElement { foodId: "apple";  label: "Apple"  }
        ListElement { foodId: "lemon";  label: "Lemon"  }
        ListElement { foodId: "grapes"; label: "Grapes" }
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

                Text {
                    text: "Skins"
                    color: fg
                    font.pixelSize: titleSize
                    font.weight: Font.Bold
                }
                Text {
                    text: "Choose snake & food style"
                    color: muted
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 185
            radius: radiusCard
            color: panelBg
            border.width: 1
            border.color: panelBorder

            layer.enabled: true
            layer.effect: CardShadow { }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: "Snake"
                    color: muted
                    font.pixelSize: sectionSize
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }

                SwipeView {
                    id: snakeSwipe
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Repeater {
                        model: skinsModel
                        delegate: Rectangle {
                            radius: radiusInner
                            color: cardBg
                            border.width: (SwipeView.isCurrentItem ? 2 : 1)
                            border.color: (SwipeView.isCurrentItem ? activeBorder : cardBorder)

                            Column {
                                anchors.centerIn: parent
                                spacing: 10

                                Canvas {
                                    width: 190
                                    height: 52
                                    Component.onCompleted: requestPaint()

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        if (!ctx) return
                                        ctx.clearRect(0, 0, width, height)

                                        var cell = 22
                                        var gap  = 6
                                        var totalW = 6*cell + 5*gap
                                        var startX = (width - totalW) / 2
                                        var y = (height - cell) / 2

                                        for (var i = 0; i < 5; i++) {
                                            painter.drawCellRounded(ctx, startX + i*(cell+gap), y, cell, body, stroke)
                                        }

                                        var hx = startX + 5*(cell+gap)
                                        painter.drawCellRounded(ctx, hx, y, cell, head, stroke)
                                        painter.drawHeadEyes(ctx, hx, y, cell, 1, 0)
                                    }
                                }

                                Text {
                                    text: label
                                    color: fg
                                    font.pixelSize: labelSize
                                    font.weight: Font.Medium
                                    horizontalAlignment: Text.AlignHCenter
                                    width: parent.width
                                }
                            }
                        }
                    }

                    onCurrentIndexChanged: {
                        if (!root.uiReady) return
                        const el = skinsModel.get(currentIndex)
                        if (el) root.pendingSnakeSkinId = el.skinId
                    }
                }

                Row {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter

                    Repeater {
                        model: snakeSwipe.count
                        delegate: Rectangle {
                            width: 7
                            height: 7
                            radius: 99
                            color: (index === snakeSwipe.currentIndex) ? dotOn : dotOff
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 165
            radius: radiusCard
            color: panelBg
            border.width: 1
            border.color: panelBorder

            layer.enabled: true
            layer.effect: CardShadow { }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: "Food"
                    color: muted
                    font.pixelSize: sectionSize
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }

                SwipeView {
                    id: foodSwipe
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Repeater {
                        model: foodModel
                        delegate: Rectangle {
                            radius: radiusInner
                            color: cardBg
                            border.width: (SwipeView.isCurrentItem ? 2 : 1)
                            border.color: (SwipeView.isCurrentItem ? activeBorder : cardBorder)

                            Column {
                                anchors.centerIn: parent
                                spacing: 10

                                Canvas {
                                    width: 56
                                    height: 56
                                    Component.onCompleted: requestPaint()

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        if (!ctx) return
                                        ctx.clearRect(0, 0, width, height)

                                        var cell = Math.min(width, height)
                                        var x = (width  - cell) / 2
                                        var y = (height - cell) / 2

                                        var prevGlow = painter.glow
                                        painter.glow = false

                                        if (foodId === "lemon") painter.drawLemon(ctx, x, y, cell)
                                        else if (foodId === "grapes") painter.drawGrapes(ctx, x, y, cell)
                                        else painter.drawApple(ctx, x, y, cell)

                                        painter.glow = prevGlow
                                    }
                                }

                                Text {
                                    text: label
                                    color: fg
                                    font.pixelSize: labelSize
                                    font.weight: Font.Medium
                                    horizontalAlignment: Text.AlignHCenter
                                    width: parent.width
                                }
                            }
                        }
                    }

                    onCurrentIndexChanged: {
                        if (!root.uiReady) return
                        const el = foodModel.get(currentIndex)
                        if (el) root.pendingFoodId = el.foodId
                    }
                }

                Row {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter

                    Repeater {
                        model: foodSwipe.count
                        delegate: Rectangle {
                            width: 7
                            height: 7
                            radius: 99
                            color: (index === foodSwipe.currentIndex) ? dotOn : dotOff
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ThemedButton {
                id: backBtn
                text: "Back"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
            }

            Item { Layout.fillWidth: true }

            ThemedButton {
                id: saveBtn
                text: "Save"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
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

    Canvas {
        id: painter
        visible: false
        width: 64
        height: 64
        renderTarget: Canvas.Image

        property real segPadK: 0.12
        property real roundK: 0.35
        property bool glow: true

        function clamp(v, a, b) { return Math.max(a, Math.min(b, v)) }

        function roundRect(ctx, x, y, w, h, r) {
            r = clamp(r, 0, Math.min(w, h) / 2)
            ctx.beginPath()
            ctx.moveTo(x + r, y)
            ctx.lineTo(x + w - r, y)
            ctx.quadraticCurveTo(x + w, y, x + w, y + r)
            ctx.lineTo(x + w, y + h - r)
            ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
            ctx.lineTo(x + r, y + h)
            ctx.quadraticCurveTo(x, y + h, x, y + h - r)
            ctx.lineTo(x, y + r)
            ctx.quadraticCurveTo(x, y, x + r, y)
            ctx.closePath()
        }

        function drawCellRounded(ctx, x, y, cell, fill, stroke) {
            var pad = cell * segPadK
            var r = cell * roundK
            var px = x + pad
            var py = y + pad
            var w = cell - pad * 2
            var h = cell - pad * 2

            ctx.save()
            if (glow) { ctx.shadowBlur = cell * 0.25; ctx.shadowColor = fill }
            else ctx.shadowBlur = 0

            roundRect(ctx, px, py, w, h, r)
            ctx.fillStyle = fill
            ctx.fill()

            ctx.shadowBlur = 0
            if (stroke) {
                ctx.lineWidth = Math.max(1, cell * 0.06)
                ctx.strokeStyle = stroke
                ctx.stroke()
            }
            ctx.restore()
        }

        function drawHeadEyes(ctx, x, y, cell, dx, dy) {
            var cx = x + cell/2
            var cy = y + cell/2

            var vx = dx, vy = dy
            if (vx === 0 && vy === 0) { vx = 1; vy = 0 }

            var px = -vy, py = vx

            var f = cell * 0.18
            var s = cell * 0.13
            var er = cell * 0.10
            var pr = cell * 0.05

            var ex1 = cx + vx*f + px*s
            var ey1 = cy + vy*f + py*s
            var ex2 = cx + vx*f - px*s
            var ey2 = cy + vy*f - py*s

            ctx.save()
            ctx.shadowBlur = 0

            ctx.fillStyle = "#eaffef"
            ctx.beginPath(); ctx.arc(ex1, ey1, er, 0, Math.PI*2); ctx.fill()
            ctx.beginPath(); ctx.arc(ex2, ey2, er, 0, Math.PI*2); ctx.fill()

            ctx.fillStyle = "#0a2a12"
            ctx.beginPath(); ctx.arc(ex1 + vx*cell*0.05, ey1 + vy*cell*0.05, pr, 0, Math.PI*2); ctx.fill()
            ctx.beginPath(); ctx.arc(ex2 + vx*cell*0.05, ey2 + vy*cell*0.05, pr, 0, Math.PI*2); ctx.fill()

            ctx.fillStyle = "#ffffff"
            ctx.beginPath(); ctx.arc(ex1 - pr*0.3, ey1 - pr*0.3, pr*0.35, 0, Math.PI*2); ctx.fill()
            ctx.beginPath(); ctx.arc(ex2 - pr*0.3, ey2 - pr*0.3, pr*0.35, 0, Math.PI*2); ctx.fill()

            ctx.restore()
        }

        function drawApple(ctx, x, y, cell) {
            var cx = x + cell/2
            var cy = y + cell/2
            var r  = cell * 0.35

            ctx.save()
            if (glow) { ctx.shadowBlur = cell * 0.35; ctx.shadowColor = "#ff3b30" }
            else ctx.shadowBlur = 0

            var g = ctx.createRadialGradient(cx - r*0.35, cy - r*0.35, r*0.1, cx, cy, r*1.2)
            g.addColorStop(0.0, "#ff6b60")
            g.addColorStop(0.6, "#ff3b30")
            g.addColorStop(1.0, "#b31212")

            ctx.fillStyle = g
            ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI*2); ctx.fill()

            ctx.shadowBlur = 0
            ctx.lineWidth = Math.max(1, cell * 0.06)
            ctx.strokeStyle = "#5a0a0a"
            ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI*2); ctx.stroke()

            ctx.globalAlpha = 0.35
            ctx.fillStyle = "#ffffff"
            ctx.beginPath()
            ctx.ellipse(cx - r*0.35, cy - r*0.25, r*0.25, r*0.18, -0.4, 0, Math.PI*2)
            ctx.fill()
            ctx.globalAlpha = 1.0

            ctx.strokeStyle = "#7a4a1a"
            ctx.lineWidth = Math.max(2, cell * 0.08)
            ctx.beginPath()
            ctx.moveTo(cx, cy - r*0.95)
            ctx.lineTo(cx + r*0.15, cy - r*1.35)
            ctx.stroke()

            ctx.fillStyle = "#2ecc71"
            ctx.strokeStyle = "#0b6a2b"
            ctx.lineWidth = Math.max(1, cell * 0.04)
            ctx.beginPath()
            ctx.ellipse(cx + r*0.45, cy - r*1.15, r*0.35, r*0.18, 0.4, 0, Math.PI*2)
            ctx.fill()
            ctx.stroke()

            ctx.restore()
        }

        function drawLemon(ctx, x, y, cell) {
            var cx = x + cell/2
            var cy = y + cell/2

            var w = cell * 0.78
            var h = cell * 0.52
            var rx = w / 2
            var ry = h / 2
            var ang = -0.25

            ctx.save()
            ctx.translate(cx, cy)
            ctx.rotate(ang)
            ctx.translate(-cx, -cy)

            if (glow) { ctx.shadowBlur = cell * 0.20; ctx.shadowColor = "#ffd60a" }
            else ctx.shadowBlur = 0

            var leftX  = cx - rx
            var rightX = cx + rx
            var topY   = cy - ry
            var botY   = cy + ry

            ctx.beginPath()
            ctx.moveTo(leftX, cy)
            ctx.quadraticCurveTo(cx - rx*0.55, topY, cx, topY)
            ctx.quadraticCurveTo(cx + rx*0.55, topY, rightX, cy)
            ctx.quadraticCurveTo(cx + rx*0.55, botY, cx, botY)
            ctx.quadraticCurveTo(cx - rx*0.55, botY, leftX, cy)
            ctx.closePath()

            var g = ctx.createRadialGradient(cx - rx*0.25, cy - ry*0.25, cell*0.05, cx, cy, rx*1.2)
            g.addColorStop(0.0, "#fff7b8")
            g.addColorStop(0.55, "#ffd60a")
            g.addColorStop(1.0, "#c79300")

            ctx.fillStyle = g
            ctx.fill()

            ctx.shadowBlur = 0
            ctx.lineWidth = Math.max(1, cell * 0.07)
            ctx.strokeStyle = "#6a5200"
            ctx.stroke()

            ctx.globalAlpha = 0.25
            ctx.fillStyle = "#ffffff"
            ctx.beginPath()
            ctx.ellipse(cx - rx*0.18, cy - ry*0.18, rx*0.22, ry*0.18, -0.5, 0, Math.PI*2)
            ctx.fill()
            ctx.globalAlpha = 1.0

            ctx.strokeStyle = "#7a4a1a"
            ctx.lineWidth = Math.max(1.5, cell * 0.06)
            ctx.beginPath()
            ctx.moveTo(cx + rx*0.10, topY + cell*0.02)
            ctx.lineTo(cx + rx*0.28, topY - cell*0.12)
            ctx.stroke()

            ctx.fillStyle = "#2ecc71"
            ctx.strokeStyle = "#0b6a2b"
            ctx.lineWidth = Math.max(1, cell * 0.04)
            ctx.beginPath()
            ctx.ellipse(cx + rx*0.45, topY - cell*0.12, cell*0.16, cell*0.09, 0.6, 0, Math.PI*2)
            ctx.fill()
            ctx.stroke()

            ctx.restore()
        }

        function drawGrapes(ctx, x, y, cell) {
            var cx = x + cell/2
            var cy = y + cell/2
            var r = cell * 0.115

            ctx.save()
            if (glow) { ctx.shadowBlur = cell * 0.12; ctx.shadowColor = "#7b2cff" }
            else ctx.shadowBlur = 0

            var pts = [
                [0, -2.0],
                [-1.0, -1.2], [1.0, -1.2],
                [-1.6, -0.1], [0, -0.1], [1.6, -0.1],
                [-1.0,  1.0], [1.0,  1.0],
                [0,  2.1]
            ]

            for (var i = 0; i < pts.length; i++) {
                var px = cx + pts[i][0] * r * 1.25
                var py = cy + pts[i][1] * r * 1.25

                var g = ctx.createRadialGradient(px - r*0.35, py - r*0.35, r*0.1, px, py, r*1.2)
                g.addColorStop(0.0, "#d9c7ff")
                g.addColorStop(0.6, "#7b2cff")
                g.addColorStop(1.0, "#3a0a7a")

                ctx.fillStyle = g
                ctx.beginPath()
                ctx.arc(px, py, r, 0, Math.PI*2)
                ctx.fill()

                ctx.shadowBlur = 0
                ctx.lineWidth = Math.max(1, cell * 0.015)
                ctx.strokeStyle = "rgba(31, 5, 63, 0.8)"
                ctx.stroke()

                ctx.globalAlpha = 0.22
                ctx.fillStyle = "#ffffff"
                ctx.beginPath()
                ctx.arc(px - r*0.25, py - r*0.25, r*0.22, 0, Math.PI*2)
                ctx.fill()
                ctx.globalAlpha = 1.0

                if (glow) { ctx.shadowBlur = cell * 0.12; ctx.shadowColor = "#7b2cff" }
            }

            ctx.shadowBlur = 0
            ctx.strokeStyle = "#7a4a1a"
            ctx.lineWidth = Math.max(1.5, cell * 0.03)
            ctx.beginPath()
            ctx.moveTo(cx - r*0.4, cy - r*3.0)
            ctx.lineTo(cx + r*0.5, cy - r*2.1)
            ctx.stroke()

            ctx.fillStyle = "#2ecc71"
            ctx.strokeStyle = "#0b6a2b"
            ctx.lineWidth = Math.max(1, cell * 0.02)
            ctx.beginPath()
            ctx.ellipse(cx + r*1.7, cy - r*3.0, r*1.1, r*0.6, 0.5, 0, Math.PI*2)
            ctx.fill()
            ctx.stroke()

            ctx.restore()
        }
    }
}