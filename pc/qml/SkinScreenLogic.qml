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
        source: "SkinScreen.ui.qml"

        onLoaded: {
            if (!ui.item || !appWin) return

            ui.item.theme = appWin.theme

            const currentSnakeId = appWin.snakeSkinSession || "classic"

            var snakeIdx = 0
            for (var i = 0; i < ui.item.skinsModel.count; i++) {
                if (ui.item.skinsModel.get(i).skinId === currentSnakeId) { snakeIdx = i; break }
            }

            const currentFoodId = appWin.foodSkinSession || "apple"

            var foodIdx = 0
            for (var j = 0; j < ui.item.foodModel.count; j++) {
                if (ui.item.foodModel.get(j).foodId === currentFoodId) { foodIdx = j; break }
            }

            ui.item.uiReady = false

            ui.item.snakeSwipe.currentIndex = snakeIdx
            ui.item.pendingSnakeSkinId = ui.item.skinsModel.get(snakeIdx).skinId

            ui.item.foodSwipe.currentIndex = foodIdx
            ui.item.pendingFoodId = ui.item.foodModel.get(foodIdx).foodId

            ui.item.uiReady = true
        }
    }

    Connections {
        target: (ui.item && ui.item.saveButton) ? ui.item.saveButton : null
        function onClicked() {
            if (!ui.item || !appWin) return

            const chosenSnake = ui.item.pendingSnakeSkinId || "classic"
            const chosenFood  = ui.item.pendingFoodId || "apple"

            appWin.snakeSkinSession = chosenSnake
            appWin.foodSkinSession  = chosenFood

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