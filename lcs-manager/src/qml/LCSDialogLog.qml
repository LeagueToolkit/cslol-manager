import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

ApplicationWindow {
    id: lcsDialogLog
    modality: Qt.NonModal
    // standardButtons: StandardButton.NoButton    
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowMinMaxButtonsHint
    function open() {
        show()
    }
    onClosing: {
        close.accepted = false
        hide()
    }
    title: "Log - " + LCS_VERSION

    LCSLogView {
        id: lcsLogView
        width: parent.width
        height: parent.height
    }
}
