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
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextField {
                id: searchField
                placeholderText: "Enter artist name..."
                onAccepted: if (text.trim() !== "") artistService.searchByName(text.trim())
            }

            RowLayout {
                Button {
                    text: "Search"
                    onClicked: if (searchField.text.trim() !== "") artistService.searchByName(searchField.text.trim())
                }

                Button {
                    text: "Clear DB"
                    onClicked: artistService.clearDb()
                }
            }

            Rectangle {
                Layout.preferredWidth: 300
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
                width: parent.width
                height: 40
                color: index % 2 ? "lightgrey" : "white"

                Row {
                    spacing: 10
                    Text { text: artistName }
                    Button {
                        text: "Remove"
                        onClicked: sessionManager.removeArtist(index)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        sessionManager.addArtist("123", "Aphex Twin")
        sessionManager.addArtist("456", "Squarepusher")
    }
}

