TEMPLATE        = app
TARGET          = sqlbrowser

QT              += sql widgets

HEADERS         = browser.h connectionwidget.h qsqlconnectiondialog.h
SOURCES         = main.cpp browser.cpp connectionwidget.cpp qsqlconnectiondialog.cpp

FORMS           = browserwidget.ui qsqlconnectiondialog.ui
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/sqlbrowser
sources.files = $$SOURCES $$HEADERS $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/sqlbrowser
INSTALLS += target sources


wince*: {
    DEPLOYMENT_PLUGIN += qsqlite
}
