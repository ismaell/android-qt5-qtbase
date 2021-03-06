# -*- Mode: makefile -*-

ARTHUR=$$QT_SOURCE_TREE/tests/arthur
COMMON_FOLDER = $$ARTHUR/common
include($$ARTHUR/arthurtester.pri)
TEMPLATE = app
INCLUDEPATH += $$ARTHUR
DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += xml svg network testlib

contains(QT_CONFIG, opengl):QT += opengl

include($$ARTHUR/datagenerator/datagenerator.pri)

CONFIG += testcase

# Input
HEADERS += atWrapper.h
SOURCES += atWrapperAutotest.cpp atWrapper.cpp

TARGET = tst_atwrapper

#include($$COMMON_FOLDER/common.pri)
