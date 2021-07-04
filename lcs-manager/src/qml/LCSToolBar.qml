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
