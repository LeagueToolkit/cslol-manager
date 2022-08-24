import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ColumnLayout {
    id: cslolModInfoEdit
    width: parent.width
    property alias image: fieldImage.text

    function clear() {
        fieldName.text = ""
        fieldAuthor.text = ""
        fieldVersion.text = ""
        fieldDescription.text = ""
        fieldHome.text = ""
        fieldHeart.text = ""
        fieldImage.text = ""
    }

    function getInfoData() {
        if (fieldName.text === "") {
            window.showUserError("Edit mod", "Mod name can't be empty!")
            return
        }
        let name = fieldName.text == "" ? "UNKNOWN" : fieldName.text
        let author = fieldAuthor.text == "" ? "UNKNOWN" : fieldAuthor.text
        let version = fieldVersion.text == "" ? "1.0" : fieldVersion.text
        let description = fieldDescription.text
        let home = fieldHome.text
        let heart = fieldHeart.text
        let info = {
            "Name": name,
            "Author": author,
            "Version": version,
            "Description": description,
            "Home": home,
            "Heart": heart,
        }
        return info
    }

    function setInfoData(info) {
        fieldName.text = info["Name"]
        fieldAuthor.text = info["Author"]
        fieldVersion.text = info["Version"]
        fieldDescription.text = info["Description"]
        fieldHome.text = info["Home"]
        fieldHeart.text = info["Heart"]
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Name: ")
        }
        TextField {
            id: fieldName
            Layout.fillWidth: true
            placeholderText: "Name"
            selectByMouse: true
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
            selectByMouse: true
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
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: window.validVersion
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Home: ")
        }
        TextField {
            id: fieldHome
            Layout.fillWidth: true
            placeholderText: "Home or update link for a skin"
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: window.validUrl
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Heart: ")
        }
        TextField {
            id: fieldHeart
            Layout.fillWidth: true
            placeholderText: "Authors homepage link or twitter or whatever"
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: window.validUrl
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
            placeholderText: "Description"
            Layout.fillWidth: true
            selectByMouse: true
        }
    }

    Image {
        id: imagePreview
        Layout.fillHeight: true
        Layout.fillWidth: true
        fillMode: Image.PreserveAspectFit
        source: CSLOLUtils.toFile(fieldImage.text)
    }

    RowLayout {
        Layout.fillWidth: true
        Button {
            text: qsTr("Select Image")
            onClicked: dialogImage.open()
        }
        TextField {
            id: fieldImage
            Layout.fillWidth: true
            placeholderText: ""
            readOnly: true
            selectByMouse: true
        }
        ToolButton {
            text: "\uf00d"
            font.family: "FontAwesome"
            onClicked: fieldImage.text = ""
        }
    }

    CSLOLDialogNewModImage {
        id: dialogImage
        onAccepted: fieldImage.text = CSLOLUtils.fromFile(file)
    }
}
