/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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
 
#include <QtWidgets/QApplication>
#include <QtGui/QSurfaceFormat>
#include <QRect>
#include <QScreen>

#ifdef QT_NO_OPENGL
    #include <QLabel>
#else
    #include "program.h"
#endif

#define MINIMIZED_SCALE     4

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set minimum depth buffer size to 24
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

#ifndef QT_NO_OPENGL
    // Display 3D figures
    Program program;
    //Show information of screen (all monitors)
    // If 2 screen availabe, chose the larger as it is usually default
    QScreen *screen;
    if (QGuiApplication::screens().size() <= 1)
    {
        screen = QGuiApplication::primaryScreen();
    }
    else
    {
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
    program.setWindowTitle("Qt 3D graphics with textures");
    program.setMaximumSize(width, height);
    // resize to set minimum window
    program.resize(width/MINIMIZED_SCALE, height/MINIMIZED_SCALE);
    program.showMaximized();
#else
    // Show message if OpenGLES 2.0 is not available
    QLabel note("OpenGL Support required");
    note.show();
#endif

    return app.exec();
}
