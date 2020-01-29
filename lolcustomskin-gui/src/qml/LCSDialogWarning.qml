import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

MessageDialog {
    id: lcsDialogWarning
    modality: Qt.ApplicationModal
    standardButtons: StandardButton.Ok
    title: "Warning"
    icon: StandardIcon.Warning
    onAccepted: {
        close()
    }
}
