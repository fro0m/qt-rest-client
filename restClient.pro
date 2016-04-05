#-------------------------------------------------
#
# Project created by QtCreator 2016-03-25T15:42:30
#
#-------------------------------------------------

QT       += core gui network qml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#include(C:\Users\kofr\Desktop\qtrest-master\com_github_kafeg_qtrest.pri)


TARGET = restClient
TEMPLATE = app

CONFIG += C++11

SOURCES += main.cpp\
        mainwindowe.cpp \
    #deletelater/couponmodel.cpp \
    #deletelater/api/skidkzapi.cpp \
    HttpRequestWorker.cpp

HEADERS  += mainwindowe.h \
    #deletelater/couponmodel.h \
    #deletelater/api/skidkzapi.h \
    HttpRequestWorker.h

DISTFILES +=
