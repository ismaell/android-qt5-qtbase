/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QSemaphore>
#include <QThread>

class tst_QMutexLockerThread : public QThread
{
public:
    QMutex mutex;
    QSemaphore semaphore, testSemaphore;

    void waitForTest()
    {
        semaphore.release();
        testSemaphore.acquire();
    }

    tst_QMutexLockerThread()
        : mutex(QMutex::Recursive)
    {
    }
};

class tst_QMutexLocker : public QObject
{
    Q_OBJECT

public:
    tst_QMutexLockerThread *thread;

    void waitForThread()
    {
        thread->semaphore.acquire();
    }
    void releaseThread()
    {
        thread->testSemaphore.release();
    }

private slots:
    void scopeTest();
    void unlockAndRelockTest();
    void lockerStateTest();
};

void tst_QMutexLocker::scopeTest()
{
    class ScopeTestThread : public tst_QMutexLockerThread
    {
    public:
        void run()
        {
            waitForTest();

            {
                QMutexLocker locker(&mutex);
                waitForTest();
            }

            waitForTest();
        }
    };

    thread = new ScopeTestThread;
    thread->start();

    waitForThread();
    // mutex should be unlocked before entering the scope that creates the QMutexLocker
    QVERIFY(thread->mutex.tryLock());
    thread->mutex.unlock();
    releaseThread();

    waitForThread();
    // mutex should be locked by the QMutexLocker
    QVERIFY(!thread->mutex.tryLock());
    releaseThread();

    waitForThread();
    // mutex should be unlocked when the QMutexLocker goes out of scope
    QVERIFY(thread->mutex.tryLock());
    thread->mutex.unlock();
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = 0;
}


void tst_QMutexLocker::unlockAndRelockTest()
{
    class UnlockAndRelockThread : public tst_QMutexLockerThread
    {
    public:
        void run()
        {
            QMutexLocker locker(&mutex);

            waitForTest();

            locker.unlock();

            waitForTest();

            locker.relock();

            waitForTest();
        }
    };

    thread = new UnlockAndRelockThread;
    thread->start();

    waitForThread();
    // mutex should be locked by the QMutexLocker
    QVERIFY(!thread->mutex.tryLock());
    releaseThread();

    waitForThread();
    // mutex has been explicitly unlocked via QMutexLocker
    QVERIFY(thread->mutex.tryLock());
    thread->mutex.unlock();
    releaseThread();

    waitForThread();
    // mutex has been explicitly relocked via QMutexLocker
    QVERIFY(!thread->mutex.tryLock());
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = 0;
}

void tst_QMutexLocker::lockerStateTest()
{
    class LockerStateThread : public tst_QMutexLockerThread
    {
    public:
        void run()
        {
            {
                QMutexLocker locker(&mutex);
                locker.relock();
                locker.unlock();

                waitForTest();
            }

            waitForTest();
        }
    };

    thread = new LockerStateThread;
    thread->start();

    waitForThread();
    // even though we relock() after creating the QMutexLocker, it shouldn't lock the mutex more than once
    QVERIFY(thread->mutex.tryLock());
    thread->mutex.unlock();
    releaseThread();

    waitForThread();
    // if we call QMutexLocker::unlock(), its destructor should do nothing
    QVERIFY(thread->mutex.tryLock());
    thread->mutex.unlock();
    releaseThread();

    QVERIFY(thread->wait());

    delete thread;
    thread = 0;
}

QTEST_MAIN(tst_QMutexLocker)
#include "tst_qmutexlocker.moc"
