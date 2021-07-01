import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.0

FileDialog {
    visible: false
    title: "Select .wad.client files"
    fileMode: FileDialog.OpenFiles
    nameFilters: ".wad.client (*.wad.client)"
}
