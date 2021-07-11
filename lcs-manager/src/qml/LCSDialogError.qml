import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogError
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "\uf071 Error - " + category
    font.family: "FontAwesome"

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property string category: ""
    property alias message: logErrorMessageTextArea.text
    onOpened: {
        let scrollbar = logTextScroll.ScrollBar;
        scrollbar.horizontal.position = 0
        scrollbar.vertical.setPosition(1.0 - scrollbar.vertical.size)
    }

    ColumnLayout {
        id: lcsLogView
        spacing: 5
        width: parent.width
        height: parent.height
        TextArea {
            id: logErrorMessageTextArea
            readOnly: true
            text: message
            width: parent.width
            Layout.fillWidth: true
        }
        ScrollView {
            id: logTextScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: logTextArea
                readOnly: true
                Layout.fillWidth: true
                text: "```\n" + window.log_data + "\n```"
                textFormat: Text.MarkdownText
                font.pointSize: 10
            }
        }
    }

    footer: RowLayout {
        Layout.fillWidth: true
        Label {
            text: " Please use copy button to report your errors"
            font.italic: true
            Layout.fillWidth: true
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
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Puts log contents in your clipboard")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            ToolButton {
                text: qsTr("Discard")
                onClicked: {
                    lcsDialogError.close()
                }
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            }
        }
    }
}

