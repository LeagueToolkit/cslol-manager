import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4

StatusBar {
    property bool isBussy: false
    property string statusMessage: ""

    RowLayout {
        Item {
            height: statusMessageLabel.height
            width: statusMessageLabel.height
            BusyIndicator {
                anchors.fill: parent
                visible: isBussy
                running: visible
            }
        }
        Label {
            id: statusMessageLabel
            text: isBussy ? statusMessage : "Idle"
        }
    }
}
