import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: cslolDialogEditMod
    modal: true
    standardButtons: Dialog.Close
    closePolicy: Popup.NoAutoClose
    visible: false
    title: qsTr("Edit mod: ") + fileName
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property string fileName: ""
    property alias lastWadFileFolder: dialogWadFiles.folder
    property alias lastRawFolder: dialogRawFolder.folder
    property alias removeUnknownNames: checkBoxRemoveUnknownNames.checked

    signal changeInfoData(var infoData, string image)
    signal removeWads(var wads)
    signal addWad(var wad, bool removeUnknownNames)

    function infoDataChanged(infoData, image) {
        cslolModInfoEdit.setInfoData(infoData)
        cslolModInfoEdit.image = image
    }

    function wadsRemoved(wads) {
        for (let i in wads) {
            for (let c = 0; c < itemsModel.count; c++) {
                if (itemsModel.get(c)["Name"] === wads[i]) {
                    itemsModel.remove(c)
                    break
                }
            }
        }
    }

    function wadsAdded(wads) {
        for(let i in wads) {
            itemsModel.append({ "Name": wads[i] })
        }
    }

    function load(fileName, info, image, wads, isnew) {
        cslolDialogEditMod.fileName = fileName
        cslolModInfoEdit.setInfoData(info)
        cslolModInfoEdit.image = image
        itemsModel.clear()
        for(let i in wads) {
            itemsModel.append({ "Name": wads[i] })
        }
        if (isnew) {
            dialogEditModToolbar.currentIndex = 1
        }
    }

    ListModel {
        id: itemsModel
    }

    Column {
        id: dialogEditModLayout
        anchors.fill: parent
        spacing: 5
        TabBar {
            id: dialogEditModToolbar
            width: dialogEditModStackLayout.width

            TabButton {
                text: qsTr("Info")
                width: dialogEditModStackLayout.width / 2
            }
            TabButton {
                text: qsTr("Files")
                width: dialogEditModStackLayout.width / 2
            }
        }
        StackLayout {
            id: dialogEditModStackLayout
            width: parent.width
            height: parent.height - dialogEditModToolbar.height - 10
            currentIndex: dialogEditModToolbar.currentIndex
            // Info tab
            CSLOLModInfoEdit {
                width: parent.width
                height: parent.height
                id: cslolModInfoEdit
            }
            // Files tab
            ScrollView {
                width: parent.width
                height: parent.height
                padding: 5
                ListView {
                    id: itemsView
                    model: itemsModel
                    flickableDirection: ListView.HorizontalAndVerticalFlick
                    clip: true
                    spacing: 5
                    delegate: RowLayout{
                        width: itemsView.width
                        Label {
                            Layout.fillWidth: true
                            text: model.Name
                            elide: Text.ElideLeft
                        }
                        ToolButton {
                            text: "\uf00d"
                            font.family: "FontAwesome"
                            onClicked: {
                                let modName = model.Name
                                cslolDialogEditMod.removeWads([modName])
                            }
                        }
                    }
                    DropArea {
                        id: fileDropArea
                        anchors.fill: parent
                        onDropped: function(drop) {
                            if (drop.hasUrls) {
                                cslolDialogEditMod.addWad(CSLOLUtils.fromFile(drop.urls[0]), checkBoxRemoveUnknownNames.checked)
                            }
                        }
                    }
                }
            }
        }
    }

    footer: StackLayout {
        Layout.fillWidth: true
        currentIndex: dialogEditModToolbar.currentIndex
        RowLayout {
            Layout.fillWidth: true
            Item {
                Layout.fillWidth: true
            }
            DialogButtonBox {
                ToolButton {
                    text: qsTr("Close")
                    DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                    onClicked: cslolDialogEditMod.close()
                }
                ToolButton {
                    text: qsTr("Apply")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    onClicked: {
                        let infoData = cslolModInfoEdit.getInfoData();
                        let image = cslolModInfoEdit.image;
                        cslolDialogEditMod.changeInfoData(infoData, image)
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                id: checkBoxRemoveUnknownNames
                checkable: true
                checked: true
                text: qsTr("Remove unknown")
                Layout.leftMargin: 5
                CSLOLToolTip {
                    text: qsTr("Uncheck this if you are adding new files to game!")
                    visible: parent.hovered
                }
            }
            Item {
                Layout.fillWidth: true
            }
            DialogButtonBox {
                ToolButton {
                    text: qsTr("Close")
                    DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                    onClicked: cslolDialogEditMod.close()
                }
                ToolButton {
                    text: qsTr("Add WAD")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    onClicked: dialogWadFiles.open()
                }
                ToolButton {
                    text: qsTr("Add RAW")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    onClicked: dialogRawFolder.open()
                }
                ToolButton {
                    text: qsTr("Browse")
                    DialogButtonBox.buttonRole: DialogButtonBox.InvalidRole
                    onClicked: Qt.openUrlExternally(CSLOLUtils.toFile("./installed/" + fileName))
                }
            }
        }
    }

    CSLOLDialogNewModWadFiles {
        id: dialogWadFiles
        onAccepted: {
            cslolDialogEditMod.addWad(CSLOLUtils.fromFile(file), checkBoxRemoveUnknownNames.checked)
        }
    }

    CSLOLDialogNewModRAWFolder {
        id: dialogRawFolder
        onAccepted: {
            cslolDialogEditMod.addWad(CSLOLUtils.fromFile(folder), checkBoxRemoveUnknownNames.checked)
        }
    }
}
