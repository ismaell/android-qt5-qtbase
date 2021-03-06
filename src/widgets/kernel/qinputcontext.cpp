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

/****************************************************************************
**
** Implementation of QInputContext class
**
** Copyright (C) 2003-2004 immodule for Qt Project.  All rights reserved.
**
** This file is written to contribute to Nokia Corporation and/or its subsidiary(-ies) under their own
** license. You may use this file under your Qt license. Following
** description is copied from their original file headers. Contact
** immodule-qt@freedesktop.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

//#define QT_NO_IM_PREEDIT_RELOCATION

#include "qinputcontext.h"

#ifndef QT_NO_IM

#include "qplatformdefs.h"

#include "qapplication.h"
#include "qmenu.h"
#include "qtextformat.h"
#include "qpalette.h"
#include <QtGui/qinputpanel.h>
#include <QtGui/qevent.h>

#include <stdlib.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

/*!
    \class QInputContext
    \brief The QInputContext class abstracts the input method dependent data and composing state.

    \ingroup i18n
    \inmodule QtWidgets

    An input method is responsible for inputting complex text that cannot
    be inputted via simple keymap. It converts a sequence of input
    events (typically key events) into a text string through the input
    method specific converting process. The class of the processes are
    widely ranging from simple finite state machine to complex text
    translator that pools a whole paragraph of a text with text
    editing capability to perform grammar and semantic analysis.

    To abstract such different input method specific intermediate
    information, Qt offers the QInputContext as base class. The
    concept is well known as 'input context' in the input method
    domain. An input context is created for a text widget in response
    to a demand. It is ensured that an input context is prepared for
    an input method before input to a text widget.

    Multiple input contexts that belong to a single input method
    may concurrently coexist. Suppose multi-window text editor. Each
    text widget of window A and B holds different QInputContext
    instance which contains different state information such as
    partially composed text.

    \section1 Groups of Functions

    \table
    \header \o Context \o Functions

    \row \o Receiving information \o
        x11FilterEvent(),
        filterEvent(),
        mouseHandler()

    \row \o Sending back composed text \o
        sendEvent()

    \row \o State change notification \o
        setFocusWidget(),
        reset()

    \row \o Context information \o
        identifierName(),
        language(),
        font(),
        isComposing()

    \endtable

    \section1 Licensing Information

    \legalese
    Copyright (C) 2003-2004 immodule for Qt Project.  All rights reserved.

    This file is written to contribute to Nokia Corporation and/or its subsidiary(-ies) under their own
    license. You may use this file under your Qt license. Following
    description is copied from their original file headers. Contact
    immodule-qt@freedesktop.org if any conditions of this licensing are
    not clear to you.
    \endlegalese

    \sa QInputContextPlugin, QInputContextFactory, QApplication::setInputContext()
*/

/*!
    Constructs an input context with the given \a parent.
*/
QInputContext::QInputContext(QObject* parent)
    : QObject(parent)
{
}


/*!
    Destroys the input context.
*/
QInputContext::~QInputContext()
{
}

/*!
    Returns the widget that has an input focus for this input
    context.

    The return value may differ from holderWidget() if the input
    context is shared between several text widgets.

    \warning To ensure platform independence and support flexible
    configuration of widgets, ordinary input methods should not call
    this function directly.

    \sa setFocusWidget()
*/
QWidget *QInputContext::focusWidget() const
{
    return qobject_cast<QWidget *>(qApp->inputPanel()->inputItem());
}


/*!
    Sets the \a widget that has an input focus for this input context.

    \warning Ordinary input methods must not call this function
    directly.

    \sa focusWidget()
*/
void QInputContext::setFocusWidget(QWidget *widget)
{
    qApp->inputPanel()->setInputItem(widget);
}

/*!
    \fn bool QInputContext::isComposing() const
    \obsolete

    This function indicates whether InputMethodStart event had been
    sent to the current focus widget. It is ensured that an input
    context can send InputMethodCompose or InputMethodEnd event safely
    if this function returned true.

    The state is automatically being tracked through sendEvent().

    \sa sendEvent()
*/


/*!
  Sends an input method event specified by \a event to the current focus
  widget. Implementations of QInputContext should call this method to
  send the generated input method events and not
  QApplication::sendEvent(), as the events might have to get dispatched
  to a different application on some platforms.

  Some complex input methods route the handling to several child
  contexts (e.g. to enable language switching). To account for this,
  QInputContext will check if the parent object is a QInputContext. If
  yes, it will call the parents sendEvent() implementation instead of
  sending the event directly.

  \sa QInputMethodEvent
*/
void QInputContext::sendEvent(const QInputMethodEvent &event)
{

    QObject *focus = qApp->inputPanel()->inputItem();
    if (!focus)
	return;

    QInputMethodEvent e(event);
    QApplication::sendEvent(focus, &e);
}


/*!
    This function can be reimplemented in a subclass to handle mouse
    press, release, double-click, and move events within the preedit
    text. You can use the function to implement mouse-oriented user
    interface such as text selection or popup menu for candidate
    selection.

    The \a x parameter is the offset within the string that was sent
    with the InputMethodCompose event. The alteration boundary of \a
    x is ensured as character boundary of preedit string accurately.

    The \a event parameter is the event that was sent to the editor
    widget. The event type is QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick or
    QEvent::MouseMove. The event's button and state indicate
    the kind of operation that was performed.
*/
void QInputContext::mouseHandler(int x, QMouseEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease)
        qApp->inputPanel()->invokeAction(QInputPanel::Click, x);
}


/*!
    Returns the font of the current input widget
*/
QFont QInputContext::font() const
{
    QWidget *focus = focusWidget();
    if (!focus)
        return QApplication::font();

    return qvariant_cast<QFont>(focus->inputMethodQuery(Qt::ImFont));
}

/*!
    This virtual function is called when a state in the focus widget
    has changed. QInputContext can then use
    QWidget::inputMethodQuery() to query the new state of the widget.
*/
void QInputContext::update()
{
    qApp->inputPanel()->update(Qt::ImQueryAll);
}

/*!
    This virtual function is called when the specified \a widget is
    destroyed. The \a widget is a widget on which this input context
    is installed.
*/
void QInputContext::widgetDestroyed(QWidget *widget)
{
    Q_UNUSED(widget)
    // nothing to be done here, as we use a weak pointer in the input panel
}

/*!
    \fn void QInputContext::reset()

    This function can be reimplemented in a subclass to reset the
    state of the input method.

    This function is called by several widgets to reset input
    state. For example, a text widget call this function before
    inserting a text to make widget ready to accept a text.

    Default implementation is sufficient for simple input method. You
    can override this function to reset external input method engines
    in complex input method. In the case, call QInputContext::reset()
    to ensure proper termination of inputting.

    In a reimplementation of reset(), you must not send any
    QInputMethodEvent containing preedit text. You can only commit
    string and attributes; otherwise, you risk breaking input state
    consistency.
*/
void QInputContext::reset()
{
    qApp->inputPanel()->reset();
}


/*!
  \fn QString QInputContext::identifierName()

    This function must be implemented in any subclasses to return the
    identifier name of the input method.

    Return value is the name to identify and specify input methods for
    the input method switching mechanism and so on. The name has to be
    consistent with QInputContextPlugin::keys(). The name has to
    consist of ASCII characters only.

    There are two different names with different responsibility in the
    input method domain. This function returns one of them. Another
    name is called 'display name' that stands for the name for
    endusers appeared in a menu and so on.

    \sa QInputContextPlugin::keys(), QInputContextPlugin::displayName()
*/
QString QInputContext::identifierName()
{
    return QLatin1String("qpa");
}


/*!
    \fn QString QInputContext::language()

    This function must be implemented in any subclasses to return a
    language code (e.g. "zh_CN", "zh_TW", "zh_HK", "ja", "ko", ...)
    of the input context. If the input context can handle multiple
    languages, return the currently used one. The name has to be
    consistent with QInputContextPlugin::language().

    This information will be used by language tagging feature in
    QInputMethodEvent. It is required to distinguish unified han characters
    correctly. It enables proper font and character code
    handling. Suppose CJK-awared multilingual web browser
    (that automatically modifies fonts in CJK-mixed text) and XML editor
    (that automatically inserts lang attr).
*/
QString QInputContext::language()
{
    return QString();
}


/*!
    This is a preliminary interface for Qt 4.
*/
QList<QAction *> QInputContext::actions()
{
    return QList<QAction *>();
}

/*!
    \enum QInputContext::StandardFormat

    \value PreeditFormat  The preedit text.
    \value SelectionFormat  The selection text.

    \sa standardFormat()
*/

/*!
    Returns a QTextFormat object that specifies the format for
    component \a s.
*/
QTextFormat QInputContext::standardFormat(StandardFormat s) const
{
    QWidget *focus = focusWidget();
    const QPalette &pal = focus ? focus->palette() : QApplication::palette();

    QTextCharFormat fmt;
    QColor bg;
    switch (s) {
    case QInputContext::PreeditFormat: {
        fmt.setUnderlineStyle(QTextCharFormat::DashUnderline);
        break;
    }
    case QInputContext::SelectionFormat: {
        bg = pal.text().color();
        fmt.setBackground(QBrush(bg));
        fmt.setForeground(pal.background());
        break;
    }
    }
    return fmt;
}

QT_END_NAMESPACE

#endif //Q_NO_IM
