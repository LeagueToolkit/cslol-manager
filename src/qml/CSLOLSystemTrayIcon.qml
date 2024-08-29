import QtQuick 2.15
import Qt.labs.platform 1.0

SystemTrayIcon {
    id: root
    visible: systemTrayManager.available
    iconSource: "qrc:/icon.png"
    tooltip: "cslol-manager"

    property bool windowVisible: true

    function updateWindowVisibility(isVisible) {
        windowVisible = isVisible
        showHideMenuItem.text = windowVisible ? qsTr("Hide") : qsTr("Show")
    }

    menu: Menu {
        MenuItem {
            id: showHideMenuItem
            text: root.windowVisible ? qsTr("Hide") : qsTr("Show")
            onTriggered: {
                if (root.windowVisible) {
                    systemTrayManager.hideWindow()
                } else {
                    systemTrayManager.showWindow()
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
            text: qsTr("Updates")
            onTriggered: systemTrayManager.openUpdateUrl()
        }
        MenuItem {
            text: qsTr("Exit")
            onTriggered: systemTrayManager.quit()
        }
    }

    onActivated: {
        if (reason === SystemTrayIcon.Trigger) {
            if (root.windowVisible) {
                systemTrayManager.hideWindow()
            } else {
                systemTrayManager.showWindow()
            }
        }
    }
}