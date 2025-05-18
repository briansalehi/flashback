import QtQuick

Item {
    id: scene

    property color color

    Rectangle {
        id: background
        anchors.fill: parent
        color: scene.color
        radius: height * 5 / 100

        Text {
            id: name
            text: "Flashback"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: parent.height * 30 / 100
            width: parent.width
            font.pixelSize: 42
            font.bold: true
            color: '#FFFFFF'
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            id: description
            text: "You can see this because you are a tester!"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: parent.height * 60 / 100
            width: parent.width * 70 / 100
            font.pixelSize: 22
            font.bold: true
            wrapMode: Text.WordWrap
            color: '#AAAAAA'
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
