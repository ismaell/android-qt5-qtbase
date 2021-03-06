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

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY


#include "qwindowsaccessibility.h"
#include "qwindowscontext.h"

#include <private/qsystemlibrary_p.h>

#include <QtCore/qmap.h>
#include <QtCore/qsettings.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qpair.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qplatformnativeinterface_qpa.h>
#include <QtGui/qwindow.h>
#include <QtGui/qaccessible2.h>
#include <OleAcc.h>

//#include <uiautomationcoreapi.h>
#ifndef UiaRootObjectId
#define UiaRootObjectId        -25
#endif

#include <winuser.h>
#if !defined(WINABLEAPI)
#  if defined(Q_WS_WINCE)
#    include <bldver.h>
#  endif
#  include <winable.h>
#endif

#include <oleacc.h>
#if !defined(Q_CC_BOR) && !defined (Q_CC_GNU)
#include <comdef.h>
#endif

#ifdef Q_WS_WINCE
#include "qguifunctions_wince.h"
#endif

QT_BEGIN_NAMESPACE

//#define DEBUG_SHOW_ATCLIENT_COMMANDS
#ifdef DEBUG_SHOW_ATCLIENT_COMMANDS
QT_BEGIN_INCLUDE_NAMESPACE
#include <qdebug.h>
QT_END_INCLUDE_NAMESPACE

void showDebug(const char* funcName, const QAccessibleInterface *iface)
{
    qDebug() << "Role:" << qAccessibleRoleString(iface->role(0))
             << "Name:" << iface->text(QAccessible::Name, 0)
             << "State:" << QString::number(int(iface->state(0)), 16)
             << QLatin1String(funcName);
}
#else
# define showDebug(f, iface)
#endif

typedef QSharedPointer<QAccessibleInterface> QAIPointer;

// This stuff is used for widgets/items with no window handle:
typedef QMap<int, QPair<QObject*,int> > NotifyMap;
Q_GLOBAL_STATIC(NotifyMap, qAccessibleRecentSentEvents)
static int eventNum = 0;

class QWindowsEnumerate : public IEnumVARIANT
{
public:
    QWindowsEnumerate(const QVector<int> &a)
        : ref(0), current(0),array(a)
    {
    }

    virtual ~QWindowsEnumerate() {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **ppEnum);
    HRESULT STDMETHODCALLTYPE Next(unsigned long  celt, VARIANT FAR*  rgVar, unsigned long FAR*  pCeltFetched);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Skip(unsigned long celt);

private:
    ULONG ref;
    ULONG current;
    QVector<int> array;
};

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::QueryInterface(REFIID id, LPVOID *iface)
{
    *iface = 0;
    if (id == IID_IUnknown)
        *iface = (IUnknown*)this;
    else if (id == IID_IEnumVARIANT)
        *iface = (IEnumVARIANT*)this;

    if (*iface) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE QWindowsEnumerate::AddRef()
{
    return ++ref;
}

ULONG STDMETHODCALLTYPE QWindowsEnumerate::Release()
{
    if (!--ref) {
        delete this;
        return 0;
    }
    return ref;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Clone(IEnumVARIANT **ppEnum)
{
    QWindowsEnumerate *penum = 0;
    *ppEnum = 0;

    penum = new QWindowsEnumerate(array);
    if (!penum)
        return E_OUTOFMEMORY;
    penum->current = current;
    penum->array = array;
    penum->AddRef();
    *ppEnum = penum;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Next(unsigned long  celt, VARIANT FAR*  rgVar, unsigned long FAR*  pCeltFetched)
{
    if (pCeltFetched)
        *pCeltFetched = 0;

    ULONG l;
    for (l = 0; l < celt; l++) {
        VariantInit(&rgVar[l]);
        if ((current+1) > (ULONG)array.size()) {
            *pCeltFetched = l;
            return S_FALSE;
        }

        rgVar[l].vt = VT_I4;
        rgVar[l].lVal = array[(int)current];
        ++current;
    }
    *pCeltFetched = l;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Reset()
{
    current = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsEnumerate::Skip(unsigned long celt)
{
    current += celt;
    if (current > (ULONG)array.size()) {
        current = array.size();
        return S_FALSE;
    }
    return S_OK;
}

/*
*/
class QWindowsAccessible : public IAccessible, IOleWindow
{
public:
    QWindowsAccessible(QAccessibleInterface *a)
        : ref(0), accessible(a)
    {
    }

    virtual ~QWindowsAccessible()
    {
        delete accessible;
    }

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IDispatch */
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int *);
    HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int, unsigned long, ITypeInfo **);
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(const _GUID &, wchar_t **, unsigned int, unsigned long, long *);
    HRESULT STDMETHODCALLTYPE Invoke(long, const _GUID &, unsigned long, unsigned short, tagDISPPARAMS *, tagVARIANT *, tagEXCEPINFO *, unsigned int *);

    /* IAccessible */
    HRESULT STDMETHODCALLTYPE accHitTest(long xLeft, long yTop, VARIANT *pvarID);
    HRESULT STDMETHODCALLTYPE accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varID);
    HRESULT STDMETHODCALLTYPE accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEnd);
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT varChildID, IDispatch** ppdispChild);
    HRESULT STDMETHODCALLTYPE get_accChildCount(long* pcountChildren);
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** ppdispParent);

    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT varID);
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT varID, BSTR* pszDefaultAction);
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT varID, BSTR* pszDescription);
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT varID, BSTR *pszHelp);
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT varID, BSTR *pszKeyboardShortcut);
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT varID, BSTR* pszName);
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT varChild, BSTR szName);
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT varID, VARIANT *pvarRole);
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT varID, VARIANT *pvarState);
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT varID, BSTR* pszValue);
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT varChild, BSTR szValue);

    HRESULT STDMETHODCALLTYPE accSelect(long flagsSelect, VARIANT varID);
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT *pvarID);
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT *pvarChildren);

    /* IOleWindow */
    HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

private:
    ULONG ref;
    QAccessibleInterface *accessible;

    QAIPointer childPointer(VARIANT varID)
    {
        return QAIPointer(accessible->child(varID.lVal - 1));
    }
};

static inline BSTR QStringToBSTR(const QString &str)
{
    BSTR bstrVal;

    int wlen = str.length()+1;
    bstrVal = SysAllocStringByteLen(0, wlen*2);
    memcpy(bstrVal, str.unicode(), sizeof(QChar)*(wlen));
    bstrVal[wlen] = 0;

    return bstrVal;
}

/*
*/

/*
  IUnknown
*/
HRESULT STDMETHODCALLTYPE QWindowsAccessible::QueryInterface(REFIID id, LPVOID *iface)
{
    *iface = 0;
    if (id == IID_IUnknown)
        *iface = (IUnknown*)(IDispatch*)this;
    else if (id == IID_IDispatch)
        *iface = (IDispatch*)this;
    else if (id == IID_IAccessible)
        *iface = (IAccessible*)this;
    else if (id == IID_IOleWindow)
        *iface = (IOleWindow*)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE QWindowsAccessible::AddRef()
{
    return ++ref;
}

ULONG STDMETHODCALLTYPE QWindowsAccessible::Release()
{
    if (!--ref) {
        delete this;
        return 0;
    }
    return ref;
}

/*
  IDispatch
*/

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetTypeInfoCount(unsigned int * pctinfo)
{
    // We don't use a type library
    *pctinfo = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetTypeInfo(unsigned int, unsigned long, ITypeInfo **pptinfo)
{
    // We don't use a type library
    *pptinfo = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetIDsOfNames(const _GUID &, wchar_t **rgszNames, unsigned int, unsigned long, long *rgdispid)
{
#if !defined(Q_CC_BOR) && !defined(Q_CC_GNU)
    // PROPERTIES:  Hierarchical
    if (_bstr_t(rgszNames[0]) == _bstr_t(L"accParent"))
        rgdispid[0] = DISPID_ACC_PARENT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accChildCount"))
        rgdispid[0] = DISPID_ACC_CHILDCOUNT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accChild"))
        rgdispid[0] = DISPID_ACC_CHILD;

    // PROPERTIES:  Descriptional
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accName("))
        rgdispid[0] = DISPID_ACC_NAME;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accValue"))
        rgdispid[0] = DISPID_ACC_VALUE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accDescription"))
        rgdispid[0] = DISPID_ACC_DESCRIPTION;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accRole"))
        rgdispid[0] = DISPID_ACC_ROLE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accState"))
        rgdispid[0] = DISPID_ACC_STATE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accHelp"))
        rgdispid[0] = DISPID_ACC_HELP;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accHelpTopic"))
        rgdispid[0] = DISPID_ACC_HELPTOPIC;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accKeyboardShortcut"))
        rgdispid[0] = DISPID_ACC_KEYBOARDSHORTCUT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accFocus"))
        rgdispid[0] = DISPID_ACC_FOCUS;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accSelection"))
        rgdispid[0] = DISPID_ACC_SELECTION;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accDefaultAction"))
        rgdispid[0] = DISPID_ACC_DEFAULTACTION;

    // METHODS
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accSelect"))
        rgdispid[0] = DISPID_ACC_SELECT;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accLocation"))
        rgdispid[0] = DISPID_ACC_LOCATION;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accNavigate"))
        rgdispid[0] = DISPID_ACC_NAVIGATE;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accHitTest"))
        rgdispid[0] = DISPID_ACC_HITTEST;
    else if (_bstr_t(rgszNames[0]) == _bstr_t(L"accDoDefaultAction"))
        rgdispid[0] = DISPID_ACC_DODEFAULTACTION;
    else
        return DISP_E_UNKNOWNINTERFACE;

    return S_OK;
#else
    Q_UNUSED(rgszNames);
    Q_UNUSED(rgdispid);

    return DISP_E_MEMBERNOTFOUND;
#endif
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::Invoke(long dispIdMember, const _GUID &, unsigned long, unsigned short wFlags, tagDISPPARAMS *pDispParams, tagVARIANT *pVarResult, tagEXCEPINFO *, unsigned int *)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    switch (dispIdMember)
    {
        case DISPID_ACC_PARENT:
            if (wFlags == DISPATCH_PROPERTYGET) {
                if (!pVarResult)
                    return E_INVALIDARG;
                hr = get_accParent(&pVarResult->pdispVal);
            } else {
                hr = DISP_E_MEMBERNOTFOUND;
            }
            break;

        case DISPID_ACC_CHILDCOUNT:
            if (wFlags == DISPATCH_PROPERTYGET) {
                if (!pVarResult)
                    return E_INVALIDARG;
                hr = get_accChildCount(&pVarResult->lVal);
            } else {
                hr = DISP_E_MEMBERNOTFOUND;
            }
            break;

        case DISPID_ACC_CHILD:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accChild(pDispParams->rgvarg[0], &pVarResult->pdispVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_NAME:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accName(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else if (wFlags == DISPATCH_PROPERTYPUT)
                hr = put_accName(pDispParams->rgvarg[0], pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_VALUE:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accValue(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else if (wFlags == DISPATCH_PROPERTYPUT)
                hr = put_accValue(pDispParams->rgvarg[0], pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_DESCRIPTION:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accDescription(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_ROLE:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accRole(pDispParams->rgvarg[0], pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_STATE:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accState(pDispParams->rgvarg[0], pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_HELP:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accHelp(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_HELPTOPIC:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accHelpTopic(&pDispParams->rgvarg[2].bstrVal, pDispParams->rgvarg[1], &pDispParams->rgvarg[0].lVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_KEYBOARDSHORTCUT:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accKeyboardShortcut(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_FOCUS:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accFocus(pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_SELECTION:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accSelection(pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_DEFAULTACTION:
            if (wFlags == DISPATCH_PROPERTYGET)
                hr = get_accDefaultAction(pDispParams->rgvarg[0], &pVarResult->bstrVal);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_SELECT:
            if (wFlags == DISPATCH_METHOD)
                hr = accSelect(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0]);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_LOCATION:
            if (wFlags == DISPATCH_METHOD)
                hr = accLocation(&pDispParams->rgvarg[4].lVal, &pDispParams->rgvarg[3].lVal, &pDispParams->rgvarg[2].lVal, &pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0]);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_NAVIGATE:
            if (wFlags == DISPATCH_METHOD)
                hr = accNavigate(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0], pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_HITTEST:
            if (wFlags == DISPATCH_METHOD)
                hr = accHitTest(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].lVal, pVarResult);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_ACC_DODEFAULTACTION:
            if (wFlags == DISPATCH_METHOD)
                hr = accDoDefaultAction(pDispParams->rgvarg[0]);
            else
                hr = DISP_E_MEMBERNOTFOUND;
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }

    if (!SUCCEEDED(hr)) {
        return hr;
    }
    return hr;
}

/*
  IAccessible

IAccessible::accHitTest documents the value returned in pvarID like this:

| *Point location*                                       | *vt member* | *Value member*          |
+========================================================+=============+=========================+
| Outside of the object's boundaries, and either inside  | VT_EMPTY    | None.                   |
| or outside of the object's bounding rectangle.         |             |                         |
+--------------------------------------------------------+-------------+-------------------------+
|  Within the object but not within a child element or a | VT_I4       | lVal is CHILDID_SELF    |
|  child object.                                         |             |                         |
+--------------------------------------------------------+-------------+-------------------------+
| Within a child element.                                | VT_I4       | lVal contains           |
|                                                        |             | the child ID.           |
+--------------------------------------------------------+-------------+-------------------------+
| Within a child object.                                 | VT_DISPATCH | pdispVal is set to the  |
|                                                        |             | child object's IDispatch|
|                                                        |             | interface pointer       |
+--------------------------------------------------------+-------------+-------------------------+
*/
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accHitTest(long xLeft, long yTop, VARIANT *pvarID)
{

    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *child = accessible->childAt(xLeft, yTop);
    if (child == 0) {
        // no child found, return this item if it contains the coordinates
        if (accessible->rect().contains(xLeft, yTop)) {
            (*pvarID).vt = VT_I4;
            (*pvarID).lVal = CHILDID_SELF;
            return S_OK;
        }
    } else {
        QWindowsAccessible* wacc = new QWindowsAccessible(child);
        IDispatch *iface = 0;
        wacc->QueryInterface(IID_IDispatch, (void**)&iface);
        if (iface) {
            (*pvarID).vt = VT_DISPATCH;
            (*pvarID).pdispVal = iface;
            return S_OK;
        }
    }

    // Did not find anything
    (*pvarID).vt = VT_EMPTY;
    return S_FALSE;
}

/*
 It is recommended to read
    "Implementing a Microsoft Active Accessibility (MSAA) Server.
    Practical Tips for Developers and How Mozilla Does It"
    (https://developer.mozilla.org/En/Accessibility/Implementing_an_MSAA_Server)

 to get an overview of what's important to implement and what parts of MSAA
 can be ignored. All stuff prefixed with "moz" are information from that page.
*/
// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varID)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QRect rect;
    if (varID.lVal) {
        QAIPointer child = QAIPointer(accessible->child(varID.lVal - 1));
        if (child->isValid())
            rect = child->rect();
    } else {
        rect = accessible->rect();
    }

    *pxLeft = rect.x();
    *pyTop = rect.y();
    *pcxWidth = rect.width();
    *pcyHeight = rect.height();

    return S_OK;
}

// moz: [important, but no need to implement up/down/left/right]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEnd)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *acc = 0;
    switch (navDir) {
    case NAVDIR_FIRSTCHILD:
        acc = accessible->child(0);
        break;
    case NAVDIR_LASTCHILD:
        acc = accessible->child(accessible->childCount() - 1);
        break;
    case NAVDIR_NEXT:
    case NAVDIR_PREVIOUS:
        if (!varStart.lVal){
            QAccessibleInterface *parent = accessible->parent();
            if (parent) {
                int index = parent->indexOfChild(accessible);
                index += (navDir == NAVDIR_NEXT) ? 1 : -1;
                if (index >= 0 && index < parent->childCount())
                    acc = parent->child(index);
                delete parent;
            }
        } else {
            int index = varStart.lVal;
            index += (navDir == NAVDIR_NEXT) ? 1 : -1;
            if (index > 0 && index <= accessible->childCount())
                acc = accessible->child(index - 1);
        }
        break;

    // Geometrical
    case NAVDIR_UP:
    case NAVDIR_DOWN:
    case NAVDIR_LEFT:
    case NAVDIR_RIGHT:
        if (QAccessibleInterface *pIface = accessible->parent()) {

            QRect startg = accessible->rect();
            QPoint startc = startg.center();
            QAccessibleInterface *candidate = 0;
            unsigned mindist = UINT_MAX;    // will work on screen sizes at least up to 46340x46340
            const int sibCount = pIface->childCount();
            for (int i = 0; i < sibCount; ++i) {
                QAccessibleInterface *sibling = 0;
                sibling = pIface->child(i);
                Q_ASSERT(sibling);
                if ((accessible->relationTo(sibling) & QAccessible::Self) || sibling->state().invisible) {
                    //ignore ourself and invisible siblings
                    delete sibling;
                    continue;
                }

                QRect sibg = sibling->rect();
                QPoint sibc = sibg.center();
                QPoint sibp;
                QPoint startp;
                QPoint distp;
                switch (navDir) {
                case NAVDIR_LEFT:
                    startp = QPoint(startg.left(), startg.top() + startg.height() / 2);
                    sibp = QPoint(sibg.right(), sibg.top() + sibg.height() / 2);
                    if (QPoint(sibc - startc).x() >= 0) {
                        delete sibling;
                        continue;
                    }
                    distp = sibp - startp;
                    break;
                case NAVDIR_RIGHT:
                    startp = QPoint(startg.right(), startg.top() + startg.height() / 2);
                    sibp = QPoint(sibg.left(), sibg.top() + sibg.height() / 2);
                    if (QPoint(sibc - startc).x() <= 0) {
                        delete sibling;
                        continue;
                    }
                    distp = sibp - startp;
                    break;
                case NAVDIR_UP:
                    startp = QPoint(startg.left() + startg.width() / 2, startg.top());
                    sibp = QPoint(sibg.left() + sibg.width() / 2, sibg.bottom());
                    if (QPoint(sibc - startc).y() >= 0) {
                        delete sibling;
                        continue;
                    }
                    distp = sibp - startp;
                    break;
                case NAVDIR_DOWN:
                    startp = QPoint(startg.left() + startg.width() / 2, startg.bottom());
                    sibp = QPoint(sibg.left() + sibg.width() / 2, sibg.top());
                    if (QPoint(sibc - startc).y() <= 0) {
                        delete sibling;
                        continue;
                    }
                    distp = sibp - startp;
                    break;
                default:
                    break;
                }

                // Since we're *comparing* (and not measuring) distances, we can compare the
                // squared distance, (thus, no need to take the sqrt()).
                unsigned dist = distp.x() * distp.x() + distp.y() * distp.y();
                if (dist < mindist) {
                    delete candidate;
                    candidate = sibling;
                    mindist = dist;
                } else {
                    delete sibling;
                }
            }
            delete pIface;
            acc = candidate;
        }
        break;
    default:
        break;
    }
    if (!acc) {
        (*pvarEnd).vt = VT_EMPTY;
        return S_FALSE;
    }
    QWindowsAccessible* wacc = new QWindowsAccessible(acc);

    IDispatch *iface = 0;
    wacc->QueryInterface(IID_IDispatch, (void**)&iface);
    if (iface) {
        (*pvarEnd).vt = VT_DISPATCH;
        (*pvarEnd).pdispVal = iface;
        return S_OK;
    } else {
        delete wacc;
    }

    (*pvarEnd).vt = VT_EMPTY;
    return S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accChild(VARIANT varChildID, IDispatch** ppdispChild)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    if (varChildID.vt == VT_EMPTY)
        return E_INVALIDARG;


    int childIndex = varChildID.lVal;
    QAccessibleInterface *acc = 0;

    if (childIndex < 0) {
        const int entry = childIndex;
        QPair<QObject*, int> ref = qAccessibleRecentSentEvents()->value(entry);
        if (ref.first) {
            acc = QAccessible::queryAccessibleInterface(ref.first);
            if (acc && ref.second) {
                if (ref.second) {
                    QAccessibleInterface *res = acc->child(ref.second - 1);
                    delete acc;
                    if (!res)
                        return E_INVALIDARG;
                    acc = res;
                }
            }
        }
    } else {
        if (childIndex) {
            acc = accessible->child(childIndex - 1);
        } else {
            // FIXME
            Q_ASSERT(0);
        }
    }

    if (acc) {
        QWindowsAccessible* wacc = new QWindowsAccessible(acc);
        wacc->QueryInterface(IID_IDispatch, (void**)ppdispChild);
        return S_OK;
    }

    *ppdispChild = 0;
    return S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accChildCount(long* pcountChildren)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    *pcountChildren = accessible->childCount();
    return S_OK;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accParent(IDispatch** ppdispParent)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *acc = accessible->parent();
    if (acc) {
        QWindowsAccessible* wacc = new QWindowsAccessible(acc);
        wacc->QueryInterface(IID_IDispatch, (void**)ppdispParent);

        if (*ppdispParent)
            return S_OK;
    }

    *ppdispParent = 0;
    return S_FALSE;
}

/*
  Properties and methods
*/
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accDoDefaultAction(VARIANT varID)
{
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    if (QAccessibleActionInterface *actionIface = accessible->actionInterface()) {
        const QString def = actionIface->actionNames().value(0);
        if (!def.isEmpty()) {
            actionIface->doAction(def);
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accDefaultAction(VARIANT varID, BSTR* pszDefaultAction)
{
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    *pszDefaultAction = 0;
    if (QAccessibleActionInterface *actionIface = accessible->actionInterface()) {
        const QString def = actionIface->actionNames().value(0);
        if (!def.isEmpty())
            *pszDefaultAction = QStringToBSTR(def);
    }
    return *pszDefaultAction ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accDescription(VARIANT varID, BSTR* pszDescription)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;


    QString descr;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        descr = child->text(QAccessible::Description);
    } else {
        descr = accessible->text(QAccessible::Description);
    }
    if (descr.size()) {
        *pszDescription = QStringToBSTR(descr);
        return S_OK;
    }

    *pszDescription = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accHelp(VARIANT varID, BSTR *pszHelp)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QString help;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        help = child->text(QAccessible::Help);
    } else {
        help = accessible->text(QAccessible::Help);
    }
    if (help.size()) {
        *pszHelp = QStringToBSTR(help);
        return S_OK;
    }

    *pszHelp = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accHelpTopic(BSTR *, VARIANT, long *)
{
    return DISP_E_MEMBERNOTFOUND;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accKeyboardShortcut(VARIANT varID, BSTR *pszKeyboardShortcut)
{
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    *pszKeyboardShortcut = 0;
    if (QAccessibleActionInterface *actionIface = accessible->actionInterface()) {
        const QString def = actionIface->actionNames().value(0);
        if (!def.isEmpty()) {
            const QString keyBoardShortCut = actionIface->keyBindingsForAction(def).value(0);
            if (!keyBoardShortCut.isEmpty())
                *pszKeyboardShortcut = QStringToBSTR(keyBoardShortCut);
        }
    }
    return *pszKeyboardShortcut ? S_OK : S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accName(VARIANT varID, BSTR* pszName)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QString name;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        name = child->text(QAccessible::Name);
    } else {
        name = accessible->text(QAccessible::Name);
    }
    if (name.size()) {
        *pszName = QStringToBSTR(name);
        return S_OK;
    }

    *pszName = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::put_accName(VARIANT, BSTR)
{
    showDebug(__FUNCTION__, accessible);
    return DISP_E_MEMBERNOTFOUND;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accRole(VARIANT varID, VARIANT *pvarRole)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessible::Role role;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child)
            return E_FAIL;
        role = child->role();
    } else {
        role = accessible->role();
    }

    if (role != QAccessible::NoRole) {
        if (role == QAccessible::LayeredPane)
            role = QAccessible::Pane;
        (*pvarRole).vt = VT_I4;
        (*pvarRole).lVal = role;
    } else {
        (*pvarRole).vt = VT_EMPTY;
    }
    return S_OK;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accState(VARIANT varID, VARIANT *pvarState)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessible::State state;
    if (varID.lVal) {
        QAIPointer child = childPointer(varID);
        if (!child.data())
            return E_FAIL;
        state = child->state();
    } else {
        state = accessible->state();
    }

    LONG st = 0;
    if (state.animated)
        st |= STATE_SYSTEM_ANIMATED;
    if (state.busy)
        st |= STATE_SYSTEM_BUSY;
    if (state.checked)
        st |= STATE_SYSTEM_CHECKED;
    if (state.collapsed)
        st |= STATE_SYSTEM_COLLAPSED;
    if (state.defaultButton)
        st |= STATE_SYSTEM_DEFAULT;
    if (state.expanded)
        st |= STATE_SYSTEM_EXPANDED;
    if (state.extSelectable)
        st |= STATE_SYSTEM_EXTSELECTABLE;
    if (state.focusable)
        st |= STATE_SYSTEM_FOCUSABLE;
    if (state.focused)
        st |= STATE_SYSTEM_FOCUSED;
    if (state.hasPopup)
        st |= STATE_SYSTEM_HASPOPUP;
    if (state.hotTracked)
        st |= STATE_SYSTEM_HOTTRACKED;
    if (state.invisible)
        st |= STATE_SYSTEM_INVISIBLE;
    if (state.linked)
        st |= STATE_SYSTEM_LINKED;
    if (state.marqueed)
        st |= STATE_SYSTEM_MARQUEED;
    if (state.mixed)
        st |= STATE_SYSTEM_MIXED;
    if (state.movable)
        st |= STATE_SYSTEM_MOVEABLE;
    if (state.multiSelectable)
        st |= STATE_SYSTEM_MULTISELECTABLE;
    if (state.offscreen)
        st |= STATE_SYSTEM_OFFSCREEN;
    if (state.pressed)
        st |= STATE_SYSTEM_PRESSED;
    if (state.passwordEdit)
        st |= STATE_SYSTEM_PROTECTED;
    if (state.readOnly)
        st |= STATE_SYSTEM_READONLY;
    if (state.selectable)
        st |= STATE_SYSTEM_SELECTABLE;
    if (state.selected)
        st |= STATE_SYSTEM_SELECTED;
    if (state.selfVoicing)
        st |= STATE_SYSTEM_SELFVOICING;
    if (state.sizeable)
        st |= STATE_SYSTEM_SIZEABLE;
    if (state.traversed)
        st |= STATE_SYSTEM_TRAVERSED;

    (*pvarState).vt = VT_I4;
    (*pvarState).lVal = st;
    return S_OK;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accValue(VARIANT varID, BSTR* pszValue)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid() || varID.lVal)
        return E_FAIL;

    QString value;
    if (accessible->valueInterface()) {
        value = QString::number(accessible->valueInterface()->currentValue().toDouble());
    } else {
        value = accessible->text(QAccessible::Value);
    }
    if (!value.isNull()) {
        *pszValue = QStringToBSTR(value);
        return S_OK;
    }

    *pszValue = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::put_accValue(VARIANT, BSTR)
{
    showDebug(__FUNCTION__, accessible);
    return DISP_E_MEMBERNOTFOUND;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::accSelect(long flagsSelect, VARIANT varID)
{
    Q_UNUSED(flagsSelect);
    Q_UNUSED(varID);
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    bool res = false;

/*
  ### Check for accessibleTableInterface() or accessibleTextInterface()

  ### and if there are no ia2 interfaces we should do nothing??
    if (flagsSelect & SELFLAG_TAKEFOCUS)
        res = accessible->doAction(SetFocus, varID.lVal, QVariantList());
    if (flagsSelect & SELFLAG_TAKESELECTION) {
        accessible->doAction(ClearSelection, 0, QVariantList());
        res = accessible->doAction(AddToSelection, varID.lVal, QVariantList());
    }
    if (flagsSelect & SELFLAG_EXTENDSELECTION)
        res = accessible->doAction(ExtendSelection, varID.lVal, QVariantList());
    if (flagsSelect & SELFLAG_ADDSELECTION)
        res = accessible->doAction(AddToSelection, varID.lVal, QVariantList());
    if (flagsSelect & SELFLAG_REMOVESELECTION)
        res = accessible->doAction(RemoveSelection, varID.lVal, QVariantList());
*/
    return res ? S_OK : S_FALSE;
}

// moz: [important]
HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accFocus(VARIANT *pvarID)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    QAccessibleInterface *acc = 0;
    int control = accessible->navigate(QAccessible::FocusChild, 1, &acc);
    if (control == -1) {
        (*pvarID).vt = VT_EMPTY;
        return S_FALSE;
    }
    if (!acc || control == 0) {
        (*pvarID).vt = VT_I4;
        (*pvarID).lVal = control ? control : CHILDID_SELF;
        return S_OK;
    }

    QWindowsAccessible* wacc = new QWindowsAccessible(acc);
    IDispatch *iface = 0;
    wacc->QueryInterface(IID_IDispatch, (void**)&iface);
    if (iface) {
        (*pvarID).vt = VT_DISPATCH;
        (*pvarID).pdispVal = iface;
        return S_OK;
    } else {
        delete wacc;
    }

    (*pvarID).vt = VT_EMPTY;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::get_accSelection(VARIANT *pvarChildren)
{
    showDebug(__FUNCTION__, accessible);
    if (!accessible->isValid())
        return E_FAIL;

    int cc = accessible->childCount();
    QVector<int> sel(cc);
    int selIndex = 0;
    for (int i = 0; i < cc; ++i) {
        bool isSelected = false;
        QAccessibleInterface *child = accessible->child(i);
        if (child) {
            isSelected = child->state().selected;
            delete child;
        }
        if (isSelected)
            sel[selIndex++] = i+1;
    }
    sel.resize(selIndex);
    if (sel.isEmpty()) {
        (*pvarChildren).vt = VT_EMPTY;
        return S_FALSE;
    }
    if (sel.size() == 1) {
        (*pvarChildren).vt = VT_I4;
        (*pvarChildren).lVal = sel[0];
        return S_OK;
    }
    IEnumVARIANT *iface = new QWindowsEnumerate(sel);
    IUnknown *uiface;
    iface->QueryInterface(IID_IUnknown, (void**)&uiface);
    (*pvarChildren).vt = VT_UNKNOWN;
    (*pvarChildren).punkVal = uiface;

    return S_OK;
}

static QWindow *window_helper(const QAccessibleInterface *iface)
{
    QWindow *window = iface->window();
    if (!window) {
        QAccessibleInterface *acc = iface->parent();
        while (acc && !window) {
            window = acc->window();
            QAccessibleInterface *par = acc->parent();
            delete acc;
            acc = par;
        }
    }
    return window;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::GetWindow(HWND *phwnd)
{
    *phwnd = 0;
    if (!accessible->isValid())
        return E_UNEXPECTED;

    QWindow *window = window_helper(accessible);
    if (!window)
        return E_FAIL;

    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    Q_ASSERT(platform);
    *phwnd = (HWND)platform->nativeResourceForWindow("handle", window);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsAccessible::ContextSensitiveHelp(BOOL)
{
    return S_OK;
}


QWindowsAccessibility::QWindowsAccessibility()
{
}


void QWindowsAccessibility::notifyAccessibilityUpdate(QObject *o, int who, QAccessible::Event reason)
{
    QString soundName;
    switch (reason) {
    case QAccessible::PopupMenuStart:
        soundName = QLatin1String("MenuPopup");
        break;

    case QAccessible::MenuCommand:
        soundName = QLatin1String("MenuCommand");
        break;

    case QAccessible::Alert:
        {
        /*      ### FIXME
#ifndef QT_NO_MESSAGEBOX
            QMessageBox *mb = qobject_cast<QMessageBox*>(o);
            if (mb) {
                switch (mb->icon()) {
                case QMessageBox::Warning:
                    soundName = QLatin1String("SystemExclamation");
                    break;
                case QMessageBox::Critical:
                    soundName = QLatin1String("SystemHand");
                    break;
                case QMessageBox::Information:
                    soundName = QLatin1String("SystemAsterisk");
                    break;
                default:
                    break;
                }
            } else
#endif // QT_NO_MESSAGEBOX
*/
            {
                soundName = QLatin1String("SystemAsterisk");
            }

        }
        break;
    default:
        break;
    }

    if (!soundName.isEmpty()) {
#ifndef QT_NO_SETTINGS
        QSettings settings(QLatin1String("HKEY_CURRENT_USER\\AppEvents\\Schemes\\Apps\\.Default\\") + soundName,
                           QSettings::NativeFormat);
        QString file = settings.value(QLatin1String(".Current/.")).toString();
#else
        QString file;
#endif
        if (!file.isEmpty()) {
            PlaySound(reinterpret_cast<const wchar_t *>(soundName.utf16()), 0, SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
        }
    }

    typedef void (WINAPI *PtrNotifyWinEvent)(DWORD, HWND, LONG, LONG);

#if defined(Q_WS_WINCE) // ### TODO: check for NotifyWinEvent in CE 6.0
    // There is no user32.lib nor NotifyWinEvent for CE
    return;
#else
    static PtrNotifyWinEvent ptrNotifyWinEvent = 0;
    static bool resolvedNWE = false;
    if (!resolvedNWE) {
        resolvedNWE = true;
        ptrNotifyWinEvent = (PtrNotifyWinEvent)QSystemLibrary::resolve(QLatin1String("user32"), "NotifyWinEvent");
    }
    if (!ptrNotifyWinEvent)
        return;

    // An event has to be associated with a window,
    // so find the first parent that is a widget and that has a WId
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
    QWindow *window = iface ? window_helper(iface) : 0;

    if (!window) {
        window = QGuiApplication::activeWindow();
        if (!window)
            return;
    }

    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    HWND hWnd = (HWND)platform->nativeResourceForWindow("handle", window);

    if (reason != QAccessible::MenuCommand) { // MenuCommand is faked
        // See comment "SENDING EVENTS TO OBJECTS WITH NO WINDOW HANDLE"
        eventNum %= 50;              //[0..49]
        int eventId = - eventNum - 1;

        qAccessibleRecentSentEvents()->insert(eventId, qMakePair(o, who));
        ptrNotifyWinEvent(reason, hWnd, OBJID_CLIENT, eventId );

        ++eventNum;
    }
#endif // Q_WS_WINCE

}

/*
void QWindowsAccessibility::setRootObject(QObject *o)
{

}

void QWindowsAccessibility::initialize()
{

}

void QWindowsAccessibility::cleanup()
{

}

*/

bool QWindowsAccessibility::handleAccessibleObjectFromWindowRequest(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
        /* For UI Automation
      */
    } else if ((DWORD)lParam == OBJID_CLIENT) {
#if 1
        // Ignoring all requests while starting up
        // ### Maybe QPA takes care of this???
        if (QApplication::startingUp() || QApplication::closingDown())
            return false;
#endif

        typedef LRESULT (WINAPI *PtrLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);
        static PtrLresultFromObject ptrLresultFromObject = 0;
        static bool oleaccChecked = false;

        if (!oleaccChecked) {
            oleaccChecked = true;
#if !defined(Q_OS_WINCE)
            ptrLresultFromObject = (PtrLresultFromObject)QSystemLibrary::resolve(QLatin1String("oleacc"), "LresultFromObject");
#endif
        }

        if (ptrLresultFromObject) {
            QWindow *window = QWindowsContext::instance()->findWindow(hwnd);
            if (window) {
                QAccessibleInterface *acc = window->accessibleRoot();
                if (acc) {
                    QWindowsAccessible *winacc = new QWindowsAccessible(acc);
                    IAccessible *iface;
                    HRESULT hr = winacc->QueryInterface(IID_IAccessible, (void**)&iface);
                    if (SUCCEEDED(hr)) {
                        *lResult = ptrLresultFromObject(IID_IAccessible, wParam, iface);  // ref == 2
                        if (*lResult) {
                            iface->Release(); // the client will release the object again, and then it will destroy itself
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
