TEMPLATE = subdirs
contains(QT_CONFIG, dbus) {
!macx:!win32:SUBDIRS += ibus
}
linux-g++-maemo: SUBDIRS += meego
