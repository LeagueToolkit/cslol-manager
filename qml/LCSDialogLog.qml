import QtQuick 2.10
import QtQuick.Window 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
//import QtQuick.Dialogs 1.2

Window {
    id: lcsDialogLog
    modality: Qt.NonModal
    // standardButtons: StandardButton.NoButton    
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowMinMaxButtonsHint
    function open() {
        show()
    }
    onClosing: {
        close.accepted = false
        hide()
    }
    title: "Log"

    property alias text: logTextArea.text

    function log(message) {
        text += message
    }

    ColumnLayout {
        width: parent.width
        height: parent.height
        TextArea {
            id: logTextArea
            readOnly: true
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        Button {
            text: qsTr("Clear")
            onClicked: lcsDialogLog.text = ""
        }
    }
}
