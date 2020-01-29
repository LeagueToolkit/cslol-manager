import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

Dialog {
    id: newProfileDialog
    visible: false
    title: "New Profile Name"
    standardButtons: StandardButton.Ok | StandardButton.Cancel

    property alias text: newProfileName.text

    RowLayout {
        width: parent.width
        TextField {
            Layout.fillWidth: true
            id: newProfileName
            placeholderText: qsTr("Profile name");
            validator: RegExpValidator {
                regExp: new RegExp("[0-9a-zA-Z_ ]{3,20}")
            }
        }
    }
}
