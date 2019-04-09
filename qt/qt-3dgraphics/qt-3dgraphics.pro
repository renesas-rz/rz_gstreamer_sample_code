#-------------------------------------------------
#
# Project created by QtCreator 2017-02-15T13:17:07
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt-3dgraphics
TEMPLATE = app


SOURCES += main.cpp\
        vertex.cpp \
    cube.cpp \
    figure.cpp \
    program.cpp \
    sphere.cpp \
    cone.cpp
    cone.cpp \

HEADERS  += \
        vertex.h \
    cube.h \
    figure.h \
    program.h \
    sphere.h \
    cone.h
    cone.h \

DISTFILES +=

RESOURCES += \
    shaders.qrc
