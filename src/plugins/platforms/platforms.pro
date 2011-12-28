TEMPLATE = subdirs

qpa:CONFIG(android): SUBDIRS += android
else:SUBDIRS += minimal

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac {
    SUBDIRS += cocoa
}

win32: SUBDIRS += windows
