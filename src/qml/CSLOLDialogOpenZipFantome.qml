import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.platform 1.0

FileDialog {
    id: cslolDialogOpenZipFantome
    visible: false
    title: qsTr("Select Mod File")
    fileMode: FileDialog.OpenFile
    nameFilters: "Fantome Mod files (*.fantome *.zip)"
}
