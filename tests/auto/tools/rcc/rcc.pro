CONFIG += testcase
QT = core testlib
TARGET = tst_rcc

SOURCES += tst_rcc.cpp

wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

