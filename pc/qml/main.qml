import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: appWin
    visible: true
    title: "Snake"

    width: 300
    height: 400

    AppSettings { id: appSettings }

    property int snakeSpeedSession: 1
    property bool restrictionSession: false
    property int barriersSession: 0

    property string snakeSkinSession: "classic"
    property string foodSkinSession: "apple"

    Theme {
        id: theme
        dark: appSettings.darkTheme
    }

    property alias appSettings: appSettings
    property alias theme: theme

    function centerOnScreen() {
        var s = appWin.screen
        if (!s) s = Screen

        appWin.x = s.virtualX + Math.round((s.width  - appWin.width)  / 2)
        appWin.y = s.virtualY + Math.round((s.height - appWin.height) / 2)
    }

    Timer {
        id: centerTimer
        interval: 0
        repeat: false
        onTriggered: appWin.centerOnScreen()
    }

    onVisibleChanged: { if (visible) centerTimer.restart() }
    onWidthChanged: centerTimer.restart()
    onHeightChanged: centerTimer.restart()

    StackView {
        id: stack
        anchors.fill: parent

        initialItem: StartScreenLogic {
            stackView: stack
            appWin: appWin
        }
    }
}