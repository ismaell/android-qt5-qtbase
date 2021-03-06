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

#ifndef QOPENGLFRAMEBUFFEROBJECT_P_H
#define QOPENGLFRAMEBUFFEROBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

QT_BEGIN_INCLUDE_NAMESPACE

#include <qopenglframebufferobject.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>

QT_END_INCLUDE_NAMESPACE

#ifndef QT_OPENGL_ES
#define DEFAULT_FORMAT GL_RGBA8
#else
#define DEFAULT_FORMAT GL_RGBA
#endif

class QOpenGLFramebufferObjectFormatPrivate
{
public:
    QOpenGLFramebufferObjectFormatPrivate()
        : ref(1),
          samples(0),
          attachment(QOpenGLFramebufferObject::NoAttachment),
          target(GL_TEXTURE_2D),
          internal_format(DEFAULT_FORMAT),
          mipmap(false)
    {
    }
    QOpenGLFramebufferObjectFormatPrivate
            (const QOpenGLFramebufferObjectFormatPrivate *other)
        : ref(1),
          samples(other->samples),
          attachment(other->attachment),
          target(other->target),
          internal_format(other->internal_format),
          mipmap(other->mipmap)
    {
    }
    bool equals(const QOpenGLFramebufferObjectFormatPrivate *other)
    {
        return samples == other->samples &&
               attachment == other->attachment &&
               target == other->target &&
               internal_format == other->internal_format &&
               mipmap == other->mipmap;
    }

    QAtomicInt ref;
    int samples;
    QOpenGLFramebufferObject::Attachment attachment;
    GLenum target;
    GLenum internal_format;
    uint mipmap : 1;
};

class QOpenGLFramebufferObjectPrivate
{
public:
    QOpenGLFramebufferObjectPrivate() : fbo_guard(0), texture_guard(0), depth_buffer_guard(0)
                                  , stencil_buffer_guard(0), color_buffer_guard(0)
                                  , valid(false) {}
    ~QOpenGLFramebufferObjectPrivate() {}

    void init(QOpenGLFramebufferObject *q, const QSize& sz,
              QOpenGLFramebufferObject::Attachment attachment,
              GLenum internal_format, GLenum texture_target,
              GLint samples = 0, bool mipmap = false);
    bool checkFramebufferStatus() const;
    QOpenGLSharedResourceGuard *fbo_guard;
    QOpenGLSharedResourceGuard *texture_guard;
    QOpenGLSharedResourceGuard *depth_buffer_guard;
    QOpenGLSharedResourceGuard *stencil_buffer_guard;
    QOpenGLSharedResourceGuard *color_buffer_guard;
    GLenum target;
    QSize size;
    QOpenGLFramebufferObjectFormat format;
    uint valid : 1;
    QOpenGLFramebufferObject::Attachment fbo_attachment;
    QOpenGLExtensions funcs;

    inline GLuint fbo() const { return fbo_guard ? fbo_guard->id() : 0; }
};


QT_END_NAMESPACE

#endif // QOPENGLFRAMEBUFFEROBJECT_P_H
