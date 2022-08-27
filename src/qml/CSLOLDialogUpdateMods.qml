import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

Dialog {
    id: cslolDialogUpdateMods
    width: parent.width * 0.9
    height: parent.height * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: Dialog.Ok
    closePolicy: Popup.CloseOnEscape
    modal: true
    title: qsTr("Mod updates and fixes:")
    Overlay.modal: Rectangle {
        color: "#aa333333"
    }
    onOpened: window.show()

    property int columnCount: 1
    property real rowHeight: 0
    property alias updatedMods: cslolUpdateModsView.model

    updatedMods: []

    ScrollView {
        id: cslolUpdateModsScrollView
        width: parent.width
        height: parent.height
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
        padding: ScrollBar.vertical.width
        spacing: 5

        GridView {
            id: cslolUpdateModsView
            cellWidth: cslolUpdateModsView.width / cslolDialogUpdateMods.columnCount
            cellHeight: 75
            delegate: Pane {
                property var model: modelData
                width: cslolUpdateModsView.width / cslolDialogUpdateMods.columnCount - cslolUpdateModsScrollView.spacing
                Component.onCompleted: {
                    let newCellHeight = height + cslolUpdateModsScrollView.spacing
                    if (cslolUpdateModsView.cellHeight < newCellHeight) {
                        cslolUpdateModsView.cellHeight = newCellHeight;
                    }
                }
                Material.elevation: 3
                Row {
                    width: parent.width
                    Row {
                        id: modUpdateButtons
                        width: parent.width * 0.3

                        ToolButton {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "\uf0ed "
                            font.family: "FontAwesome"
                            onClicked: {
                                let url = model.Download
                                if (window.validUrl.test(url)) {
                                    Qt.openUrlExternally(url)
                                }
                            }
                            CSLOLToolTip {
                                text: qsTr("Download")
                                visible: parent.hovered
                            }
                        }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: model.Name
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    Column {
                        width: parent.width * 0.39
                        anchors.verticalCenter: parent.verticalCenter
                        Label {
                            horizontalAlignment: Text.AlignHCenter
                            text: "V" + model.Version + " by " + model.Author
                            elide: Text.ElideRight
                            width: parent.width
                        }
                        Label {
                            horizontalAlignment: Text.AlignHCenter
                            text: model.Description ? model.Description : ""
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            width: parent.width
                        }
                    }
                    Row {
                        id: modUpdateButtons2
                        width: parent.width * 0.3
                        anchors.verticalCenter: parent.verticalCenter
                        layoutDirection: Qt.RightToLeft
                        ToolButton {
                            text: "\uf059"
                            font.family: "FontAwesome"
                            onClicked: {
                                let url = model.Home
                                if (window.validUrl.test(url)) {
                                    Qt.openUrlExternally(url)
                                }
                            }
                            CSLOLToolTip {
                                text: qsTr("Mod updates")
                                visible: parent.hovered
                            }
                        }
                        ToolButton {
                            text: "\uf004"
                            font.family: "FontAwesome"
                            onClicked: {
                                let url = model.Heart
                                if (window.validUrl.test(url)) {
                                    Qt.openUrlExternally(url)
                                }
                            }
                            CSLOLToolTip {
                                text: qsTr("Support this author")
                                visible: parent.hovered
                            }
                        }
                    }
                }
            }
        }
    }
}
