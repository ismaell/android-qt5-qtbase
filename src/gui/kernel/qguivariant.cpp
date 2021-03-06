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

#include "qvariant.h"

// Gui types
#include "qbitmap.h"
#include "qbrush.h"
#include "qcolor.h"
#include "qcursor.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qfont.h"
#include "qimage.h"
#include "qkeysequence.h"
#include "qtransform.h"
#include "qmatrix.h"
#include "qpalette.h"
#include "qpen.h"
#include "qpixmap.h"
#include "qpolygon.h"
#include "qregion.h"
#include "qtextformat.h"
#include "qmatrix4x4.h"
#include "qvector2d.h"
#include "qvector3d.h"
#include "qvector4d.h"
#include "qquaternion.h"

// Core types
#include "qvariant.h"
#include "qbitarray.h"
#include "qbytearray.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qmap.h"
#include "qdatetime.h"
#include "qeasingcurve.h"
#include "qlist.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qurl.h"
#include "qlocale.h"
#include "quuid.h"

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#include "qline.h"
#endif

#include <float.h>

#include "private/qvariant_p.h"
#include <private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler();

namespace {
template<typename T>
struct TypeDefiniton {
    static const bool IsAvailable = true;
};
// Ignore these types, as incomplete
#ifdef QT_NO_GEOM_VARIANT
template<> struct TypeDefiniton<QRect> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QRectF> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QSize> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QSizeF> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QLine> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QLineF> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QPoint> { static const bool IsAvailable = false; };
template<> struct TypeDefiniton<QPointF> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_SHORTCUT
template<> struct TypeDefiniton<QKeySequence> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_CURSOR
template<> struct TypeDefiniton<QCursor> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_MATRIX4X4
template<> struct TypeDefiniton<QMatrix4x4> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR2D
template<> struct TypeDefiniton<QVector2D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR3D
template<> struct TypeDefiniton<QVector3D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR4D
template<> struct TypeDefiniton<QVector4D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_QUATERNION
template<> struct TypeDefiniton<QQuaternion> { static const bool IsAvailable = false; };
#endif

struct GuiTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QTypeModuleInfo<T>::IsGui && TypeDefiniton<T>::IsAvailable;
    };
};
} // namespace used to hide TypeDefinition

namespace {
static void construct(QVariant::Private *x, const void *copy)
{
    const int type = x->type;
    if (Q_UNLIKELY(type == 62)) {
        // small 'trick' to let a QVariant(Qt::blue) create a variant
        // of type QColor
        // TODO Get rid of this hack.
        x->type = QVariant::Color;
        QColor color(*reinterpret_cast<const Qt::GlobalColor *>(copy));
        v_construct<QColor>(x, &color);
        return;
    }
    QVariantConstructor<GuiTypesFilter> constructor(x, copy);
    QMetaTypeSwitcher::switcher<void>(constructor, type, 0);
}

static void clear(QVariant::Private *d)
{
    QVariantDestructor<GuiTypesFilter> destructor(d);
    QMetaTypeSwitcher::switcher<void>(destructor, d->type, 0);
}

// This class is a hack that customizes access to QPolygon
template<class Filter>
class QGuiVariantIsNull : public QVariantIsNull<Filter> {
    typedef QVariantIsNull<Filter> Base;
public:
    QGuiVariantIsNull(const QVariant::Private *d)
        : QVariantIsNull<Filter>(d)
    {}
    template<typename T>
    bool delegate(const T *p) { return Base::delegate(p); }
    bool delegate(const QPolygon*) { return v_cast<QPolygon>(Base::m_d)->isEmpty(); }
    bool delegate(const void *p) { return Base::delegate(p); }
};
static bool isNull(const QVariant::Private *d)
{
    QGuiVariantIsNull<GuiTypesFilter> isNull(d);
    return QMetaTypeSwitcher::switcher<bool>(isNull, d->type, 0);
}

// This class is a hack that customizes access to QPixmap, QBitmap and QCursor
template<class Filter>
class QGuiVariantComparator : public QVariantComparator<Filter> {
    typedef QVariantComparator<Filter> Base;
public:
    QGuiVariantComparator(const QVariant::Private *a, const QVariant::Private *b)
        : QVariantComparator<Filter>(a, b)
    {}
    template<typename T>
    bool delegate(const T *p)
    {
        return Base::delegate(p);
    }
    bool delegate(const QPixmap*)
    {
        return v_cast<QPixmap>(Base::m_a)->cacheKey() == v_cast<QPixmap>(Base::m_b)->cacheKey();
    }
    bool delegate(const QBitmap*)
    {
        return v_cast<QBitmap>(Base::m_a)->cacheKey() == v_cast<QBitmap>(Base::m_b)->cacheKey();
    }
#ifndef QT_NO_CURSOR
    bool delegate(const QCursor*)
    {
        return v_cast<QCursor>(Base::m_a)->shape() == v_cast<QCursor>(Base::m_b)->shape();
    }
#endif
    bool delegate(const void *p) { return Base::delegate(p); }
};

static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    QGuiVariantComparator<GuiTypesFilter> comparator(a, b);
    return QMetaTypeSwitcher::switcher<bool>(comparator, a->type, 0);
}

static bool convert(const QVariant::Private *d, QVariant::Type t,
                 void *result, bool *ok)
{
    switch (t) {
    case QVariant::ByteArray:
        if (d->type == QVariant::Color) {
            *static_cast<QByteArray *>(result) = v_cast<QColor>(d)->name().toLatin1();
            return true;
        }
        break;
    case QVariant::String: {
        QString *str = static_cast<QString *>(result);
        switch (d->type) {
#ifndef QT_NO_SHORTCUT
        case QVariant::KeySequence:
            *str = QString(*v_cast<QKeySequence>(d));
            return true;
#endif
        case QVariant::Font:
            *str = v_cast<QFont>(d)->toString();
            return true;
        case QVariant::Color:
            *str = v_cast<QColor>(d)->name();
            return true;
        default:
            break;
        }
        break;
    }
    case QVariant::Pixmap:
        if (d->type == QVariant::Image) {
            *static_cast<QPixmap *>(result) = QPixmap::fromImage(*v_cast<QImage>(d));
            return true;
        } else if (d->type == QVariant::Bitmap) {
            *static_cast<QPixmap *>(result) = *v_cast<QBitmap>(d);
            return true;
        } else if (d->type == QVariant::Brush) {
            if (v_cast<QBrush>(d)->style() == Qt::TexturePattern) {
                *static_cast<QPixmap *>(result) = v_cast<QBrush>(d)->texture();
                return true;
            }
        }
        break;
    case QVariant::Image:
        if (d->type == QVariant::Pixmap) {
            *static_cast<QImage *>(result) = v_cast<QPixmap>(d)->toImage();
            return true;
        } else if (d->type == QVariant::Bitmap) {
            *static_cast<QImage *>(result) = v_cast<QBitmap>(d)->toImage();
            return true;
        }
        break;
    case QVariant::Bitmap:
        if (d->type == QVariant::Pixmap) {
            *static_cast<QBitmap *>(result) = *v_cast<QPixmap>(d);
            return true;
        } else if (d->type == QVariant::Image) {
            *static_cast<QBitmap *>(result) = QBitmap::fromImage(*v_cast<QImage>(d));
            return true;
        }
        break;
#ifndef QT_NO_SHORTCUT
    case QVariant::Int:
        if (d->type == QVariant::KeySequence) {
            *static_cast<int *>(result) = (int)(*(v_cast<QKeySequence>(d)));
            return true;
        }
        break;
#endif
    case QVariant::Font:
        if (d->type == QVariant::String) {
            QFont *f = static_cast<QFont *>(result);
            f->fromString(*v_cast<QString>(d));
            return true;
        }
        break;
    case QVariant::Color:
        if (d->type == QVariant::String) {
            static_cast<QColor *>(result)->setNamedColor(*v_cast<QString>(d));
            return static_cast<QColor *>(result)->isValid();
        } else if (d->type == QVariant::ByteArray) {
            static_cast<QColor *>(result)->setNamedColor(QString::fromLatin1(
                                *v_cast<QByteArray>(d)));
            return true;
        } else if (d->type == QVariant::Brush) {
            if (v_cast<QBrush>(d)->style() == Qt::SolidPattern) {
                *static_cast<QColor *>(result) = v_cast<QBrush>(d)->color();
                return true;
            }
        }
        break;
    case QVariant::Brush:
        if (d->type == QVariant::Color) {
            *static_cast<QBrush *>(result) = QBrush(*v_cast<QColor>(d));
            return true;
        } else if (d->type == QVariant::Pixmap) {
            *static_cast<QBrush *>(result) = QBrush(*v_cast<QPixmap>(d));
            return true;
        }
        break;
#ifndef QT_NO_SHORTCUT
    case QVariant::KeySequence: {
        QKeySequence *seq = static_cast<QKeySequence *>(result);
        switch (d->type) {
        case QVariant::String:
            *seq = QKeySequence(*v_cast<QString>(d));
            return true;
        case QVariant::Int:
            *seq = QKeySequence(d->data.i);
            return true;
        default:
            break;
        }
    }
#endif
    default:
        break;
    }
    return qcoreVariantHandler()->convert(d, t, result, ok);
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(Q_BROKEN_DEBUG_STREAM)
static void streamDebug(QDebug dbg, const QVariant &v)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(&v.data_ptr());
    QVariantDebugStream<GuiTypesFilter> stream(dbg, d);
    QMetaTypeSwitcher::switcher<void>(stream, d->type, 0);
}
#endif

const QVariant::Handler qt_gui_variant_handler = {
    construct,
    clear,
    isNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    compare,
    convert,
    0,
#if !defined(QT_NO_DEBUG_STREAM) && !defined(Q_BROKEN_DEBUG_STREAM)
    streamDebug
#else
    0
#endif
};

#define QT_IMPL_METATYPEINTERFACE_GUI_TYPES(MetaTypeName, MetaTypeId, RealName) \
    QT_METATYPE_INTERFACE_INIT(RealName),

static const QMetaTypeInterface qVariantGuiHelper[] = {
    QT_FOR_EACH_STATIC_GUI_CLASS(QT_IMPL_METATYPEINTERFACE_GUI_TYPES)
};

#undef QT_IMPL_METATYPEINTERFACE_GUI_TYPES
} // namespace used to hide QVariant handler

extern Q_CORE_EXPORT const QMetaTypeInterface *qMetaTypeGuiHelper;

void qRegisterGuiVariant()
{
    QVariantPrivate::registerHandler(QModulesPrivate::Gui, &qt_gui_variant_handler);
    qMetaTypeGuiHelper = qVariantGuiHelper;
}
Q_CONSTRUCTOR_FUNCTION(qRegisterGuiVariant)

void qUnregisterGuiVariant()
{
    QVariantPrivate::unregisterHandler(QModulesPrivate::Gui);
    qMetaTypeGuiHelper = 0;
}
Q_DESTRUCTOR_FUNCTION(qUnregisterGuiVariant)


QT_END_NAMESPACE
