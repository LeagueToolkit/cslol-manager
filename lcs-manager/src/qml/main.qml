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
        property alias suppressInstallConflicts: lcsDialogSettings.suppressInstallConflicts
        property alias disableUpdates: lcsDialogSettings.disableUpdates
        property alias themeDarkMode: lcsDialogSettings.themeDarkMode
        property alias themePrimaryColor: lcsDialogSettings.themePrimaryColor
        property alias themeAccentColor: lcsDialogSettings.themeAccentColor

        property alias lastZipDirectory: lcsDialogOpenZipFantome.folder

        property alias windowHeight: window.height
        property alias windowWidth: window.width
        property bool windowMaximised
        property alias logHeight: lcsDialogLog.height
        property alias logWidth: lcsDialogLog.width

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
    function logError(category, stack_trace, message) {
        log_data += "[Error] " + category + ": " + stack_trace + "\n"
        lcsDialogError.category = category
        lcsDialogError.message = message
        lcsDialogError.open()
    }
    function checkGamePath() {
        if (lcsTools.leaguePath === "") {
            lcsDialogGame.open();
            return false;
        }
        return true;
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
            if (checkGamePath()) {
                lcsTools.saveProfile(name, mods, run, settings.suppressInstallConflicts)
            }
        }

        onStopProfile: function() {
            lcsTools.stopProfile()
        }

        onLoadProfile: function() {
            lcsTools.loadProfile(lcsToolBar.profilesCurrentName)
        }

        onNewProfile: function() {
            lcsDialogNewProfile.open()
        }

        onRemoveProfile: function() {
            lcsTools.deleteProfile(lcsToolBar.profilesCurrentName)
        }

    }

    LCSDialogSettings {
        id: lcsDialogSettings

        onChangeGamePath: function() {
            lcsDialogGame.open()
        }

        onBlacklistChanged: function() {
            lcsTools.changeBlacklist(blacklist)
        }

        onIgnorebadChanged: function() {
            lcsTools.changeIgnorebad(ignorebad)
        }

        onShowLogs: function() {
            if (!lcsDialogLog.visible) {
                lcsDialogLog.visible = true
            }
        }
    }

    LCSModsView {
        id: lcsModsView
        anchors.fill: parent
        isBussy: window.isBussy
        rowHeight: lcsToolBar.height
        columnCount: Math.max(1, Math.floor(window.width / window.minimumWidth))

        onModRemoved: function(fileName) {
            lcsTools.deleteMod(fileName)
        }
        onModExport: function(fileName) {
            if (checkGamePath()) {
                lcsDialogSaveZipFantome.modName = fileName
                lcsDialogSaveZipFantome.currentFiles = [ "file:///" + fileName + ".fantome" ]
                lcsDialogSaveZipFantome.open()
            }
        }
        onModEdit: function(fileName) {
            lcsTools.startEditMod(fileName)
        }
        onImportFile: function(files) {
            if (checkGamePath()) {
                lcsTools.installFantomeZip(files)
            }
        }
        onInstallFantomeZip: function() {
            if (checkGamePath()) {
                lcsDialogOpenZipFantome.open()
            }
        }
        onCreateNewMod: {
            if (checkGamePath()) {
                lcsDialogNewMod.open()
                lcsDialogNewMod.clear()
            }
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
        onAccepted: function() {
            lcsTools.installFantomeZip([ LCSUtils.fromFile(file) ])
        }
    }

    LCSDialogSaveZipFantome {
        id: lcsDialogSaveZipFantome
        onAccepted: function() {
            lcsTools.exportMod(modName, LCSUtils.fromFile(file))
        }
    }

    LCSDialogNewMod {
        id: lcsDialogNewMod
        enabled: !isBussy
        onSave: function(fileName, infoData, image) {
            lcsTools.makeMod(fileName, infoData, image)
        }
    }

    LCSDialogEditMod {
        id: lcsDialogEditMod
        visible: false
        enabled: !isBussy

        onChangeInfoData: function(infoData, image) {
            lcsTools.changeModInfo(fileName, infoData, image)
        }
        onRemoveWads: function(wads) {
            lcsTools.removeModWads(fileName, wads)
        }
        onAddWads: function(wads, removeUnknownNames) {
            if (checkGamePath()) {
                lcsTools.addModWads(fileName, wads, removeUnknownNames)
            }
        }
    }

    LCSDialogNewProfile {
        id: lcsDialogNewProfile
        enabled: !isBussy
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

    LCSDialogLog {
        id: lcsDialogLog
        width: 640
        height: 480
        visible: false
    }

    LCSDialogGame {
        id: lcsDialogGame
        onSelected: function(orgPath) {
            let path = LCSUtils.checkGamePath(orgPath)
            if (path === "") {
                window.logUserError("Bad game directory",  "There is no \"League of Legends.exe\" in " + orgPath)
            } else {
                lcsTools.leaguePath = path
                lcsDialogGame.close()
            }
        }
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
                lcsDialogGame.open()
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
        onModCreated: function(fileName, infoData, image) {
            lcsModsView.addMod(fileName, infoData, false)
            lcsDialogEditMod.load(fileName, infoData, image, [], true)
            lcsDialogEditMod.open()
        }
        onModEditStarted: function(fileName, infoData, image, wads) {
            lcsDialogEditMod.load(fileName, infoData, image, wads, false)
            lcsDialogEditMod.open()
        }
        onModInfoChanged: function(fileName, infoData, image) {
            lcsModsView.updateModInfo(fileName, infoData)
            lcsDialogEditMod.infoDataChanged(infoData, image)
        }
        onModWadsAdded: function(fileName, wads) {
            lcsDialogEditMod.wadsAdded(wads)
        }
        onModWadsRemoved: function(fileName, wads) {
            lcsDialogEditMod.wadsRemoved(wads)
        }
        onReportError: function(category, stack_trace, message) {
            logError(category, stack_trace, message)
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
