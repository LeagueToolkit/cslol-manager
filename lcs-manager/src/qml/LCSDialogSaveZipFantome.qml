import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import Qt.labs.platform 1.0

FileDialog {
    id: lcsLCSDialogSaveZipFantome
    visible: false
    title: "Save Fantome Mod"
    fileMode: FileDialog.SaveFile
    nameFilters: "Fantome Mod files (*.fantome *.zip)"
    property string modName: "mod.fantome"
}
