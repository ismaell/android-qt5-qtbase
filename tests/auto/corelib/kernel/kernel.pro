TEMPLATE=subdirs
SUBDIRS=\
    qcoreapplication \
    qeventloop \
    qmath \
    qmetaobject \
    qmetaobjectbuilder \
    qmetaproperty \
    qmetatype \
    qmimedata \
    qobject \
    qpointer \
    qsignalmapper \
    qsocketnotifier \
    qtimer \
    qtranslator \
    qvariant \
    qwineventnotifier

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qsocketnotifier

# This test is only applicable on Windows
!win32*:SUBDIRS -= qwineventnotifier

