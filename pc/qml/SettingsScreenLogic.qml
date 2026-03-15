import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: logic

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    property StackView stackView
    property ApplicationWindow appWin

    Loader {
        id: ui
        anchors.fill: parent
        source: "SettingsScreen.ui.qml"

        onLoaded: {
            if (!ui.item) return

            if (appWin) {

                ui.item.theme = appWin.theme
                ui.item.themeSwitch.checked = appWin.appSettings.darkTheme

                ui.item.restrictionSwitch.checked = appWin.restrictionSession

                ui.item.uiReady = false

                ui.item.pendingBarriersLevel = Number(appWin.barriersSession || 0)

                ui.item.barriers0Btn.checked = (ui.item.pendingBarriersLevel === 0)
                ui.item.barriers1Btn.checked = (ui.item.pendingBarriersLevel === 1)
                ui.item.barriers2Btn.checked = (ui.item.pendingBarriersLevel === 2)
                ui.item.barriers3Btn.checked = (ui.item.pendingBarriersLevel === 3)
                ui.item.barriers4Btn.checked = (ui.item.pendingBarriersLevel === 4)
                ui.item.barriers5Btn.checked = (ui.item.pendingBarriersLevel === 5)

                ui.item.pendingSpeedLevel = Number(appWin.snakeSpeedSession)
                if (isNaN(ui.item.pendingSpeedLevel)) ui.item.pendingSpeedLevel = 1
                if (ui.item.pendingSpeedLevel < 0) ui.item.pendingSpeedLevel = 0
                if (ui.item.pendingSpeedLevel > 2) ui.item.pendingSpeedLevel = 2

                ui.item.speed1Btn.checked = (ui.item.pendingSpeedLevel === 0)
                ui.item.speed2Btn.checked = (ui.item.pendingSpeedLevel === 1)
                ui.item.speed3Btn.checked = (ui.item.pendingSpeedLevel === 2)

                ui.item.updateMapPreview()
                ui.item.uiReady = true
            }

            backend.config()
        }
    }

    Connections {
        target: (ui.item && ui.item.saveButton) ? ui.item.saveButton : null

        function onClicked() {
            if (!ui.item || !appWin) return

            const newDark = ui.item.themeSwitch.checked
            appWin.theme.dark = newDark
            appWin.appSettings.darkTheme = newDark

            const restrictionEnabled = ui.item.restrictionSwitch.checked
            appWin.restrictionSession = restrictionEnabled
            if (restrictionEnabled) backend.restrictionOn()
            else backend.restrictionOff()

            const speed = (ui.item.pendingSpeedLevel !== undefined) ? ui.item.pendingSpeedLevel : 1
            appWin.snakeSpeedSession = speed
            backend.setSpeed(speed)

            const wallLevel = (ui.item.pendingBarriersLevel !== undefined) ? ui.item.pendingBarriersLevel : 0
            appWin.barriersSession = wallLevel
            backend.walls(wallLevel)

            ui.item.updateMapPreview()

            if (appWin) { appWin.width = 300; appWin.height = 400 }
            if (stackView) stackView.pop()
        }
    }

    Connections {
        target: (ui.item && ui.item.backButton) ? ui.item.backButton : null
        function onClicked() {
            if (backend && backend.backToMenu) backend.backToMenu()
            if (appWin) { appWin.width = 300; appWin.height = 400 }
            if (stackView) stackView.pop()
        }
    }

    Connections {
        target: (ui.item && ui.item.overlayBackButton) ? ui.item.overlayBackButton : null
        function onClicked() {
            if (ui.item && ui.item.clearStmOverlay) ui.item.clearStmOverlay()
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
}