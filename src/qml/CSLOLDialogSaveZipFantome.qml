import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.platform 1.0

FileDialog {
    id: cslolCSLOLDialogSaveZipFantome
    visible: false
    title: qsTr("Save Fantome Mod")
    fileMode: FileDialog.SaveFile
    nameFilters: "Fantome Mod files (*.fantome *.zip)"
    property string modName: "mod.fantome"
}
