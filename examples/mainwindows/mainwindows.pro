TEMPLATE      = subdirs
SUBDIRS       = application \
		dockwidgets \
                mainwindow \
                mdi \
                menus \
                recentfiles \
                sdi
# ### fixme
# mac* && !qpa: SUBDIRS += macmainwindow

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS mainwindows.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows
INSTALLS += target sources

QT += widgets
