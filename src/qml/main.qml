import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.settings 1.0
import customskinlol.tools 1.0
import QtQuick.Controls.Material 2.15


ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 640
    minimumHeight: 640
    minimumWidth: 640
    title: qsTr("CustomSkin for LoL - " + CSLOL_VERSION)

    Settings {
        id: settings
        property alias leaguePath: cslolTools.leaguePath
        property alias logVisible: cslolDialogLog.visible

        property alias blacklist: cslolDialogSettings.blacklist
        property alias ignorebad: cslolDialogSettings.ignorebad
        property alias suppressInstallConflicts: cslolDialogSettings.suppressInstallConflicts
        property alias disableUpdates: cslolDialogSettings.disableUpdates
        property alias themeDarkMode: cslolDialogSettings.themeDarkMode
        property alias themePrimaryColor: cslolDialogSettings.themePrimaryColor
        property alias themeAccentColor: cslolDialogSettings.themeAccentColor
        property alias statsUrl: cslolDialogUpdate.stats_url

        property alias removeUnknownNames: cslolDialogEditMod.removeUnknownNames
        property alias lastZipDirectory: cslolDialogOpenZipFantome.folder

        property alias windowHeight: window.height
        property alias windowWidth: window.width
        property bool windowMaximised
        property alias logHeight: cslolDialogLog.height
        property alias logWidth: cslolDialogLog.width

        fileName: "config.ini"
    }

    property var validName: new RegExp(/[\p{L}\p{M}\p{Z}\p{N}\w]{3,50}/u)
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

    Material.theme: cslolDialogSettings.themeDarkMode ? Material.Dark : Material.Light
    Material.primary: cslolDialogSettings.colors_LIST[cslolDialogSettings.themePrimaryColor]
    Material.accent: cslolDialogSettings.colors_LIST[cslolDialogSettings.themeAccentColor]

    property bool isBussy: cslolTools.state !== CSLOLTools.StateIdle
    property string log_data: "[INFO] Version: " + CSLOL_VERSION + "\n"

    function logClear() {
        log_data = "[INFO] Version: " + CSLOL_VERSION + "\n"
    }
    function logInfo(name, message) {
        log_data += "[INFO] " + name + ": " + message + "\n"
    }
    function logUserError(name, message) {
        log_data += "[Warning] " + name + ": " + message + "\n"
        cslolDialogUserError.text = message
        cslolDialogUserError.open()
    }
    function logError(name, message, stack_trace) {
        log_data += "[ERROR] " + name + ": " + message + "\n"
        if (stack_trace) {
            log_data += stack_trace.trim() + "\n"
        }
        cslolDialogError.name = name
        cslolDialogError.message = message
        cslolDialogError.open()
    }
    function checkGamePath() {
        if (cslolTools.leaguePath === "") {
            cslolDialogGame.open();
            return false;
        }
        return true;
    }


    header: CSLOLToolBar {
        id: cslolToolBar
        isBussy: window.isBussy
        patcherRunning: cslolTools.state === CSLOLTools.StateRunning

        profilesModel: [ "Default Profile" ]

        onOpenSideMenu: cslolDialogSettings.open()

        onSaveProfileAndRun: function(run) {
            let name = cslolToolBar.profilesCurrentName
            let mods = cslolModsView.saveProfile()
            if (checkGamePath()) {
                cslolTools.saveProfile(name, mods, run, settings.suppressInstallConflicts)
            }
        }

        onStopProfile: function() {
            cslolTools.stopProfile()
        }

        onLoadProfile: function() {
            cslolTools.loadProfile(cslolToolBar.profilesCurrentName)
        }

        onNewProfile: function() {
            cslolDialogNewProfile.open()
        }

        onRemoveProfile: function() {
            cslolTools.deleteProfile(cslolToolBar.profilesCurrentName)
        }
    }

    CSLOLDialogSettings {
        id: cslolDialogSettings

        onChangeGamePath: function() {
            cslolDialogGame.open()
        }

        onBlacklistChanged: function() {
            cslolTools.changeBlacklist(blacklist)
        }

        onIgnorebadChanged: function() {
            cslolTools.changeIgnorebad(ignorebad)
        }

        onShowLogs: function() {
            if (!cslolDialogLog.visible) {
                cslolDialogLog.visible = true
            }
        }
    }

    CSLOLModsView {
        id: cslolModsView
        anchors.fill: parent
        isBussy: window.isBussy
        rowHeight: cslolToolBar.height
        columnCount: Math.max(1, Math.floor(window.width / window.minimumWidth))

        onModRemoved: function(fileName) {
            cslolTools.deleteMod(fileName)
        }
        onModExport: function(fileName) {
            if (checkGamePath()) {
                cslolDialogSaveZipFantome.modName = fileName
                cslolDialogSaveZipFantome.currentFiles = [ "file:///" + fileName + ".fantome" ]
                cslolDialogSaveZipFantome.open()
            }
        }
        onModEdit: function(fileName) {
            cslolTools.startEditMod(fileName)
        }
        onImportFile: function(file) {
            if (checkGamePath()) {
                cslolTools.installFantomeZip(file)
            }
        }
        onInstallFantomeZip: function() {
            if (checkGamePath()) {
                cslolDialogOpenZipFantome.open()
            }
        }
        onCreateNewMod: {
            if (checkGamePath()) {
                cslolDialogNewMod.open()
                cslolDialogNewMod.clear()
            }
        }
    }

    footer: CSLOLStatusBar {
        id: cslolStatusBar
        statusMessage: cslolTools.status
        visible: window.isBussy
    }

    CSLOLDialogOpenZipFantome {
        id: cslolDialogOpenZipFantome
        onAccepted: function() {
            cslolTools.installFantomeZip(CSLOLUtils.fromFile(file))
        }
    }

    CSLOLDialogSaveZipFantome {
        id: cslolDialogSaveZipFantome
        onAccepted: function() {
            cslolTools.exportMod(modName, CSLOLUtils.fromFile(file))
        }
    }

    CSLOLDialogNewMod {
        id: cslolDialogNewMod
        enabled: !isBussy
        onSave: function(fileName, infoData, image) {
            cslolTools.makeMod(fileName, infoData, image)
        }
    }

    CSLOLDialogEditMod {
        id: cslolDialogEditMod
        visible: false
        enabled: !isBussy

        onChangeInfoData: function(infoData, image) {
            cslolTools.changeModInfo(fileName, infoData, image)
        }
        onRemoveWads: function(wads) {
            cslolTools.removeModWads(fileName, wads)
        }
        onAddWad: function(wad, removeUnknownNames) {
            if (checkGamePath()) {
                cslolTools.addModWad(fileName, wad, removeUnknownNames)
            }
        }
    }

    CSLOLDialogNewProfile {
        id: cslolDialogNewProfile
        enabled: !isBussy
        onAccepted: {
            for(let i in  cslolToolBar.profilesModel) {
                if (text.toLowerCase() ===  cslolToolBar.profilesModel[i].toLowerCase()) {
                    logError("Bad profile", "This profile already exists!")
                    return
                }
            }
            let index =  cslolToolBar.profilesModel.length
            cslolToolBar.profilesModel[index] = text
            cslolToolBar.profilesModel = cslolToolBar.profilesModel
            cslolToolBar.profilesCurrentIndex = index
        }
    }

    CSLOLDialogLog {
        id: cslolDialogLog
        width: 640
        height: 480
        visible: false
    }

    CSLOLDialogGame {
        id: cslolDialogGame
        onSelected: function(orgPath) {
            let path = CSLOLUtils.checkGamePath(orgPath)
            if (path === "") {
                window.logUserError("Bad game directory",  "There is no \"League of Legends.exe\" in " + orgPath)
            } else {
                cslolTools.leaguePath = path
                cslolDialogGame.close()
            }
        }
    }

    CSLOLDialogError {
        id: cslolDialogError
    }

    CSLOLDialogErrorUser {
        id: cslolDialogUserError
    }

    CSLOLDialogUpdate {
        id: cslolDialogUpdate
        disableUpdates: settings.disableUpdates
    }

    CSLOLTools {
        id: cslolTools

        onStatusChanged: function(status) {
            logInfo("Status changed", status)
        }
        onProcessLog: function(line) {
            line = line.trim();
            if (line) {
                log_data += "[EXTERNAL] " + line + "\n";
            }
        }
        onInitialized: function(mods, profiles, profileName, profileMods) {
            cslolToolBar.profilesModel = profiles
            cslolToolBar.profilesCurrentIndex = 0
            for(let i in profiles) {
                if (profiles[i] === profileName) {
                    cslolToolBar.profilesCurrentIndex = i
                    break
                }
            }
            for(let fileName in mods) {
                cslolModsView.addMod(fileName, mods[fileName], fileName in profileMods)
            }
            if(cslolTools.leaguePath == "") {
                cslolDialogGame.open()
            }
        }
        onModDeleted: {}
        onInstalledMod: function(fileName, infoData) {
            cslolModsView.addMod(fileName, infoData, false)
        }
        onProfileSaved: {}
        onProfileLoaded: function(name, profileMods) {
            cslolModsView.loadProfile(profileMods)
        }
        onProfileDeleted: {
            let index = cslolToolBar.profilesCurrentIndex
            if (cslolToolBar.profilesModel.length > 1) {
                cslolToolBar.profilesModel.splice(index, 1)
                cslolToolBar.profilesModel = cslolToolBar.profilesModel
            } else {
                cslolToolBar.profilesModel = [ "Default Profile" ]
            }
            if (index > 0) {
                cslolToolBar.profilesCurrentIndex = index - 1
            } else {
                cslolToolBar.profilesCurrentIndex = 0
            }
        }
        onUpdateProfileStatus: function(message) {
            log_data += message
        }
        onModCreated: function(fileName, infoData, image) {
            cslolModsView.addMod(fileName, infoData, false)
            cslolDialogEditMod.load(fileName, infoData, image, [], true)
            cslolDialogEditMod.open()
        }
        onModEditStarted: function(fileName, infoData, image, wads) {
            cslolDialogEditMod.load(fileName, infoData, image, wads, false)
            cslolDialogEditMod.open()
        }
        onModInfoChanged: function(fileName, infoData, image) {
            cslolModsView.updateModInfo(fileName, infoData)
            cslolDialogEditMod.infoDataChanged(infoData, image)
        }
        onModWadsAdded: function(fileName, wads) {
            cslolDialogEditMod.wadsAdded(wads)
        }
        onModWadsRemoved: function(fileName, wads) {
            cslolDialogEditMod.wadsRemoved(wads)
        }
        onReportError: function(name, message, stack_trace) {
            logError(name, message, stack_trace)
        }
    }

    Component.onCompleted: {
        if (settings.windowMaximised) {
            if (window.visibility !== ApplicationWindow.Maximized) {
                window.visibility = ApplicationWindow.Maximized;
            }
        }
        firstTick = true;
        cslolTools.init()
        cslolDialogUpdate.checkForUpdates()
        cslolDialogUpdate.checkForStats()
    }
}
