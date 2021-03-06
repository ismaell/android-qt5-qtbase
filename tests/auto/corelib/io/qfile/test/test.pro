CONFIG += testcase
QT = core network testlib
TARGET = ../tst_qfile
SOURCES = ../tst_qfile.cpp
RESOURCES += ../qfile.qrc ../rename-fallback.qrc ../copy-fallback.qrc

TESTDATA += ../dosfile.txt ../noendofline.txt ../testfile.txt \
            ../testlog.txt ../two.dots.file ../tst_qfile.cpp \
            ../Makefile ../forCopying.txt ../forRenaming.txt \
            ../resources/file1.ext1

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qfile
    } else {
        TARGET = ../../release/tst_qfile
    }
    LIBS+=-lole32 -luuid
}

mac*:CONFIG+=insignificant_test
