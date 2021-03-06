CONFIG += testcase

SOURCES += tst_qsslsocket.cpp
!wince*:win32:LIBS += -lws2_32
QT += core-private network-private testlib
QT -= gui

TARGET = tst_qsslsocket

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

# OpenSSL support
contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    include($$QT_SOURCE_TREE/config.tests/unix/openssl/openssl.pri)
    # Add optional SSL libs
    LIBS += $$OPENSSL_LIBS
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"

    certFiles.files = certs ssl.tar.gz
    certFiles.path    = .
    DEPLOYMENT += certFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

# QTBUG-23575
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC

requires(contains(QT_CONFIG,private_tests))
