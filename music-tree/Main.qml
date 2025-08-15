import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: "Music Tree"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 10

        TextField {
            id: searchField
            placeholderText: "Enter artist name..."
            Layout.fillWidth: true
            onAccepted: {
                if (searchField.text.trim() !== "") {
                    discogsManager.searchArtistByName(searchField.text.trim())
                }
            }
        }

        Button {
            text: "Search"
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                if (searchField.text.trim() !== "") {
                    discogsManager.searchArtistByName(searchField.text.trim())
                }
            }
        }

        Button {
            text: "Fetch Daft Punk"
            onClicked: {
                discogsManager.fetchArtist(1289)
            }
        }

        Connections {
            target: discogsManager
            function onArtistDataReady(json) {
                console.log("Artist JSON:", json)
            }
            function onReleasesDataReady(json) {
                console.log("Releases JSON:", json)
            }
            function filterMastersAndLog(json) {
                console.log("Masters JSON:", json)
        }

    }
}
}

