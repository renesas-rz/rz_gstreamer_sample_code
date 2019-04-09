import QtQuick 2.1

Rectangle {
    id: root

    // Declare the variable
    property string text
    property color bgColor: "transparent"
    property color bgColorSelected: "#14aaff"
    property color textColor: "white"
    property alias enabled: mouseArea.enabled
    property bool active: true
    property alias horizontalAlign: text.horizontalAlignment

    // Declare the signal
    signal clicked

    // These properties are own of the top-level Rectangle
    color: "transparent"
    height: itemHeight
    width: itemWidth
    
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
            horizontalAlignment: Text.AlignHCenter
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

