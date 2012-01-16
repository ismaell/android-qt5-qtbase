CONFIG += testcase
TARGET = tst_qmovie
QT += widgets testlib
SOURCES += tst_qmovie.cpp
MOC_DIR=tmp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
!contains(QT_CONFIG, no-mng):DEFINES += QTEST_HAVE_MNG

RESOURCES += resources.qrc
TESTDATA += animations/*
