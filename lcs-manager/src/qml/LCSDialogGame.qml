import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogGame
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
                    lcsDialogGame.selected(LCSUtils.fromFile(drop.urls[0]))
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
                text: qsTr("Browse")
                onClicked: lcsDialogLolPath.open()
            }
            ToolButton {
                text: qsTr("Close")
                onClicked: lcsDialogGame.close()
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
    }

    LCSDialogLoLPath {
        id: lcsDialogLolPath
        folder: LCSUtils.toFile(lcsTools.leaguePath)
        onAccepted: {
            lcsDialogGame.selected(LCSUtils.fromFile(file))
        }
    }
}

