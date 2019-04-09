import QtQuick 2.1
import Qt.labs.folderlistmodel 2.1

Rectangle {
    id: fileBrowser

    property string folder
    property bool shown: loader.sourceComponent

    signal fileSelected(string file)

    // Select a media file to load
    function selectFile(file) {
        if (file !== "") {
            folder = loader.item.folders.folder
            fileBrowser.fileSelected(file)
        }
        loader.sourceComponent = undefined
    }

    // Set the id of objects
    function show() {
        loader.sourceComponent = fileBrowserComponent
        loader.item.parent = fileBrowser
        loader.item.anchors.fill = fileBrowser
        loader.item.folder = fileBrowser.folder
    }

    color: "transparent"
    z: 4

    Loader {
        id: loader
    }

    Component {
        id: fileBrowserComponent

        Rectangle {
            id: root

            property bool showFocusHighlight: false
            property variant folders: folders1
            property variant view: view1
            property alias folder: folders1.folder
            property color textColor: "white"

            // Set states of the path to back
            function down(path) {
                if (folders == folders1) {
                    view = view2
                    folders = folders2;
                    view1.state = "exitLeft";
                } else {
                    view = view1
                    folders = folders1;
                    view2.state = "exitLeft";
                }
                view.x = root.width;
                view.state = "current";
                view.focus = true;
                folders.folder = path;
            }
            // Set states of the path to up
            function up() {
                var path = folders.parentFolder;
                if (path.toString().length === 0 || path.toString() === 'file:')
                    return;
                if (folders == folders1) {
                    view = view2
                    folders = folders2;
                    view1.state = "exitRight";
                } else {
                    view = view1
                    folders = folders1;
                    view2.state = "exitRight";
                }
                view.x = -root.width;
                view.state = "current";
                view.focus = true;
                folders.folder = path;
            }
            // Set cases when pressing directional key
            function keyPressed(key) {
                switch (key) {
                    case Qt.Key_Up:
                    case Qt.Key_Down:
                    case Qt.Key_Left:
                    case Qt.Key_Right:
                        root.showFocusHighlight = true;
                    break;
                    default:
                        // do nothing
                    break;
                }
            }

            color: "black"

            // Access to information about the contents of a folder in the local file system
            FolderListModel {
                id: folders1
                folder: folder
            }
            FolderListModel {
                id: folders2
                folder: folder
            }
            // Provide information about the standard colors used for application windows, buttons and other features
            SystemPalette {
                id: palette
            }

            Component {
                id: folderDelegate

                Rectangle {
                    id: wrapper
                    // Display the path of file browser
                    function launch() {
                        var path = "file://";
                        // Windows drive logic, see QUrl::fromLocalFile()
                        if (filePath.length > 2 && filePath[1] === ':')
                            path += '/';
                        path += filePath;
                        if (folders.isFolder(index)) {
                            down(path);
                        } else {
                            fileBrowser.selectFile(path)
                        }
                    }

                    width: root.width * scalew
                    height: itemHeight * scaleh
                    color: "transparent"

                    Rectangle {
                        id: highlight; visible: false
                        anchors.fill: parent
                        color: palette.highlight
                        gradient: Gradient {
                            GradientStop { id: t1; position: 0.0; color: palette.highlight }
                            GradientStop { id: t2; position: 1.0; color: Qt.lighter(palette.highlight) }
                        }
                    }
                    // Load and Display the folder icon
                    Item {
                        width: (itemHeight + scaledMargin) * scaleh ; height: (itemHeight + scaledMargin) * scaleh
                        Image {
                            source: "qrc:/images/icon_Folder.png"
                            fillMode: Image.PreserveAspectFit
                            anchors.fill: parent
                            anchors.margins: scaledMargin
                            visible: folders.isFolder(index)
                        }
                    }

                    Text {
                        id: nameText
                        anchors.fill: parent; verticalAlignment: Text.AlignVCenter
                        text: fileName
                        anchors.leftMargin: (itemHeight + scaledMargin) * scaleh
                        font.pixelSize: fontSize * scaleh
                        color: (wrapper.ListView.isCurrentItem && root.showFocusHighlight) ? palette.highlightedText : textColor
                        elide: Text.ElideRight
                    }

                    MouseArea {
                        id: mouseRegion
                        anchors.fill: parent
                        onPressed: {
                            root.showFocusHighlight = false;
                            wrapper.ListView.view.currentIndex = index;
                        }
                        onClicked: {
                            if (folders == wrapper.ListView.view.model)
                                launch()
                        }
                    }

                    states: [
                        State {
                            name: "pressed"
                            when: mouseRegion.pressed
                            PropertyChanges { target: highlight; visible: true }
                            PropertyChanges { target: nameText; color: palette.highlightedText }
                        }
                    ]
                }
            }
            // Display data from models created
            ListView {
                id: view1
                anchors.top: titleBar.bottom
                anchors.bottom: cancelButton.top
                x: 0
                width: parent.width
                model: folders1
                delegate: folderDelegate
                highlight: Rectangle {
                    color: palette.highlight
                    visible: root.showFocusHighlight && view1.count != 0
                    gradient: Gradient {
                        GradientStop { id: t1; position: 0.0; color: palette.highlight }
                        GradientStop { id: t2; position: 1.0; color: Qt.lighter(palette.highlight) }
                    }
                    width: view1.currentItem == null ? 0 : view1.currentItem.width
                }
                highlightMoveVelocity: 1000
                pressDelay: 100
                focus: true
                state: "current"
                states: [
                    State {
                        name: "current"
                        PropertyChanges { target: view1; x: 0 }
                    },
                    State {
                        name: "exitLeft"
                        PropertyChanges { target: view1; x: -root.width }
                    },
                    State {
                        name: "exitRight"
                        PropertyChanges { target: view1; x: root.width }
                    }
                ]
                transitions: [
                    Transition {
                        to: "current"
                        SequentialAnimation {
                            NumberAnimation { properties: "x"; duration: 250 }
                        }
                    },
                    Transition {
                        NumberAnimation { properties: "x"; duration: 250 }
                        NumberAnimation { properties: "x"; duration: 250 }
                    }
                ]
                Keys.onPressed: root.keyPressed(event.key)
            }
            // Display data from models created
            ListView {
                id: view2
                anchors.top: titleBar.bottom
                anchors.bottom: parent.bottom
                x: parent.width
                width: parent.width
                model: folders2
                delegate: folderDelegate
                highlight: Rectangle {
                    color: palette.highlight
                    visible: root.showFocusHighlight && view2.count != 0
                    gradient: Gradient {
                        GradientStop { id: t1; position: 0.0; color: palette.highlight }
                        GradientStop { id: t2; position: 1.0; color: Qt.lighter(palette.highlight) }
                    }
                    width: view1.currentItem == null ? 0 : view1.currentItem.width
                }
                highlightMoveVelocity: 1000
                pressDelay: 100
                states: [
                    State {
                        name: "current"
                        PropertyChanges { target: view2; x: 0 }
                    },
                    State {
                        name: "exitLeft"
                        PropertyChanges { target: view2; x: -root.width }
                    },
                    State {
                        name: "exitRight"
                        PropertyChanges { target: view2; x: root.width }
                    }
                ]
                transitions: [
                    Transition {
                        to: "current"
                        SequentialAnimation {
                            NumberAnimation { properties: "x"; duration: 250 }
                        }
                    },
                    Transition {
                        NumberAnimation { properties: "x"; duration: 250 }
                    }
                ]
                Keys.onPressed: root.keyPressed(event.key)
            }
            // Use custom item defined in Button.qml file
            Button {
                id: cancelButton
                width: itemWidth * scalew
                height: itemHeight * scaleh
                color: "#353535"
                anchors { bottom: parent.bottom; right: parent.right; margins: 5 * scaledMargin }
                Text {
                    text: "Cancel"
                    font.pointSize: itemHeight * 0.4 * scaleh
                    color: "#ffffff"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                onClicked: fileBrowser.selectFile("")
            }

            Keys.onPressed: {
                root.keyPressed(event.key);
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Select || event.key === Qt.Key_Right) {
                    view.currentItem.launch();
                    event.accepted = true;
                } else if (event.key === Qt.Key_Left) {
                    up();
                }
            }

            // Title bar
            Rectangle {
                color: "black"
                width: parent.width
                height: itemHeight
                id: titleBar

                Rectangle {
                    id: upButton
                    width: titleBar.height
                    height: titleBar.height
                    color: "transparent"
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: scaledMargin

                    Image { anchors.fill: parent; anchors.margins: scaledMargin; source: "qrc:/images/icon_BackArrow.png" }
                    MouseArea { id: upRegion; anchors.fill: parent; onClicked: up() }
                    states: [
                        State {
                            name: "pressed"
                            when: upRegion.pressed
                            PropertyChanges { target: upButton; color: palette.highlight }
                        }
                    ]
                }

                Text {
                    anchors.left: upButton.right; anchors.right: parent.right; height: parent.height
                    anchors.leftMargin: 10; anchors.rightMargin: 4
                    text: folders.folder
                    color: "white"
                    elide: Text.ElideLeft; horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    font.pixelSize: fontSize
                }
            }

            Rectangle {
                color: "#353535"
                width: parent.width  * scalew
                height: 1 * scaleh
                anchors.top: titleBar.bottom
            }
        }
    }
}
