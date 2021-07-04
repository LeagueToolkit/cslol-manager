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
            text: "\u2630"
            onClicked: lcsToolBar.openSideMenu()
        }
        ComboBox {
            id: profilesComboBox
            Layout.fillWidth: true
            currentIndex: 0
            enabled: !isBussy
            model: lcsToolBar.profilesModel
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Select a profile to save, load or remove")
        }
        ToolButton {
            text: qsTr("Save")
            onClicked: lcsToolBar.saveProfileAndRun(false)
            enabled: !isBussy
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save enabled mods for selected profile")
        }
        ToolButton {
            text: qsTr("Load")
            onClicked: lcsToolBar.loadProfile()
            enabled: !isBussy
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Load enabled mods for selected profile")
        }
        ToolButton {
            text: qsTr("Delete")
            onClicked: lcsToolBar.removeProfile()
            enabled: !isBussy
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete selected profile")
        }
        ToolButton {
            text: qsTr("New")
            onClicked: lcsToolBar.newProfile()
            enabled: !isBussy
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Create new profile")
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
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Runs patcher for currently selected mods")
        }
    }
}
