import QtQuick 2.1
import QtMultimedia 5.0

// This VideoOutput component is used for displaying video on the monitor
VideoOutput {
    id: root

    property alias mediaSource: mediaPlayer.source
    property bool started: false

    signal fatalError

    // Start video
    function start() {
        mediaPlayer.play()
        started = true
    }

    source: mediaPlayer

    // Add media playback to a scene
    MediaPlayer {
        id: mediaPlayer
        autoLoad: false
        muted: true
        // Display error MediaPlayer
        onError: {
            if (MediaPlayer.NoError != error) {
                console.log("[qt-videomulti] VideoItem.onError error " + error + " errorString " + errorString)
                root.fatalError()
            }
        }
        onStopped: started = false
    }    
}
