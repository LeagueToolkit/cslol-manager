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
            window.logUserError("Edit mod", "Mod name can't be empty!")
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

    GridLayout {
        width: parent.width
        height: parent.height
        columns: height > width ? 1 : 2
        enabled: !isBussy
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            GroupBox {
                title: qsTr("Info")
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Name: ")
                        }
                        TextField {
                            id: fieldName
                            Layout.fillWidth: true
                            placeholderText: "Name"
                            validator: RegularExpressionValidator {
                                regularExpression: window.validName
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Author: ")
                        }
                        TextField {
                            id: fieldAuthor
                            Layout.fillWidth: true
                            placeholderText: "Author"
                            validator: RegularExpressionValidator {
                                regularExpression: window.validName
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Version: ")
                        }
                        TextField {
                            id: fieldVersion
                            Layout.fillWidth: true
                            placeholderText: "0.0.0"
                            validator: RegularExpressionValidator {
                                regularExpression: /([0-9]{1,3})(\.[0-9]{1,3}){0,3}/
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Description: ")
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
                            text: qsTr("Apply")
                            onClicked: lcsDialogEditMod.changeInfoData(lcsDialogEditMod.buildInfoData())
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Image")
                Layout.fillWidth: true
                RowLayout {
                    width: parent.width
                    Button {
                        text: qsTr("Browse")
                        onClicked: dialogImage.open()
                    }
                    TextField {
                        id: fieldImage
                        Layout.fillWidth: true
                        placeholderText: ""
                        readOnly: true
                    }
                    ToolButton {
                        text: "\uf00d"
                        font.family: "FontAwesome"
                        onClicked: lcsDialogEditMod.removeImage()
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
                                    lcsDialogEditMod.addWads(wads)
                                }
                            }
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

    LCSDialogNewModImage {
        id: dialogImage
        onAccepted: lcsDialogEditMod.changeImage(lcsTools.fromFile(file))
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
