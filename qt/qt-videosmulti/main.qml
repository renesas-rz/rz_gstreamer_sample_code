import QtQuick 2.6
import QtQuick.Window 2.2
import QtMultimedia 5.0

Rectangle {
    id: root

    // Declare source file
    property string source1
    property string source2

    // Declare intermediate variables
    property int widthNumber1
    property int widthNumber2
    property int heightNumber1
    property int heightNumber2

    // Declare variable
    property int pixDens: Math.ceil(Screen.pixelDensity)
    property int itemWidth: 15 * pixDens
    property int itemHeight: 10 * pixDens
    property int scaledMargin: 2 * pixDens
    property int fontSize: 5 * pixDens

    // First initialization
    function init() {
        video1.initialize()
        // Check the existence of the second screen
        if (widthScreen2 !== 0) 
            video2.initialize()
        
        // Set value for intermediate variables
        widthNumber1 = widthScreen1
        widthNumber2 = widthScreen2
        heightNumber1 = heightScreen1
        heightNumber2 = heightScreen2
        
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
                onClicked: {
                    // Play application based on status of video
                    if (content().isStop()) {
                        content().start()
                    } else {
                        content().pause()
                    }
                }
            }
        }
    }

    // First video
    Content {
        id: video1
        color: "black"
        // Set location first screen
        x: 0
        y: 0
        source: parent.source1
        // Set size first screen
        width: widthNumber1
        height: heightNumber1

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
        // Set location second screen
        x: widthNumber1
        y: 0
        source: parent.source2
        // Set size second screen
        width: widthNumber2
        height: heightNumber2

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

    // Exit button
    Button {
        id: btn1
        text: "Exit"
        onClicked: {
            Qt.quit()
        }
    }
}
