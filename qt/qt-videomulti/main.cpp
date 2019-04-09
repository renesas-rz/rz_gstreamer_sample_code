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

#include <QtGui/QGuiApplication>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtGui/QScreen>

#define VIDEOSDIRPATH "file:///home/media/videos"

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
        if ((screen->geometry().y() == 0) && (screen->geometry().x() == 0)) {
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
    QQuickView viewer;
    
    viewer.setSource(QUrl(QLatin1String("qrc:/main.qml")));
    // Call function dataScreens
    dataScreens();
    // Passing data to QML components
    viewer.rootContext()->setContextProperty("widthScreen1", widthScreen1);
    viewer.rootContext()->setContextProperty("widthScreen2", widthScreen2);
    viewer.rootContext()->setContextProperty("heightScreen1", heightScreen1);
    viewer.rootContext()->setContextProperty("heightScreen2", heightScreen2);
    viewer.rootContext()->setContextProperty("videosDirPath", VIDEOSDIRPATH);
    // Check max height screen
    if (heightScreen1>heightScreen2) {
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
