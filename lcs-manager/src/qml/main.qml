import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import Qt.labs.settings 1.0
import lolcustomskin.tools 1.0
import QtQuick.Controls.Material 2.12


ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("LolCustomSkin Manager - " + LCS_VERSION)

    Settings {
        id: settings
        property alias leaguePath: lcsTools.leaguePath
        property alias logVisible: lcsDialogLog.visible

        property alias blacklist: lcsMainMenu.blacklist
        property alias ignorebad: lcsMainMenu.ignorebad
        property alias disableUpdates: lcsMainMenu.disableUpdates
        property alias themeDarkMode: lcsMainMenu.themeDarkMode
        property alias themePrimaryColor: lcsMainMenu.themePrimaryColor
        property alias themeAccentColor: lcsMainMenu.themeAccentColor

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

    Material.theme: lcsMainMenu.themeDarkMode ? Material.Dark : Material.Light
    Material.primary: lcsMainMenu.colors_LIST[lcsMainMenu.themePrimaryColor]
    Material.accent: lcsMainMenu.colors_LIST[lcsMainMenu.themeAccentColor]

    property bool isBussy: lcsTools.state !== LCSTools.StateIdle
    property string log_data: "[INFO] Version: " + LCS_VERSION + "\n"

    function logClear() {
        log_data = "[INFO] Version: " + LCS_VERSION + "\n"
    }

    function logInfo(name, message) {
        log_data += "[INFO] " + name + ": " + message + "\n"
    }

    function logUserError(name, error) {
        log_data += "[Warning] " + name + ": " + error + "\n"
        lcsDialogUserError.text = error
        lcsDialogUserError.open()
    }

    function logError(name, error) {
        log_data += "[Error] " + name + ": " + error + "\n"
        lcsDialogError.text = name
        lcsDialogError.open()
    }


    header: LCSToolBar {
        id: lcsToolBar
        isBussy: window.isBussy
        patcherRunning: lcsTools.state === LCSTools.StateRunning

        profilesModel: [ "Default Profile" ]

        onOpenSideMenu: lcsMainMenu.open()

        onSaveProfileAndRun:  {
            let name = lcsToolBar.profilesCurrentName
            let mods = lcsModsView.saveProfile()
            lcsTools.saveProfile(name, mods, run)
        }

        onStopProfile: lcsTools.stopProfile()

        onLoadProfile: lcsTools.loadProfile(lcsToolBar.profilesCurrentName)

        onNewProfile: lcsDialogNewProfile.open()

        onRemoveProfile: lcsTools.deleteProfile(lcsToolBar.profilesCurrentName)

    }

    LCSSideMenu {
        id: lcsMainMenu

        onInstallFantomeZip: lcsDialogOpenZipFantome.open()

        onCreateNewMod: {
            lcsDialogNewMod.clear()
            lcsDialogNewMod.open()
        }

        onChangeGamePath: lcsDialogLolPath.open()

        onBlacklistChanged: lcsTools.changeBlacklist(blacklist)

        onIgnorebadChanged: lcsTools.changeIgnorebad(ignorebad)

        onShowLogs: if (!lcsDialogLog.visible) lcsDialogLog.visible = true

        onExit: Qt.quit()
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
            lcsDialogSaveZipFantome.currentFile = lcsDialogSaveZipFantome.folder + "/" + fileName + ".fantome"
        }

        onModEdit: lcsTools.startEditMod(fileName)

        onImportFile: lcsTools.installFantomeZip(file)
    }

    footer: LCSStatusBar {
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

    LCSDialogErrorUser {
        id: lcsDialogUserError
    }

    LCSDialogUpdate {
        id: lcsDialogUpdate
        disableUpdates: settings.disableUpdates
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
        onUpdateProfileStatus: log_data += message
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
        lcsDialogUpdate.checkForUpdates()
    }
}
