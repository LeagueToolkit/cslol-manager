import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.platform 1.0

FileDialog {
    id: lolPathDialog
    visible: false
    title: qsTr("Select League of Legends.exe")
    fileMode: FileDialog.OpenFile
    nameFilters: "League of Legends.exe (*.exe)"
    folder: ""
}
