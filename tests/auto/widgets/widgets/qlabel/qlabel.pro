CONFIG += testcase
TARGET = tst_qlabel

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += tst_qlabel.cpp
wince*::DEFINES += SRCDIR=\\\"\\\"
else:DEFINES += SRCDIR=\\\"$$PWD/\\\"
wince* {
    addFiles.files = *.png \
        testdata
    addFiles.path = .
    DEPLOYMENT += addFiles
}

