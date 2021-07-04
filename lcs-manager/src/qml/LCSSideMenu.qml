import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Drawer {
    id: lcsMainMenu
    height: parent.height
    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property bool isBussy: false
    property alias blacklist: blacklistCheck.checked
    property alias ignorebad: ignorebadCheck.checked
    property alias disableUpdates: disableUpdatesCheck.checked
    property alias themeDarkMode: themeDarkModeCheck.checked
    property alias themePrimaryColor: themePrimaryColorBox.currentIndex
    property alias themeAccentColor: themeAccentColorBox.currentIndex

    property var colors_LIST: [
        "Red",
        "Pink",
        "Purple",
        "DeepPurple",
        "Indigo",
        "Blue",
        "LightBlue",
        "Cyan",
        "Teal",
        "Green",
        "LightGreen",
        "Lime",
        "Yellow",
        "Amber",
        "Orange",
        "DeepOrange",
        "Brown",
        "Grey",
        "BlueGrey",
    ]


    signal showLogs()
    signal installFantomeZip()
    signal createNewMod()
    signal changeGamePath()
    signal exit()

    ScrollView {
        padding: 5
        anchors.fill: parent
        ColumnLayout {
            width: parent.width
            MenuItem {
                text: qsTr("Install a new mod")
                enabled: !isBussy
                onTriggered: lcsMainMenu.installFantomeZip()
                Layout.fillWidth: true
            }
            MenuItem {
                text: qsTr("Create a new mod")
                enabled: !isBussy
                onTriggered: lcsMainMenu.createNewMod()
                Layout.fillWidth: true
            }
            MenuItem {
                text: qsTr("Log window")
                onTriggered: lcsMainMenu.showLogs()
                Layout.fillWidth: true
            }
            MenuSeparator {
                Layout.fillWidth: true
            }
            MenuItem {
                text: qsTr("Change Game folder")
                enabled: !isBussy
                onTriggered: lcsMainMenu.changeGamePath()
                Layout.fillWidth: true
            }
            MenuItem {
                id: blacklistCheck
                text: qsTr("Blacklist extra gamemods")
                checkable: true
                checked: true
                Layout.fillWidth: true
            }
            MenuItem {
                id: ignorebadCheck
                text: qsTr("Ignore faulty .wad's")
                checkable: true
                checked: false
                Layout.fillWidth: true
            }
            MenuItem {
                id: disableUpdatesCheck
                text: qsTr("Disable updates")
                checkable: true
                checked: false
                Layout.fillWidth: true
            }
            MenuItem {
                id: themeDarkModeCheck
                text: qsTr("Dark mode")
                checkable: true
                checked: true
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Primary color")
                }
                ComboBox {
                    id: themePrimaryColorBox
                    model: colors_LIST
                    currentIndex: 4
                    Layout.fillWidth: true
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Accent color")
                }
                ComboBox {
                    id: themeAccentColorBox
                    model: colors_LIST
                    currentIndex: 1
                    Layout.fillWidth: true
                }
            }
            MenuSeparator {
                Layout.fillWidth: true
            }
            MenuItem {
                text: qsTr("Exit")
                onTriggered: lcsMainMenu.exit()
                Layout.fillWidth: true
            }
        }
    }
}
