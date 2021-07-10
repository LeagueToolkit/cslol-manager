import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogError
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: Dialog.Ok
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "\uf071 Error - " + category
    font.family: "FontAwesome"

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property string category: ""
    property alias message: logErrorMessageTextArea.text

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
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: logTextArea
                readOnly: true
                Layout.fillWidth: true
                text: "```\n" + window.log_data + "\n```"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                text: qsTr("Copy")
                onClicked:  {
                    logTextArea.selectAll()
                    logTextArea.copy()
                    logTextArea.deselect()
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Puts log contents in your clipboard")
            }
            Label {
                text: "Please use copy button to report your errors"
            }
        }
    }

    onAccepted: {
        close()
    }
}

