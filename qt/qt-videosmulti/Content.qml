import QtQuick 2.1

Rectangle {
    id: root

    property string source

    signal initialized

    // Load source VideoItem.qml file
    function initialize() {
        contentLoader.source = "VideoItem.qml"
        if (contentLoader.item) {
            contentLoader.item.parent = root
            contentLoader.item.anchors.fill = root
        }
        root.initialized()
    }
    // Call function start in VideoItem.qml file
    function start() {
        if (contentLoader.item) {
            contentLoader.item.mediaSource = root.source
            contentLoader.item.start()
        }
    }
    // Call function pause in VideoItem.qml file
    function pause() {
        if (contentLoader.item)
            contentLoader.item.pause()
    }
    // Check status video
    function isStop() {
        if (contentLoader.item.started)
            return false
        return true
    }

    // Object properties
    color: "transparent"

    // Use to dynamically load QML components
    Loader {
        id: contentLoader
    }
    // Display error Connections
    Connections {
        id: errorConnection
        target: contentLoader.item
        onFatalError: {
            console.log("[qt-videomulti] Content.onFatalError")
        }
        ignoreUnknownSignals: true
    }
}
