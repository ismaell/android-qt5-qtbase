QT += widgets

HEADERS       = client.h
SOURCES       = client.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/fortuneclient
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fortuneclient.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/fortuneclient
INSTALLS += target sources
