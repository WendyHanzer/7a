#-------------------------------------------------
#
# Project created by QtCreator 2014-09-24T16:52:26
#
#-------------------------------------------------

QT       += core gui opengl widgets

TARGET = cs791a
TEMPLATE = app

INCLUDEPATH += /usr/include \
            /usr/include/gdal \
            /usr/local/include \
            /usr/loca/include/gdal

SOURCES += main.cpp\
        mainwindow.cpp \
    engine.cpp \
    graphics.cpp \
    camera.cpp \
    terrain.cpp \
    shape.cpp

HEADERS  += mainwindow.h \
    engine.h \
    graphics.h \
    gl.h \
    camera.h \
    terrain.h \
    vertex.h \
    shape.h

CONFIG += c++11
QMAKE_CXX = clang++
QMAKE_CC = clang

unix {
    LIBS += -lGLEW
}

mac {
    LIBS += -framework OpenGL
}

LIBS += -lboost_program_options -lboost_system -lboost_filesystem -lgdal
