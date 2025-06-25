import QtQuick 2.15
import Qt.labs.platform 1.0

SystemTrayIcon {
    id: root
    visible: systemTrayManager.available
    iconSource: "qrc:/icon.png"
    tooltip: "cslol-manager"

    property bool windowVisible: true
    property bool patcherRunning: false

    function updateWindowVisibility(isVisible) {
        windowVisible = isVisible
        showHideMenuItem.text = windowVisible ? qsTr("Hide") : qsTr("Show")
    }

    function updatePatcherRunning(isRunning) {
        patcherRunning = isRunning
        runStopMenuItem.text = patcherRunning ? qsTr("Stop") : qsTr("Run")
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
            text: root.patcherRunning ? qsTr("Stop") : qsTr("Run")
            onTriggered: {
                if (root.patcherRunning) {
                    systemTrayManager.stopProfile()
                } else {
                    systemTrayManager.runProfile()
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