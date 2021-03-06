QT += core gui widgets

TEMPLATE = app

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(../src/src.pri)
include(../git-version/git-version.pri)

TARGET = e2designer

SOURCES += \
        main.cpp

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

unix {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }

    target.path = $$PREFIX/bin

    desktopFiles.files = ../misc/e2designer.desktop
    desktopFiles.path = $$PREFIX/share/applications/

    iconFiles.files = ../misc/e2designer.png
    iconFiles.path = $$PREFIX/share/icons/hicolor/256x256/apps/

    miscFiles.files = ../misc/e2designer.appdata.xml
    miscFiles.path = $$PREFIX/share/metainfo/

    INSTALLS += desktopFiles iconFiles miscFiles
}

macx {
    icon_file.target = icon.icns
    icon_file.depends = ../misc/e2designer.png
    icon_file.commands = ../mkicns.sh $${icon_file.depends} $${icon_file.target}
    QMAKE_EXTRA_TARGETS += icon_file
    ICON = $${icon_file.target}
}

INSTALLS += target
