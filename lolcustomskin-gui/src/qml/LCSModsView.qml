import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4


TableView {
    id: lcsModsView
    enabled: !isBussy

    property bool isBussy: false

    property real rowHeight: 0

    signal modRemoved(string fileName)

    signal modExport(string fileName)

    signal modEdit(string fileName)

    signal importFile(string file)

    function addMod(fileName, info, enabled) {
        lcsModsViewModel.append({
                                    "FileName": fileName,
                                    "Name": info["Name"],
                                    "Version": info["Version"],
                                    "Description": info["Description"],
                                    "Author": info["Author"],
                                    "Enabled": enabled === true
                                })
    }

    function loadProfile(mods) {
        let modsCount = lcsModsViewModel.count
        for(let i = 0; i < modsCount; i++) {
            let obj = lcsModsViewModel.get(i)
            lcsModsViewModel.setProperty(i, "Enabled", mods[obj["FileName"]] === true)
        }
    }

    function updateModInfo(fileName, info) {
        let modsCount = lcsModsViewModel.count
        for(let i = 0; i < modsCount; i++) {
            let obj = lcsModsViewModel.get(i)
            if (obj["FileName"] === fileName) {
                if (obj["Name"] !== info["Name"]) {
                    lcsModsViewModel.setProperty(i, "Name", info["Name"])
                }
                if (obj["Version"] !== info["Version"]) {
                    lcsModsViewModel.setProperty(i, "Version", info["Version"])
                }
                if (obj["Description"] !== info["Description"]) {
                    lcsModsViewModel.setProperty(i, "Description", info["Description"])
                }
                if (obj["Author"] !== info["Author"]) {
                    lcsModsViewModel.setProperty(i, "Author", info["Author"])
                }
                break
            }
        }
    }

    function saveProfile() {
        let mods = {}
        let modsCount = lcsModsViewModel.count
        for(let i = 0; i < modsCount; i++) {
            let obj = lcsModsViewModel.get(i)
            if(obj["Enabled"] === true) {
                mods[obj["FileName"]] = true
            }
        }
        return mods
    }

    function clearEnabled() {
        let modsCount = lcsModsViewModel.count
        for(let i = 0; i < modsCount; i++) {
            lcsModsViewModel.setProperty(i, "Enabled", false)
        }
    }

    DropArea {
        id: fileDropArea
        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls) {
                let url = drop.urls[0]
                if (url.endsWith('.zip')) {
                    lcsModsView.importFile(url)
                }
            }
        }
    }

    model: ListModel {
        id: lcsModsViewModel
        dynamicRoles: true
    }

    selectionMode: SelectionMode.NoSelection

    rowDelegate: Rectangle{
        SystemPalette {
            id: myPalette
            colorGroup: SystemPalette.Active
        }
        color: styleData.alternate ? myPalette.alternateBase : myPalette.base
        height: rowHeight
        width: parent.width
    }

    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

    TableViewColumn {
        role: "Name"
        title: "Name"
        delegate: Item {
            anchors.fill: parent
            CheckBox {
                id: modCheckbox
                anchors.verticalCenter: parent.verticalCenter
                text: model ? model.Name : ""
                property bool installed: model ? model.Enabled : false
                onInstalledChanged: {
                    if (checked != installed) {
                        checked = installed
                    }
                }
                checked: false
                onCheckedChanged: {
                    if (checked != installed) {
                        let row = styleData.row
                        lcsModsViewModel.setProperty(row, "Enabled", checked)
                    }
                }
            }
        }
        width: parent.width * 0.3
        resizable: false
    }

    TableViewColumn {
        role: "Version"
        title: "Version"
        width: parent.width * 0.15
        resizable: false
        delegate: Item {
            anchors.fill: parent
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: styleData.value
            }
        }
    }

    TableViewColumn {
        role: "Author"
        title: "Author"
        width: parent.width * 0.15
        resizable: false
        delegate: Item {
            anchors.fill: parent
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: styleData.value
            }
        }
    }

    TableViewColumn {
        role: "Description"
        title: "Description"
        width: parent.width * 0.3
        resizable: false
        delegate: Item {
            anchors.fill: parent
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: styleData.value
            }
        }
    }

    TableViewColumn {
        role: "FileName"
        title: ""
        horizontalAlignment: Text.AlignRight
        width: parent.width * 0.1
        resizable: false
        delegate: RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            Item {
                Layout.fillWidth: true
            }
            ToolButton {
                //text: "\u2BC6"
                menu: Menu {
                    MenuItem {
                        text: qsTr("Delete")
                        onTriggered: {
                            let modName = styleData.value
                            let row = styleData.row
                            lcsModsViewModel.remove(row, 1)
                            lcsModsView.modRemoved(modName)
                        }
                    }
                    MenuItem {
                        text: qsTr("Export")
                        onTriggered: {
                            let modName = styleData.value
                            lcsModsView.modExport(modName)
                        }
                    }
                    MenuItem {
                        text: qsTr("Edit")
                        onTriggered: {
                            let modName = styleData.value
                            lcsModsView.modEdit(modName)
                        }
                    }
                }
            }
        }
    }
}
