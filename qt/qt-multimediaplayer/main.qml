/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtMultimedia 5.0
import QtQuick.Window 2.2

Rectangle {
    id: applicationWindow

    property string fileName
    property int windowWidth
    property int windowHeight

    // Declare coeffecient scale
    property double scalew: 1.0
    property double scaleh: 1.0

    //Calculate change aspects
    property int pixDens: Math.ceil(Screen.pixelDensity)
    property int itemWidth: 25 * pixDens
    property int itemHeight: 10 * pixDens
    property int scaledMargin: 2 * pixDens
    property int fontSize: 5 * pixDens
    property real volumeBeforeMuted: 1.0

    signal resetTimer

    // Set application window size
    function init() {
        //Calculate coeffecient scale
        scalew = (availableWidthScreen / 1920)     // Default width is 1920
        scaleh = (availableHeightScreen / 1080)    // Default height is 1080

        // Initialize screen width & height
        windowWidth = availableWidthScreen
        windowHeight= availableHeightScreen

        if (Qt.platform.os === "linux" || Qt.platform.os === "windows" || Qt.platform.os === "osx" || Qt.platform.os === "unix") {
            if (Screen.desktopAvailableWidth > 740)
                windowWidth = 740
            if (Screen.desktopAvailableHeight > 400)
                windowHeight = 400
        }

        height = windowHeight
        width = windowWidth
        videoFileBrowser.folder = videoPath
        imageFileBrowser.folder = imagePath
        fileName=videoPath
        // re-calculate coefficient scale to suite with LVDS
        if (scalew < 0.5)
            scalew = 0.5
        if (scaleh < 0.5)
            scaleh = 0.5
    }
    // Open the video
    function openVideo() {
        videoFileBrowser.show()
    }
    // Open the image
    function openImage() {
        imageFileBrowser.show()
    }

    focus: true
    color: "black"
    anchors.fill:parent

    // Enables mouse handling
    MouseArea {
        id: mouseActivityMonitor
        anchors.fill: parent

        hoverEnabled: true
        onClicked: {
            if (controlBar.state === "VISIBLE") {
                controlBar.hide();
            } else {
                controlBar.show();
                controlBarTimer.restart();
            }
        }
    }


    onResetTimer: {
        controlBar.show();
        controlBarTimer.restart();
    }

    // Set the interval between triggers, in milliseconds.
    Timer {
        id: controlBarTimer
        interval: 4000
        running: false
    }
    // Use custom item defined in Content.qml file
    Content {
        id: content
        anchors.fill: parent
    }
    // Use custom item defined in ControlBar.qml file
    ControlBar {
        id: controlBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: applicationWindow.bottom
        mediaPlayer: content.videoPlayer.mediaPlayer

    }
    // Use custom item defined in FileOpen.qml file
    FileOpen {
        id: fileOpen
        state: "collapsed"
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            margins: scaledMargin
        }
        width: itemHeight + scaledMargin
        z: 2
        opacity: 0.9

        states: [
            State {
                name: "expanded"
                PropertyChanges {
                    target: fileOpen
                    width: itemWidth * 1.5
                    opacity: 0.8
                }
            },
            State {
                name: "collapsed"
                PropertyChanges {
                    target: fileOpen
                    width: itemHeight + scaledMargin
                    opacity: 0.9
                }
            }
        ]

        transitions: [
            Transition {
                NumberAnimation { target: fileOpen; property: "width"; duration: 100 }
                NumberAnimation { target: fileOpen; property: "opacity"; duration: 100 }
            }
        ]
    }

    // Use custom item defined in FileBrowser.qml file
    FileBrowser {
        id: videoFileBrowser
        anchors.fill: applicationWindow
        Component.onCompleted: fileSelected.connect(content.openVideo)
   }
    FileBrowser {
        id: imageFileBrowser
        anchors.fill: applicationWindow
        Component.onCompleted: fileSelected.connect(content.openImage)

    }
    
    // Call function to open a video or an image
    Component.onCompleted: {
        controlBar.hide()
        fileOpen.openVideo.connect(openVideo)
        fileOpen.openImage.connect(openImage)
    }

    Keys.onPressed: {
        applicationWindow.resetTimer();
        if (event.key === Qt.Key_Up || event.key === Qt.Key_VolumeUp) {
            content.videoPlayer.mediaPlayer.volume = Math.min(1, content.videoPlayer.mediaPlayer.volume + 0.1);
            return;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_VolumeDown) {
            if (event.modifiers & Qt.ControlModifier) {
                if (content.videoPlayer.mediaPlayer.volume) {
                    volumeBeforeMuted = content.videoPlayer.mediaPlayer.volume;
                    content.videoPlayer.mediaPlayer.volume = 0
                } else {
                    content.videoPlayer.mediaPlayer.volume = volumeBeforeMuted;
                }
            } else {
                content.videoPlayer.mediaPlayer.volume = Math.max(0, content.videoPlayer.mediaPlayer.volume - 0.1);
            }
            return;
        }

        // What's next should be handled only if there's a loaded media
        if (content.videoPlayer.mediaPlayer.status !== MediaPlayer.Loaded
                && content.videoPlayer.mediaPlayer.status !== MediaPlayer.Buffered) {
            return;
        }

        if (event.key === Qt.Key_Space) {
            if (content.videoPlayer.mediaPlayer.playbackState === MediaPlayer.PlayingState) {
                content.videoPlayer.mediaPlayer.pause()
            } else if (content.videoPlayer.mediaPlayer.playbackState === MediaPlayer.PausedState
                     || content.videoPlayer.mediaPlayer.playbackState === MediaPlayer.StoppedState) {
                content.videoPlayer.mediaPlayer.play()
            }
        } else if (event.key === Qt.Key_Left) {
            content.videoPlayer.mediaPlayer.seek(Math.max(0, content.videoPlayer.mediaPlayer.position - 30000)); {
            return;
            }
        } else if (event.key === Qt.Key_Right) {
            content.videoPlayer.mediaPlayer.seek(Math.min(content.videoPlayer.mediaPlayer.duration, content.videoPlayer.mediaPlayer.position + 30000)); {
            return;
            }
        }
    }
 }


