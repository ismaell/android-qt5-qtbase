CONFIG += testcase parallel_test
TARGET = tst_largefile
QT = core testlib
SOURCES = tst_largefile.cpp

wince*: SOURCES += $$QT_SOURCE_TREE/src/corelib/kernel/qfunctions_wince.cpp
