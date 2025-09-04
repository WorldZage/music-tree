import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQml.XmlListModel
import appmusictree 1.0

ApplicationWindow {
    width: 1200   // wider default
    height: 700
    visible: true
    title: "Music Tree"

    SplitView {
        anchors.fill: parent

        // Left panel
        ColumnLayout {
            width: SplitView.view ? SplitView.width : 1000
            Layout.fillHeight: true

            RowLayout {
                width: parent.width
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "Enter artist name..."
                    onAccepted: if (text.trim() !== "") artistService.searchByName(text.trim())
                }
                Button {
                    text: "Search"
                    onClicked: if (searchField.text.trim() !== "") artistService.searchByName(searchField.text.trim())
                }
            }

            Rectangle {
                Layout.preferredWidth: parent.width
                Layout.fillHeight: true
                color: "white"
                GraphView {
                    id: graph
                    objectName: "graph"
                    width: parent.width * 0.85   // larger default width
                    height: parent.height * 0.7
                }
            }
        }

        // Right panel
        ColumnLayout {
            width: SplitView.view ? SplitView.width : 400
            Layout.fillHeight: true

            Button {
                text: "Clear DB"
                onClicked: artistService.clearDb()
            }

            RowLayout {
                width: parent.width
                Button {
                    text: "Load Artists from file"
                    onClicked: artistService.loadArtistsFromFile()
                }
                Button {
                    text: "Save Artists to file"
                    onClicked: artistService.saveArtistsToFile()
                }
            }

            ListView {
                id: artistList
                Layout.preferredWidth: 350
                Layout.fillHeight: true
                model: sessionArtistModel

                delegate: Rectangle {
                    width: ListView.view ? ListView.view.width : 0
                    height: 40
                    color: index % 2 ? "lightgrey" : "white"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            text: artistName + qsTr(" (") + artistId + qsTr(")")
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Button {
                            text: "Refresh"
                            onClicked: {
                                artistService.refreshSessionArtist(artistId)
                            }
                        }

                        Button {
                            text: "Remove"
                            onClicked: artistService.removeSessionArtistById(artistId)
                        }
                    }
                }
            }
        }
    }
}
