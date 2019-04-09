import QtQuick 2.5
import QtQuick.Window 2.2
import QtMultimedia 5.5

// This Window component will create a new top-level window with title name "Video Play (no sound)" for a Qt Quick scene
Window {
    id: mainWindow
    // Scale 1/4 full screen
    property int minimized_scale: 4

    // These properties are own of the window component
    visible: true
    width: Screen.desktopAvailableWidth/minimized_scale
    height: Screen.desktopAvailableHeight/minimized_scale
    title: qsTr("Video Play (no sound)")
    color: "black"
    visibility: "Minimized"

    Component.onCompleted: {
            mainWindow.maximumWidth = Screen.desktopAvailableWidth
            mainWindow.maximumHeight = Screen.desktopAvailableHeight
            mainWindow.minimumWidth = Screen.desktopAvailableWidth/minimized_scale
            mainWindow.minimumHeight = Screen.desktopAvailableHeight/minimized_scale
            mainWindow.showMaximized();
            console.log("Width: ", Screen.desktopAvailableWidth, "  Height: ", Screen.desktopAvailableHeight)
    }

    // This Text component will display text message named "Click to play video" at the center of the application.
    Text {
        id: infoText
        text: "Click to play video"
        anchors.centerIn: parent
        color: "white"
        font.pointSize: 15
    }

    // This Text component will create a text message and display video information, such as: file name, codec and resolution
    Text {
        color: "white"
        text:
            "File name: h264-hd-30.mp4\n" +
            "Codec: " + playVideo.metaData.videoCodec + "\n" +
            "Resolution: " + playVideo.metaData.resolution + "\n"
    }

    // This MediaPlayer component adds a media file and plays it
    MediaPlayer {
        id: playVideo
        source: videosDirPath + "/h264-hd-30.mp4"
        autoLoad: true
        autoPlay: false
        loops: MediaPlayer.Infinite
    }

    // This VideoOutput component is used for displaying video and camera on the monitor
    VideoOutput {
        anchors.fill: parent
        source: playVideo
    }

    // This MouseArea component starts and pauses the video.
    MouseArea {
        anchors.fill: parent
        onClicked: {
            // Disable "Click to play video" message
            // We need it to display once
            if (infoText.visible == true)
                infoText.visible = false;
            
            // Toggle the video when users click the application
            if (playVideo.playbackState == MediaPlayer.PlayingState) {
                playVideo.pause();
            } else {
                playVideo.play();
            }
        }
    }
}
