import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15



ColumnLayout {
    id: lcsModsView
    layoutDirection: Qt.RightToLeft
    spacing: 5

    property int columnCount: 1

    property bool isBussy: false

    property real rowHeight: 0

    signal modRemoved(string fileName)

    signal modExport(string fileName)

    signal modEdit(string fileName)

    signal importFile(var files)

    signal installFantomeZip()

    signal createNewMod()

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

    ScrollView {
        padding: 5
        spacing: 5
        Layout.fillHeight: true
        Layout.fillWidth: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        GridView {
            id: lcsModsViewView
            DropArea {
                id: fileDropArea
                anchors.fill: parent
                enabled: !isBussy
                onDropped: function(drop) {
                    if (drop.hasUrls) {
                        let urls = []
                        for(let i = 0; i < drop.urls.length; ++i) {
                            urls[i] = LCSUtils.fromFile(drop.urls[i]);
                        }
                        lcsModsView.importFile(urls)
                    }
                }
            }
            cellWidth: lcsModsViewView.width / lcsModsView.columnCount
            cellHeight: 75

            model: ListModel {
                id: lcsModsViewModel
            }

            delegate: Pane {
                enabled: !isBussy
                width: lcsModsViewView.width / lcsModsView.columnCount - 5
                Component.onCompleted: {
                    let newCellHeight = height + 5
                    if (lcsModsViewView.cellHeight < newCellHeight) {
                        lcsModsViewView.cellHeight = newCellHeight;
                    }
                }

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
                        ToolTip {
                            text: qsTr("Enable this mod")
                            visible: parent.hovered
                        }
                    }

                    Column {
                        width: parent.width * 0.39
                        anchors.verticalCenter: parent.verticalCenter
                        Label {
                            horizontalAlignment: Text.AlignHCenter
                            text: "V" + model.Version + " by " + model.Author
                            elide: Text.ElideRight
                            width: parent.width
                        }

                        Label {
                            horizontalAlignment: Text.AlignHCenter
                            text: model.Description
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            width: parent.width
                        }
                    }

                    Row {
                        width: parent.width * 0.3
                        layoutDirection: Qt.RightToLeft
                        anchors.verticalCenter: parent.verticalCenter
                        ToolButton {
                            text: "\uf00d"
                            font.family: "FontAwesome"
                            onClicked: {
                                let modName = model.FileName
                                lcsModsViewModel.remove(index, 1)
                                lcsModsView.modRemoved(modName)
                            }
                            ToolTip {
                                text: qsTr("Remove this mod")
                                visible: parent.hovered
                            }
                        }
                        ToolButton {
                            text: "\uf1c6"
                            font.family: "FontAwesome"
                            onClicked: {
                                let modName = model.FileName
                                lcsModsView.modExport(modName)
                            }
                            ToolTip {
                                text: qsTr("Export this mod")
                                visible: parent.hovered
                            }
                        }
                        ToolButton {
                            text: "\uf044"
                            font.family: "FontAwesome"
                            onClicked: {
                                let modName = model.FileName
                                lcsModsView.modEdit(modName)
                            }
                            ToolTip {
                                text: qsTr("Edit this mod")
                                visible: parent.hovered
                            }
                        }
                    }
                }
            }
        }
    }
    Row {
        enabled: !isBussy
        spacing: 5
        RoundButton {
            text: "\uf067"
            font.family: "FontAwesome"
            onClicked: lcsModsView.createNewMod()
            Material.background: Material.primaryColor
            ToolTip {
                text: qsTr("Create new mod")
                visible: parent.hovered
            }
        }
        RoundButton {
            text: "\uf1c6"
            font.family: "FontAwesome"
            onClicked: lcsModsView.installFantomeZip()
            Material.background: Material.primaryColor
            ToolTip {
                text: qsTr("Import new mod")
                visible: parent.hovered
            }
        }
    }
}
