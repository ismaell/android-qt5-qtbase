CONFIG += testcase
SOURCES += ../tst_qdbusmarshall.cpp
TARGET = ../tst_qdbusmarshall

QT = core-private dbus-private testlib

LIBS += $$QT_LIBS_DBUS
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
