import os
os.environ["QT_QUICK_CONTROLS_STYLE"] = "Basic"

import sys
from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtCore import QCoreApplication
from PySide6.QtGui import QIcon

from backend import Backend


def main():
    app = QApplication(sys.argv)

    QCoreApplication.setOrganizationName("SnakeDev")
    QCoreApplication.setOrganizationDomain("snakedev.local")
    QCoreApplication.setApplicationName("Snake")

    app.setWindowIcon(QIcon("logo/logo.png"))

    engine = QQmlApplicationEngine()

    backend = Backend(port=None, baudrate=9600, poll_ms=20)
    engine.rootContext().setContextProperty("backend", backend)

    engine.load("qml/main.qml")
    if not engine.rootObjects():
        return -1
    return app.exec()

if __name__ == "__main__":
    raise SystemExit(main())



