import QtQuick
import QtQuick.Controls

Item {
    id: logic

    width:  StackView.view ? StackView.view.width  : (parent ? parent.width  : 0)
    height: StackView.view ? StackView.view.height : (parent ? parent.height : 0)

    property StackView stackView
    property ApplicationWindow appWin

    Loader {
        id: ui
        anchors.fill: parent
        source: "StartScreen.ui.qml"

        onLoaded: {
            if (ui.item && appWin)
                ui.item.theme = appWin.theme
        }
    }

    Keys.onPressed: function(e) {
        if (e.key === Qt.Key_L) { sendDebug(0x02); e.accepted = true; return }
    }

    Connections {
        target: ui.item ? ui.item.startButton : null
        function onClicked() {
            backend.startGame()

            if (appWin) { appWin.width = 1000; appWin.height = 1000 }
            if (stackView) {
                stackView.push(Qt.resolvedUrl("GameScreenLogic.qml"),
                               { "stackView": stackView, "appWin": appWin })
            }
        }
    }

    Connections {
        target: ui.item ? ui.item.settingsButton : null
        function onClicked() {

            backend.config()
            if (appWin) { appWin.width = 400; appWin.height = 700 }
            if (stackView) {
                stackView.push(Qt.resolvedUrl("SettingsScreenLogic.qml"),
                               { "stackView": stackView, "appWin": appWin })
            }
        }
    }

    Connections {
        target: ui.item ? ui.item.skinButton : null
        function onClicked() {

            if (appWin) { appWin.width = 400; appWin.height = 550 }
            if (stackView) {
                stackView.push(Qt.resolvedUrl("SkinScreenLogic.qml"),
                               { "stackView": stackView, "appWin": appWin })
            }
        }
    }

    Connections {
        target: ui.item ? ui.item.exitButton : null
        function onClicked() {
            backend.exitApp()
            Qt.quit()
        }
    }
}
