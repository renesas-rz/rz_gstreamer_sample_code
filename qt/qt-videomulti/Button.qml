import QtQuick 2.0

Rectangle {
    id: root

    // Declare the variable
    property string text
    property color bgColor: "transparent"
    property color bgColorSelected: "#14aaff"
    property color textColor: "black"
    property bool active: true

    // Declare the signal
    signal clicked

    // These properties are own of the top-level Rectangle
    color: "#E1E0E0"
    height: itemHeight
    width: itemWidth
    radius: 1

    // Child objects
    Rectangle {
        anchors { fill: parent; margins: 1 }
        color: mouseArea.pressed ? bgColorSelected : bgColor

        Text {
            id: text
            clip: true
            text: root.text
            anchors { fill: parent; margins: scaledMargin }
            font.pixelSize: fontSize
            color: textColor
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                // This signal is emitted when there is a click.
                root.clicked()
            }
            enabled: active
        }
    }
}
