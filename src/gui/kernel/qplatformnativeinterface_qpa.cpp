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

#include "qplatformnativeinterface_qpa.h"

QT_BEGIN_NAMESPACE

void *QPlatformNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    Q_UNUSED(resource);
    Q_UNUSED(window);
    return 0;
}

void *QPlatformNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    Q_UNUSED(resource);
    Q_UNUSED(context);
    return 0;
}

void * QPlatformNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore)
{
    Q_UNUSED(resource);
    Q_UNUSED(backingStore);
    return 0;
}

/*!
    Contains generic window properties that the platform may utilize.
*/
QVariantMap QPlatformNativeInterface::windowProperties(QPlatformWindow *window) const
{
    Q_UNUSED(window)
    return QVariantMap();
}

/*!
    Returns a window property with \a name.

    If the property does not exist, returns a default-constructed value.
*/
QVariant QPlatformNativeInterface::windowProperty(QPlatformWindow *window, const QString &name) const
{
    Q_UNUSED(window);
    Q_UNUSED(name);
    return QVariant();
}

/*!
    Returns a window property with \a name. If the value does not exist, defaultValue is returned.
*/
QVariant QPlatformNativeInterface::windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const
{
    Q_UNUSED(window);
    Q_UNUSED(name);
    Q_UNUSED(defaultValue);
    return QVariant();
}

/*!
    Sets a window property with \a name to \a value.
*/
void QPlatformNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    Q_UNUSED(window);
    Q_UNUSED(name);
    Q_UNUSED(value);
}

QT_END_NAMESPACE
