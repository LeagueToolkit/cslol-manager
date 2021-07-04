import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15



ScrollView {
    id: lcsModsView
    enabled: !isBussy
    padding: 5

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

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ListView {
        id: lcsModsViewView
        DropArea {
            id: fileDropArea
            anchors.fill: parent
            onDropped: {
                if (drop.hasUrls) {
                    let url = drop.urls[0]
                    lcsModsView.importFile(lcsTools.fromFile(url))
                }
            }
        }

        spacing: 5

        model: ListModel {
            id: lcsModsViewModel
        }

        delegate: Pane {
            width: lcsModsViewView.width
            Material.elevation: 3
            Row {
                width: parent.width
                property string modName: model.Name
                CheckBox {
                    width: parent.width * 0.3
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
                            lcsModsViewModel.setProperty(index, "Enabled", checked)
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Enable this mod")
                }

                Column {
                    width: parent.width * 0.39
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "V" + model.Version + " by " + model.Author
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: model.Description
                    }
                }

                Row {
                    width: parent.width * 0.3
                    layoutDirection: Qt.RightToLeft
                    anchors.verticalCenter: parent.verticalCenter
                    ToolButton {
                        text: qsTr("\uf00d")
                        onClicked: {
                            let modName = model.FileName
                            lcsModsViewModel.remove(index, 1)
                            lcsModsView.modRemoved(modName)
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Remove this mod")
                    }
                    ToolButton {
                        text: qsTr("\uf1c6")
                        font.family: "FontAwesome"
                        onClicked: {
                            let modName = model.FileName
                            lcsModsView.modExport(modName)
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Export this mod")
                    }
                    ToolButton {
                        text: qsTr("\uf044")
                        font.family: "FontAwesome"
                        onClicked: {
                            let modName = model.FileName
                            lcsModsView.modEdit(modName)
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Edit this mod")
                    }
                }
            }
        }
    }
}
