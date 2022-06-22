import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15


Dialog {
    id: cslolDialogSettings
    modal: true
    standardButtons: Dialog.Close
    closePolicy: Popup.NoAutoClose
    visible: false
    title: qsTr("Settings ")
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
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
    property alias suppressInstallConflicts: suppressInstallConflictsCheck.checked

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


    signal changeGamePath()

    Column {
        id: settingsLayout
        anchors.fill: parent
        TabBar {
            id: settingsTabBar
            width: settingsStackLayout.width

            TabButton {
                text: qsTr("Game")
                width: settingsStackLayout.width / 2
            }
            TabButton {
                text: qsTr("Theme")
                width: settingsStackLayout.width / 2
            }
        }

        StackLayout {
            id: settingsStackLayout
            width: parent.width
            Layout.fillHeight: true
            currentIndex: settingsTabBar.currentIndex
            ColumnLayout {
                id: settingsGameTab
                spacing: 5
                Button {
                    text: qsTr("Change Game folder")
                    enabled: !isBussy
                    onClicked: cslolDialogSettings.changeGamePath()
                    Layout.fillWidth: true
                }
                Switch {
                    id: blacklistCheck
                    text: qsTr("Blacklist extra gamemods")
                    checked: true
                    Layout.fillWidth: true
                }
                Switch {
                    id: suppressInstallConflictsCheck
                    text: qsTr("Suppress install conflicts")
                    checked: false
                    Layout.fillWidth: true
                }
                Switch {
                    id: ignorebadCheck
                    text: qsTr("Ignore faulty .wad's")
                    checked: false
                    Layout.fillWidth: true
                }
                Switch {
                    id: disableUpdatesCheck
                    text: qsTr("Disable updates")
                    checked: false
                    Layout.fillWidth: true
                }
            }
            ColumnLayout {
                id: settingsThemeTab
                spacing: 5
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
                Switch {
                    id: themeDarkModeCheck
                    text: qsTr("Dark mode")
                    checked: true
                    Layout.fillWidth: true
                }
            }
        }
    }
}
