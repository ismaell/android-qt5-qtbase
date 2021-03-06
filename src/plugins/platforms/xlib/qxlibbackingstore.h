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

#ifndef QWINDOWSURFACE_TESTLITE_H
#define QWINDOWSURFACE_TESTLITE_H

#include <QtGui/qplatformbackingstore_qpa.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QXlibWindow;
class QXlibIntegration;
class QXlibScreen;
class QXlibShmImageInfo;

class QXlibBackingStore : public QPlatformBackingStore
{
public:
    QXlibBackingStore (QWindow *window);
    ~QXlibBackingStore();

    QPaintDevice *paintDevice();
    void flush(QWindow *window, const QRegion &region, const QPoint &offset);

    void resize(const QSize &size, const QRegion &staticContents);

    void beginPaint(const QRegion &region);
    void endPaint();

private:
    bool painted;
    void resizeBuffer(QSize);
    QSize bufferSize() const;


    void resizeShmImage(int width, int height);

    QImage shm_img;
    QXlibShmImageInfo *image_info;

    QXlibWindow *xw;

};


QT_END_NAMESPACE

#endif
