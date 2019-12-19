#-------------------------------------------------
#
# Project created by QtCreator 2019-12-16T21:49:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = virtualdisk
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    file_creator.cpp \
    file_item.cpp

HEADERS += \
    file_operator.h \
        mainwindow.h \
    defines.h \
    fat32_file.h \
    fat32_file_reader.h \
    dbr_operator.h \
    file_creator.h \
    file_extracter.h \
    file_item.h \
    tree.h

FORMS += \
        mainwindow.ui \
    file_creator.ui

