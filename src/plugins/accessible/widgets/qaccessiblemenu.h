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

#ifndef QACCESSIBLEMENU_H
#define QACCESSIBLEMENU_H

#include <QtWidgets/qaccessiblewidget.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

#ifndef QT_NO_MENU
class QMenu;
class QMenuBar;
class QAction;

class QAccessibleMenu : public QAccessibleWidget
{
public:
    explicit QAccessibleMenu(QWidget *w);

    int childCount() const;
    QAccessibleInterface *childAt(int x, int y) const;

    QString text(QAccessible::Text t) const;
    QAccessible::Role role() const;
    QAccessibleInterface *child(int index) const;
    QAccessibleInterface *parent() const;
    int indexOfChild( const QAccessibleInterface *child ) const;

protected:
    QMenu *menu() const;
};

#ifndef QT_NO_MENUBAR
class QAccessibleMenuBar : public QAccessibleWidget
{
public:
    explicit QAccessibleMenuBar(QWidget *w);

    QAccessibleInterface *child(int index) const;
    int childCount() const;

    int indexOfChild(const QAccessibleInterface *child) const;

protected:
    QMenuBar *menuBar() const;
};
#endif // QT_NO_MENUBAR


class QAccessibleMenuItem : public QAccessibleInterface, public QAccessibleActionInterface
{
public:
    explicit QAccessibleMenuItem(QWidget *owner, QAction *w);

    ~QAccessibleMenuItem();
    void *interface_cast(QAccessible::InterfaceType t);

    int childCount() const;
    QAccessibleInterface *childAt(int x, int y) const;
    bool isValid() const;
    int indexOfChild(const QAccessibleInterface * child) const;

    QAccessibleInterface *parent() const;
    QAccessibleInterface *child(int index) const;
    int navigate(QAccessible::RelationFlag relation, int entry, QAccessibleInterface ** target) const;
    QObject * object() const;
    QRect rect() const;
    QAccessible::Role role() const;
    void setText(QAccessible::Text t, const QString & text);
    QAccessible::State state() const;
    QString text(QAccessible::Text t) const;

    // QAccessibleActionInterface
    QStringList actionNames() const;
    void doAction(const QString &actionName);
    QStringList keyBindingsForAction(const QString &actionName) const;

    QWidget *owner() const;
protected:
    QAction *action() const;
private:
    QAction *m_action;
    QWidget *m_owner; // can hold either QMenu or the QMenuBar that contains the action
};

#endif // QT_NO_MENU

QT_END_NAMESPACE
#endif // QT_NO_ACCESSIBILITY
#endif // QACCESSIBLEMENU_H
