import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

Dialog {
    id: lcsDialogProgress
    modality: Qt.ApplicationModal
    standardButtons: StandardButton.Ok
    visible: false
    property alias itemDone: progressBarItems.value
    property alias itemTotal: progressBarItems.maximumValue
    property alias dataDone: progressBarData.value
    property alias dataTotal: progressBarData.maximumValue
    property string currentName: ""

    ColumnLayout {
        width: parent.width
        Label {
            Layout.fillWidth: true
            text: "Items: " + itemDone + " / " +  itemTotal
        }
        ProgressBar {
            id: progressBarItems
            Layout.fillWidth: true
            minimumValue: 0
            maximumValue: 0
        }
        Label {
            Layout.fillWidth: true
            text: "Data: " + (dataDone / 1024 / 1024).toFixed(2) + "MB / " +  (dataTotal / 1024 / 1024).toFixed(2) + "MB"
        }
        ProgressBar {
            id: progressBarData
            Layout.fillWidth: true
            minimumValue: 0
            maximumValue: 0
        }
        Label {
            Layout.fillWidth: true
            text: currentName
        }
    }
}
