import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ApplicationWindow {
    id: lcsDialogLog
    modality: Qt.NonModal
    // standardButtons: StandardButton.NoButton    
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowMinMaxButtonsHint
    function open() {
        show()
    }
    onClosing: {
        hide()
    }
    title: "Log - " + LCS_VERSION

    ColumnLayout {
        id: lcsLogView
        spacing: 5
        width: parent.width
        height: parent.height
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: logTextArea
                readOnly: true
                text: "```\n" + window.log_data + "\n```"
                textFormat: Text.MarkdownText
                font.pointSize: 10
                Layout.fillWidth: true
                selectByMouse: true
            }
        }
        RowLayout {
            width: parent.width
            Button {
                text: qsTr("Copy To Clipboard")
                onClicked:  {
                    logTextArea.selectAll()
                    logTextArea.copy()
                    logTextArea.deselect()
                }
                ToolTip {
                    text: qsTr("Puts log contents in your clipboard")
                    visible: parent.hovered
                }
            }
            Button {
                text: qsTr("Clear")
                onClicked: window.logClear()
                ToolTip {
                    text: qsTr("Clears all logs")
                    visible: parent.hovered
                }
            }
        }
    }
}
