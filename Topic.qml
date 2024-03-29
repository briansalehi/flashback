import QtQuick

Item {
    id: root
    width: parent.width - 5
    height: 40

    property alias color: element.color
    property alias title: label.text
    signal clicked

    Rectangle {
        id: element
        radius: 5
        anchors.fill: parent
        color: 'lightblue'
    }

    Text {
        id: label
        text: 'Untitled'
        font.family: 'OpenSans'
        font.pixelSize: 17
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        width: parent.width
        elide: Text.ElideRight
        padding: 10
    }

    MouseArea {
        anchors.fill: parent
        onClicked: { root.clicked(); }
    }
}
