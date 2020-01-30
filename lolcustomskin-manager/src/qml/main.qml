import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0
import Qt.labs.platform 1.0
import lolcustomskin.tools 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("LolCustomSkin Manager")

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

    property bool isBussy: lcsTools.state !== LCSTools.StateIdle

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
        isBussy: window.isBussy
        patcherRunning: lcsTools.state === LCSTools.StateRunning

        profilesModel: [ "Default Profile" ]

        onInstallFantomeZip: lcsDialogOpenZipFantome.open()

        onCreateNewMod: {
            lcsDialogNewMod.clear()
            lcsDialogNewMod.open()
        }

        onChangeGamePath: lcsDialogLolPath.open()

        onShowLogs: if (!lcsDialogLog.visible) lcsDialogLog.visible = true

        onExit: window.exit()

        onSaveProfileAndRun:  {
            let name = lcsToolBar.profilesCurrentName
            let mods = lcsModsView.saveProfile()
            lcsTools.saveProfile(name, mods, run, lcsToolBar.wholeMerge)
        }

        onStopProfile: lcsTools.stopProfile()

        onLoadProfile: lcsTools.loadProfile(lcsToolBar.profilesCurrentName)

        onNewProfile: lcsDialogNewProfile.open()

        onRemoveProfile: lcsTools.deleteProfile(lcsToolBar.profilesCurrentName)

    }

    LCSModsView {
        id: lcsModsView
        anchors.fill: parent
        isBussy: window.isBussy
        rowHeight: lcsToolBar.height

        onModRemoved: lcsTools.deleteMod(fileName)

        onModExport: {
            lcsDialogSaveZipFantome.modName = fileName
            lcsDialogSaveZipFantome.open()
            lcsDialogSaveZipFantome.currentFile = lcsDialogSaveZipFantome.folder + "/" + fileName + ".zip"
        }

        onModEdit: lcsTools.startEditMod(fileName)

        onImportFile: lcsTools.installFantomeZip(file)
    }

    statusBar: LCSStatusBar {
        id: lcsStatusBar
        isBussy: window.isBussy
        statusMessage: lcsTools.status
        visible: window.isBussy
    }

    LCSDialogOpenZipFantome {
        id: lcsDialogOpenZipFantome
        onAccepted: lcsTools.installFantomeZip(lcsTools.fromFile(file))
    }

    LCSDialogSaveZipFantome {
        id: lcsDialogSaveZipFantome
        onAccepted: lcsTools.exportMod(modName, lcsTools.fromFile(file))
    }

    LCSDialogNewMod {
        id: lcsDialogNewMod
        onSave: lcsTools.makeMod(fileName, image, infoData, items)
    }

    LCSDialogEditMod {
        id: lcsDialogEditMod
        visible: false
        isBussy: window.isBussy

        onChangeInfoData: lcsTools.changeModInfo(fileName, infoData)
        onChangeImage: lcsTools.changeModImage(fileName, image)
        onRemoveImage: lcsTools.removeModImage(fileName)
        onRemoveWads: lcsTools.removeModWads(fileName, wads)
        onAddWads: lcsTools.addModWads(fileName, wads)
    }

    LCSDialogNewProfile {
        id: lcsDialogNewProfile
        onAccepted: {
            for(let i in  lcsToolBar.profilesModel) {
                if (text.toLowerCase() ===  lcsToolBar.profilesModel[i].toLowerCase()) {
                    logError("Bad profile", "This profile already exists!")
                    return
                }
            }
            let index =  lcsToolBar.profilesModel.length
            lcsToolBar.profilesModel[index] = text
            lcsToolBar.profilesModel = lcsToolBar.profilesModel
            lcsToolBar.profilesCurrentIndex = index
        }
    }

    LCSDialogLoLPath {
        id: lcsDialogLolPath
        folder: lcsTools.toFile(lcsTools.leaguePath)
        onAccepted: lcsTools.leaguePath = lcsTools.fromFile(folder)
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
            lcsToolBar.profilesModel = profiles
            lcsToolBar.profilesCurrentIndex = 0
            for(let i in profiles) {
                if (profiles[i] === profileName) {
                    lcsToolBar.profilesCurrentIndex = i
                    break
                }
            }
            for(let fileName in mods) {
                lcsModsView.addMod(fileName, mods[fileName], fileName in profileMods)
            }
            if(lcsTools.leaguePath == "") {
                lcsDialogLolPath.open()
            }
        }
        onModDeleted: {}
        onInstalledMod: lcsModsView.addMod(fileName, infoData, false)
        onProfileSaved: {}
        onProfileLoaded: lcsModsView.loadProfile(profileMods)
        onProfileDeleted: {
            let index = lcsToolBar.profilesCurrentIndex
            if (lcsToolBar.profilesModel.length > 1) {
                lcsToolBar.profilesModel.splice(index, 1)
                lcsToolBar.profilesModel = lcsToolBar.profilesModel
            } else {
                lcsToolBar.profilesModel = [ "Default Profile" ]
            }
            if (index > 0) {
                lcsToolBar.profilesCurrentIndex = index - 1
            } else {
                lcsToolBar.profilesCurrentIndex = 0
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
        lcsTools.init()
    }
}
