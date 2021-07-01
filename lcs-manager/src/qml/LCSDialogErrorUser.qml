import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

Dialog {
    id: lcsDialogUserError
    width: parent.width * 0.5
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: Dialog.Ok
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "Warning"
    property alias text: warningTextLabel.text
    RowLayout {
        width: parent.width
        Label {
            text: "\uf071"
            font.family: "FontAwesome"
            font.pointSize: 32
        }
        Label {
            id: warningTextLabel
            Layout.fillWidth: true
            text: "Oopsie!"
        }
    }
    onAccepted: {
        close()
    }
}
