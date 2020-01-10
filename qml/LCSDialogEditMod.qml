import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

Dialog {
    id: lcsDialogEditMod
    modality: Qt.ApplicationModal
    standardButtons: StandardButton.Close
    visible: false
    title: "Edit " + fileName

    property string fileName: ""
    property bool isBussy: false
    property alias lastImageFolder: dialogImage.folder
    property alias lastWadFileFolder: dialogWadFiles.folder
    property alias lastRawFolder: dialogRawFolder.folder

    signal changeInfoData(var infoData)
    signal changeImage(string image)
    signal removeImage()
    signal removeWads(var wads)
    signal addWads(var wads)

    function buildInfoData() {
        if (fieldName.text === "") {
            window.logWarning("Edit mod", "Mod name can't be empty!")
            return
        }
        let name = fieldName.text == "" ? "UNKNOWN" : fieldName.text
        let author = fieldAuthor.text == "" ? "UNKNOWN" : fieldAuthor.text
        let version = fieldVersion.text == "" ? "1.0" : fieldVersion.text
        let description = fieldDescription.text
        let info = {
            "Name": name,
            "Author": author,
            "Version": version,
            "Description": description
        }
        return info
    }

    function infoDataChanged(info) {
        fieldName.text = info["Name"]
        fieldAuthor.text = info["Author"]
        fieldVersion.text = info["Version"]
        fieldDescription.text = info["Description"]
    }

    function imageChanged(image) {
        fieldImage.text = image
    }

    function imageRemoved() {
        fieldImage.text = ""
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

    function load(fileName, info, image, wads) {
        lcsDialogEditMod.fileName = fileName
        infoDataChanged(info)
        imageChanged(image)
        itemsModel.clear()
        for(let i in wads) {
            itemsModel.append({ "Name": wads[i] })
        }
    }

    ListModel {
        id: itemsModel
    }

    ColumnLayout {
        width: parent.width
        height: parent.height
        enabled: !isBussy
        GroupBox {
            title: "Info"
            Layout.fillWidth: true
            ColumnLayout {
                width: parent.width
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Name: "
                    }
                    TextField {
                        id: fieldName
                        Layout.fillWidth: true
                        placeholderText: "Name"
                        validator: RegExpValidator {
                            regExp: new RegExp("[0-9a-zA-Z_ ]{3,20}")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Author: "
                    }
                    TextField {
                        id: fieldAuthor
                        Layout.fillWidth: true
                        placeholderText: "Author"
                        validator: RegExpValidator {
                            regExp: new RegExp("[0-9a-zA-Z_ ]{3,20}")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Version: "
                    }
                    TextField {
                        id: fieldVersion
                        Layout.fillWidth: true
                        placeholderText: "0.0.0"
                        validator: RegExpValidator {
                            regExp: new RegExp("([0-9]+)(\\.[0-9]+){0,3}")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "Description: "
                    }
                    TextField {
                        id: fieldDescription
                        Layout.fillWidth: true
                        placeholderText: "Description"
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Item {
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Apply"
                        onClicked: lcsDialogEditMod.changeInfoData(lcsDialogEditMod.buildInfoData())
                    }
                }
            }
        }
        GroupBox {
            title: "Image"
            Layout.fillWidth: true
            RowLayout {
                width: parent.width
                TextField {
                    id: fieldImage
                    Layout.fillWidth: true
                    placeholderText: ""
                    readOnly: true
                }

                Button {
                    text: "Remove"
                    onClicked: lcsDialogEditMod.removeImage()
                }

                Button {
                    text: "Browse"
                    onClicked: dialogImage.open()
                }
            }
        }
        GroupBox {
            title: "Files"
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                width: parent.width
                height: parent.height

                TableView {
                    id: itemsView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: itemsModel
                    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                    selectionMode: SelectionMode.MultiSelection
                    TableViewColumn {
                        title: "Name"
                        role: "Name"
                        width: itemsView.width
                        resizable: false
                    }
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: "Remove"
                        onClicked: {
                            let wads = []
                            itemsView.selection.forEach(function(i) {
                                wads[wads.length] = itemsModel.get(i)["Name"].toString()
                            })
                            itemsView.selection.clear()
                            lcsDialogEditMod.removeWads(wads)
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Add .wad files"
                        onClicked: dialogWadFiles.open()
                    }
                    Button {
                        text: "Add RAW folder"
                        onClicked: dialogRawFolder.open()
                    }
                }
            }
        }
    }

    LCSDialogNewModImage {
        id: dialogImage
        onAccepted: lcsDialogEditMod.changeImage(file.toString().replace("file:///", ""))
    }

    LCSDialogNewModWadFiles {
        id: dialogWadFiles
        onAccepted: {
            let wads = []
            for(let i = 0; i < files.length; i++) {
                wads[wads.length] = files[i].toString().replace("file:///", "")
            }
            lcsDialogEditMod.addWads(wads)
        }
    }

    LCSDialogNewModRAWFolder {
        id: dialogRawFolder
        onAccepted: {
            lcsDialogEditMod.addWads([folder.toString().replace("file:///", "")])
        }
    }
}
