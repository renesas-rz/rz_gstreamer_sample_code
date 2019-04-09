import QtQuick 2.5
import QtQuick.Window 2.2

//This Window component will create a new top-level window with title name "Hello World" for a Qt Quick scene
Window {
    // Scale 1/4 full screen
    property int minimized_scale: 4
    //These properties are own of the window component
    visible: true
    width: Screen.desktopAvailableWidth/minimized_scale
    height: Screen.desktopAvailableHeight/minimized_scale
    title: "Hello World"
    color: "lightblue"
    visibility: "Minimized"
    id: mainWindow

    Component.onCompleted: {
            mainWindow.maximumWidth = Screen.desktopAvailableWidth
            mainWindow.maximumHeight = Screen.desktopAvailableHeight
            mainWindow.minimumWidth = Screen.desktopAvailableWidth/minimized_scale
            mainWindow.minimumHeight = Screen.desktopAvailableHeight/minimized_scale
            mainWindow.showMaximized();
            console.log("Width: ", Screen.desktopAvailableWidth, "  Height: ", Screen.desktopAvailableHeight)
    }

    // This Text component will display text message named "Hello World" at the center of the application.
    Text {
        anchors.centerIn: parent
        // Bold red "Hello World" text with size 20pts
        text: "Hello World"
        color: "red"
        font.bold: true
        font.pointSize: 20
    }
}
