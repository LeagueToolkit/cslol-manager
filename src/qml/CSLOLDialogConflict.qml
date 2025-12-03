import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Dialog {
    id: dialog
    modal: true
    standardButtons: Dialog.NoButton
    title: qsTr("Conflict Detected")

    property string modName: ""
    property string newPath: ""

    signal resolve(bool overwrite)

    ColumnLayout {
        Label {
            text: qsTr("The mod '%1' already exists. Do you want to overwrite it?").arg(dialog.modName)
        }
        Label {
            text: qsTr("New file: %1").arg(CSLOLUtils.fromFile(dialog.newPath))
            font.italic: true
            elide: Text.ElideLeft
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Skip")
            onClicked: {
                dialog.resolve(false)
                dialog.close()
            }
        }
        Button {
            text: qsTr("Overwrite")
            onClicked: {
                dialog.resolve(true)
                dialog.close()
            }
            highlighted: true
        }
    }
}