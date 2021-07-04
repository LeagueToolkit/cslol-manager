import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15


Dialog {
    id: lcsDialogSettings
    modal: true
    standardButtons: Dialog.Close
    closePolicy: Popup.NoAutoClose
    visible: false
    title: qsTr("Settings ")
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

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
    signal changeGamePath()

    RowLayout {
        spacing: 5
        anchors.fill: parent
        GroupBox {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: qsTr("General")
            ColumnLayout {
                MenuItem {
                    text: qsTr("Change Game folder")
                    enabled: !isBussy
                    onTriggered: lcsDialogSettings.changeGamePath()
                    Layout.fillWidth: true
                }
                MenuItem {
                    text: qsTr("Log window")
                    onTriggered: lcsDialogSettings.showLogs()
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
            }
        }
        GroupBox {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: qsTr("Theme")
            ColumnLayout {
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
            }
        }
    }
}
