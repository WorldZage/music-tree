import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQml.XmlListModel
import appmusictree 1.0


ApplicationWindow {
    width: 1000
    height: 700
    visible: true
    title: "Music Tree"

    SplitView {
        anchors.fill: parent

        // Left panel
        ColumnLayout {
            width: SplitView.view ? SplitView.width : 0
            //Layout.fillWidth: true
            Layout.fillHeight: true

            TextField {
                id: searchField
                width: parent.width / 4
                placeholderText: "Enter artist name..."
                onAccepted: if (text.trim() !== "") artistService.searchByName(text.trim())
            }

            RowLayout {
                width: parent.width
                Button {
                    width: parent.width / 8
                    text: "Search"
                    onClicked: if (searchField.text.trim() !== "") artistService.searchByName(searchField.text.trim())
                }

                Button {
                    width: parent.width / 8
                    text: "Clear DB"
                    onClicked: artistService.clearDb()
                }
                Button {
                    width: parent.width / 8
                    text: "Load Artists from file"
                    onClicked: artistService.loadArtistsFromFile()
                }
            }

            Rectangle {
                Layout.preferredWidth: parent.width
                Layout.fillHeight: true
                color: "white"
                GraphView {
                    id: graph
                    objectName: "graph"
                    width: parent.width * 0.7
                    height: parent.height * 0.7
                }
                Text {
                    anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 20 }
                    text: "test"
                }
            }
        }

        // Right panel
        ListView {
            id: artistList
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            model: sessionArtistModel

            delegate: Rectangle {
                width: ListView.view ? ListView.view.width : 0
                height: 40
                color: index % 2 ? "lightgrey" : "white"

                Row {
                    spacing: 10
                    Text { text: artistName + qsTr(" (") + artistId + qsTr(")")}
                    Button {
                        text: "Remove"
                        onClicked: sessionArtistModel.removeSessionArtistByListIndex(index)
                    }
                }
            }
        }
    }
}

