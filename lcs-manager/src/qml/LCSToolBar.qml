import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ToolBar {
    id: lcsToolBar

    property bool isBussy: false
    property bool patcherRunning: false
    property var profilesModel: []
    property alias profilesCurrentIndex: profilesComboBox.currentIndex
    property alias profilesCurrentName: profilesComboBox.currentText
    property alias menuButtonHeight: mainMenuButton.height

    signal openSideMenu()
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
            text: "\uf013"
            font.family: "FontAwesome"
            onClicked: lcsToolBar.openSideMenu()
            ToolTip {
                text: qsTr("Open settings dialog")
                visible: parent.hovered
            }
        }
        ComboBox {
            id: profilesComboBox
            Layout.fillWidth: true
            currentIndex: 0
            enabled: !isBussy
            model: lcsToolBar.profilesModel
            ToolTip {
                text: qsTr("Select a profile to save, load or remove")
                visible: parent.hovered
            }
        }
        ToolButton {
            text: qsTr("Save")
            onClicked: lcsToolBar.saveProfileAndRun(false)
            enabled: !isBussy
            ToolTip {
                text: qsTr("Save enabled mods for selected profile")
                visible: parent.hovered
            }
        }
        ToolButton {
            text: qsTr("Load")
            onClicked: lcsToolBar.loadProfile()
            enabled: !isBussy
            ToolTip {
                text: qsTr("Load enabled mods for selected profile")
                visible: parent.hovered
            }
        }
        ToolButton {
            text: qsTr("Delete")
            onClicked: lcsToolBar.removeProfile()
            enabled: !isBussy
            ToolTip {
                text: qsTr("Delete selected profile")
                visible: parent.hovered
            }
        }
        ToolButton {
            text: qsTr("New")
            onClicked: lcsToolBar.newProfile()
            enabled: !isBussy
            ToolTip {
                text: qsTr("Create new profile")
                visible: parent.hovered
            }
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
            ToolTip {
                text: qsTr("Runs patcher for currently selected mods")
                visible: parent.hovered
            }
        }
    }
}
