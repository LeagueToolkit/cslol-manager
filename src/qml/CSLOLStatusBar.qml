import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ToolBar {
    property string statusMessage: ""

    RowLayout {
        id: row
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 5
            id: statusMessageLabel
            text: statusMessage
        }
    }
}
