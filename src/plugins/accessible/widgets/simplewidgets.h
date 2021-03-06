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

#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

#include <QtCore/qcoreapplication.h>
#include <QtGui/qaccessible2.h>
#include <QtWidgets/qaccessiblewidget.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

class QAbstractButton;
class QLineEdit;
class QToolButton;
class QProgressBar;

class QAccessibleButton : public QAccessibleWidget
{
    Q_DECLARE_TR_FUNCTIONS(QAccessibleButton)
public:
    QAccessibleButton(QWidget *w, QAccessible::Role r);

    QString text(QAccessible::Text t) const;
    QAccessible::State state() const;

    QStringList actionNames() const;
    void doAction(const QString &actionName);
    QStringList keyBindingsForAction(const QString &actionName) const;

protected:
    QAbstractButton *button() const;
};

#ifndef QT_NO_TOOLBUTTON
class QAccessibleToolButton : public QAccessibleButton
{
public:
    QAccessibleToolButton(QWidget *w, QAccessible::Role role);

    QAccessible::State state() const;

    int childCount() const;
    QAccessibleInterface *child(int index) const;

    QString text(QAccessible::Text t) const;

    // QAccessibleActionInterface
    QStringList actionNames() const;
    void doAction(const QString &actionName);

protected:
    QToolButton *toolButton() const;

    bool isSplitButton() const;
};
#endif // QT_NO_TOOLBUTTON

class QAccessibleDisplay : public QAccessibleWidget, public QAccessibleImageInterface
{
public:
    explicit QAccessibleDisplay(QWidget *w, QAccessible::Role role = QAccessible::StaticText);

    QString text(QAccessible::Text t) const;
    QAccessible::Role role() const;

    QAccessible::Relation relationTo(const QAccessibleInterface *other) const;
    int navigate(QAccessible::RelationFlag, int entry, QAccessibleInterface **target) const;
    void *interface_cast(QAccessible::InterfaceType t);

    // QAccessibleImageInterface
    QString imageDescription() const;
    QSize imageSize() const;
    QRect imagePosition(QAccessible2::CoordinateType coordType) const;
};

#ifndef QT_NO_LINEEDIT
class QAccessibleLineEdit : public QAccessibleWidget, public QAccessibleTextInterface,
                            public QAccessibleSimpleEditableTextInterface
{
public:
    explicit QAccessibleLineEdit(QWidget *o, const QString &name = QString());

    QString text(QAccessible::Text t) const;
    void setText(QAccessible::Text t, const QString &text);
    QAccessible::State state() const;
    void *interface_cast(QAccessible::InterfaceType t);

    // QAccessibleTextInterface
    void addSelection(int startOffset, int endOffset);
    QString attributes(int offset, int *startOffset, int *endOffset) const;
    int cursorPosition() const;
    QRect characterRect(int offset, QAccessible2::CoordinateType coordType) const;
    int selectionCount() const;
    int offsetAtPoint(const QPoint &point, QAccessible2::CoordinateType coordType) const;
    void selection(int selectionIndex, int *startOffset, int *endOffset) const;
    QString text(int startOffset, int endOffset) const;
    QString textBeforeOffset (int offset, QAccessible2::BoundaryType boundaryType,
            int *startOffset, int *endOffset) const;
    QString textAfterOffset(int offset, QAccessible2::BoundaryType boundaryType,
            int *startOffset, int *endOffset) const;
    QString textAtOffset(int offset, QAccessible2::BoundaryType boundaryType,
            int *startOffset, int *endOffset) const;
    void removeSelection(int selectionIndex);
    void setCursorPosition(int position);
    void setSelection(int selectionIndex, int startOffset, int endOffset);
    int characterCount() const;
    void scrollToSubstring(int startIndex, int endIndex);

protected:
    QLineEdit *lineEdit() const;
};
#endif // QT_NO_LINEEDIT

#ifndef QT_NO_PROGRESSBAR
class QAccessibleProgressBar : public QAccessibleDisplay, public QAccessibleValueInterface
{
public:
    explicit QAccessibleProgressBar(QWidget *o);
    void *interface_cast(QAccessible::InterfaceType t);

    // QAccessibleValueInterface
    QVariant currentValue() const;
    QVariant maximumValue() const;
    QVariant minimumValue() const;
    inline void setCurrentValue(const QVariant &) {}

protected:
    QProgressBar *progressBar() const;
};
#endif

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // SIMPLEWIDGETS_H
