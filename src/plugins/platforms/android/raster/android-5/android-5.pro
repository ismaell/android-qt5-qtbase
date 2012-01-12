TARGET = android-5

DEFINES += QT_STATICPLUGIN

CONFIG += dll

INCLUDEPATH += $$NDK_ROOT/platforms/android-5/arch-arm/usr/include

INCLUDEPATH += $$QT_SOURCE_TREE/src/plugins/platforms/android/src/native/include
INCLUDEPATH += $$QT_SOURCE_TREE/src/plugins/platforms/android/src/native/include/android
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/skia/include

SOURCES += $$QT_SOURCE_TREE/src/plugins/platforms/android/src/native/graphics/jni/bitmap.cpp
SOURCES += $$QT_SOURCE_TREE/src/plugins/platforms/android/src/native/android/asset_manager.cpp

INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/android/precompiled/android-5/arch-arm/include
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/android/precompiled/android-5/arch-arm/include/core

LIBS += -L$$QT_SOURCE_TREE/src/3rdparty/android/precompiled/android-5/arch-arm/lib -landroid -lutils
LIBS += $$QT_SOURCE_TREE/src/3rdparty/skia/prebuilt/libskia.a

include($$QT_SOURCE_TREE/src/plugins/platforms/android/src/src.pri)
include($$QT_SOURCE_TREE/src/plugins/platforms/android/src/raster/raster.pri)

INSTALLS += target
