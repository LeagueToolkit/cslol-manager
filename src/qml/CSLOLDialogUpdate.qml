import QtQuick 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Dialog {
    id: cslolDialogUpdate
    width: parent.width * 0.9
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    standardButtons: Dialog.Ok
    closePolicy: Popup.CloseOnEscape
    modal: true
    title: qsTr("Update please!")
    Overlay.modal: Rectangle {
        color: "#aa333333"
    }
    onOpened: window.show()

    property string update_url: "https://github.com/LeagueToolkit/cslol-manager/releases/latest"
    property int lastUpdateUTCMinutes: 0

    onAccepted: Qt.openUrlExternally(update_url)

    RowLayout {
        width: parent.width
        Label {
            text: qsTr("You will be redirected to download page after pressing OK button.\n") + update_url
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
    }

    property int enableUpdates: Qt.PartiallyChecked

    function makeRequest(url, onDone) {
        let request = new XMLHttpRequest();
        request.onreadystatechange = function() {
            if (request.readyState === XMLHttpRequest.DONE && request.status == 200) {
                onDone(JSON.parse(request.responseText))
            }
        }
        request.open("GET", url);
        request.send();
    }

    function checkForUpdates() {
        let cur_time = Date.now() / (1000 * 60)
        if (cur_time - lastUpdateUTCMinutes < 60) {
            return
        }
        lastUpdateUTCMinutes = cur_time
        let url = "https://api.github.com/repos/LeagueToolkit/cslol-manager";
        makeRequest(url + "/releases", function(releases) {
            for (let index in releases) {
                let release = releases[index]
                switch (enableUpdates) {
                case Qt.Unchecked:
                    // recieve only mandatory updates
                    if (release["prerelease"] || !release["body"].includes("mandatory")) {
                        continue;
                    }
                    break;
                case Qt.PartiallyChecked:
                    // only receive non-prerelease updates
                    if (release["prerelease"]) {
                        continue;
                    }
                    break;
                case Qt.Checked:
                    // allways receive the update
                    break;
                }
                let tag_name = release["tag_name"];
                console.log("Fetching release " + tag_name)
                makeRequest(url + "/git/ref/tags/" + tag_name, function(ref) {
                    let commit_sha = ref["object"]["sha"];
                    let commit_url = ref["object"]["url"];
                    if (commit_sha === CSLOL_COMMIT) {
                        return;
                    }
                    makeRequest(commit_url, function(commit) {
                        let current_date = Date.parse(CSLOL_DATE)
                        let commit_date = Date.parse(commit["committer"]["date"]);
                        if (commit_date > current_date) {
                            cslolDialogUpdate.open();
                        }
                    })
                })
                break;
            }
        })
    }
}
