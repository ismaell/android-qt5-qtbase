TEMPLATE = app
SOURCES = raycasting.cpp
RESOURCES += raycasting.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/embedded/raycasting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/embedded/raycasting
INSTALLS += target sources
QT += widgets widgets
