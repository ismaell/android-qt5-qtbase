TEMPLATE = lib
CONFIG += dll
CONFIG -= staticlib
SOURCES		= mylib.c
TARGET = mylib
DESTDIR = ../
QT = core

wince*: DEFINES += WIN32_MSVC
win32-msvc: DEFINES += WIN32_MSVC
win32-borland: DEFINES += WIN32_BORLAND

# This project is testdata for tst_qlibrary
target.path = $$[QT_INSTALL_TESTS]/tst_qlibrary
INSTALLS += target
