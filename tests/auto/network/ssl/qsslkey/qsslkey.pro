CONFIG += testcase

SOURCES += tst_qsslkey.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslkey

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

wince* {
    keyFiles.files = keys
    keyFiles.path    = .

    passphraseFiles.files = rsa-without-passphrase.pem rsa-with-passphrase.pem
    passphraseFiles.path    = .

    DEPLOYMENT += keyFiles passphraseFiles
    DEFINES += SRCDIR=\\\".\\\"
} else {
    DEFINES+= SRCDIR=\\\"$$PWD\\\"
    TARGET.CAPABILITY = NetworkServices
}
