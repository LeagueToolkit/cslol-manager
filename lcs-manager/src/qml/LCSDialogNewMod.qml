import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: lcsDialogNewMod
    modal: true
    standardButtons: Dialog.Save | Dialog.Close
    closePolicy: Popup.NoAutoClose
    visible: false
    title: "New Mod"
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    Overlay.modal: Rectangle {
        color: "#aa333333"
    }

    property alias lastImageFolder: dialogImage.folder
    property alias lastWadFileFolder: dialogWadFiles.folder
    property alias lastRawFolder: dialogRawFolder.folder

    signal save(string fileName, string image, var infoData, var items)
    onAccepted: {
        if (fieldName.text === "") {
            window.logUserError("Add new mod", "Mod name can't be empty!")
            return
        }
        let name = fieldName.text == "" ? "UNKNOWN" : fieldName.text
        let author = fieldAuthor.text == "" ? "UNKNOWN" : fieldAuthor.text
        let version = fieldVersion.text == "" ? "1.0" : fieldVersion.text
        let description = fieldDescription.text
        let image = fieldImage.text
        let items = []
        for(let i = 0; i < itemsModel.count; i++) {
            items[items.length] = itemsModel.get(i)["Path"]
        }
        let fileName = name + " - " + version + " (by " + author + ")"
        let infoData = {
            "Name": name,
            "Author": author,
            "Version": version,
            "Description": description
        }
        save(fileName, image, infoData, items)
    }

    function clear() {
        fieldName.text = ""
        fieldAuthor.text = ""
        fieldVersion.text = ""
        fieldDescription.text = ""
        fieldImage.text = ""
        itemsModel.clear()
    }

    ListModel {
        id: itemsModel
    }


    RowLayout {
        width: parent.width
        height: parent.height
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                        onClicked: fieldImage.text = ""
                    }
                    Button {
                        text: "Browse"
                        onClicked: dialogImage.open()
                    }
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

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 5
                    ListView {
                        id: itemsView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: itemsModel
                        flickableDirection: ListView.HorizontalAndVerticalFlick
                        clip: true
                        spacing: 5
                        delegate: RowLayout{
                            width: itemsView.width
                            Label {
                                Layout.fillWidth: true
                                text: model.Path
                                elide: Text.ElideLeft
                            }
                            ToolButton {
                                text: qsTr("\uf00d")
                                onClicked: {
                                    let modName = model.Path
                                    for(let i = 0; i < itemsModel.count; i++) {
                                        let item = itemsModel.get(i)
                                        if (item.Path === modName) {
                                            itemsModel.remove(i);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        DropArea {
                            id: fileDropArea
                            anchors.fill: parent
                            onDropped: {
                                if (drop.hasUrls) {
                                    let files = drop.urls
                                    for(let i in drop.urls) {
                                        itemsModel.append({ "Path": lcsTools.fromFile(files[i]) })
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: "Clear"
                        onClicked: itemsModel.clear()
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
        onAccepted:  {
            fieldImage.text = lcsTools.fromFile(file)
        }
    }

    LCSDialogNewModWadFiles {
        id: dialogWadFiles
        onAccepted: {
            for(let i = 0; i < files.length; i++) {
                itemsModel.append({ "Path": lcsTools.fromFile(files[i]) })
            }
        }
    }

    LCSDialogNewModRAWFolder {
        id: dialogRawFolder
        onAccepted:  {
            itemsModel.append({ "Path": lcsTools.fromFile(folder) })
        }
    }
}
