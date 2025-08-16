import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQml.XmlListModel
import MyApp 1.0


ApplicationWindow {
    width: 1000
    height: 700
    visible: true
    title: "Music Tree"

    Row {
        Column {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            spacing: 8

            TextField {
                id: searchField
                placeholderText: "Enter artist name..."
                onAccepted: {
                    if (text.trim() !== "") {
                        discogsManager.searchArtistByName(text.trim())
                    }
                }
            }
            Button {
                text: "Search"
                onClicked: {
                    if (searchField.text.trim() !== "") {
                        discogsManager.searchArtistByName(searchField.text.trim())
                    }
                }
            }
            Rectangle {
                width: 1000
                height: 700
                color: "white"
                GraphView {
                    id: graph
                    objectName: "graph"  // <-- add this line
                    width: parent.width * 0.7
                    height: parent.height * 0.7
                }
                Text {
                    anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 20 }
                    text: "test"
                }

            }

        }



    }
}
