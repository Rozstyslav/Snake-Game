import QtQuick
import QtQuick.Controls

Item {
    id: logic

    width:  StackView.view ? StackView.view.width  : (parent ? parent.width  : 0)
    height: StackView.view ? StackView.view.height : (parent ? parent.height : 0)

    property StackView stackView
    property ApplicationWindow appWin

    property int fieldSize: 50
    property int stmFieldSize: 50

    property int score: 0

    function isStmOverlayActive() {
        if (!ui.item) return false
        return (!ui.item.stmOk) || (!!ui.item.stmOverlaySticky)
    }

    property int dirX: 1
    property int dirY: 0

    property bool gameOverVisible: false
    property int  gameOverScore: 0
    property int  bestScore: 0
    property bool isNewRecord: false
    property int  oldRecord: 0

    readonly property int cell_empty: 0
    readonly property int cell_head:  1
    readonly property int cell_body:  2
    readonly property int cell_food:  3
    readonly property int cell_wall:  4

    property var board: []
    property bool boardDirty: false

    property int headX: -1
    property int headY: -1

    property bool havePendingHead: false
    property int pendingHx: -1
    property int pendingHy: -1

    property int pendingWallFlag: 0

    property bool paintPending: false
    function schedulePaint() {
        if (paintPending) return
        paintPending = true
        Qt.callLater(function() {
            paintPending = false
            canvas.requestPaint()
        })
    }

    property string skinId: (appWin && appWin.snakeSkinSession) ? appWin.snakeSkinSession : "classic"
    property string foodSkinId: (appWin && appWin.foodSkinSession) ? appWin.foodSkinSession : "apple"

    function skinColors(id) {
        switch (id) {
        case "neon":    return { head: "#00D9FF", body: "#006BFF", stroke: "#0a4a9a" }
        case "lava":    return { head: "#FF3B30", body: "#FF9500", stroke: "#a33b00" }
        case "mono":    return { head: "#FFFFFF", body: "#BBBBBB", stroke: "#444444" }
        default:        return { head: "#00FF66", body: "#00CC55", stroke: "#0a7a34" }
        }
    }

    property string headColor: "#00ff66"
    property string bodyColor: "#00cc55"
    property string snakeStroke: "#0a7a34"

    function applySkinNow() {
        var c = skinColors(skinId)
        headColor = c.head
        bodyColor = c.body
        snakeStroke = c.stroke
        schedulePaint()
    }

    property bool countdownVisible: false
    property int countdownValue: 3
    property bool countdownRunning: false

    function startCountdown(seconds) {
        countdownRunning = true
        countdownVisible = true
        countdownValue = seconds

        if (backend && backend.pauseGame) backend.pauseGame()

        if (ui.item && ui.item.pauseResumeButton) {
            ui.item.pauseResumeButton.paused = true
            ui.item.pauseResumeButton.text = qsTr("Resume")
        }

        countdownTimer.start()
    }

    property int playTimeSec: 0
    property string playTimeText: "00:00"

    function formatTime(sec) {
        var m = Math.floor(sec / 60)
        var s = sec % 60
        function two(n){ return (n < 10 ? "0" : "") + n }
        return two(m) + ":" + two(s)
    }

    function resetPlayTime() {
        playTimeSec = 0
        playTimeText = "00:00"
        if (ui.item) ui.item.playTimeText = playTimeText
    }

    function startPlayTimer() { playTimer.start() }
    function stopPlayTimer()  { playTimer.stop() }

    property bool restrictionMode: (appWin && appWin.restrictionSession !== undefined) ? !!appWin.restrictionSession : false

    property real borderFlashK: 0.0

    function triggerBorderFlash() {
        borderFlashK = 1.0
        borderFlashTimer.restart()
        schedulePaint()
    }

    Timer {
        id: borderFlashTimer
        interval: 220
        repeat: false
        onTriggered: {
            logic.borderFlashK = 0.0
            logic.schedulePaint()
        }
    }

    Connections {
        target: appWin
        function onSnakeSkinSessionChanged() {
            logic.skinId = appWin.snakeSkinSession || "classic"
            logic.applySkinNow()
        }
        function onFoodSkinSessionChanged() {
            logic.foodSkinId = appWin.foodSkinSession || "apple"
            logic.schedulePaint()
        }
        function onThemeChanged() { logic.schedulePaint() }
    }

    onSkinIdChanged: applySkinNow()

    focus: true
    Keys.enabled: true

    function wrap(v) {
        var m = v % fieldSize
        return (m < 0) ? (m + fieldSize) : m
    }

    function mapCoord(raw) { return wrap(raw) }

    function idx(x, y) { return y * fieldSize + x }

    function initBoardIfNeeded() {
        if (!board || board.length !== fieldSize * fieldSize) {
            board = new Array(fieldSize * fieldSize)
            for (var i = 0; i < board.length; i++) board[i] = cell_empty
        }
    }

    function clearBoard() {
        board = new Array(fieldSize * fieldSize)
        for (var i = 0; i < board.length; i++) board[i] = cell_empty
        headX = -1
        headY = -1
        havePendingHead = false
        pendingWallFlag = 0
        boardDirty = true
        schedulePaint()
    }

    function clearWallsOnly() {
        initBoardIfNeeded()
        for (var i = 0; i < board.length; i++) {
            if (board[i] === cell_wall) board[i] = cell_empty
        }
        board = board
        schedulePaint()
    }

    function getCell(x, y) { return board[idx(x, y)] }

    function setCell(x, y, v) {
        var i = idx(x, y)
        if (board[i] === v) return
        board[i] = v
        boardDirty = true
    }

    function applyDelta(hx, hy, x2, y2, wallFlag) {
        initBoardIfNeeded()

        if (headX >= 0 && headY >= 0) {
            var dx = hx - headX
            var dy = hy - headY

            if (dx >  fieldSize/2) dx -= fieldSize
            if (dx < -fieldSize/2) dx += fieldSize
            if (dy >  fieldSize/2) dy -= fieldSize
            if (dy < -fieldSize/2) dy += fieldSize

            var man = Math.abs(dx) + Math.abs(dy)
            if (man === 1) {
                if (dx !== 0) { dirX = (dx > 0 ? 1 : -1); dirY = 0 }
                else          { dirX = 0; dirY = (dy > 0 ? 1 : -1) }
            }
        }

        if (wallFlag === 1) {
            setCell(hx, hy, cell_wall)
            setCell(x2, y2, cell_wall)
            if (boardDirty) { board = board; boardDirty = false }
            schedulePaint()
            return
        }

        var c2 = getCell(x2, y2)
        if (c2 === cell_empty) {
            setCell(x2, y2, cell_food)
        } else {
            if (c2 !== cell_wall) setCell(x2, y2, cell_empty)
        }

        if (headX >= 0 && headY >= 0) {
            if (getCell(headX, headY) === cell_head) {
                setCell(headX, headY, cell_body)
            }
        }

        setCell(hx, hy, cell_head)
        headX = hx
        headY = hy

        if (boardDirty) { board = board; boardDirty = false }
        schedulePaint()
    }

    function sendMove(dir) {
        if (backend && backend.move) backend.move(dir)
    }

    function sendDebug(flag) {
        if (backend && backend.setDebugMode) backend.setDebugMode(flag)
    }

    function resetGameState() {
        countdownTimer.stop()
        playTimer.stop()
        borderFlashTimer.stop()

        gameOverVisible = false
        countdownVisible = false
        countdownRunning = false
        countdownValue = 3

        playTimeSec = 0
        playTimeText = "00:00"
        if (ui.item) ui.item.playTimeText = playTimeText

        score = 0
        if (ui.item) ui.item.score = 0

        if (ui.item && ui.item.pauseResumeButton) {
            ui.item.pauseResumeButton.paused = false
            ui.item.pauseResumeButton.text = qsTr("Pause")
        }

        clearBoard()

        forceActiveFocus()
    }

    Keys.onPressed: function(e) {
        if (gameOverVisible) { e.accepted = true; return }
        if (e.key === Qt.Key_W) { sendMove(0); e.accepted = true; return }
        if (e.key === Qt.Key_D) { sendMove(3); e.accepted = true; return }
        if (e.key === Qt.Key_S) { sendMove(1); e.accepted = true; return }
        if (e.key === Qt.Key_A) { sendMove(2); e.accepted = true; return }
        if (e.key === Qt.Key_L) { sendDebug(0x02); e.accepted = true; return }
    }

    Loader {
        id: ui
        anchors.fill: parent
        source: "GameScreen.ui.qml"

        onLoaded: {
            if (ui.item && appWin) ui.item.theme = appWin.theme
            if (ui.item) ui.item.score = score

            if (ui.item && ui.item.playField) {
                canvas.parent = ui.item.playField
                canvas.anchors.fill = ui.item.playField

                focusCatcher.parent = ui.item.playField
                focusCatcher.anchors.fill = ui.item.playField
            }

            applySkinNow()
            logic.forceActiveFocus()

            gameOverVisible = false
            clearBoard()

            if (appWin) {
                clearWallsOnly()
                if (backend && backend.walls) backend.walls(appWin.barriersSession || 0)
            }

            logic.startCountdown(3)
            schedulePaint()
        }
    }

    Connections {
        target: ui.item
        function onOverlayPauseChanged(paused) {
            if (paused) {
                logic.stopPlayTimer()
            }
        }
    }

    Connections {
        target: appWin
        function onBarriersSessionChanged() {
            logic.clearWallsOnly()
            logic.schedulePaint()
        }
    }

    Connections {
        target: appWin
        function onRestrictionSessionChanged() {
            logic.restrictionMode = (appWin && appWin.restrictionSession !== undefined) ? !!appWin.restrictionSession : false
            logic.schedulePaint()
        }
    }

    MouseArea {
        id: focusCatcher
        visible: !logic.gameOverVisible
        enabled: !logic.gameOverVisible
        onPressed: logic.forceActiveFocus()
        acceptedButtons: Qt.LeftButton
        z: 999999
    }

    Connections {
        target: (ui.item && ui.item.pauseResumeButton) ? ui.item.pauseResumeButton : null
        function onClicked() {
            if (!backend || !ui.item || !ui.item.pauseResumeButton) return
            if (logic.gameOverVisible) return
            if (logic.countdownRunning) return
            if (ui.item.pauseResumeButton.paused) {
                backend.pauseGame()
                logic.stopPlayTimer()
                logic.schedulePaint()
            } else {
                backend.resumeGame()
                logic.startPlayTimer()
                logic.schedulePaint()
            }
        }
    }

    Connections {
        target: (ui.item && ui.item.backButton) ? ui.item.backButton : null
        function onClicked() {
            if (logic.gameOverVisible) return
            if (logic.countdownRunning) return
            if (backend && backend.backToMenu) backend.backToMenu()
            if (appWin) { appWin.width = 300; appWin.height = 400 }
            if (stackView) stackView.pop()
        }
    }

    Connections {
        target: (ui.item && ui.item.overlayBackButton) ? ui.item.overlayBackButton : null
        function onClicked() {
            if (ui.item && ui.item.clearStmOverlay) ui.item.clearStmOverlay()
            resetGameState()
            if (backend && backend.backToMenu) backend.backToMenu()
            if (appWin) { appWin.width = 300; appWin.height = 400 }
            if (stackView) stackView.pop()
        }
    }

    Connections {
        target: ui.item ? ui.item.overlayExitButton : null
        function onClicked() {
            if (ui.item && ui.item.clearStmOverlay) ui.item.clearStmOverlay()
            backend.exitApp()
            Qt.quit()
        }
    }

    Connections {
        target: backend

        function normWallFlag(v) {
            if (v === undefined || v === null) return 0
            return (v ? 1 : 0)
        }

        function onHeadChanged(hxRaw, hyRaw, wallFlag) {
            if (logic.isStmOverlayActive()) return

            pendingHx = mapCoord(hxRaw)
            pendingHy = mapCoord(hyRaw)
            havePendingHead = true
            logic.pendingWallFlag = normWallFlag(wallFlag)
        }

        function onFoodChanged(x2Raw, y2Raw, wallFlag) {
            if (logic.isStmOverlayActive()) return

            var x2 = mapCoord(x2Raw)
            var y2 = mapCoord(y2Raw)

            var wf = (wallFlag !== undefined && wallFlag !== null) ? normWallFlag(wallFlag)
                                                                  : logic.pendingWallFlag

            if (havePendingHead) {
                applyDelta(pendingHx, pendingHy, x2, y2, wf)
                havePendingHead = false
            } else {
                if (headX < 0 || headY < 0) return
                applyDelta(headX, headY, x2, y2, wf)
            }

            logic.pendingWallFlag = 0
        }

        function onScoreChanged(sc) {
            if (logic.isStmOverlayActive()) return

            score = sc
            if (ui.item) ui.item.score = sc
        }

        function onBestChanged(best) { bestScore = best }

        function onGameOver(sc, best) {
            if (logic.isStmOverlayActive()) return
            gameOverScore = sc
            bestScore = best

            if (sc > best) {
                isNewRecord = true
                oldRecord = best
            } else {
                isNewRecord = false
            }

            gameOverVisible = true

            logic.borderFlashK = 0.0
            borderFlashTimer.stop()

            logic.forceActiveFocus()
            logic.stopPlayTimer()
            logic.schedulePaint()
        }
    }

    Timer {
        id: countdownTimer
        interval: 900
        repeat: true
        running: false

        onTriggered: {
            if (logic.countdownValue > 1) {
                logic.countdownValue -= 1
            } else {
                stop()
                logic.countdownVisible = false
                logic.countdownRunning = false

                if (backend && backend.startGame) backend.startGame()
                if (backend && backend.resumeGame) backend.resumeGame()

                if (ui.item && ui.item.pauseResumeButton) {
                    ui.item.pauseResumeButton.paused = false
                    ui.item.pauseResumeButton.text = qsTr("Pause")
                }
                logic.forceActiveFocus()
                logic.startPlayTimer()
            }
        }
    }

    Timer {
        id: playTimer
        interval: 1000
        repeat: true
        running: false
        onTriggered: {
            if (logic.isStmOverlayActive()) { stop(); return }
            logic.playTimeSec += 1
            logic.playTimeText = logic.formatTime(logic.playTimeSec)
            if (ui.item) ui.item.playTimeText = logic.playTimeText
        }
    }

    Rectangle {
        id: overlay

        parent: logic
        anchors.fill: parent

        visible: logic.gameOverVisible
        z: 1000000

        property bool isDark: (appWin && appWin.theme && appWin.theme.dark) ? true : false
        color: isDark ? "#AA000000" : "#66000000"

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
            onPressed: { }
            onWheel: { }
        }

        Rectangle {
            id: gameOverDialog
            anchors.centerIn: parent

            width: Math.min(parent.width - 48, 460)
            height: Math.min(parent.height - 48, 260)

            radius: 16
            color: overlay.isDark ? "#111111" : "#FFFFFF"
            border.color: overlay.isDark ? "#FFFFFF" : "#202020"
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: 12
                width: parent.width * 0.80

                Text {
                    text: "GAME OVER"
                    color: overlay.isDark ? "white" : "#111111"
                    font.pixelSize: 30
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                }

                Text {
                    text: logic.isNewRecord
                          ? ("New record: " + logic.gameOverScore + "    Previous Record: " + logic.oldRecord)
                          : ("Score: " + logic.gameOverScore + "    Best: " + logic.bestScore)
                    color: overlay.isDark ? "#dddddd" : "#333333"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                }

                Row {
                    spacing: 12
                    anchors.horizontalCenter: parent.horizontalCenter

                    Timer {
                        id: restartTimer
                        interval: 120
                        repeat: false
                        onTriggered: {
                            if (backend && backend.startGame) backend.startGame()
                        }
                    }

                    Button {
                        text: "Restart"
                        width: 140
                        height: 40

                        background: Rectangle {
                            radius: 6
                            color: overlay.isDark ? "#2a2a2a" : "#EDEDED"
                            border.width: 1
                            border.color: overlay.isDark ? "#3a3a3a" : "#C8C8C8"
                        }
                        contentItem: Text {
                            text: "Restart"
                            color: overlay.isDark ? "#FFFFFF" : "#111111"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }

                        onClicked: {
                            logic.gameOverVisible = false
                            logic.score = 0
                            if (ui.item) ui.item.score = 0
                            logic.clearBoard()
                            logic.forceActiveFocus()
                            applySkinNow()
                            restartTimer.restart()
                            logic.resetPlayTime()
                            logic.stopPlayTimer()
                            logic.startCountdown(3)
                        }
                    }

                    Button {
                        text: "Menu"
                        width: 140
                        height: 40

                        background: Rectangle {
                            radius: 6
                            color: overlay.isDark ? "#2a2a2a" : "#EDEDED"
                            border.width: 1
                            border.color: overlay.isDark ? "#3a3a3a" : "#C8C8C8"
                        }
                        contentItem: Text {
                            text: "Menu"
                            color: overlay.isDark ? "#FFFFFF" : "#111111"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }

                        onClicked: {
                            logic.gameOverVisible = false
                            logic.clearBoard()
                            if (backend && backend.backToMenu) backend.backToMenu()
                            if (appWin) { appWin.width = 300; appWin.height = 400 }
                            if (stackView) stackView.pop()
                            logic.stopPlayTimer()
                        }
                    }
                }
            }
        }

        Rectangle {
            id: countdownOverlay

            parent: logic
            anchors.fill: parent

            visible: logic.countdownVisible && !logic.gameOverVisible
            z: 900000

            property bool isDark: (appWin && appWin.theme && appWin.theme.dark) ? true : false
            color: isDark ? "#66000000" : "#44000000"

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                hoverEnabled: true
                onPressed: { }
                onWheel: { }
            }

            Rectangle {
                width: Math.min(parent.width - 48, 320)
                height: 190
                anchors.centerIn: parent
                radius: 16
                color: countdownOverlay.isDark ? "#111111" : "#FFFFFF"
                border.width: 1
                border.color: countdownOverlay.isDark ? "#FFFFFF" : "#202020"

                Column {
                    anchors.centerIn: parent
                    spacing: 10
                    width: parent.width * 0.85

                    Text {
                        text: "READY"
                        font.pixelSize: 20
                        font.bold: true
                        color: countdownOverlay.isDark ? "#FFFFFF" : "#111111"
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width
                    }

                    Text {
                        text: String(logic.countdownValue)
                        font.pixelSize: 56
                        font.bold: true
                        color: countdownOverlay.isDark ? "#FFFFFF" : "#111111"
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width
                    }

                    Text {
                        text: "Get set..."
                        font.pixelSize: 14
                        color: countdownOverlay.isDark ? "#cccccc" : "#444444"
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width
                    }
                }
            }
        }
    }

    Canvas {
        id: canvas

        renderTarget: Canvas.FramebufferObject

        property real segPadK: 0.12
        property real roundK: 0.35

        property bool drawGrid: true
        property bool fastMode: true

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

        function drawHazardBorder(ctx, bx, by, bw, bh, t, isDark, flashK) {
            var base = isDark ? "#ff3b30" : "#cc1f1a"
            var glowCol = isDark ? "#ff6b60" : "#ff3b30"
            var flash = "#ffffff"

            ctx.save()
            ctx.lineJoin = "miter"
            ctx.lineWidth = t

            ctx.shadowBlur = 0
            ctx.shadowColor = glowCol

            ctx.strokeStyle = base
            ctx.strokeRect(bx + t/2, by + t/2, bw - t, bh - t)

            ctx.shadowBlur = 0

            if (flashK > 0.001) {
                ctx.globalAlpha = Math.min(1.0, 0.55 * flashK)
                ctx.strokeStyle = flash
                ctx.lineWidth = t + 2
                ctx.strokeRect(bx + (t/2), by + (t/2), bw - t, bh - t)
            }

            ctx.restore()
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
            ctx.lineWidth = Math.max(1, cell * 0.06)
            ctx.strokeStyle = stroke
            ctx.stroke()
            ctx.restore()
        }

        function drawCellRoundedFast(ctx, x, y, cell, fill, stroke) {
            var pad = cell * segPadK
            var r = cell * roundK
            var px = x + pad
            var py = y + pad
            var w = cell - pad * 2
            var h = cell - pad * 2

            ctx.save()
            ctx.shadowBlur = 0

            roundRect(ctx, px, py, w, h, r)
            ctx.fillStyle = fill
            ctx.fill()

            ctx.lineWidth = Math.max(1, cell * 0.06)
            ctx.strokeStyle = stroke
            ctx.stroke()
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

            if (glow) {
                ctx.shadowBlur = cell * 0.25
                ctx.shadowColor = "#ff3b30"
            } else {
                ctx.shadowBlur = 0
            }

            var g = ctx.createRadialGradient(cx - r*0.35, cy - r*0.35, r*0.1, cx, cy, r*1.2)
            g.addColorStop(0.0, "#ff6b60")
            g.addColorStop(0.6, "#ff3b30")
            g.addColorStop(1.0, "#b31212")

            ctx.fillStyle = g
            ctx.beginPath()
            ctx.arc(cx, cy, r, 0, Math.PI*2)
            ctx.fill()

            ctx.shadowBlur = 0

            ctx.lineWidth = Math.max(1, cell * 0.06)
            ctx.strokeStyle = "#5a0a0a"
            ctx.beginPath()
            ctx.arc(cx, cy, r, 0, Math.PI*2)
            ctx.stroke()

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

            if (glow) {
                ctx.shadowBlur = cell * 0.20
                ctx.shadowColor = "#ffd60a"
            } else {
                ctx.shadowBlur = 0
            }

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

            ctx.save()
            ctx.globalAlpha = 0.25
            ctx.fillStyle = "#ffffff"
            ctx.beginPath()
            ctx.ellipse(cx - rx*0.18, cy - ry*0.18, rx*0.22, ry*0.18, -0.5, 0, Math.PI*2)
            ctx.fill()
            ctx.restore()

            ctx.fillStyle = "rgba(90, 70, 0, 0.35)"
            ctx.beginPath()
            ctx.arc(leftX + cell*0.03, cy, cell*0.03, 0, Math.PI*2)
            ctx.fill()
            ctx.beginPath()
            ctx.arc(rightX - cell*0.03, cy, cell*0.03, 0, Math.PI*2)
            ctx.fill()

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
            var r  = cell * 0.16

            ctx.save()

            if (glow) { ctx.shadowBlur = cell * 0.22; ctx.shadowColor = "#7b2cff" }
            else ctx.shadowBlur = 0

            var pts = [
                [0, -2], [-1, -1.2], [1, -1.2],
                [-2, 0], [0, 0], [2, 0],
                [-1, 1.4], [1, 1.4],
                [0, 2.6]
            ]

            for (var i = 0; i < pts.length; i++) {
                var px = cx + pts[i][0] * r * 1.1
                var py = cy + pts[i][1] * r * 1.1

                var g = ctx.createRadialGradient(px - r*0.35, py - r*0.35, r*0.1, px, py, r*1.2)
                g.addColorStop(0.0, "#caa6ff")
                g.addColorStop(0.6, "#7b2cff")
                g.addColorStop(1.0, "#3a0a7a")

                ctx.fillStyle = g
                ctx.beginPath()
                ctx.arc(px, py, r, 0, Math.PI*2)
                ctx.fill()

                ctx.shadowBlur = 0
                ctx.lineWidth = Math.max(1, cell * 0.03)
                ctx.strokeStyle = "#1f053f"
                ctx.stroke()
                if (glow) { ctx.shadowBlur = cell * 0.22; ctx.shadowColor = "#7b2cff" }
            }

            ctx.shadowBlur = 0

            ctx.strokeStyle = "#7a4a1a"
            ctx.lineWidth = Math.max(2, cell * 0.06)
            ctx.beginPath()
            ctx.moveTo(cx - r*0.8, cy - r*3.2)
            ctx.lineTo(cx + r*0.3, cy - r*2.1)
            ctx.stroke()

            ctx.fillStyle = "#2ecc71"
            ctx.strokeStyle = "#0b6a2b"
            ctx.lineWidth = Math.max(1, cell * 0.04)
            ctx.beginPath()
            ctx.ellipse(cx + r*1.8, cy - r*3.0, r*1.2, r*0.7, 0.4, 0, Math.PI*2)
            ctx.fill()
            ctx.stroke()

            ctx.restore()
        }

        onPaint: {
            var ctx = getContext("2d")
            if (!ctx) return

            ctx.clearRect(0, 0, width, height)

            var bg = (appWin && appWin.theme) ? appWin.theme.bg : "#000000"
            var border = (appWin && appWin.theme) ? appWin.theme.fieldBorder : "#ffffff"

            ctx.fillStyle = bg
            ctx.fillRect(0, 0, width, height)

            var cell = Math.min(width, height) / logic.fieldSize
            var ox = (width  - cell * logic.fieldSize) / 2
            var oy = (height - cell * logic.fieldSize) / 2

            var t = 3
            var align = (t % 2 === 1) ? 0.5 : 0.0

            var isDark = (appWin && appWin.theme && appWin.theme.dark) ? true : false

            var bx = Math.round(ox) + align
            var by = Math.round(oy) + align
            var bw = Math.round(logic.fieldSize * cell)
            var bh = Math.round(logic.fieldSize * cell)

            var inflate = 1
            var bxB = bx - inflate
            var byB = by - inflate
            var bwB = bw + inflate * 2
            var bhB = bh + inflate * 2

            logic.initBoardIfNeeded()

            for (var y = 0; y < logic.fieldSize; y++) {
                for (var x = 0; x < logic.fieldSize; x++) {
                    var st = logic.getCell(x, y)
                    if (st === logic.cell_empty) continue

                    var px = ox + x * cell
                    var py = oy + y * cell

                    if (st === logic.cell_body) {
                        drawCellRoundedFast(ctx, px, py, cell, logic.bodyColor, logic.snakeStroke)
                    } else if (st === logic.cell_head) {
                        drawCellRounded(ctx, px, py, cell, logic.headColor, logic.snakeStroke)
                        drawHeadEyes(ctx, px, py, cell, logic.dirX, logic.dirY)
                    } else if (st === logic.cell_food) {
                        if (logic.foodSkinId === "lemon") drawLemon(ctx, px, py, cell)
                        else if (logic.foodSkinId === "grapes") drawGrapes(ctx, px, py, cell)
                        else drawApple(ctx, px, py, cell)
                    } else if (st === logic.cell_wall) {
                        ctx.fillStyle = isDark ? "#FFFFFF" : "#000000"
                        ctx.fillRect(px, py, cell, cell)
                    }
                }
            }

            if (drawGrid && !fastMode) {
                ctx.save()
                ctx.globalAlpha = 0.12
                ctx.strokeStyle = border
                ctx.lineWidth = 1
                for (var i = 1; i < fieldSize; i++) {
                    ctx.beginPath()
                    ctx.moveTo(ox + i*cell, oy)
                    ctx.lineTo(ox + i*cell, oy + fieldSize*cell)
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.moveTo(ox, oy + i*cell)
                    ctx.lineTo(ox + fieldSize*cell, oy + i*cell)
                    ctx.stroke()
                }
                ctx.restore()
            }

            var hazardActive = logic.restrictionMode && !logic.gameOverVisible

            if (hazardActive) {
                drawHazardBorder(ctx, bxB, byB, bwB, bhB, t, isDark, logic.borderFlashK)
            } else {
                ctx.fillStyle = border
                ctx.fillRect(bxB, byB, bwB, t)
                ctx.fillRect(bxB, byB + bhB - t, bwB, t)
                ctx.fillRect(bxB, byB, t, bhB)
                ctx.fillRect(bxB + bwB - t, byB, t, bhB)
            }
        }
    }
}