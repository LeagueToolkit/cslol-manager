import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import Qt.labs.platform 1.1
import Qt.labs.settings 1.1
import lolcustomskin.tools 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("League Custom Skin Tools")

    SystemTrayIcon {
        id: trayIcon
        visible: lcsToolBar.enableTray || !window.visible
        iconSource: "qrc:/icon.png"
        menu: Menu {
            MenuItem {
                text: !window.visible ? qsTr("Show") : qsTr("Minimzie")
                onTriggered: window.visible ? window.hide() : window.show()
            }
            MenuItem {
                text: qsTr("Log")
                onTriggered: lcsDialogLog.open()
            }
            MenuItem {
                text: qsTr("Exit")
                onTriggered: window.exit()
            }
        }
        onActivated: {
            if (reason === SystemTrayIcon.Context) {
                menu.open()
            } else if(reason === SystemTrayIcon.DoubleClick) {
                window.show()
            }
        }
    }

    onClosing: {
        if (trayIcon.available && (lcsToolBar.enableTray || !window.visible)) {
            close.accepted = false
            window.hide()
        }
    }

    Settings {
        id: settings
        property alias profiles: window.savedProfiles
        property alias profilesCurrentIndex: lcsToolBar.profilesCurrentIndex
        property alias leaguePath: lcsTools.leaguePath
        property alias wholeMerge: lcsToolBar.wholeMerge
        property alias logVisible: lcsDialogLog.visible
        property alias enableTray: lcsToolBar.enableTray

        property alias lastZipDirectory: lcsDialogOpenZipFantome.folder
        property alias lastImageFolder: lcsDialogNewMod.lastImageFolder
        property alias lastWadFileFolder: lcsDialogNewMod.lastWadFileFolder
        property alias lastRawFolder: lcsDialogNewMod.lastRawFolder

        property alias windowHeight: window.height
        property alias windowWidth: window.width
        property alias logHeight: lcsDialogLog.height
        property alias logWidth: lcsDialogLog.width

        fileName: "config.ini"
    }

    property bool criticalFail: false
    property bool loaded: false
    property var savedProfiles: [
        { "Name": "Default", "Mods": {}, "Time": 0 },
    ]

    function exit() {
        if (trayIcon.available && trayIcon.visible) {
            trayIcon.hide()
        }
        Qt.quit()
    }

    function logInfo(name, message) {
        lcsDialogLog.log("INFO: " + name + ": " + message + "\n")
    }

    function logWarning(name, error) {
        lcsDialogLog.log("Warning  " + name + ": " + error + "\n")
        lcsDialogWarning.text = "Failed to" + name
        lcsDialogWarning.detailedText = error
        lcsDialogWarning.open()
    }

    function logError(name, error) {
        lcsDialogLog.log("Failed to " + name + ": " + error + "\n")
        lcsDialogError.text = "Failed to" + name
        lcsDialogError.detailedText = error
        lcsDialogError.open()
    }


    toolBar: LCSToolBar {
        id: lcsToolBar
        enabled: !window.criticalFail
        isBussy: lcsTools.state !== LCSTools.StateIdle
        patcherRunning: lcsTools.state === LCSTools.StateExternalRunning

        profilesModel: savedProfiles

        onInstallFantomeZip: lcsDialogOpenZipFantome.open()

        onCreateNewMod: lcsDialogNewMod.open()

        onChangeGamePath: lcsDialogLolPath.open()

        onShowLogs: if (!lcsDialogLog.visible) lcsDialogLog.visible = true

        onExit: window.exit()

        onSaveProfileAndRun:  {
            let index = lcsToolBar.profilesCurrentIndex
            let savedProfile = window.savedProfiles[index]
            let newMods = lcsModsView.saveProfile()
            let name = savedProfile["Name"]
            lcsTools.saveProfile(name, newMods, run, lcsToolBar.wholeMerge)
        }

        onStopProfile: lcsTools.stopProfile()

        onLoadProfile: {
            let index = lcsToolBar.profilesCurrentIndex
            lcsModsView.loadProfile(window.savedProfiles[index]["Mods"])
        }

        onNewProfile: lcsDialogNewProfile.open()

        onRemoveProfile: {
            let index = lcsToolBar.profilesCurrentIndex
            lcsTools.deleteProfile(savedProfiles[index]["Name"])
        }
    }

    LCSModsView {
        id: lcsModsView
        anchors.fill: parent
        enabled: !lcsTools.bussy && !window.criticalFail
        rowHeight: lcsToolBar.height

        onModRemoved: lcsTools.deleteMod(fileName)

        onModExport: {
            lcsDialogSaveZipFantome.modName = fileName
            lcsDialogSaveZipFantome.open()
            lcsDialogSaveZipFantome.currentFile = lcsDialogSaveZipFantome.folder + "/" + fileName + ".zip"
        }

        onModEdit: lcsTools.startEditMod(fileName)

    }

    statusBar: LCSStatusBar {
        id: lcsStatusBar
        isBussy: lcsTools.state !== LCSTools.StateIdle
        statusMessage: lcsTools.status
        visible: isBussy
    }

    LCSDialogOpenZipFantome {
        id: lcsDialogOpenZipFantome
        onAccepted: lcsTools.installFantomeZip(file.toString().replace("file:///", ""))
    }

    LCSDialogSaveZipFantome {
        id: lcsDialogSaveZipFantome
        onAccepted: lcsTools.exportMod(modName, file.toString().replace("file:///", ""))
    }

    LCSDialogNewMod {
        id: lcsDialogNewMod
        onSave: lcsTools.makeMod(fileName, image, infoData, items)
    }

    LCSDialogEditMod {
        id: lcsDialogEditMod
        visible: false
        isBussy: lcsTools.state !== LCSTools.StateIdle

        onChangeInfoData: lcsTools.changeModInfo(fileName, infoData)
        onChangeImage: lcsTools.changeModImage(fileName, image)
        onRemoveImage: lcsTools.removeModImage(fileName)
        onRemoveWads: lcsTools.removeModWads(fileName, wads)
        onAddWads: lcsTools.addModWads(fileName, wads)
    }

    LCSDialogNewProfile {
        id: lcsDialogNewProfile
        onAccepted: {
            for(let i = 0; i < window.savedProfiles.length; i++) {
                if (text.toLowerCase() === window.savedProfiles[i]["Name"].toLowerCase()) {
                    logError("Bad profile", "This profile already exists!")
                    return
                }
            }
            let index = lcsToolBar.profilesCurrentIndex
            window.savedProfiles[window.savedProfiles.length] = {
                "Name": text,
                "Mods": {}
            }
            window.savedProfiles = window.savedProfiles
            lcsToolBar.profilesCurrentIndex = index
        }
    }

    LCSDialogLoLPath {
        id: lcsDialogLolPath
        folder: lcsTools.leaguePath != "" ? "file:///" + lcsTools.leaguePath : ""
        onAccepted:  {
            lcsTools.leaguePath = folder.toString().replace("file:///", "");
        }
    }

    LCSDialogLog {
        id: lcsDialogLog
        width: 640
        height: 480
        visible: false
    }

    LCSDialogError {
        id: lcsDialogError
    }

    LCSDialogWarning {
        id: lcsDialogWarning
    }

    LCSTools {
        id: lcsTools

        onStatusChanged: logInfo("Status changed", status)
        onProgressStart: lcsStatusBar.start(itemsTotal, dataTotal)
        onProgressItems: lcsStatusBar.updateItem(itemsDone)
        onProgressData: lcsStatusBar.updateData(dataDone)
        onProgressEnd: lcsStatusBar.isCopying = false

        onInitialized: {
            let index = lcsToolBar.profilesCurrentIndex
            let profileMods = window.savedProfiles[index]["Mods"]
            for(let fileName in mods) {
                lcsModsView.addMod(fileName, mods[fileName], profileMods[fileName])
            }
            if(lcsTools.leaguePath == "") {
                lcsDialogLolPath.open()
            }
        }
        onModDeleted: {}
        onInstalledMod: lcsModsView.addMod(fileName, infoData, false)
        onProfileSaved: {
            let index = lcsToolBar.profilesCurrentIndex
            window.savedProfiles[index] = {
                "Name": name,
                "Mods": mods,
            }
            window.savedProfiles = window.savedProfiles
            lcsToolBar.profilesCurrentIndex = index
        }
        onProfileDeleted: {
            let index = lcsToolBar.profilesCurrentIndex
            if (index > 0) {
                window.savedProfiles.splice(index, 1)
                window.savedProfiles = window.savedProfiles
                lcsToolBar.profilesCurrentIndex = index - 1
            } else {
                lcsModsView.clearEnabled()
                window.savedProfiles[index] = {
                    "Name": "Default",
                    "Mods": [],
                }
                window.savedProfiles = window.savedProfiles
                lcsToolBar.profilesCurrentIndex = index
            }
        }
        onUpdateProfileStatus: lcsDialogLog.log(message)
        onModEditStarted: {
            lcsDialogEditMod.load(fileName, infoData, image, wads)
            lcsDialogEditMod.open()
        }
        onModInfoChanged: {
            lcsModsView.updateModInfo(fileName, infoData)
            lcsDialogEditMod.infoDataChanged(infoData)
        }
        onModImageChanged: lcsDialogEditMod.imageChanged(image)
        onModImageRemoved: lcsDialogEditMod.imageRemoved()
        onModWadsAdded: lcsDialogEditMod.wadsAdded(wads)
        onModWadsRemoved: lcsDialogEditMod.wadsRemoved(wads)

        onReportError:  {
            logError(category, message)
            lcsStatusBar.isCopying = false
        }
        onReportWarning: {
            logError(category, message)
            lcsStatusBar.isCopying = false
        }
    }

    Component.onCompleted: {
        lcsTools.init(savedProfiles)
    }
}
