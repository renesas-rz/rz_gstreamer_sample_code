import QtQuick 2.6
import QtQuick.Window 2.2

Rectangle {
    id: root

    // Declare the stacking order of the video
    property int low: 0
    property int high: 1

    // Declare the size of the video
    property int widthVideo: 500
    property int heightVideo: 300

    // Declare the position of the video
    property int position1: 0
    property int position2: 50
    property int position3: 100

    // Declare source file
    property string source1
    property string source2

    // First initialization
    function init() {
        video1.initialize()
        video2.initialize()
        video3.initialize()

        //set value of source file
        source1 = videosDirPath + "/vga1.h264"
        source2 = videosDirPath + "/vga2.h264"
    }

    // Object properties
    visible: true

    // Control video and display information
    Component {
        id: startStopComponent

        Rectangle {
            id: root
            color: "transparent"
            // Function return the QML class is used
            function content() {
                return root.parent
            }

            Text {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                    margins: 20
                }
                // Set content text based on status of video
                text: !(content().isStop()) ? " " : "Tap to start"
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                // Move video
                drag.target: content()
                onClicked: {
                    // Change the video display
                    switch(content()) {
                    case video1: video1.z=high; video2.z=low; video3.z=low; break;
                    case video2: video1.z=low; video2.z=high; video3.z=low; break;
                    case video3: video1.z=low; video2.z=low; video3.z=high; break;
                    }
                    // Play video
                    if (content().isStop())
                        content().start()
                }
            }
        }
    }

    // First video
    Content {
        id: video1
        color: "black"
        // Set location and order first screen
        x: position1
        y: position1
        z: low
        source: parent.source1
        // Set size first screen
        width: widthVideo
        height: heightVideo

        Loader {
            id: video1StartStopLoader
            onLoaded: {
                item.parent = video1
                item.anchors.fill = video1
            }
        }
        // Load startStopComponent
        onInitialized: video1StartStopLoader.sourceComponent = startStopComponent
    }

    // Second screen
    Content {
        id: video2
        color: "black"
        // Set location and order second screen
        x: position2
        y: position2
        z: low
        source: parent.source1
        // Set size second screen
        width: widthVideo
        height: heightVideo

        Loader {
            id: video2StartStopLoader
            onLoaded: {
                item.parent = video2
                item.anchors.fill = video2
            }
        }
        // Load startStopComponent
        onInitialized: video2StartStopLoader.sourceComponent = startStopComponent
    }

    Content {
        id: video3
        color: "black"
        // Set location and order third screen
        x: position3
        y: position3
        z: low
        source:  parent.source2
        // Set size third screen
        width: widthVideo
        height: heightVideo

        Loader {
            id: video3StartStopLoader
            onLoaded: {
                item.parent = video3
                item.anchors.fill = video3
            }
        }
        // Load startStopComponent
        onInitialized: video3StartStopLoader.sourceComponent = startStopComponent
    }
}
