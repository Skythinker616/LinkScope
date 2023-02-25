QT       += core gui
QT       += network
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS \
    APP_VERSION=\\\"1.3.0\\\"

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutwindow.cpp \
    configwindow.cpp \
    gdbprocess.cpp \
    graphwindow.cpp \
    helpwindow.cpp \
    listwindow.cpp \
    logwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    openocd.cpp \
    serialocd.cpp

HEADERS += \
    aboutwindow.h \
    configwindow.h \
    gdbprocess.h \
    graphwindow.h \
    helpwindow.h \
    listwindow.h \
    logwindow.h \
    mainwindow.h \
    openocd.h \
    serialocd.h \
    vartype.h

FORMS += \
    aboutwindow.ui \
    configwindow.ui \
    graphwindow.ui \
    helpwindow.ui \
    listwindow.ui \
    logwindow.ui \
    mainwindow.ui

RC_ICONS = icon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
