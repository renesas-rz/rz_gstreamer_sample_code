/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtQml/QQmlContext>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QRect>
#include <QDebug>
#include <QtGui/QScreen>
#include "filereader.h"
#include "trace.h"

#define MINIMIZED_SCALE        4

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    //Show information of screen (all monitors)
    // If 2 screen availabe, chose the larger as it is usually default
    QScreen *screen;
    if (QGuiApplication::screens().size() <= 1) {
        screen = QGuiApplication::primaryScreen();
    } else {
        QScreen *tmp_screen;
        if(QGuiApplication::screens().at(0) == NULL)
            screen = QGuiApplication::screens().at(1);
          else if(QGuiApplication::screens().at(1) == NULL)
            screen = QGuiApplication::screens().at(0);
          else {
            screen = QGuiApplication::screens().at(0);
            tmp_screen = QGuiApplication::screens().at(1);
            if((screen->availableSize().width() * screen->availableSize().height()) < (tmp_screen->availableSize().width() * tmp_screen->availableSize().height()))
                screen = QGuiApplication::screens().at(1);
          }
    }
    qDebug() << "Information for screen:" << screen->name();
    qDebug() << "  Available geometry:" << screen->availableGeometry().x() << screen->availableGeometry().y() << screen->availableGeometry().width() << "x" << screen->availableGeometry().height();
    qDebug() << "  Available size:" << screen->availableSize().width() << "x" << screen->availableSize().height();

    int width = screen->availableSize().width();
    int height = screen->availableSize().height();

    // Load and display a QML scene
    QQuickView viewer;
    viewer.setSource(QUrl(QLatin1String("qrc:/main.qml")));

    // Construct a QUrl by parsing a local file path.
    const QUrl appPath(QUrl::fromLocalFile(app.applicationDirPath()));
    const QUrl imagePath = appPath ;
    viewer.rootContext()->setContextProperty("imagePath", imagePath);
    const QUrl videoPath =  appPath ;

    viewer.rootContext()->setContextProperty("videoPath", videoPath);
    viewer.setTitle("Qt multimediaplayer");
    viewer.setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint |
                          Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    // Set available width and height of the screen 
    viewer.rootContext()->setContextProperty("availableWidthScreen", width);
    viewer.rootContext()->setContextProperty("availableHeightScreen", height);

    viewer.setMaximumSize(QSize(width, height));
    // resize to set minimum window
    viewer.resize(width/MINIMIZED_SCALE, height/MINIMIZED_SCALE);
    viewer.showMaximized();

    // Call function init
    QMetaObject::invokeMethod(viewer.rootObject(), "init", Qt::QueuedConnection);

    // Exit program
    QObject::connect((QObject*)viewer.engine(), SIGNAL(quit()), &app, SLOT(quit()));

    return app.exec();
}




