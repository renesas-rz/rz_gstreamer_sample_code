import QtQuick 2.5
import QtQuick.Window 2.2
import QtMultimedia 5.5

//This Window component will create a new top-level window with title name “Audio Play” for a Qt Quick scene
Window {
    // Scale 1/4 full screen
    property int minimized_scale : 4
    // These properties are own of the Window component
    visible: true
    width: Screen.desktopAvailableWidth/minimized_scale
    height: Screen.desktopAvailableHeight/minimized_scale
    title: qsTr("Audio Play")
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

    // This Text component will display text message named "Click to play audio" at the center of the application.
    Text {
        id: textInfo
        text: qsTr("Click to play audio")
        anchors.centerIn: parent
        color: "red"
        font.bold: true
        font.pointSize: 20
    }

    // This Text component creates a text message and displays audio information, such as: file name, size, bit rate, and codec.
    Text {
        text:
            "File name: audio.ogg\n" +
            "Bit rate: " + playMusic.metaData.audioBitRate + " (bps)\n" +
            "Codec: " + playMusic.metaData.audioCodec + "\n"
    }

    // This MediaPlayer component adds a media file and plays it
    MediaPlayer {
        id: playMusic
        source: audiosDirPath + "/audio.ogg"
        autoLoad: true
        autoPlay: false
        loops: MediaPlayer.Infinite
    }

    // This MouseArea component starts and pauses the audio.
    MouseArea {
        anchors.fill: parent
        onClicked: {
            // Toggle the audio when users click the application
            if (playMusic.playbackState == MediaPlayer.PlayingState) {
                textInfo.text = "Audio is paused";
                playMusic.pause();
            } else {
                textInfo.text = "Audio is playing";
                playMusic.play();
            }
        }
    }
}
