import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4

ToolBar {
    id: lcsToolBar

    property bool isBussy: false
    property bool patcherRunning: false
    property alias enableProgressBar: progressBarCheck.checked
    property alias profilesModel: profilesComboBox.model
    property alias profilesCurrentIndex: profilesComboBox.currentIndex
    property alias menuButtonHeight: mainMenuButton.height
    property alias enableTray: enableTrayCheck.checked
    property alias wholeMerge: wholeMergeCheck.checked


    signal showLogs()
    signal installFantomeZip()
    signal createNewMod()
    signal changeGamePath()
    signal exit()

    signal saveProfileAndRun(bool run)
    signal stopProfile()
    signal loadProfile()
    signal newProfile()
    signal removeProfile()

    RowLayout {
        id: toolbarRow
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        ToolButton {
            id: mainMenuButton
            text: "\u2630"
            onClicked: mainMenu.popup()
              Menu {
                id: mainMenu
                MenuItem {
                    text: qsTr("Install Fantome .zip mod")
                    enabled: !isBussy
                    onTriggered: lcsToolBar.installFantomeZip()
                }
                MenuItem {
                    text: qsTr("Create a new mod")
                    enabled: !isBussy
                    onTriggered: lcsToolBar.createNewMod()
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Change Game Folder")
                    enabled: !isBussy
                    onTriggered: lcsToolBar.changeGamePath()
                }
                MenuItem {
                    id: wholeMergeCheck
                    text: qsTr("Merge whole")
                    checkable: true
                    checked: false
                }
                MenuItem {
                    id: progressBarCheck
                    text: qsTr("Progress bar")
                    checkable: true
                    checked: true
                }
                MenuItem {
                    id: enableTrayCheck
                    text: qsTr("Close to tray")
                    checkable: true
                    checked: true
                }
                MenuItem {
                    text: qsTr("Log Window")
                    onTriggered: lcsToolBar.showLogs()
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Exit")
                    onTriggered: lcsToolBar.exit()
                }
            }
        }
        ComboBox {
            id: profilesComboBox
            Layout.fillWidth: true
            currentIndex: 0
            textRole: "Name"
            enabled: !isBussy
        }
        ToolButton {
            text: qsTr("Save")
            onClicked: lcsToolBar.saveProfileAndRun(false)
            enabled: !isBussy
        }
        ToolButton {
            text: qsTr("Load")
            onClicked: lcsToolBar.loadProfile()
            enabled: !isBussy
        }
        ToolButton {
            text: qsTr("Delete")
            onClicked: lcsToolBar.removeProfile()
            enabled: !isBussy
        }
        ToolButton {
            text: qsTr("New")
            onClicked: lcsToolBar.newProfile()
            enabled: !isBussy
        }
        ToolButton {
            text: patcherRunning ? qsTr("Stop") : qsTr("Run")
            onClicked: {
                if (patcherRunning) {
                    lcsToolBar.stopProfile()
                } else {
                    lcsToolBar.saveProfileAndRun(true)
                }
            }
            enabled: !isBussy || patcherRunning
        }
    }
}