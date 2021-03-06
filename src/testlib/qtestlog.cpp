/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include <QtTest/qtestassert.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qabstracttestlogger_p.h>
#include <QtTest/private/qplaintestlogger_p.h>
#include <QtTest/private/qxunittestlogger_p.h>
#include <QtTest/private/qxmltestlogger_p.h>
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>

#include <stdlib.h>
#include <string.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QTest {

    struct IgnoreResultList
    {
        inline IgnoreResultList(QtMsgType tp, const char *message)
            : type(tp), next(0)
        { msg = qstrdup(message); }
        inline ~IgnoreResultList()
        { delete [] msg; }

        static inline void clearList(IgnoreResultList *&list)
        {
            while (list) {
                IgnoreResultList *current = list;
                list = list->next;
                delete current;
            }
        }

        QtMsgType type;
        char *msg;
        IgnoreResultList *next;
    };

    static IgnoreResultList *ignoreResultList = 0;

    struct LoggerList
    {
        QAbstractTestLogger *logger;
        LoggerList *next;
    };

    class TestLoggers
    {
    public:
        static void addLogger(QAbstractTestLogger *logger)
        {
            LoggerList *l = new LoggerList;
            l->logger = logger;
            l->next = loggers;
            loggers = l;
        }

        static void destroyLoggers()
        {
            while (loggers) {
                LoggerList *l = loggers;
                loggers = loggers->next;
                delete l->logger;
                delete l;
            }
        }

#define FOREACH_LOGGER(operation) \
        LoggerList *l = loggers; \
        while (l) { \
            QAbstractTestLogger *logger = l->logger; \
            Q_UNUSED(logger); \
            operation; \
            l = l->next; \
        }

        static void startLogging()
        {
            FOREACH_LOGGER(logger->startLogging());
        }

        static void stopLogging()
        {
            FOREACH_LOGGER(logger->stopLogging());
        }

        static void enterTestFunction(const char *function)
        {
            FOREACH_LOGGER(logger->enterTestFunction(function));
        }

        static void leaveTestFunction()
        {
            FOREACH_LOGGER(logger->leaveTestFunction());
        }

        static void addIncident(QAbstractTestLogger::IncidentTypes type, const char *description,
                                const char *file = 0, int line = 0)
        {
            FOREACH_LOGGER(logger->addIncident(type, description, file, line));
        }

        static void addBenchmarkResult(const QBenchmarkResult &result)
        {
            FOREACH_LOGGER(logger->addBenchmarkResult(result));
        }

        static void addMessage(QAbstractTestLogger::MessageTypes type, const char *message,
                               const char *file = 0, int line = 0)
        {
            FOREACH_LOGGER(logger->addMessage(type, message, file, line));
        }

        static void outputString(const char *msg)
        {
            FOREACH_LOGGER(logger->outputString(msg));
        }

        static int loggerCount()
        {
            int count = 0;
            FOREACH_LOGGER(++count);
            return count;
        }

    private:
        static LoggerList *loggers;
    };

#undef FOREACH_LOGGER

    LoggerList *TestLoggers::loggers = 0;
    static bool loggerUsingStdout = false;

    static int verbosity = 0;
    static int maxWarnings = 2002;

    static QtMsgHandler oldMessageHandler;

    static bool handleIgnoredMessage(QtMsgType type, const char *msg)
    {
        IgnoreResultList *last = 0;
        IgnoreResultList *list = ignoreResultList;
        while (list) {
            if (list->type == type && strcmp(msg, list->msg) == 0) {
                // remove the item from the list
                if (last)
                    last->next = list->next;
                else if (list->next)
                    ignoreResultList = list->next;
                else
                    ignoreResultList = 0;

                delete list;
                return true;
            }

            last = list;
            list = list->next;
        }
        return false;
    }

    static void messageHandler(QtMsgType type, const char *msg)
    {
        static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(QTest::maxWarnings);

        if (!msg || QTest::TestLoggers::loggerCount() == 0) {
            // if this goes wrong, something is seriously broken.
            qInstallMsgHandler(oldMessageHandler);
            QTEST_ASSERT(msg);
            QTEST_ASSERT(QTest::TestLoggers::loggerCount() != 0);
        }

        if (handleIgnoredMessage(type, msg))
            // the message is expected, so just swallow it.
            return;

        if (type != QtFatalMsg) {
            if (counter.load() <= 0)
                return;

            if (!counter.deref()) {
                QTest::TestLoggers::addMessage(QAbstractTestLogger::QSystem,
                        "Maximum amount of warnings exceeded. Use -maxwarnings to override.");
                return;
            }
        }

        switch (type) {
        case QtDebugMsg:
            QTest::TestLoggers::addMessage(QAbstractTestLogger::QDebug, msg);
            break;
        case QtCriticalMsg:
            QTest::TestLoggers::addMessage(QAbstractTestLogger::QSystem, msg);
            break;
        case QtWarningMsg:
            QTest::TestLoggers::addMessage(QAbstractTestLogger::QWarning, msg);
            break;
        case QtFatalMsg:
            QTest::TestLoggers::addMessage(QAbstractTestLogger::QFatal, msg);
            /* Right now, we're inside the custom message handler and we're
             * being qt_message_output in qglobal.cpp. After we return from
             * this function, it will proceed with calling exit() and abort()
             * and hence crash. Therefore, we call these logging functions such
             * that we wrap up nicely, and in particular produce well-formed XML. */
            QTestResult::addFailure("Received a fatal error.", "Unknown file", 0);
            QTestLog::leaveTestFunction();
            QTestLog::stopLogging();
            break;
        }
    }
}

void QTestLog::enterTestFunction(const char* function)
{
    if (printAvailableTags)
        return;

    QTEST_ASSERT(function);

    QTest::TestLoggers::enterTestFunction(function);
}

int QTestLog::unhandledIgnoreMessages()
{
    int i = 0;
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        ++i;
        list = list->next;
    }
    return i;
}

void QTestLog::leaveTestFunction()
{
    if (printAvailableTags)
        return;

    QTest::IgnoreResultList::clearList(QTest::ignoreResultList);
    QTest::TestLoggers::leaveTestFunction();
}

void QTestLog::printUnhandledIgnoreMessages()
{
    char msg[1024];
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        qsnprintf(msg, 1024, "Did not receive message: \"%s\"", list->msg);
        QTest::TestLoggers::addMessage(QAbstractTestLogger::Info, msg);

        list = list->next;
    }
}

void QTestLog::addPass(const char *msg)
{
    if (printAvailableTags)
        return;

    QTEST_ASSERT(msg);

    QTest::TestLoggers::addIncident(QAbstractTestLogger::Pass, msg);
}

void QTestLog::addFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    QTest::TestLoggers::addIncident(QAbstractTestLogger::Fail, msg, file, line);
}

void QTestLog::addXFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::TestLoggers::addIncident(QAbstractTestLogger::XFail, msg, file, line);
}

void QTestLog::addXPass(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::TestLoggers::addIncident(QAbstractTestLogger::XPass, msg, file, line);
}

void QTestLog::addSkip(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::TestLoggers::addMessage(QAbstractTestLogger::Skip, msg, file, line);
}

void QTestLog::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTest::TestLoggers::addBenchmarkResult(result);
}

void QTestLog::startLogging()
{
    QTest::TestLoggers::startLogging();
    QTest::oldMessageHandler = qInstallMsgHandler(QTest::messageHandler);
}

void QTestLog::stopLogging()
{
    qInstallMsgHandler(QTest::oldMessageHandler);
    QTest::TestLoggers::stopLogging();
    QTest::TestLoggers::destroyLoggers();
    QTest::loggerUsingStdout = false;
}

void QTestLog::addLogger(LogMode mode, const char *filename)
{
    if (filename && strcmp(filename, "-") == 0)
        filename = 0;
    if (!filename)
        QTest::loggerUsingStdout = true;

    QAbstractTestLogger *logger = 0;
    switch (mode) {
    case QTestLog::Plain:
        logger = new QPlainTestLogger(filename);
        break;
    case QTestLog::XML:
        logger = new QXmlTestLogger(QXmlTestLogger::Complete, filename);
        break;
    case QTestLog::LightXML:
        logger = new QXmlTestLogger(QXmlTestLogger::Light, filename);
        break;
    case QTestLog::XunitXML:
        logger = new QXunitTestLogger(filename);
        break;
    }
    QTEST_ASSERT(logger);
    QTest::TestLoggers::addLogger(logger);
}

int QTestLog::loggerCount()
{
    return QTest::TestLoggers::loggerCount();
}

bool QTestLog::loggerUsingStdout()
{
    return QTest::loggerUsingStdout;
}

void QTestLog::warn(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::TestLoggers::loggerCount() > 0)
        QTest::TestLoggers::addMessage(QAbstractTestLogger::Warn, msg, file, line);
}

void QTestLog::info(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    QTest::TestLoggers::addMessage(QAbstractTestLogger::Info, msg, file, line);
}

void QTestLog::setVerboseLevel(int level)
{
    QTest::verbosity = level;
}

int QTestLog::verboseLevel()
{
    return QTest::verbosity;
}

void QTestLog::addIgnoreMessage(QtMsgType type, const char *msg)
{
    QTEST_ASSERT(msg);

    QTest::IgnoreResultList *item = new QTest::IgnoreResultList(type, msg);

    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    if (!list) {
        QTest::ignoreResultList = item;
        return;
    }
    while (list->next)
        list = list->next;
    list->next = item;
}

void QTestLog::setMaxWarnings(int m)
{
    QTest::maxWarnings = m <= 0 ? INT_MAX : m + 2;
}

bool QTestLog::printAvailableTags = false;

void QTestLog::setPrintAvailableTagsMode()
{
    printAvailableTags = true;
}

QT_END_NAMESPACE
