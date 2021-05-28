import QtQuick 2.10
import QtQuick.Window 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
//import QtQuick.Dialogs 1.2

ApplicationWindow {
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
    title: "Log - " + LCS_VERSION

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
            text: "[INFO] Version: " + LCS_VERSION + "\n"
        }
        RowLayout {
            width: parent.width
            Button {
                text: "Copy To Clipboard"
                onClicked:  {
                    logTextArea.selectAll()
                    logTextArea.copy()
                    logTextArea.deselect()
                }
            }
            Button {
                text: qsTr("Clear")
                onClicked: lcsDialogLog.text = "[INFO] Version: " + LCS_VERSION + "\n"
            }
        }
    }
}
