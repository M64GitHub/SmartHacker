QT       += core gui
QT       += serialport

QT += widgets

CONFIG += c++11

SOURCES += \
    aes-gcm.c \
    aes.c \
    bytebuffer.cpp \
    dlms-apdu.cpp \
    gcm.c \
    main.cpp \
    mainwindow.cpp \
    rv_hex_edit.cpp \
    rv_hex_edit_undocommands.cpp

HEADERS += \
    aes-gcm.h \
    aes.h \
    bytebuffer.h \
    detect_platform.h \
    dlms-apdu.h \
    gcm.h \
    mainwindow.h \
    rv_hex_edit.h \
    rv_hex_edit_undocommands.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
