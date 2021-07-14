import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogEditMod
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

    signal changeInfoData(var infoData, string image)
    signal removeWads(var wads)
    signal addWads(var wads, bool removeUnknownNames)

    function infoDataChanged(infoData, image) {
        lcsModInfoEdit.setInfoData(infoData)
        lcsModInfoEdit.image = image
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
        lcsDialogEditMod.fileName = fileName
        lcsModInfoEdit.setInfoData(info)
        lcsModInfoEdit.image = image
        itemsModel.clear()
        for(let i in wads) {
            itemsModel.append({ "Name": wads[i] })
        }
        dialogEditModToolbar.currentIndex = isnew ? 1 : 0
        checkBoxRemoveUnknownNames.checked = true
    }

    function addWadsFromUrls(files) {
        let wads = []
        for(let i = 0; i < files.length; i++) {
            wads[wads.length] = LCSUtils.fromFile(files[i])
        }
        lcsDialogEditMod.addWads(wads, checkBoxRemoveUnknownNames.checked)
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
            LCSModInfoEdit {
                width: parent.width
                height: parent.height
                id: lcsModInfoEdit
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
                                lcsDialogEditMod.removeWads([modName])
                            }
                        }
                    }
                    DropArea {
                        id: fileDropArea
                        anchors.fill: parent
                        onDropped: function(drop) {
                            if (drop.hasUrls) {
                                lcsDialogEditMod.addWadsFromUrls(drop.urls)
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
                    onClicked: lcsDialogEditMod.close()
                }
                ToolButton {
                    text: qsTr("Apply")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    onClicked: {
                        let infoData = lcsModInfoEdit.getInfoData();
                        let image = lcsModInfoEdit.image;
                        lcsDialogEditMod.changeInfoData(infoData, image)
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
                ToolTip {
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
                    onClicked: lcsDialogEditMod.close()
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
            }
        }
    }

    LCSDialogNewModWadFiles {
        id: dialogWadFiles
        onAccepted: lcsDialogEditMod.addWadsFromUrls(files)
    }

    LCSDialogNewModRAWFolder {
        id: dialogRawFolder
        onAccepted: lcsDialogEditMod.addWadsFromUrls([folder])
    }
}
