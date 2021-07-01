import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

Dialog {
    id: lcsDialogError
    width: parent.width * 0.5
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: Dialog.Ok
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "Error"
    property alias text: errorTextLabel.text
    RowLayout {
        width: parent.width
        Label {
            text: "\uf071"
            font.family: "FontAwesome"
            font.pointSize: 32
        }
        ColumnLayout {
            Layout.fillWidth: true
            Label {
                id: errorTextLabel
                Layout.fillWidth: true
                text: "Oopsie!"
            }
            Label {
                Layout.fillWidth: true
                text: "Check logs for any errors!"
            }
        }
    }
    onAccepted: {
        close()
    }
}

