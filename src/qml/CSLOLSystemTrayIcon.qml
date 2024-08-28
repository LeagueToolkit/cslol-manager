import QtQuick 2.15
import Qt.labs.platform 1.0

SystemTrayIcon {
    visible: systemTrayManager.available
    iconSource: "qrc:/icon.png"
    tooltip: "cslol-manager"

    menu: Menu {
        MenuItem {
            id: showHideMenuItem
            text: qsTr("Show")
            onTriggered: {
                if (text === qsTr("Show")) {
                    systemTrayManager.showWindow()
                    text = qsTr("Hide")
                } else {
                    systemTrayManager.hideWindow()
                    text = qsTr("Show")
                }
            }
        }
        MenuItem {
            id: runStopMenuItem
            text: qsTr("Run")
            onTriggered: {
                if (text === qsTr("Run")) {
                    systemTrayManager.runProfile()
                    text = qsTr("Stop")
                } else {
                    systemTrayManager.stopProfile()
                    text = qsTr("Run")
                }
            }
        }
        MenuItem {
            text: qsTr("Logs")
            onTriggered: systemTrayManager.openLogs()
        }
        MenuItem {
            text: qsTr("Exit")
            onTriggered: systemTrayManager.quit()
        }
    }

    onActivated: {
        if (reason === SystemTrayIcon.Trigger) {
            systemTrayManager.showWindow()
            showHideMenuItem.text = qsTr("Hide")
        }
    }
}