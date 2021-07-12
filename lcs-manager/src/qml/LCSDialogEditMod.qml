import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogEditMod
    modal: true
    standardButtons: Dialog.Close
    closePolicy: Popup.NoAutoClose
    visible: false
    title: "Edit " + fileName
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property string fileName: ""
    property bool isBussy: false
    property alias lastWadFileFolder: dialogWadFiles.folder
    property alias lastRawFolder: dialogRawFolder.folder

    signal changeInfoData(var infoData, string image)
    signal removeWads(var wads)
    signal addWads(var wads, bool removeUnknownNames, bool removeUnchangedEntries)

    function infoDataChanged(infoData, image) {
        lcsModInfoEdit.setInfoData(info)
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
    }

    ListModel {
        id: itemsModel
    }

    GridLayout {
        width: parent.width
        height: parent.height
        columns: height > width ? 1 : 2
        enabled: !isBussy
        GroupBox {
            title: qsTr("Info")
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                width: parent.width
                LCSModInfoEdit {
                    id: lcsModInfoEdit
                }
                RowLayout {
                    Layout.fillWidth: true
                    Item {
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Apply")
                        onClicked: lcsDialogEditMod.changeInfoData(lcsModInfoEdit.buildInfoData())
                    }
                }
            }
        }
        GroupBox {
            title: qsTr("Files")
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                width: parent.width
                height: parent.height
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                                    let files = drop.urls
                                    let wads = []
                                    for(let i in drop.urls) {
                                        wads[wads.length] = lcsTools.fromFile(files[i])
                                    }
                                    lcsDialogEditMod.addWads(wads,
                                                             checkBoxRemoveUnknownNames.checked,
                                                             checkBoxRemoveUnchangedEntries.checked)
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    width: parent.width
                    CheckBox {
                        id: checkBoxRemoveUnknownNames
                        checkable: true
                        checked: true
                        text: qsTr("Remove unknown")
                        ToolTip {
                            text: qsTr("Uncheck this if you are adding new files to game!")
                            visible: parent.hovered
                        }
                    }
                    CheckBox {
                        id: checkBoxRemoveUnchangedEntries
                        checkable: true
                        checked: true
                        text: qsTr("Remove unchanged")
                        ToolTip {
                            text: qsTr("Uncheck this if you are forcing existing files into different wad!")
                            visible: parent.hovered
                        }
                    }
                }
                RowLayout {
                    width: parent.width
                    Button {
                        text: qsTr("Add WAD")
                        onClicked: dialogWadFiles.open()
                    }
                    Button {
                        text: qsTr("Add RAW")
                        onClicked: dialogRawFolder.open()
                    }
                }
            }
        }
    }


    LCSDialogNewModWadFiles {
        id: dialogWadFiles
        onAccepted: {
            let wads = []
            for(let i = 0; i < files.length; i++) {
                wads[wads.length] = lcsTools.fromFile(files[i])
            }
            lcsDialogEditMod.addWads(wads)
        }
    }

    LCSDialogNewModRAWFolder {
        id: dialogRawFolder
        onAccepted: {
            lcsDialogEditMod.addWads([lcsTools.fromFile(folder)])
        }
    }
}
