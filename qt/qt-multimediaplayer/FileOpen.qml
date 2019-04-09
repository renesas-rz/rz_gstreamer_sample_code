/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1

Rectangle {
    id: root

    signal openVideo
    signal openImage

    color: "transparent"

    // Display the icon of menu
    Rectangle {
        id: menuField
        height: itemHeight
        width: itemHeight
        color: "transparent"
        anchors.right: parent.right

        Image {
            id: menu
            source: "qrc:///images/icon_Menu.png"
            anchors {
                right: parent.right
                left: parent.left
                bottom: parent.bottom
                top: parent.top
                margins: scaledMargin
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: fileOpen.state == "expanded" ? fileOpen.state = "collapsed" : fileOpen.state = "expanded"
        }
    }

    // Create a column includes the buttons such as "Open media", "Open image", "Exit"
    Column {
        anchors {
            top: menuField.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
            topMargin: (itemHeight / 6) * scaleh
        }

        spacing: 10 * scaleh
        visible: fileOpen.state == "expanded"

        Rectangle {
            width: itemWidth * scalew
            height: 1 * scaleh
            color: "#788ac5"
            anchors.left: parent.left
        }

        Button {
            Text {
                text: "Open media"
                font.pointSize: itemHeight * 0.4 * scaleh
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            height: itemHeight  * scaleh
            width: 2 * itemWidth * scalew
            color: "#788ac5"
            onClicked: {
                fileOpen.state = "collapsed"
                root.openVideo()
            }
        }

       Rectangle {
            width: itemWidth * scalew
            height: 1 * scaleh
            color: "#788ac5"
            anchors.left: parent.left
        }

        Button {
            Text {
                text: "Open image"
                font.pointSize: itemHeight * 0.4 * scaleh
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            height: itemHeight * scaleh
            width: 2 * itemWidth * scalew
            color: "#788ac5"
            onClicked: {
                fileOpen.state = "collapsed"
                root.openImage()
            }
        }

        Rectangle {
            width: itemWidth * scalew
            height: 1 * scaleh
            color: "#788ac5"
            anchors.left: parent.left
         }

        Button {
            Text {
                text: "Exit"
                font.pointSize: itemHeight * 0.4 * scaleh
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            height: itemHeight * scaleh
            width: 2 * itemWidth * scalew
            color: "#788ac5"
            onClicked: {
               Qt.quit()
             }
         }

        Rectangle {
            width: 2 * itemWidth * scalew
            height: 1 * scaleh
            color: "#788ac5"
            anchors.left: parent.left
        }
    }
}
