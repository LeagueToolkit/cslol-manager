import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: cslolDialogGame
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: DialogButtonBox.Ok
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "Select \"League of Legends.exe\""
    font.family: "FontAwesome"
    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    signal selected(string path)

    Item {
        anchors.fill: parent
        Label {
            anchors.centerIn: parent
            font.pointSize: 48
            text: "\uf0ee"
            font.family: "FontAwesome"
        }
        DropArea {
            anchors.fill: parent
            onDropped: function(drop) {
                if (drop.hasUrls) {
                    cslolDialogGame.selected(CSLOLUtils.fromFile(drop.urls[0]))
                }
            }
        }
    }

    footer: RowLayout {
        Layout.fillWidth: true
        Item {
            Layout.fillWidth: true
        }
        DialogButtonBox {
            id: dialogButtonBox
            ToolButton {
                text: qsTr("Detect")
                onClicked: {
                    let detected = CSLOLUtils.detectGamePath()
                    if (detected === "") {
                        window.showUserError("Failed to detect game path", "Make sure LeagueClient is running to enable detect feature!")
                    } else {
                        cslolDialogGame.selected(detected)
                    }
                }
            }
            ToolButton {
                text: qsTr("Browse")
                onClicked: cslolDialogLolPath.open()
            }
            ToolButton {
                text: qsTr("Close")
                onClicked: cslolDialogGame.close()
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
    }

    CSLOLDialogLoLPath {
        id: cslolDialogLolPath
        folder: CSLOLUtils.toFile(cslolTools.leaguePath)
        onAccepted: {
            cslolDialogGame.selected(CSLOLUtils.fromFile(file))
        }
    }
}

