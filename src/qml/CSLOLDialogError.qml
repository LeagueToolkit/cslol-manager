import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: cslolDialogError
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "\uf071 Error - " + name
    font.family: "FontAwesome"

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property string name: ""
    property string message: ""
    property string log_data: ""

    onOpened: {
        let scrollbar = logTextScroll.ScrollBar;
        scrollbar.horizontal.position = 0
        scrollbar.vertical.setPosition(1.0 - scrollbar.vertical.size)
        window.show()
    }

    ColumnLayout {
        id: cslolLogView
        spacing: 5
        width: parent.width
        height: parent.height
        ScrollView {
            Layout.fillWidth: true
            TextArea {
                id: logErrorMessageTextArea
                readOnly: true
                text: message
                Layout.fillWidth: true
                font.pointSize: 11
            }
        }
        ScrollView {
            id: logTextScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: logTextArea
                readOnly: true
                Layout.fillWidth: true
                text: "```\n" + log_data + "\n```"
                textFormat: Text.MarkdownText
                font.pointSize: 10
            }
        }
    }

    footer: RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Please use copy button to report your errors")
            font.italic: true
            Layout.fillWidth: true
            leftPadding: 10
        }
        DialogButtonBox {
            id: dialogButtonBox
            ToolButton {
                text: qsTr("Copy")
                onClicked:  {
                    logTextArea.selectAll()
                    logTextArea.copy()
                    logTextArea.deselect()
                }
                CSLOLToolTip {
                    text: qsTr("Puts log contents in your clipboard")
                    visible: parent.hovered
                }
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            ToolButton {
                text: qsTr("Discard")
                onClicked: {
                    cslolDialogError.close()
                }
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            }
        }
    }
}

