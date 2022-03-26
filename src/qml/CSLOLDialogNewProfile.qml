import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: newProfileDialog
    visible: false
    title: qsTr("New Profile Name")
    standardButtons: Dialog.Ok | Dialog.Cancel
    closePolicy: Popup.NoAutoClose
    modal: true
    width: parent.width * 0.5
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    property alias text: newProfileName.text

    RowLayout {
        width: parent.width
        TextField {
            Layout.fillWidth: true
            id: newProfileName
            placeholderText: qsTr("Profile name")
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: /[\w ]{3,50}/
            }
        }
    }
}
