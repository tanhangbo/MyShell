#-------------------------------------------------
#
# Project created by QtCreator 2015-07-14T13:22:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = serial_tan
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp

HEADERS  += dialog.h \

FORMS    += dialog.ui


QT += serialport

RC_ICONS += connect.ico
