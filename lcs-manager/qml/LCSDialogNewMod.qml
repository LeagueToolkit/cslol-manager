import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogNewMod
    modal: true
    standardButtons: Dialog.Save | Dialog.Close
    closePolicy: Popup.NoAutoClose
    visible: false
    title: qsTr("New mod")
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    signal save(string fileName, var infoData, string image)

    function clear() {
        lcsModInfoEdit.clear()
    }

    onAccepted: {
        let infoData = lcsModInfoEdit.getInfoData()
        let fileName = infoData.Name + " V" + infoData.Version + " by " + infoData.Author
        lcsDialogNewMod.save(fileName, infoData, lcsModInfoEdit.image)
    }

    ListModel {
        id: itemsModel
    }

    LCSModInfoEdit  {
        id: lcsModInfoEdit
        anchors.fill: parent
    }
}
