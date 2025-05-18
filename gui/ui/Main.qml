import QtQuick
import QtQuick.Controls.Material

Window {
    id: window
    visible: true
    visibility: Window.Maximized
    title: qsTr("Flashback")
    Material.theme: Material.Dark

    onWidthChanged: { scene.width = calculate_scene_size(width, height).width }
    onHeightChanged: { scene.height = calculate_scene_size(width, height).height }

    Rectangle {
        id: background
        color: '#0A0A1B'
        anchors.fill: parent
    }

    Scene {
        id: scene
        anchors.centerIn: parent
        color: Qt.lighter(background.color, 1.3)
    }

    function calculate_scene_size(width, height)
    {
        var w, h;

        if (width > height)
        {
            w = 70 * width / 100;
            h = 95 * height / 100;
        }
        else
        {
            w = 80 * width / 100;
            h = 90 * height / 100;
        }

        return Qt.size(w, h);
    }
}
