import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQml.XmlListModel

ApplicationWindow {
    width: 1000
    height: 700
    visible: true
    title: "Music Tree"

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        TextField {
            id: searchField
            placeholderText: "Enter artist name..."
            Layout.fillWidth: true
            onAccepted: {
                if (searchField.text.trim() !== "")
                    discogsManager.searchArtistByName(searchField.text.trim())
            }
        }

        Button {
            text: "Search"
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                if (searchField.text.trim() !== "")
                    discogsManager.searchArtistByName(searchField.text.trim())
            }
        }
    }
}
