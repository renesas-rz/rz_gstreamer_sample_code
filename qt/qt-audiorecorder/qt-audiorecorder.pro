TEMPLATE = app
TARGET = qt-audiorecorder

QT += multimedia

win32:INCLUDEPATH += $$PWD

HEADERS = \
    qt-audiorecorder.h \
    qaudiolevel.h \
    ui_qt-audiorecorder.h

SOURCES = \
    main.cpp \
    qt-audiorecorder.cpp \
    qaudiolevel.cpp

FORMS += qt-audiorecorder.ui

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/qt-audiorecorder
INSTALLS += target

QT+=widgets

DISTFILES +=
