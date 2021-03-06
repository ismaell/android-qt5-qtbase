/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSKEYMAPPER_H
#define QWINDOWSKEYMAPPER_H

#include "qtwindows_additional.h"

#include <QtCore/QLocale>

QT_BEGIN_NAMESPACE

class QWindow;

struct KeyboardLayoutItem;

class QWindowsKeyMapper
{
    Q_DISABLE_COPY(QWindowsKeyMapper)
public:
    explicit QWindowsKeyMapper();
    ~QWindowsKeyMapper();

    void changeKeyboard();

    void setUseRTLExtensions(bool e) { m_useRTLExtensions = e; }
    bool useRTLExtensions() const    { return m_useRTLExtensions; }

    bool translateKeyEvent(QWindow *widget, HWND hwnd, const MSG &msg, LRESULT *result);

    QWindow *keyGrabber() const      { return m_keyGrabber; }
    void setKeyGrabber(QWindow *w)   { m_keyGrabber = w; }

private:
    bool translateKeyEventInternal(QWindow *receiver, const MSG &msg, bool grab);
    void updateKeyMap(const MSG &msg);

    bool m_useRTLExtensions;

    QLocale keyboardInputLocale;
    Qt::LayoutDirection keyboardInputDirection;

    void clearRecordedKeys();
    void updatePossibleKeyCodes(unsigned char *kbdBuffer, quint32 scancode, quint32 vk_key);
    bool isADeadKey(unsigned int vk_key, unsigned int modifiers);
    void deleteLayouts();

    KeyboardLayoutItem *keyLayout[256];
    QWindow *m_keyGrabber;
};

enum WindowsNativeModifiers {
    ShiftLeft            = 0x00000001,
    ControlLeft          = 0x00000002,
    AltLeft              = 0x00000004,
    MetaLeft             = 0x00000008,
    ShiftRight           = 0x00000010,
    ControlRight         = 0x00000020,
    AltRight             = 0x00000040,
    MetaRight            = 0x00000080,
    CapsLock             = 0x00000100,
    NumLock              = 0x00000200,
    ScrollLock           = 0x00000400,
    ExtendedKey          = 0x01000000,

    // Convenience mappings
    ShiftAny             = 0x00000011,
    ControlAny           = 0x00000022,
    AltAny               = 0x00000044,
    MetaAny              = 0x00000088,
    LockAny              = 0x00000700
};

QT_END_NAMESPACE

#endif // QWINDOWSKEYMAPPER_H
