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

#include <qpdfwriter.h>
#include <QtCore/private/qobject_p.h>
#include "private/qpdf_p.h"
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

class QPdfWriterPrivate : public QObjectPrivate
{
public:
    QPdfWriterPrivate()
        : QObjectPrivate()
    {
        engine = new QPdfEngine();
        output = 0;
    }
    ~QPdfWriterPrivate()
    {
        delete engine;
        delete output;
    }

    QPdfEngine *engine;
    QFile *output;
};


/*! \class QPdfWriter

    \brief The QPdfWriter class is a class to generate PDFs
    that can be used as a paint device.

    \ingroup painting

    QPdfWriter generates PDF out of a series of drawing commands using QPainter.
    The newPage() method can be used to create several pages.
  */

/*!
  Constructs a PDF writer that will write the pdf to \a filename.
  */
QPdfWriter::QPdfWriter(const QString &filename)
    : QObject(*new QPdfWriterPrivate)
{
    Q_D(QPdfWriter);

    d->engine->setOutputFilename(filename);
}

/*!
  Constructs a PDF writer that will write the pdf to \a device.
  */
QPdfWriter::QPdfWriter(QIODevice *device)
    : QObject(*new QPdfWriterPrivate)
{
    Q_D(QPdfWriter);

    d->engine->d_func()->outDevice = device;
}

/*!
  Destroys the pdf writer.
  */
QPdfWriter::~QPdfWriter()
{

}

/*!
  Returns the title of the document.
  */
QString QPdfWriter::title() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->title;
}

/*!
  Sets the title of the document being created.
  */
void QPdfWriter::setTitle(const QString &title)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->title = title;
}

/*!
  Returns the creator of the document.
  */
QString QPdfWriter::creator() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->creator;
}

/*!
  Sets the creator of the document.
  */
void QPdfWriter::setCreator(const QString &creator)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->creator = creator;
}


/*!
  \reimp
  */
QPaintEngine *QPdfWriter::paintEngine() const
{
    Q_D(const QPdfWriter);

    return d->engine;
}

/*!
  \reimp
  */
void QPdfWriter::setPageSize(PageSize size)
{
    Q_D(const QPdfWriter);

    QPagedPaintDevice::setPageSize(size);
    d->engine->d_func()->paperSize = pageSizeMM() * 25.4/72.;
}

/*!
  \reimp
  */
void QPdfWriter::setPageSizeMM(const QSizeF &size)
{
    Q_D(const QPdfWriter);

    QPagedPaintDevice::setPageSizeMM(size);
    d->engine->d_func()->paperSize = pageSizeMM() * 25.4/72.;
}

/*!
    \internal

    Returns the metric for the given \a id.
*/
int QPdfWriter::metric(PaintDeviceMetric id) const
{
    Q_D(const QPdfWriter);
    return d->engine->metric(id);
}

/*!
  \reimp
*/
bool QPdfWriter::newPage()
{
    Q_D(QPdfWriter);

    return d->engine->newPage();
}


/*!
  \reimp
  */
void QPdfWriter::setMargins(const Margins &m)
{
    Q_D(QPdfWriter);

    const qreal multiplier = 72./25.4;
    d->engine->d_func()->leftMargin = m.left*multiplier;
    d->engine->d_func()->rightMargin = m.right*multiplier;
    d->engine->d_func()->topMargin = m.top*multiplier;
    d->engine->d_func()->bottomMargin = m.bottom*multiplier;
}

QT_END_NAMESPACE
