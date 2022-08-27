import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: cslolDialogUserError
    width: parent.width * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: Dialog.Ok
    closePolicy: Popup.NoAutoClose
    modal: true
    title: "\uf071 Warning"
    font.family: "FontAwesome"
    Overlay.modal: Rectangle {
        color: "#aa333333"
    }
    onOpened: window.show()

    property alias text: warningTextLabel.text

    RowLayout {
        width: parent.width
        Label {
            id: warningTextLabel
            Layout.fillWidth: true
            maximumLineCount: 3
            text: "Oopsie!"
        }
    }
    onAccepted: {
        close()
    }
}
