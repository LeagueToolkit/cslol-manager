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
    title: "\uf071 Error - " + text

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property string text: ""

    LCSLogView {
        width: parent.width
        height: parent.height
        spacing: 5
    }

    onAccepted: {
        close()
    }
}

