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

#include "qwindowsclipboard.h"
#include "qwindowscontext.h"
#include "qwindowsole.h"
#include "qwindowsmime.h"
#include "qwindowsguieventdispatcher.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QImage>

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

static const char formatTextPlainC[] = "text/plain";
static const char formatTextHtmlC[] = "text/html";

/*!
    \class QWindowsClipboard
    \brief Clipboard implementation.

    Registers a non-visible clipboard viewer window that
    receives clipboard events in its own window procedure to be
    able to receive clipboard-changed events, which
    QPlatformClipboard needs to emit. That requires housekeeping
    of the next in the viewer chain.

    \note The OLE-functions used in this class require OleInitialize().

    \ingroup qt-lighthouse-win
*/

QDebug operator<<(QDebug d, const QMimeData &m)
{
    QDebug nospace = d.nospace();
    const QStringList formats = m.formats();
    nospace << "QMimeData: " << formats.join(QStringLiteral(", ")) << '\n'
            << "  Text=" << m.hasText() << " HTML=" << m.hasHtml()
            << " Color=" << m.hasColor() << " Image=" << m.hasImage()
            << " URLs=" << m.hasUrls() << '\n';
    if (m.hasText())
        nospace << "  Text: '" << m.text() << "'\n";
    if (m.hasHtml())
        nospace << "  HTML: '" << m.html() << "'\n";
    if (m.hasColor())
        nospace << "  Color: " << qvariant_cast<QColor>(m.colorData()) << '\n';
    if (m.hasImage())
        nospace << "  Image: " << qvariant_cast<QImage>(m.imageData()).size() << '\n';
    if (m.hasUrls())
        nospace << "  URLs: " << m.urls() << '\n';
    return d;
}

/*!
    \class QWindowsInternalMimeDataBase
    \brief Base for implementations of QInternalMimeData using a IDataObject COM object.

    In clipboard handling and Drag and drop, static instances
    of QInternalMimeData implementations are kept and passed to the client.

    QInternalMimeData provides virtuals that query the formats and retrieve
    mime data on demand when the client invokes functions like QMimeData::hasHtml(),
    QMimeData::html() on the instance returned. Otherwise, expensive
    construction of a new QMimeData object containing all possible
    formats would have to be done in each call to mimeData().

    The base class introduces new virtuals to obtain and release
    the instances IDataObject from the clipboard or Drag and Drop and
    does conversion using QWindowsMime classes.

    \sa QInternalMimeData, QWindowsMime, QWindowsMimeConverter
    \ingroup qt-lighthouse-win
*/

bool QWindowsInternalMimeData::hasFormat_sys(const QString &mime) const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return false;

    const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
    const bool has = mc.converterToMime(mime, pDataObj) != 0;
    releaseDataObject(pDataObj);
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ <<  mime << has;
    return has;
}

QStringList QWindowsInternalMimeData::formats_sys() const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return QStringList();

    const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
    const QStringList fmts = mc.allMimesForFormats(pDataObj);
    releaseDataObject(pDataObj);
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ <<  fmts;
    return fmts;
}

QVariant QWindowsInternalMimeData::retrieveData_sys(const QString &mimeType,
                                                        QVariant::Type type) const
{
    IDataObject *pDataObj = retrieveDataObject();
    if (!pDataObj)
        return QVariant();

    QVariant result;
    const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
    if (const QWindowsMime *converter = mc.converterToMime(mimeType, pDataObj))
        result = converter->convertToMime(mimeType, pDataObj, type);
    releaseDataObject(pDataObj);
    if (QWindowsContext::verboseOLE) {
        QDebug nospace = qDebug().nospace();
        nospace << __FUNCTION__ <<  ' '  << mimeType << ' ' << type
                << " returns " << result.type();
        if (result.type() != QVariant::ByteArray)
            nospace << ' ' << result;
    }
    return result;
}

/*!
    \class QWindowsClipboardRetrievalMimeData
    \brief Special mime data class managing delayed retrieval of clipboard data.

    Implementation of QWindowsInternalMimeDataBase that obtains the
    IDataObject from the clipboard.

    \sa QWindowsInternalMimeDataBase, QWindowsClipboard
    \ingroup qt-lighthouse-win
*/

IDataObject *QWindowsClipboardRetrievalMimeData::retrieveDataObject() const
{
    IDataObject * pDataObj = 0;
    if (OleGetClipboard(&pDataObj) == S_OK)
        return pDataObj;
    return 0;
}

void QWindowsClipboardRetrievalMimeData::releaseDataObject(IDataObject *dataObject) const
{
    dataObject->Release();
}

extern "C" LRESULT QT_WIN_CALLBACK qClipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    if (QWindowsClipboard::instance()
        && QWindowsClipboard::instance()->clipboardViewerWndProc(hwnd, message, wParam, lParam, &result))
        return result;
    return DefWindowProc(hwnd, message, wParam, lParam);
}

QWindowsClipboard *QWindowsClipboard::m_instance = 0;

QWindowsClipboard::QWindowsClipboard() :
    m_data(0), m_clipboardViewer(0), m_nextClipboardViewer(0)
{
    QWindowsClipboard::m_instance = this;
}

QWindowsClipboard::~QWindowsClipboard()
{
    unregisterViewer(); // Should release data if owner.
    releaseIData();
    QWindowsClipboard::m_instance = 0;
}

void QWindowsClipboard::releaseIData()
{
    if (m_data) {
        delete m_data->mimeData();
        m_data->releaseQt();
        m_data->Release();
        m_data = 0;
    }
}

void QWindowsClipboard::registerViewer()
{
    m_clipboardViewer = QWindowsContext::instance()->
        createDummyWindow(QStringLiteral("Qt5ClipboardView"), L"Qt5ClipboardView",
                          qClipboardViewerWndProc, WS_OVERLAPPED);
    m_nextClipboardViewer = SetClipboardViewer(m_clipboardViewer);

    if (QWindowsContext::verboseOLE)
        qDebug("%s m_clipboardViewer: %p next=%p", __FUNCTION__,
               m_clipboardViewer, m_nextClipboardViewer);
}

void QWindowsClipboard::unregisterViewer()
{
    if (m_clipboardViewer) {
        ChangeClipboardChain(m_clipboardViewer, m_nextClipboardViewer);
        DestroyWindow(m_clipboardViewer);
        m_clipboardViewer = m_nextClipboardViewer = 0;
    }
}

void QWindowsClipboard::propagateClipboardMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
    if (!m_nextClipboardViewer)
        return;
    // In rare cases, a clipboard viewer can hang (application crashed,
    // suspended by a shell prompt 'Select' or debugger).
    if (QWindowsContext::user32dll.isHungAppWindow
        && QWindowsContext::user32dll.isHungAppWindow(m_nextClipboardViewer)) {
        qWarning("%s: Cowardly refusing to send clipboard message to hung application...", Q_FUNC_INFO);
        return;
    }
    SendMessage(m_nextClipboardViewer, message, wParam, lParam);
}

/*!
    \brief Windows procedure of the clipboard viewer. Emits changed and does
    housekeeping of the viewer chain.
*/

bool QWindowsClipboard::clipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    *result = 0;
    if (QWindowsContext::verboseOLE)
            qDebug("%s HWND=%p 0x%x %s", __FUNCTION__, hwnd, message,
                   QWindowsGuiEventDispatcher::windowsMessageName(message));

    switch (message) {
    case WM_CHANGECBCHAIN: {
        const HWND toBeRemoved = (HWND)wParam;
        if (toBeRemoved == m_nextClipboardViewer) {
            m_nextClipboardViewer = (HWND)lParam;
        } else {
            propagateClipboardMessage(message, wParam, lParam);
        }
    }
        return true;
    case WM_DRAWCLIPBOARD:
        if (QWindowsContext::verboseOLE)
            qDebug("Clipboard changed");
        emitChanged(QClipboard::Clipboard);
        // clean up the clipboard object if we no longer own the clipboard
        if (!ownsClipboard() && m_data)
            releaseIData();
        propagateClipboardMessage(message, wParam, lParam);
        return true;
    case WM_DESTROY:
        // Recommended shutdown
        if (ownsClipboard()) {
            if (QWindowsContext::verboseOLE)
                qDebug("Clipboard owner on shutdown, releasing.");
            OleFlushClipboard();
            releaseIData();
        }
        return true;
    } // switch (message)
    return false;
}

QMimeData *QWindowsClipboard::mimeData(QClipboard::Mode mode)
{
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ <<  mode;
    if (mode != QClipboard::Clipboard)
        return 0;
    if (ownsClipboard())
        return m_data->mimeData();
    return &m_retrievalData;
}

void QWindowsClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ <<  mode << *mimeData;
    if (mode != QClipboard::Clipboard)
        return;

    const bool newData = !m_data || m_data->mimeData() != mimeData;
    if (newData) {
        releaseIData();
        m_data = new QWindowsOleDataObject(mimeData);
    }

    const HRESULT src = OleSetClipboard(m_data);
    if (src != S_OK) {
        qErrnoWarning("OleSetClipboard: Failed to set mime data (%s) on clipboard: %s",
                      qPrintable(mimeData->formats().join(QStringLiteral(", "))),
                      QWindowsContext::comErrorString(src).constData());
        releaseIData();
        return;
    }
}

void QWindowsClipboard::clear()
{
    const HRESULT src = OleSetClipboard(0);
    if (src != S_OK)
        qErrnoWarning("OleSetClipboard: Failed to clear the clipboard: 0x%lx", src);
}

bool QWindowsClipboard::supportsMode(QClipboard::Mode mode) const
{
        return mode == QClipboard::Clipboard;
}

// Need a non-virtual in destructor.
bool QWindowsClipboard::ownsClipboard() const
{
    return m_data && OleIsCurrentClipboard(m_data) == S_OK;
}

bool QWindowsClipboard::ownsMode(QClipboard::Mode mode) const
{
    const bool result = mode == QClipboard::Clipboard ?
        ownsClipboard() : false;
    if (QWindowsContext::verboseOLE)
        qDebug("%s %d returns %d", __FUNCTION__, mode, result);
    return result;
}

QT_END_NAMESPACE
