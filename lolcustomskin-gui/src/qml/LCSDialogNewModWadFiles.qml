import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import Qt.labs.platform 1.0

FileDialog {
    visible: false
    title: "Select .wad.client files"
    fileMode: FileDialog.OpenFiles
    nameFilters: ".wad.client (*.wad.client)"
}
