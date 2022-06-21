import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15



ColumnLayout {
    id: cslolModsView
    spacing: 5

    property int columnCount: 1

    property bool isBussy: false

    property real rowHeight: 0

    property string search: ""

    signal modRemoved(string fileName)

    signal modExport(string fileName)

    signal modEdit(string fileName)

    signal importFile(string file)

    signal installFantomeZip()

    signal createNewMod()

    function addMod(fileName, info, enabled) {
        let infoData = {
            "FileName": fileName,
            "Name": info["Name"],
            "Version": info["Version"],
            "Description": info["Description"],
            "Author": info["Author"],
            "Enabled": enabled === true
        }
        if (searchMatches(infoData)) {
            cslolModsViewModel.append(infoData)
            resortMod_model(cslolModsViewModel.count - 1, cslolModsViewModel)
        } else {
            cslolModsViewModel2.append(infoData)
        }
    }

    function loadProfile(mods) {
        loadProfile_model(mods, cslolModsViewModel)
        loadProfile_model(mods, cslolModsViewModel2)
    }

    function updateModInfo(fileName, info) {
        let i0 = updateModInfo_model(fileName, info, cslolModsViewModel);
        if (i0 !== -1) {
            if (!searchMatches(info)) {
                cslolModsViewModel2.append(cslolModsViewModel.get(i0))
                cslolModsViewModel.remove(i0)
            } else {
                resortMod_model(i0, cslolModsViewModel)
            }
        }

        let i1 = updateModInfo_model(fileName, info, cslolModsViewModel2);
        if (i1 !== -1) {
            if (searchMatches(info)) {
                cslolModsViewModel.append(cslolModsViewModel2.get(i1))
                cslolModsViewModel2.remove(i1)
                resortMod_model(cslolModsViewModel.count - 1, cslolModsViewModel)
            }
        }
    }

    function saveProfile() {
        let mods = {}
        saveProfile_model(mods, cslolModsViewModel)
        saveProfile_model(mods, cslolModsViewModel2)
        return mods
    }

    function searchMatches(info) {
        return search == "" || info["Name"].toLowerCase().search(search) !== -1 || info["Description"].toLowerCase().search(search) !== -1
    }

    function searchUpdate() {
        let i = 0;
        while (i < cslolModsViewModel2.count) {
            let obj = cslolModsViewModel2.get(i)
            if (searchMatches(obj)) {
                cslolModsViewModel.append(obj);
                cslolModsViewModel2.remove(i, 1)
                resortMod_model(cslolModsViewModel.count - 1, cslolModsViewModel)
            } else {
                i++;
            }
        }
        let j = 0;
        while (j < cslolModsViewModel.count) {
            let obj2 = cslolModsViewModel.get(j)
            if (!searchMatches(obj2)) {
                cslolModsViewModel2.append(obj2)
                cslolModsViewModel.remove(j, 1)
            } else {
                j++;
            }
        }
    }

    function resortMod_model(index, model) {
        let info = model.get(index)
        for (let i = 0; i < index; i++) {
            if (model.get(i)["Name"].toLowerCase() >= info["Name"].toLowerCase()) {
                model.move(index, i, 1);
                return i;
            }
        }
        for (let j = model.count - 1; j > index; j--) {
            if (model.get(j)["Name"].toLowerCase() <= info["Name"].toLowerCase()) {
                model.move(index, j, 1);
                return j;
            }
        }
        return index;
    }

    function saveProfile_model(mods, model) {
        let modsCount = model.count
        for (let i = 0; i < modsCount; i++) {
            let obj = model.get(i)
            if(obj["Enabled"] === true) {
                mods[obj["FileName"]] = true
            }
        }
    }

    function loadProfile_model(mods, model) {
        let modsCount = model.count
        for(let i = 0; i < modsCount; i++) {
            let obj = model.get(i)
            model.setProperty(i, "Enabled", mods[obj["FileName"]] === true)
        }
    }

    function updateModInfo_model(fileName, info, model) {
        let modsCount = model.count
        for(let i = 0; i < modsCount; i++) {
            let obj = model.get(i)
            if (obj["FileName"] === fileName) {
                if (obj["Name"] !== info["Name"]) {
                    model.setProperty(i, "Name", info["Name"])
                }
                if (obj["Version"] !== info["Version"]) {
                    model.setProperty(i, "Version", info["Version"])
                }
                if (obj["Description"] !== info["Description"]) {
                    model.setProperty(i, "Description", info["Description"])
                }
                if (obj["Author"] !== info["Author"]) {
                    model.setProperty(i, "Author", info["Author"])
                }
                return i;
            }
        }
        return -1;
    }

    ListModel {
        id: cslolModsViewModel
    }

    ListModel {
        id: cslolModsViewModel2
    }

    ScrollView {
        id: cslolModsScrollView
        Layout.fillHeight: true
        Layout.fillWidth: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
        padding: ScrollBar.vertical.width
        spacing: 5

        GridView {
            id: cslolModsViewView
            DropArea {
                id: fileDropArea
                anchors.fill: parent
                enabled: !isBussy
                onDropped: function(drop) {
                    if (drop.hasUrls && drop.urls.length > 0) {
                        cslolModsView.importFile(CSLOLUtils.fromFile(drop.urls[0]))
                    }
                }
            }
            cellWidth: cslolModsViewView.width / cslolModsView.columnCount
            cellHeight: 75

            model: cslolModsViewModel

            delegate: Pane {
                enabled: !isBussy
                width: cslolModsViewView.width / cslolModsView.columnCount - cslolModsScrollView.spacing
                Component.onCompleted: {
                    let newCellHeight = height + cslolModsScrollView.spacing
                    if (cslolModsViewView.cellHeight < newCellHeight) {
                        cslolModsViewView.cellHeight = newCellHeight;
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
                                cslolModsViewModel.setProperty(index, "Enabled", checked)
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
                            text: model.Description ? model.Description : ""
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
                                cslolModsViewModel.remove(index, 1)
                                cslolModsView.modRemoved(modName)
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
                                cslolModsView.modExport(modName)
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
                                cslolModsView.modEdit(modName)
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

    RowLayout {
        enabled: !isBussy
        spacing: cslolModsScrollView.spacing
        Layout.fillWidth:  true
        Layout.margins: cslolModsScrollView.padding

        TextField {
            id: cslolModsViewSearchBox
            Layout.fillWidth:  true
            placeholderText: "Search..."
            onTextEdited: {
                search = text.toLowerCase()
                searchUpdate()
            }
        }

        RoundButton {
            text: "\uf067"
            font.family: "FontAwesome"
            onClicked: cslolModsView.createNewMod()
            Material.background: Material.primaryColor
            ToolTip {
                text: qsTr("Create new mod")
                visible: parent.hovered
            }
        }
        RoundButton {
            text: "\uf1c6"
            font.family: "FontAwesome"
            onClicked: cslolModsView.installFantomeZip()
            Material.background: Material.primaryColor
            ToolTip {
                text: qsTr("Import new mod")
                visible: parent.hovered
            }
        }
    }
}
