import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.settings 1.0
import lolcustomskin.tools 1.0
import QtQuick.Controls.Material 2.15


ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 640
    minimumHeight: 640
    minimumWidth: 640
    title: qsTr("LolCustomSkin Manager - " + LCS_VERSION)

    Settings {
        id: settings
        property alias leaguePath: lcsTools.leaguePath
        property alias logVisible: lcsDialogLog.visible

        property alias blacklist: lcsDialogSettings.blacklist
        property alias ignorebad: lcsDialogSettings.ignorebad
        property alias disableUpdates: lcsDialogSettings.disableUpdates
        property alias themeDarkMode: lcsDialogSettings.themeDarkMode
        property alias themePrimaryColor: lcsDialogSettings.themePrimaryColor
        property alias themeAccentColor: lcsDialogSettings.themeAccentColor

        property alias lastZipDirectory: lcsDialogOpenZipFantome.folder
        property alias lastImageFolder: lcsDialogNewMod.lastImageFolder
        property alias lastWadFileFolder: lcsDialogNewMod.lastWadFileFolder
        property alias lastRawFolder: lcsDialogNewMod.lastRawFolder

        property alias windowHeight: window.height
        property alias windowWidth: window.width
        property bool windowMaximised
        property alias logHeight: lcsDialogLog.height
        property alias logWidth: lcsDialogLog.width

        fileName: "config.ini"
    }

    property bool firstTick: false
    onVisibilityChanged: {
        if (firstTick) {
            if (window.visibility === ApplicationWindow.Maximized) {
                if (settings.windowMaximised !== true) {
                    settings.windowMaximised = true;
                }
            }
            if (window.visibility === ApplicationWindow.Windowed) {
                if (settings.windowMaximised !== false) {
                    settings.windowMaximised = false;
                }
            }
        }
    }

    Material.theme: lcsDialogSettings.themeDarkMode ? Material.Dark : Material.Light
    Material.primary: lcsDialogSettings.colors_LIST[lcsDialogSettings.themePrimaryColor]
    Material.accent: lcsDialogSettings.colors_LIST[lcsDialogSettings.themeAccentColor]

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

        onOpenSideMenu: lcsDialogSettings.open()

        onSaveProfileAndRun: function(run) {
            let name = lcsToolBar.profilesCurrentName
            let mods = lcsModsView.saveProfile()
            lcsTools.saveProfile(name, mods, run)
        }

        onStopProfile: lcsTools.stopProfile()

        onLoadProfile: lcsTools.loadProfile(lcsToolBar.profilesCurrentName)

        onNewProfile: lcsDialogNewProfile.open()

        onRemoveProfile: lcsTools.deleteProfile(lcsToolBar.profilesCurrentName)

    }

    LCSDialogSettings {
        id: lcsDialogSettings

        onChangeGamePath: lcsDialogLolPath.open()

        onBlacklistChanged: lcsTools.changeBlacklist(blacklist)

        onIgnorebadChanged: lcsTools.changeIgnorebad(ignorebad)

        onShowLogs: if (!lcsDialogLog.visible) lcsDialogLog.visible = true
    }

    LCSModsView {
        id: lcsModsView
        anchors.fill: parent
        isBussy: window.isBussy
        rowHeight: lcsToolBar.height

        onModRemoved: function(fileName) {
            lcsTools.deleteMod(fileName)
        }

        onModExport: function(fileName) {
            lcsDialogSaveZipFantome.modName = fileName
            lcsDialogSaveZipFantome.open()
            lcsDialogSaveZipFantome.currentFile = lcsDialogSaveZipFantome.folder + "/" + fileName + ".fantome"
        }
        onModEdit: function(fileName) {
            lcsTools.startEditMod(fileName)
        }

        onImportFile: function(file) {
            lcsTools.installFantomeZip(file)
        }
        onInstallFantomeZip: lcsDialogOpenZipFantome.open()
        onCreateNewMod: {
            lcsDialogNewMod.clear()
            lcsDialogNewMod.open()
        }
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
        onSave: function(fileName, image, infoData, items) {
            lcsTools.makeMod(fileName, image, infoData, items)
        }
    }

    LCSDialogEditMod {
        id: lcsDialogEditMod
        visible: false
        isBussy: window.isBussy

        onChangeInfoData: function(infoData) {
            lcsTools.changeModInfo(fileName, infoData)
        }
        onChangeImage: function(image) {
            lcsTools.changeModImage(fileName, image)
        }
        onRemoveImage: function() {
            lcsTools.removeModImage(fileName)
        }
        onRemoveWads: function(wads) {
            lcsTools.removeModWads(fileName, wads)
        }
        onAddWads: function(wads) {
            lcsTools.addModWads(fileName, wads)
        }
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

        onStatusChanged: function(status) {
            logInfo("Status changed", status)
        }
        onProgressStart: function(itemsTotal, dataTotal) {
            lcsStatusBar.start(itemsTotal, dataTotal)
        }
        onProgressItems: function(itemsDone) {
            lcsStatusBar.updateItem(itemsDone)
        }
        onProgressData: function(dataDone) {
            lcsStatusBar.updateData(dataDone)
        }
        onProgressEnd: function() {
            lcsStatusBar.isCopying = false
        }
        onInitialized: function(mods, profiles, profileName, profileMods) {
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
        onInstalledMod: function(fileName, infoData) {
            lcsModsView.addMod(fileName, infoData, false)
        }
        onProfileSaved: {}
        onProfileLoaded: function(name, profileMods) {
            lcsModsView.loadProfile(profileMods)
        }
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
        onUpdateProfileStatus: function(message) {
            log_data += message
        }
        onModEditStarted: function(fileName, infoData, image, wads) {
            lcsDialogEditMod.load(fileName, infoData, image, wads)
            lcsDialogEditMod.open()
        }
        onModInfoChanged: function(fileName, infoData) {
            lcsModsView.updateModInfo(fileName, infoData)
            lcsDialogEditMod.infoDataChanged(infoData)
        }
        onModImageChanged: function(fileName, image) {
            lcsDialogEditMod.imageChanged(image)
        }
        onModImageRemoved: function(fileName) {
            lcsDialogEditMod.imageRemoved()
        }
        onModWadsAdded: function(fileName, wads) {
            lcsDialogEditMod.wadsAdded(wads)
        }
        onModWadsRemoved: function(fileName, wads) {
            lcsDialogEditMod.wadsRemoved(wads)
        }
        onReportError: function(category, message) {
            logError(category, message)
            lcsStatusBar.isCopying = false
        }
        onReportWarning: function(category, message) {
            logError(category, message)
            lcsStatusBar.isCopying = false
        }
    }

    Component.onCompleted: {
        if (settings.windowMaximised) {
            if (window.visibility !== ApplicationWindow.Maximized) {
                window.visibility = ApplicationWindow.Maximized;
            }
        }
        firstTick = true;
        lcsTools.init()
        lcsDialogUpdate.checkForUpdates()
    }
}
