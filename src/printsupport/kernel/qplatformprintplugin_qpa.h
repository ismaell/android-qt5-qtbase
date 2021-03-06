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

#ifndef QPLATFORMPRINTERSUPPORTPLUGIN_H
#define QPLATFORMPRINTERSUPPORTPLUGIN_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPlatformPrinterSupport;

struct QPlatformPrinterSupportFactoryInterface : public QFactoryInterface
{
    virtual QPlatformPrinterSupport *create(const QString &key) = 0;
};

#define QPlatformPrinterSupportFactoryInterface_iid "org.qt-project.QPlatformPrinterSupportFactoryInterface"

Q_DECLARE_INTERFACE(QPlatformPrinterSupportFactoryInterface, QPlatformPrinterSupportFactoryInterface_iid)

class Q_PRINTSUPPORT_EXPORT QPlatformPrinterSupportPlugin : public QObject, public QPlatformPrinterSupportFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QPlatformPrinterSupportFactoryInterface:QFactoryInterface)
public:
    explicit QPlatformPrinterSupportPlugin(QObject *parent = 0);
    ~QPlatformPrinterSupportPlugin();

    virtual QStringList keys() const = 0;
    virtual QPlatformPrinterSupport *create(const QString &key) = 0;

    static QPlatformPrinterSupport *get();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMPRINTERSUPPORTPLUGIN_H
