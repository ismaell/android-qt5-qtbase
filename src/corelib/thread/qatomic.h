/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>

#ifndef QATOMIC_H
#define QATOMIC_H

#include <QtCore/qoldbasicatomic.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wextra"
#endif

// High-level atomic integer operations
class Q_CORE_EXPORT QAtomicInt : public QBasicAtomicInt
{
public:
    // Non-atomic API
    inline QAtomicInt(int value = 0)
    {
#ifdef QT_ARCH_PARISC
        this->_q_lock[0] = this->_q_lock[1] = this->_q_lock[2] = this->_q_lock[3] = -1;
#endif
        _q_value = value;
    }

    inline QAtomicInt(const QAtomicInt &other)
    {
#ifdef QT_ARCH_PARISC
        this->_q_lock[0] = this->_q_lock[1] = this->_q_lock[2] = this->_q_lock[3] = -1;
#endif
        store(other.load());
    }

    inline QAtomicInt &operator=(const QAtomicInt &other)
    {
        this->store(other.load());
        return *this;
    }

#ifdef qdoc
    static bool isReferenceCountingNative();
    static bool isReferenceCountingWaitFree();

    bool ref();
    bool deref();

    static bool isTestAndSetNative();
    static bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(int expectedValue, int newValue);
    bool testAndSetAcquire(int expectedValue, int newValue);
    bool testAndSetRelease(int expectedValue, int newValue);
    bool testAndSetOrdered(int expectedValue, int newValue);

    static bool isFetchAndStoreNative();
    static bool isFetchAndStoreWaitFree();

    int fetchAndStoreRelaxed(int newValue);
    int fetchAndStoreAcquire(int newValue);
    int fetchAndStoreRelease(int newValue);
    int fetchAndStoreOrdered(int newValue);

    static bool isFetchAndAddNative();
    static bool isFetchAndAddWaitFree();

    int fetchAndAddRelaxed(int valueToAdd);
    int fetchAndAddAcquire(int valueToAdd);
    int fetchAndAddRelease(int valueToAdd);
    int fetchAndAddOrdered(int valueToAdd);
#endif
};

// High-level atomic pointer operations
template <typename T>
class QAtomicPointer : public QBasicAtomicPointer<T>
{
public:
    inline QAtomicPointer(T *value = 0)
    {
#ifdef QT_ARCH_PARISC
        this->_q_lock[0] = this->_q_lock[1] = this->_q_lock[2] = this->_q_lock[3] = -1;
#endif
        this->store(value);
    }
    inline QAtomicPointer(const QAtomicPointer<T> &other)
    {
#ifdef QT_ARCH_PARISC
        this->_q_lock[0] = this->_q_lock[1] = this->_q_lock[2] = this->_q_lock[3] = -1;
#endif
        this->store(other.load());
    }

    inline QAtomicPointer<T> &operator=(const QAtomicPointer<T> &other)
    {
        this->store(other.load());
        return *this;
    }

#ifdef qdoc
    static bool isTestAndSetNative();
    static bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T *expectedValue, T *newValue);
    bool testAndSetAcquire(T *expectedValue, T *newValue);
    bool testAndSetRelease(T *expectedValue, T *newValue);
    bool testAndSetOrdered(T *expectedValue, T *newValue);

    static bool isFetchAndStoreNative();
    static bool isFetchAndStoreWaitFree();

    T *fetchAndStoreRelaxed(T *newValue);
    T *fetchAndStoreAcquire(T *newValue);
    T *fetchAndStoreRelease(T *newValue);
    T *fetchAndStoreOrdered(T *newValue);

    static bool isFetchAndAddNative();
    static bool isFetchAndAddWaitFree();

    T *fetchAndAddRelaxed(qptrdiff valueToAdd);
    T *fetchAndAddAcquire(qptrdiff valueToAdd);
    T *fetchAndAddRelease(qptrdiff valueToAdd);
    T *fetchAndAddOrdered(qptrdiff valueToAdd);
#endif
};

#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406)
# pragma GCC diagnostic pop
#endif

/*!
    This is a helper for the assignment operators of implicitly
    shared classes. Your assignment operator should look like this:

    \snippet doc/src/snippets/code/src.corelib.thread.qatomic.h 0
*/
template <typename T>
inline void qAtomicAssign(T *&d, T *x)
{
    if (d == x)
        return;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

/*!
    This is a helper for the detach method of implicitly shared
    classes. Your private class needs a copy constructor which copies
    the members and sets the refcount to 1. After that, your detach
    function should look like this:

    \snippet doc/src/snippets/code/src.corelib.thread.qatomic.h 1
*/
template <typename T>
inline void qAtomicDetach(T *&d)
{
    if (d->ref.load() == 1)
        return;
    T *x = d;
    d = new T(*d);
    if (!x->ref.deref())
        delete x;
}

QT_END_NAMESPACE
QT_END_HEADER

#endif // QATOMIC_H
