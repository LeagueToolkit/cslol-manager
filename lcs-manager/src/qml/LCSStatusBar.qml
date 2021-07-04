import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ToolBar {
    property bool isBussy: false
    property string statusMessage: ""
    property bool isCopying: true
    property int itemDone: 0
    property int itemTotal: 0
    property real dataDone: 0
    property real dataTotal: 0

    function start(newItemTotal, newDataTotal) {
        itemDone = 0
        itemTotal = newItemTotal
        dataDone = 0
        dataTotal = newDataTotal
        isCopying = true
    }

    function updateItem(newItemDone) {
        if (isCopying && newItemDone > itemDone) {
            itemDone = newItemDone
        }
    }

    function updateData(newDataDone) {
        if (isCopying && newDataDone > dataDone) {
            dataDone = newDataDone
        }
    }

    RowLayout {
        id: row
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        Label {
            Layout.fillWidth: true
            id: statusMessageLabel
            text: isBussy ? statusMessage : "Idle"
        }
        ProgressBar {
            id: progressBarData
            from: 0
            to: 1
            indeterminate: !isCopying
            value: dataDone / dataTotal
        }
    }
}
