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

#include <QtQml/QQmlContext>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtGui/QScreen>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>

/*
 * name: isFileExists
 * Check if the input file exists or not and if it is a file?
 *
 */
bool isFileExists (QString path) {
    QFileInfo file_info(path);
    // Check if path exists
    if (file_info.exists()) {
        // Check if path is a file
        if (file_info.isFile()) {
            return true;
        } else {
            qDebug() << "Error: " << path << " is not a file.";
            return false;
        }
    } else {
        qDebug() << "Error: " << path << "does not exist";
        return false;
    }
}

/*
 * name: isSupportedVideo
 * Check if the video format is supported?
 * Supported format: MP4, H.264, H.265
 *
 */
bool isSupportedVideo (QString path) {
    QFileInfo file_info(path);
    if ((file_info.suffix().compare("h264", Qt::CaseInsensitive) != 0)
            && (file_info.suffix().compare("h265", Qt::CaseInsensitive) != 0)
            && (file_info.suffix().compare("mp4", Qt::CaseInsensitive) != 0)) {
        qDebug() << "Error: unsupport video format.";
        qDebug() << "Supported video extensions: mp4, h264, h265.";
        return false;
    } else {
        return true;
    }
}


// Declare the variable screen size
int widthScreen1 = 0;
int widthScreen2 = 0;
int heightScreen1 = 0;
int heightScreen2 = 0;
int maxHeight = 0;

// Function dataScreens
void dataScreens()
{
    // Classify width and height for screens
    foreach (QScreen *screen, QGuiApplication::screens()) {
        // Get screen has coordinates 0 and 0
        if((screen->geometry().y() == 0) && (screen->geometry().x() == 0)) {
            // Get value width and height and set it for variable
             widthScreen1 = screen->geometry().width();
             heightScreen1 = screen->geometry().height();
             qDebug() << "  Geometry 1:" << screen->geometry().x() << screen->geometry().y() << screen->geometry().width() << "x" << screen->geometry().height();
        } else {
            widthScreen2 = screen->geometry().width();
            heightScreen2 = screen->geometry().height();
            qDebug() << "  Geometry 2:" << screen->geometry().x() << screen->geometry().y() << screen->geometry().width() << "x" << screen->geometry().height();
        }
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationVersion("1.1");
    QGuiApplication::setApplicationName("qt-videosmulti");
    QQuickView viewer;

    QCommandLineParser parser;

    parser.setApplicationDescription("Description: A simple application to play a video file");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption input_1(QStringList() << "i1" << "input1",
            "Directory of input file. (Default: /home/media/videos/vga1.h264)",
            "path to video file", "/home/media/videos/vga1.h264");
    parser.addOption(input_1);

    QCommandLineOption input_2(QStringList() << "i2" << "input2",
            "Directory of input file. (Default: /home/media/videos/vga2.h264)",
            "path to video file", "/home/media/videos/vga2.h264");
    parser.addOption(input_2);

    // Process the actual command line arguments given by the user
    parser.process(app);
    
    // Load and display a QML scene
    viewer.setSource(QUrl(QLatin1String("qrc:/main.qml")));
    // Call function dataScreens
    dataScreens();
    // Passing data to QML components
    viewer.rootContext()->setContextProperty("widthScreen1", widthScreen1);
    viewer.rootContext()->setContextProperty("widthScreen2", widthScreen2);
    viewer.rootContext()->setContextProperty("heightScreen1", heightScreen1);
    viewer.rootContext()->setContextProperty("heightScreen2", heightScreen2);

    if ((!isFileExists(parser.value(input_1))) || (!isSupportedVideo(parser.value(input_1)))) {
        return -1;
    } else {
        viewer.rootContext()->setContextProperty("videosDirPath1", parser.value(input_1));
    }

    if ((!isFileExists(parser.value(input_2))) || (!isSupportedVideo(parser.value(input_2)))) {
        return -1;
    } else {
        viewer.rootContext()->setContextProperty("videosDirPath2", parser.value(input_2));
    }

    // Check max height screen
    if(heightScreen1>heightScreen2) {
        maxHeight=heightScreen1;
    } else {
        maxHeight=heightScreen2;
    }

    // Set size application follows width and height of screen
    viewer.resize(widthScreen1 + widthScreen2, maxHeight);

    // Remove taskbar
    viewer.setFlags(Qt::FramelessWindowHint);
    // Show the window
    viewer.show();
    // Call function init
    QMetaObject::invokeMethod(viewer.rootObject(), "init", Qt::QueuedConnection);
    // Exit program
    QObject::connect((QObject*)viewer.engine(), SIGNAL(quit()), &app, SLOT(quit()));

    return app.exec();
}
