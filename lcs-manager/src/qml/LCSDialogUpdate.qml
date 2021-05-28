import QtQuick 2.10
import QtQuick.Layouts 1.10
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

MessageDialog {
    id: lcsDialogUpdate
    width: 640
    height: 480
    modality: Qt.ApplicationModal
    standardButtons: StandardButton.Ok
    title: "Update please!"
    icon: StandardIcon.Information
    onAccepted: {
        Qt.openUrlExternally("https://github.com/LoL-Fantome/lolcustomskin-tools/releases/latest")
    }
    text: "New version has been released!"

    property bool disableUpdates: false

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
        if (disableUpdates) {
            return;
        }
        let url = "https://api.github.com/repos/LoL-Fantome/lolcustomskin-tools";
        makeRequest(url + "/releases/latest", function(latest) {
            let tag_name = latest["tag_name"];
            makeRequest(url + "/git/ref/tags/" + tag_name, function(ref) {
                let commit_sha = ref["object"]["sha"];
                let commit_url = ref["object"]["url"];
                if (commit_sha === LCS_COMMIT) {
                    return;
                }
                window.logInfo("Potential update found!", commit_sha)
                makeRequest(commit_url, function(commit) {
                    let current_date = Date.parse(LCS_DATE)
                    let commit_date = Date.parse(commit["committer"]["date"]);
                    if (commit_date > current_date) {
                        lcsDialogUpdate.open();
                    }
                })
            })
        })
    }
}
