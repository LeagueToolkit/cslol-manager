import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

MessageDialog {
    id: lcsDialogError
    width: 640
    height: 480
    modality: Qt.ApplicationModal
    standardButtons: StandardButton.Ok
    title: "Error"
    icon: StandardIcon.Critical
    onAccepted: {
        close()
    }
}
