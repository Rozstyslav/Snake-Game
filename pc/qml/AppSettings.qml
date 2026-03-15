import QtQuick 2.15
import QtCore 6.5

Item {
    id: root

    property bool darkTheme: false

    Settings {
        id: store
        category: "SnakeUI"
    }

    Component.onCompleted: {
        root.darkTheme = store.value("darkTheme", false)
    }

    onDarkThemeChanged: {
        store.setValue("darkTheme", root.darkTheme)
    }
}