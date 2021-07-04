import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ColumnLayout {
    id: lcsLogView
    spacing: 5
    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        TextArea {
            id: logTextArea
            readOnly: true
            text: window.log_data
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
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Puts log contents in your clipboard")
        }
        Button {
            text: qsTr("Clear")
            onClicked: window.logClear()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Clears all logs")
        }
    }
}
