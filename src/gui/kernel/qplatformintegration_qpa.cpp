/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformintegration_qpa.h"

#include <QtGui/QPlatformFontDatabase>
#include <QtGui/QPlatformClipboard>
#include <QtGui/QPlatformAccessibility>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <private/qdnd_p.h>

QT_BEGIN_NAMESPACE

/*!
    Accessor for the platform integration's fontdatabase.

    Default implementation returns a default QPlatformFontDatabase.

    \sa QPlatformFontDatabase
*/
QPlatformFontDatabase *QPlatformIntegration::fontDatabase() const
{
    static QPlatformFontDatabase *db = 0;
    if (!db) {
        db = new QPlatformFontDatabase;
    }
    return db;
}

/*!
    Accessor for the platform integration's clipboard.

    Default implementation returns a default QPlatformClipboard.

    \sa QPlatformClipboard

*/

#ifndef QT_NO_CLIPBOARD

QPlatformClipboard *QPlatformIntegration::clipboard() const
{
    static QPlatformClipboard *clipboard = 0;
    if (!clipboard) {
        clipboard = new QPlatformClipboard;
    }
    return clipboard;
}

#endif

#ifndef QT_NO_DRAGANDDROP
/*!
    Accessor for the platform integration's drag object.

    Default implementation returns 0, implying no drag and drop support.

*/
QPlatformDrag *QPlatformIntegration::drag() const
{
    return 0;
}
#endif

QPlatformNativeInterface * QPlatformIntegration::nativeInterface() const
{
    return 0;
}

/*!
    \class QPlatformIntegration
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformIntegration class is the entry for WindowSystem specific functionality.

    QPlatformIntegration is the single entry point for windowsystem specific functionality when
    using the QPA platform. It has factory functions for creating platform specific pixmaps and
    windows. The class also controls the font subsystem.

    QPlatformIntegration is a singleton class which gets instantiated in the QGuiApplication
    constructor. The QPlatformIntegration instance do not have ownership of objects it creates in
    functions where the name starts with create. However, functions which don't have a name
    starting with create acts as accessors to member variables.

    It is not trivial to create or build a platform plugin outside of the Qt source tree. Therefore
    the recommended approach for making new platform plugin is to copy an existing plugin inside
    the QTSRCTREE/src/plugins/platform and develop the plugin inside the source tree.

    The minimal platform integration is the smallest platform integration it is possible to make,
    which makes it an ideal starting point for new plugins. For a slightly more advanced plugin,
    consider reviewing the directfb plugin, or the testlite plugin.
*/

/*!
    \fn QPlatformPixmap *QPlatformIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const

    Factory function for QPlatformPixmap. PixelType can be either PixmapType or BitmapType.
    \sa QPlatformPixmap
*/

/*!
    \fn QPlatformWindow *QPlatformIntegration::createPlatformWindow(QWindow *window) const

    Factory function for QPlatformWindow. The \a window parameter is a pointer to the top level
    window which the QPlatformWindow is supposed to be created for.

    All top level windows have to have a QPlatformWindow, and it will be created when the
    QPlatformWindow is set to be visible for the first time. If the top level window's flags are
    changed, or if the top level window's QPlatformWindowFormat is changed, then the top level
    window's QPlatformWindow is deleted and a new one is created.

    In the constructor, of the QPlatformWindow, the window flags, state, title and geometry
    of the \a window should be applied to the underlying window. If the resulting flags or state
    differs, the resulting values should be set on the \a window using QWindow::setWindowFlags()
    or QWindow::setWindowState(), respectively.

    \sa QPlatformWindow, QPlatformWindowFormat
    \sa createPlatformBackingStore(QWindow *window) const
*/

/*!
    \fn QPlatformBackingStore *QPlatformIntegration::createPlatformBackingStore(QWindow *window) const

    Factory function for QPlatformBackingStore. The QWindow parameter is a pointer to the
    top level widget(tlw) the window surface is created for. A QPlatformWindow is always created
    before the QPlatformBackingStore for tlw where the widget also requires a backing store.

    \sa QBackingStore
    \sa createPlatformWindow(QWindow *window, WId winId = 0) const
*/

/*!

    \fn QAbstractEventDispatcher *QPlatformIntegration::guiThreadEventDispatcher() const = 0

    Accessor function for the event dispatcher. The platform plugin should create
    an instance of the QAbstractEventDispatcher in its constructor and set it
    on the application using QGuiApplicationPrivate::instance()->setEventDispatcher().
    The event dispatcher is owned by QGuiApplication, the accessor should return
    a flat pointer.
    \sa QGuiApplicationPrivate
*/

bool QPlatformIntegration::hasCapability(Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QPlatformPixmap *QPlatformIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    return new QRasterPlatformPixmap(type);
}

QPlatformOpenGLContext *QPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    Q_UNUSED(context);
    qWarning("This plugin does not support createPlatformOpenGLContext!");
    return 0;
}

/*!
  Returns the platforms input context.

  The default implementation returns 0, implying no input method support.
*/
QPlatformInputContext *QPlatformIntegration::inputContext() const
{
    return 0;
}

/*!
  Returns the platforms accessibility.

  The default implementation returns 0, implying no accessibility support.
*/
QPlatformAccessibility *QPlatformIntegration::accessibility() const
{
    return 0;
}

QVariant QPlatformIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case CursorFlashTime:
        return 1000;
    case KeyboardInputInterval:
        return 400;
    case KeyboardAutoRepeatRate:
        return 30;
    case MouseDoubleClickInterval:
        return 400;
    case StartDragDistance:
        return 10;
    case StartDragTime:
        return 500;
    }

    return 0;
}

/*!
  Should be called by the implementation whenever a new screen is added.

  The first screen added will be the primary screen, used for default-created
  windows, GL contexts, and other resources unless otherwise specified.

  This adds the screen to QGuiApplication::screens(), and emits the
  QGuiApplication::screenAdded() signal.

  The screen is automatically removed when the QPlatformScreen is destroyed.
*/
void QPlatformIntegration::screenAdded(QPlatformScreen *ps)
{
    QScreen *screen = ps ? ps->screen() : 0;
    if (screen && !QGuiApplicationPrivate::screen_list.contains(screen)) {
        QGuiApplicationPrivate::screen_list << screen;
        emit qGuiApp->screenAdded(screen);
    }
}

class QPlatformTheme *QPlatformIntegration::platformTheme() const
{
    return 0;
}

QT_END_NAMESPACE
