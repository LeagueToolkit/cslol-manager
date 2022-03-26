import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.platform 1.0

FileDialog {
    visible: false
    title: qsTr("Select .wad.client files")
    fileMode: FileDialog.OpenFile
    nameFilters: ".wad.client (*.wad.client)"
}
