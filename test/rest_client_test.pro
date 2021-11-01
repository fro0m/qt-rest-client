include(../RESTclient.pri)

QT += core testlib concurrent
QT -= gui

CONFIG += c++14

TARGET = rest_client_test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
           rest_client_test.cpp \

HEADERS += rest_client_test.h \

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

