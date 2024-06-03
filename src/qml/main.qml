import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import Qt.labs.settings 1.0
import Qt.labs.platform 1.0
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

        property alias detectGamePath: cslolDialogSettings.detectGamePath
        property alias blacklist: cslolDialogSettings.blacklist
        property alias ignorebad: cslolDialogSettings.ignorebad
        property alias suppressInstallConflicts: cslolDialogSettings.suppressInstallConflicts
        property alias enableUpdates: cslolDialogSettings.enableUpdates
        property alias enableAutoRun: cslolDialogSettings.enableAutoRun
        property alias enableSystray: cslolDialogSettings.enableSystray
        property alias themeDarkMode: cslolDialogSettings.themeDarkMode
        property alias themePrimaryColor: cslolDialogSettings.themePrimaryColor
        property alias themeAccentColor: cslolDialogSettings.themeAccentColor
        property alias verbosePatcher: cslolDialogSettings.debugPatcher

        property alias removeUnknownNames: cslolDialogEditMod.removeUnknownNames
        property alias lastZipDirectory: cslolDialogOpenZipFantome.folder

        property alias windowHeight: window.height
        property alias windowWidth: window.width
        property bool windowMaximised
        property alias lastUpdateUTCMinutes: cslolDialogUpdate.lastUpdateUTCMinutes

        fileName: "config.ini"
    }

    property bool patcherRunning: cslolTools.state === CSLOLTools.StateRunning
    property bool isBussy: cslolTools.state !== CSLOLTools.StateIdle
    property var validName: new RegExp(/[\p{L}\p{M}\p{Pd}\p{Z}\p{N}\w]{3,50}/u)
    property var validVersion: new RegExp(/([0-9]{1,3})(\.[0-9]{1,3}){0,3}/)
    property var validUrl: new RegExp(/^(http(s)?:\/\/).+$/u)
    property bool firstTick: false

    function showUserError(name, message) {
        cslolDialogUserError.text = message
        cslolDialogUserError.open()
    }

    function checkGamePath() {
        let detected = settings.detectGamePath ? CSLOLUtils.detectGamePath() : "";
        if (detected === "" && cslolTools.leaguePath === "") {
            cslolDialogGame.open();
            return false;
        }
        if (detected !== "") {
            cslolTools.leaguePath = detected;
        }
        return true;
    }

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

    header: CSLOLToolBar {
        id: cslolToolBar
        isBussy: window.isBussy

        profilesModel: [ "Default Profile" ]

        onOpenSideMenu: cslolDialogSettings.open()

        onSaveProfileAndRun: function(run) {
            let name = cslolToolBar.profilesCurrentName
            let mods = cslolModsView.saveProfile()
            if (checkGamePath()) {
                cslolTools.saveProfile(name, mods, run, settings.suppressInstallConflicts, settings.verbosePatcher)
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

        onRunDiag: cslolTools.runDiag()

        onOpenLogs: Qt.openUrlExternally(CSLOLUtils.toFile("./log.txt"))
    }

    onClosing: {
        if (cslolTrayIcon.available && settings.enableSystray) {
            close.accepted = false
            window.hide()
        }
    }

    SystemTrayIcon {
        id: cslolTrayIcon
        visible: settings.enableSystray
        iconSource: "qrc:/icon.png"
        tooltip: "cslol-manager"
        menu: Menu {
            MenuItem {
                text: !window.visible ? qsTr("Show") : qsTr("Minimize")
                onTriggered: window.visible ? window.hide() : window.show()
            }
            MenuItem {
                text: window.patcherRunning ? qsTr("Stop") : qsTr("Run")
                onTriggered: {
                    if (window.patcherRunning) {
                        cslolToolBar.stopProfile()
                    } else if (!window.isBussy) {
                        cslolToolBar.saveProfileAndRun(true)
                    }
                }
            }
            MenuItem {
                text: qsTr("Logs")
                onTriggered: Qt.openUrlExternally(CSLOLUtils.toFile("./log.txt"))
            }
            MenuItem {
                text: qsTr("Updates")
                onTriggered: Qt.openUrlExternally(cslolDialogUpdate.update_url)
            }
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit()
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
        onTryRefresh: cslolTools.refreshMods()
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
                    window.showUserError("Bad profile", "This profile already exists!")
                    return
                }
            }
            let index =  cslolToolBar.profilesModel.length
            cslolToolBar.profilesModel[index] = text
            cslolToolBar.profilesModel = cslolToolBar.profilesModel
            cslolToolBar.profilesCurrentIndex = index
        }
    }

    CSLOLDialogGame {
        id: cslolDialogGame
        onSelected: function(orgPath) {
            let path = CSLOLUtils.checkGamePath(orgPath)
            if (path === "") {
                window.showUserError("Bad game directory",  "There is no \"League of Legends.exe\" in " + orgPath)
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
        enableUpdates: settings.enableUpdates
    }

    CSLOLTools {
        id: cslolTools
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
            if(checkGamePath() && settings.enableAutoRun) {
                cslolToolBar.saveProfileAndRun(true)
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
        onRefreshed: function(mods) {
            cslolModsView.refereshedMods(mods)
        }
        onUpdatedMods: function(mods) {
            cslolDialogUpdateMods.updatedMods = mods
            cslolDialogUpdateMods.open()
        }
        onReportError: function(name, message, trace) {
            let log_data = "";
            if (trace) {
                log_data += trace.trim() + "\n"
            } else {
                log_data += message.trim() + "\n"
            }
            log_data += "name: " + name + "\n"
            log_data += "version: " + CSLOL_VERSION + "\n"
            cslolDialogError.name = name
            cslolDialogError.message = message
            cslolDialogError.log_data = log_data
            cslolDialogError.open()
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
    }
}
