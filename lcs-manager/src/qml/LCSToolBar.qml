import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4

ToolBar {
    id: lcsToolBar

    property bool isBussy: false
    property bool patcherRunning: false
    property var profilesModel: []
    property alias profilesCurrentIndex: profilesComboBox.currentIndex
    property alias profilesCurrentName: profilesComboBox.currentText
    property alias menuButtonHeight: mainMenuButton.height
    property alias blacklist: blacklistCheck.checked
    property alias ignorebad: ignorebadCheck.checked


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
                    text: qsTr("Install Fantome Mod")
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
                    id: blacklistCheck
                    text: qsTr("Blacklist extra gamemods")
                    checkable: true
                    checked: true
                }
                MenuItem {
                    id: ignorebadCheck
                    text: qsTr("Ignore faulty .wad's")
                    checkable: true
                    checked: false
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
            enabled: !isBussy
            model: lcsToolBar.profilesModel
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
