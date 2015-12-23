#-------------------------------------------------
#
# Project created by QtCreator 2015-12-20T08:33:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NNCompression
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    Layer.hpp \
    Network.hpp

FORMS    += mainwindow.ui

CONFIG += c++11

