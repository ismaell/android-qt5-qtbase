/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>
#include <QtGui>
#include <QtWidgets>
#include <math.h>

#if defined(Q_OS_WIN) && defined(interface)
#   undef interface
#endif


#include "QtTest/qtestaccessible.h"

#if defined(Q_OS_WINCE)
extern "C" bool SystemParametersInfo(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
#define SPI_GETPLATFORMTYPE 257
inline bool IsValidCEPlatform() {
    wchar_t tszPlatform[64];
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(tszPlatform) / sizeof(*tszPlatform), tszPlatform, 0)) {
        QString platform = QString::fromWCharArray(tszPlatform);
        if ((platform == QLatin1String("PocketPC")) || (platform == QLatin1String("Smartphone")))
            return false;
    }
    return true;
}
#endif

typedef QSharedPointer<QAccessibleInterface> QAIPtr;

static inline bool verifyChild(QWidget *child, QAccessibleInterface *interface,
                               int index, const QRect &domain)
{
    if (!child) {
        qWarning("tst_QAccessibility::verifyChild: null pointer to child.");
        return false;
    }

    if (!interface) {
        qWarning("tst_QAccessibility::verifyChild: null pointer to interface.");
        return false;
    }

    // Verify that we get a valid QAccessibleInterface for the child.
    QAccessibleInterface *childInterface = QAccessible::queryAccessibleInterface(child);
    if (!childInterface) {
        qWarning("tst_QAccessibility::verifyChild: Failed to retrieve interface for child.");
        return false;
    }

    // QAccessibleInterface::indexOfChild():
    // Verify that indexOfChild() returns an index equal to the index passed in
    int indexFromIndexOfChild = interface->indexOfChild(childInterface);
    if (indexFromIndexOfChild != index) {
        qWarning("tst_QAccessibility::verifyChild (indexOfChild()):");
        qWarning() << "Expected:" << index;
        qWarning() << "Actual:  " << indexFromIndexOfChild;
        return false;
    }

    // Navigate to child, compare its object and role with the interface from queryAccessibleInterface(child).
    QAccessibleInterface *navigatedChildInterface = interface->child(index);
    if (navigatedChildInterface == 0)
        return false;

    const QRect rectFromInterface = navigatedChildInterface->rect();
    delete navigatedChildInterface;

    // QAccessibleInterface::childAt():
    // Calculate global child position and check that the interface
    // returns the correct index for that position.
    QPoint globalChildPos = child->mapToGlobal(QPoint(0, 0));
    QAccessibleInterface *childAtInterface = interface->childAt(globalChildPos.x(), globalChildPos.y());
    if (!childAtInterface) {
        qWarning("tst_QAccessibility::verifyChild (childAt()):");
        qWarning() << "Expected:" << childInterface;
        qWarning() << "Actual:  no child";
        return false;
    }
    if (childAtInterface->object() != childInterface->object()) {
        qWarning("tst_QAccessibility::verifyChild (childAt()):");
        qWarning() << "Expected:" << childInterface;
        qWarning() << "Actual:  " << childAtInterface;
        return false;
    }
    delete childInterface;
    delete childAtInterface;

    // QAccessibleInterface::rect():
    // Calculate global child geometry and check that the interface
    // returns a QRect which is equal to the calculated QRect.
    const QRect expectedGlobalRect = QRect(globalChildPos, child->size());
    if (expectedGlobalRect != rectFromInterface) {
        qWarning("tst_QAccessibility::verifyChild (rect()):");
        qWarning() << "Expected:" << expectedGlobalRect;
        qWarning() << "Actual:  " << rectFromInterface;
        return false;
    }

    // Verify that the child is within its domain.
    if (!domain.contains(rectFromInterface)) {
        qWarning("tst_QAccessibility::verifyChild: Child is not within its domain.");
        return false;
    }

    return true;
}

static inline int indexOfChild(QAccessibleInterface *parentInterface, QWidget *childWidget)
{
    if (!parentInterface || !childWidget)
        return -1;
    QAccessibleInterface *childInterface = QAccessible::queryAccessibleInterface(childWidget);
    if (!childInterface)
        return -1;
    int index = parentInterface->indexOfChild(childInterface);
    delete childInterface;
    return index;
}

#define EXPECT(cond) \
    do { \
        if (!errorAt && !(cond)) { \
            errorAt = __LINE__; \
            qWarning("level: %d, middle: %d, role: %d (%s)", treelevel, middle, iface->role(), #cond); \
        } \
    } while (0)

static int verifyHierarchy(QAccessibleInterface *iface)
{
    int errorAt = 0;
    static int treelevel = 0;   // for error diagnostics
    QAccessibleInterface *middleChild, *if2;
    middleChild = 0;
    ++treelevel;
    int middle = iface->childCount()/2 + 1;
    if (iface->childCount() >= 2) {
        middleChild = iface->child(middle - 1);
    }
    for (int i = 0; i < iface->childCount() && !errorAt; ++i) {
        if2 = iface->child(i);
        EXPECT(if2 != 0);
        // navigate Ancestor...
        QAccessibleInterface *parent = if2->parent();
        EXPECT(iface->object() == parent->object());
        delete parent;

            // navigate Sibling...
//            if (middleChild) {
//                entry = if2->navigate(QAccessible::Sibling, middle, &if3);
//                EXPECT(entry == 0 && if3->object() == middleChild->object());
//                if (entry == 0)
//                    delete if3;
//                EXPECT(iface->indexOfChild(middleChild) == middle);
//            }

        // verify children...
        if (!errorAt)
            errorAt = verifyHierarchy(if2);
        delete if2;
    }
    delete middleChild;

    --treelevel;
    return errorAt;
}

QRect childRect(QAccessibleInterface *iface, int index = 0)
{
    QAccessibleInterface *child = iface->child(index);
    QRect rect = child->rect();
    delete child;
    return rect;
}

class tst_QAccessibility : public QObject
{
    Q_OBJECT
public:
    tst_QAccessibility();
    virtual ~tst_QAccessibility();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void eventTest();
    void customWidget();
    void deletedWidget();

    void statesStructTest();
    void navigateHierarchy();
    void sliderTest();
    void navigateCovered();
    void textAttributes();
    void hideShowTest();

    void actionTest();

    void applicationTest();
    void mainWindowTest();
    void buttonTest();
    void scrollBarTest();
    void tabTest();
    void tabWidgetTest();
    void menuTest();
    void spinBoxTest();
    void doubleSpinBoxTest();
    void textEditTest();
    void textBrowserTest();
    void mdiAreaTest();
    void mdiSubWindowTest();
    void lineEditTest();
    void workspaceTest();
    void dialogButtonBoxTest();
    void dialTest();
    void rubberBandTest();
    void abstractScrollAreaTest();
    void scrollAreaTest();

    void listTest();
    void treeTest();
    void tableTest();

    void calendarWidgetTest();
    void dockWidgetTest();
    void comboBoxTest();
    void accessibleName();
    void labelTest();
    void accelerators();

protected slots:
    void onClicked();
private:
    int click_count;
};

const double Q_PI = 3.14159265358979323846;

QString eventName(const int ev)
{
    switch(ev) {
    case 0x0001: return "SoundPlayed";
    case 0x0002: return "Alert";
    case 0x0003: return "ForegroundChanged";
    case 0x0004: return "MenuStart";
    case 0x0005: return "MenuEnd";
    case 0x0006: return "PopupMenuStart";
    case 0x0007: return "PopupMenuEnd";
    case 0x000C: return "ContextHelpStart";
    case 0x000D: return "ContextHelpEnd";
    case 0x000E: return "DragDropStart";
    case 0x000F: return "DragDropEnd";
    case 0x0010: return "DialogStart";
    case 0x0011: return "DialogEnd";
    case 0x0012: return "ScrollingStart";
    case 0x0013: return "ScrollingEnd";
    case 0x0018: return "MenuCommand";

    case 0x0116: return "TableModelChanged";
    case 0x011B: return "TextCaretMoved";

    case 0x8000: return "ObjectCreated";
    case 0x8001: return "ObjectDestroyed";
    case 0x8002: return "ObjectShow";
    case 0x8003: return "ObjectHide";
    case 0x8004: return "ObjectReorder";
    case 0x8005: return "Focus";
    case 0x8006: return "Selection";
    case 0x8007: return "SelectionAdd";
    case 0x8008: return "SelectionRemove";
    case 0x8009: return "SelectionWithin";
    case 0x800A: return "StateChanged";
    case 0x800B: return "LocationChanged";
    case 0x800C: return "NameChanged";
    case 0x800D: return "DescriptionChanged";
    case 0x800E: return "ValueChanged";
    case 0x800F: return "ParentChanged";
    case 0x80A0: return "HelpChanged";
    case 0x80B0: return "DefaultActionChanged";
    case 0x80C0: return "AcceleratorChanged";
    default: return "Unknown Event";
    }
}

QAccessible::State state(QWidget * const widget)
{
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    if (!iface)
        qWarning() << "Cannot get QAccessibleInterface for widget";
    QAccessible::State state = (iface ? iface->state() : QAccessible::State());
    delete iface;
    return state;
}

class QtTestAccessibleWidget: public QWidget
{
    Q_OBJECT
public:
    QtTestAccessibleWidget(QWidget *parent, const char *name): QWidget(parent)
    {
        setObjectName(name);
        QPalette pal;
        pal.setColor(backgroundRole(), Qt::black);//black is beautiful
        setPalette(pal);
        setFixedSize(5, 5);
    }
};

class QtTestAccessibleWidgetIface: public QAccessibleWidget
{
public:
    QtTestAccessibleWidgetIface(QtTestAccessibleWidget *w): QAccessibleWidget(w) {}
    QString text(QAccessible::Text t) const
    {
        if (t == QAccessible::Help)
            return QString::fromLatin1("Help yourself");
        return QAccessibleWidget::text(t);
    }
    static QAccessibleInterface *ifaceFactory(const QString &key, QObject *o)
    {
        if (key == "QtTestAccessibleWidget")
            return new QtTestAccessibleWidgetIface(static_cast<QtTestAccessibleWidget*>(o));
        return 0;
    }
};

tst_QAccessibility::tst_QAccessibility()
{
    click_count = 0;
}

tst_QAccessibility::~tst_QAccessibility()
{
}

void tst_QAccessibility::onClicked()
{
    click_count++;
}

void tst_QAccessibility::initTestCase()
{
    QTestAccessibility::initialize();
    QAccessible::installFactory(QtTestAccessibleWidgetIface::ifaceFactory);
}

void tst_QAccessibility::cleanupTestCase()
{
    QTestAccessibility::cleanup();
}

void tst_QAccessibility::init()
{
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::cleanup()
{
    const EventList list = QTestAccessibility::events();
    if (!list.isEmpty()) {
        qWarning("%d accessibility event(s) were not handled in testfunction '%s':", list.count(),
                 QString(QTest::currentTestFunction()).toAscii().constData());
        for (int i = 0; i < list.count(); ++i)
            qWarning(" %d: Object: %p Event: '%s' (%d) Child: %d", i + 1, list.at(i).object,
                     eventName(list.at(i).event).toAscii().constData(), list.at(i).event, list.at(i).child);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::eventTest()
{
    QPushButton* button = new QPushButton(0);
    button->setObjectName(QString("Olaf"));

    button->show();
    QVERIFY_EVENT(button, 0, QAccessible::ObjectShow);
    button->setFocus(Qt::MouseFocusReason);
    QTestAccessibility::clearEvents();
    QTest::mouseClick(button, Qt::LeftButton, 0);
    QVERIFY_EVENT(button, 0, QAccessible::StateChanged);
    QVERIFY_EVENT(button, 0, QAccessible::StateChanged);

    button->setAccessibleName("Olaf the second");
    QVERIFY_EVENT(button, 0, QAccessible::NameChanged);
    button->setAccessibleDescription("This is a button labeled Olaf");
    QVERIFY_EVENT(button, 0, QAccessible::DescriptionChanged);

    button->hide();
    QVERIFY_EVENT(button, 0, QAccessible::ObjectHide);

    delete button;
}

void tst_QAccessibility::customWidget()
{
    QtTestAccessibleWidget* widget = new QtTestAccessibleWidget(0, "Heinz");

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)widget);
    QCOMPARE(iface->object()->objectName(), QString("Heinz"));
    QCOMPARE(iface->text(QAccessible::Help), QString("Help yourself"));

    delete iface;
    delete widget;
}

void tst_QAccessibility::deletedWidget()
{
    QtTestAccessibleWidget *widget = new QtTestAccessibleWidget(0, "Ralf");
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)widget);

    delete widget;
    widget = 0;
    QVERIFY(!iface->isValid());
    delete iface;
}

void tst_QAccessibility::statesStructTest()
{
    QAccessible::State s1;
    QVERIFY(s1.unavailable == 0);
    QVERIFY(s1.focusable == 0);
    QVERIFY(s1.modal == 0);

    QAccessible::State s2;
    QVERIFY(s2 == s1);
    s2.busy = true;
    QVERIFY(!(s2 == s1));
    s1.busy = true;
    QVERIFY(s2 == s1);
    s1 = QAccessible::State();
    QVERIFY(!(s2 == s1));
    s1 = s2;
    QVERIFY(s2 == s1);
    QVERIFY(s1.busy == 1);
}

void tst_QAccessibility::sliderTest()
{
    {
    QSlider *slider = new QSlider(0);
    slider->setObjectName(QString("Slidy"));
    slider->show();
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(slider);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());

    QCOMPARE(iface->childCount(), 0);
    QCOMPARE(iface->role(), QAccessible::Slider);

    QAccessibleValueInterface *valueIface = iface->valueInterface();
    QVERIFY(valueIface != 0);
    QCOMPARE(valueIface->minimumValue().toInt(), slider->minimum());
    QCOMPARE(valueIface->maximumValue().toInt(), slider->maximum());
    slider->setValue(50);
    QCOMPARE(valueIface->currentValue().toInt(), slider->value());
    slider->setValue(0);
    QCOMPARE(valueIface->currentValue().toInt(), slider->value());
    slider->setValue(100);
    QCOMPARE(valueIface->currentValue().toInt(), slider->value());
    valueIface->setCurrentValue(77);
    QCOMPARE(77, slider->value());

    delete iface;
    delete slider;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::navigateCovered()
{
    {
    QWidget *w = new QWidget(0);
    w->setObjectName(QString("Harry"));
    QWidget *w1 = new QWidget(w);
    w1->setObjectName(QString("1"));
    QWidget *w2 = new QWidget(w);
    w2->setObjectName(QString("2"));
    w->show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    w->setFixedSize(6, 6);
    w1->setFixedSize(5, 5);
    w2->setFixedSize(5, 5);
    w2->move(0, 0);
    w1->raise();

    QAccessibleInterface *iface1 = QAccessible::queryAccessibleInterface(w1);
    QVERIFY(iface1 != 0);
    QVERIFY(iface1->isValid());
    QAccessibleInterface *iface2 = QAccessible::queryAccessibleInterface(w2);
    QVERIFY(iface2 != 0);
    QVERIFY(iface2->isValid());
    QAccessibleInterface *iface3 = 0;

    QCOMPARE(iface1->navigate(QAccessible::Covers, -42, &iface3), -1);
    QVERIFY(iface3 == 0);
    QCOMPARE(iface1->navigate(QAccessible::Covers, 0, &iface3), -1);
    QVERIFY(iface3 == 0);
    QCOMPARE(iface1->navigate(QAccessible::Covers, 2, &iface3), -1);
    QVERIFY(iface3 == 0);

    for (int loop = 0; loop < 2; ++loop) {
        for (int x = 0; x < w->width(); ++x) {
            for (int y = 0; y < w->height(); ++y) {
                w1->move(x, y);
                if (w1->geometry().intersects(w2->geometry())) {
                    QVERIFY(iface1->relationTo(iface2) & QAccessible::Covers);
                    QVERIFY(iface2->relationTo(iface1) & QAccessible::Covered);
                    QCOMPARE(iface1->navigate(QAccessible::Covered, 1, &iface3), 0);
                    QVERIFY(iface3 != 0);
                    QVERIFY(iface3->isValid());
                    QCOMPARE(iface3->object(), iface2->object());
                    delete iface3; iface3 = 0;
                    QCOMPARE(iface2->navigate(QAccessible::Covers, 1, &iface3), 0);
                    QVERIFY(iface3 != 0);
                    QVERIFY(iface3->isValid());
                    QCOMPARE(iface3->object(), iface1->object());
                    delete iface3; iface3 = 0;
                } else {
                    QVERIFY(!(iface1->relationTo(iface2) & QAccessible::Covers));
                    QVERIFY(!(iface2->relationTo(iface1) & QAccessible::Covered));
                    QCOMPARE(iface1->navigate(QAccessible::Covered, 1, &iface3), -1);
                    QVERIFY(iface3 == 0);
                    QCOMPARE(iface1->navigate(QAccessible::Covers, 1, &iface3), -1);
                    QVERIFY(iface3 == 0);
                    QCOMPARE(iface2->navigate(QAccessible::Covered, 1, &iface3), -1);
                    QVERIFY(iface3 == 0);
                    QCOMPARE(iface2->navigate(QAccessible::Covers, 1, &iface3), -1);
                    QVERIFY(iface3 == 0);
                }
            }
        }
        if (!loop) {
            // switch children for second loop
            w2->raise();
            QAccessibleInterface *temp = iface1;
            iface1 = iface2;
            iface2 = temp;
        }
    }
    delete iface1; iface1 = 0;
    delete iface2; iface2 = 0;
    iface1 = QAccessible::queryAccessibleInterface(w1);
    QVERIFY(iface1 != 0);
    QVERIFY(iface1->isValid());
    iface2 = QAccessible::queryAccessibleInterface(w2);
    QVERIFY(iface2 != 0);
    QVERIFY(iface2->isValid());

    w1->move(0,0);
    w2->move(0,0);
    w1->raise();
    QVERIFY(iface1->relationTo(iface2) & QAccessible::Covers);
    QVERIFY(iface2->relationTo(iface1) & QAccessible::Covered);
    QVERIFY(!iface1->state().invisible);
    w1->hide();
    QVERIFY(iface1->state().invisible);
    QVERIFY(!(iface1->relationTo(iface2) & QAccessible::Covers));
    QVERIFY(!(iface2->relationTo(iface1) & QAccessible::Covered));
    QCOMPARE(iface2->navigate(QAccessible::Covered, 1, &iface3), -1);
    QVERIFY(iface3 == 0);
    QCOMPARE(iface1->navigate(QAccessible::Covers, 1, &iface3), -1);
    QVERIFY(iface3 == 0);

    delete iface1; iface1 = 0;
    delete iface2; iface2 = 0;
    delete w;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::navigateHierarchy()
{
    {
    QWidget *w = new QWidget(0);
    w->setObjectName(QString("Hans"));
    w->show();
    QWidget *w1 = new QWidget(w);
    w1->setObjectName(QString("1"));
    w1->show();
    QWidget *w2 = new QWidget(w);
    w2->setObjectName(QString("2"));
    w2->show();
    QWidget *w3 = new QWidget(w);
    w3->setObjectName(QString("3"));
    w3->show();
    QWidget *w31 = new QWidget(w3);
    w31->setObjectName(QString("31"));
    w31->show();

    QAccessibleInterface *target = 0;
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(w);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());

    target = iface->child(14);
    QVERIFY(target == 0);
    target = iface->child(-1);
    QVERIFY(target == 0);
    target = iface->child(0);
    QAccessibleInterface *interfaceW1 = iface->child(0);
    QVERIFY(target);
    QVERIFY(target->isValid());
    QCOMPARE(target->object(), (QObject*)w1);
    QVERIFY(interfaceW1 != 0);
    QVERIFY(interfaceW1->isValid());
    QCOMPARE(interfaceW1->object(), (QObject*)w1);
    delete interfaceW1;
    delete iface; iface = 0;

    iface = QAccessible::queryAccessibleInterface(w);
    target = iface->child(2);
    QVERIFY(target != 0);
    QVERIFY(target->isValid());
    QCOMPARE(target->object(), (QObject*)w3);
    delete iface; iface = 0;


    iface = target->child(1);
    QCOMPARE(iface, (QAccessibleInterface*)0);
    iface = target->child(0);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)w31);

    iface = QAccessible::queryAccessibleInterface(w);
    QAccessibleInterface *acc3 = iface->child(2);
    target = acc3->child(0);
    delete acc3;
    delete iface;
    QCOMPARE(target->object(), (QObject*)w31);

    iface = target->parent();
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)w3);
    delete iface; iface = 0;
    delete target; target = 0;

    delete w;
    }
    QTestAccessibility::clearEvents();
}

#define QSETCOMPARE(thetypename, elements, otherelements) \
    QCOMPARE((QSet<thetypename>() << elements), (QSet<thetypename>() << otherelements))

static QWidget *createWidgets()
{
    QWidget *w = new QWidget();

    QHBoxLayout *box = new QHBoxLayout(w);

    int i = 0;
    box->addWidget(new QComboBox(w));
    box->addWidget(new QPushButton(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QHeaderView(Qt::Vertical, w));
    box->addWidget(new QTreeView(w));
    box->addWidget(new QTreeWidget(w));
    box->addWidget(new QListView(w));
    box->addWidget(new QListWidget(w));
    box->addWidget(new QTableView(w));
    box->addWidget(new QTableWidget(w));
    box->addWidget(new QCalendarWidget(w));
    box->addWidget(new QDialogButtonBox(w));
    box->addWidget(new QGroupBox(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QFrame(w));
    box->addWidget(new QLineEdit(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QProgressBar(w));
    box->addWidget(new QTabWidget(w));
    box->addWidget(new QCheckBox(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QRadioButton(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QDial(w));
    box->addWidget(new QScrollBar(w));
    box->addWidget(new QSlider(w));
    box->addWidget(new QDateTimeEdit(w));
    box->addWidget(new QDoubleSpinBox(w));
    box->addWidget(new QSpinBox(w));
    box->addWidget(new QLabel(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QLCDNumber(w));
    box->addWidget(new QStackedWidget(w));
    box->addWidget(new QToolBox(w));
    box->addWidget(new QLabel(QString::fromAscii("widget text %1").arg(i++), w));
    box->addWidget(new QTextEdit(QString::fromAscii("widget text %1").arg(i++), w));

    /* Not in the list
     * QAbstractItemView, QGraphicsView, QScrollArea,
     * QToolButton, QDockWidget, QFocusFrame, QMainWindow, QMenu, QMenuBar, QSizeGrip, QSplashScreen, QSplitterHandle,
     * QStatusBar, QSvgWidget, QTabBar, QToolBar, QWorkspace, QSplitter
     */
    return w;
}

void tst_QAccessibility::accessibleName()
{
    QWidget *toplevel = createWidgets();
    toplevel->show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif
    QLayout *lout = toplevel->layout();
    for (int i = 0; i < lout->count(); i++) {
        QLayoutItem *item = lout->itemAt(i);
        QWidget *child = item->widget();

        QString name = tr("Widget Name %1").arg(i);
        child->setAccessibleName(name);
        QAccessibleInterface *acc = QAccessible::queryAccessibleInterface(child);
        QCOMPARE(acc->text(QAccessible::Name), name);

        QString desc = tr("Widget Description %1").arg(i);
        child->setAccessibleDescription(desc);
        QCOMPARE(acc->text(QAccessible::Description), desc);

    }

    delete toplevel;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::textAttributes()
{
    QTextEdit textEdit;
    int startOffset;
    int endOffset;
    QString attributes;
    QString text("<html><head></head><body>"
                 "Hello, <b>this</b> is an <i><b>example</b> text</i>."
                 "<span style=\"font-family: monospace\">Multiple fonts are used.</span>"
                 "Multiple <span style=\"font-size: 8pt\">text sizes</span> are used."
                 "Let's give some color to <span style=\"color:#f0f1f2; background-color:#14f01e\">Qt</span>."
                 "</body></html>");

    textEdit.setText(text);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&textEdit);

    QAccessibleTextInterface *textInterface=interface->textInterface();

    QVERIFY(textInterface);
    QCOMPARE(textInterface->characterCount(), 112);

    attributes = textInterface->attributes(10, &startOffset, &endOffset);
    QCOMPARE(startOffset, 7);
    QCOMPARE(endOffset, 11);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-weight:bold;")));

    attributes = textInterface->attributes(18, &startOffset, &endOffset);
    QCOMPARE(startOffset, 18);
    QCOMPARE(endOffset, 25);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-weight:bold;")));
    QVERIFY(attributes.contains(QLatin1String(";font-style:italic;")));

    attributes = textInterface->attributes(34, &startOffset, &endOffset);
    QCOMPARE(startOffset, 31);
    QCOMPARE(endOffset, 55);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-family:\"monospace\";")));

    attributes = textInterface->attributes(65, &startOffset, &endOffset);
    QCOMPARE(startOffset, 64);
    QCOMPARE(endOffset, 74);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-size:8pt;")));

    attributes = textInterface->attributes(110, &startOffset, &endOffset);
    QCOMPARE(startOffset, 109);
    QCOMPARE(endOffset, 111);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";background-color:rgb(20,240,30);")));
    QVERIFY(attributes.contains(QLatin1String(";color:rgb(240,241,242);")));
}

void tst_QAccessibility::hideShowTest()
{
    QWidget * const window = new QWidget();
    QWidget * const child = new QWidget(window);

    QVERIFY(state(window).invisible);
    QVERIFY(state(child).invisible);

    QTestAccessibility::clearEvents();

    // show() and veryfy that both window and child are not invisible and get ObjectShow events.
    window->show();
    QVERIFY(!state(window).invisible);
    QVERIFY(!state(child).invisible);
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(window, 0, QAccessible::ObjectShow)));
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(child, 0, QAccessible::ObjectShow)));
    QTestAccessibility::clearEvents();

    // hide() and veryfy that both window and child are invisible and get ObjectHide events.
    window->hide();
    QVERIFY(state(window).invisible);
    QVERIFY(state(child).invisible);
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(window, 0, QAccessible::ObjectHide)));
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(child, 0, QAccessible::ObjectHide)));
    QTestAccessibility::clearEvents();

    delete window;
    QTestAccessibility::clearEvents();
}


void tst_QAccessibility::actionTest()
{
    {
    QCOMPARE(QAccessibleActionInterface::pressAction(), QString(QStringLiteral("Press")));

    QWidget *widget = new QWidget;
    widget->setFocusPolicy(Qt::NoFocus);
    widget->show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(interface);
    QVERIFY(interface->isValid());
    QAccessibleActionInterface *actions = interface->actionInterface();
    QVERIFY(actions);

    // no actions by default, except when focusable
    QCOMPARE(actions->actionNames(), QStringList());
    widget->setFocusPolicy(Qt::StrongFocus);
    QCOMPARE(actions->actionNames(), QStringList(QAccessibleActionInterface::setFocusAction()));

    delete interface;
    delete widget;
    }
    QTestAccessibility::clearEvents();

    {
    QPushButton *button = new QPushButton;
    button->show();
    button->clearFocus();
    QCOMPARE(button->hasFocus(), false);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(button);
    QAccessibleActionInterface *actions = interface->actionInterface();
    QVERIFY(actions);

    // Make sure the "primary action" press comes first!
    QCOMPARE(actions->actionNames(), QStringList() << QAccessibleActionInterface::pressAction() << QAccessibleActionInterface::setFocusAction());

    actions->doAction(QAccessibleActionInterface::setFocusAction());
    QTest::qWait(500);
    QCOMPARE(button->hasFocus(), true);

    connect(button, SIGNAL(clicked()), this, SLOT(onClicked()));
    QCOMPARE(click_count, 0);
    actions->doAction(QAccessibleActionInterface::pressAction());
    QTest::qWait(500);
    QCOMPARE(click_count, 1);

    delete interface;
    delete button;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::applicationTest()
{
    QLatin1String name = QLatin1String("My Name");
    qApp->setApplicationName(name);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(qApp);
    QCOMPARE(interface->text(QAccessible::Name), name);
    QCOMPARE(interface->role(), QAccessible::Application);
    delete interface;
}

void tst_QAccessibility::mainWindowTest()
{
    QMainWindow *mw = new QMainWindow;
    mw->resize(300, 200);
    mw->show(); // triggers layout

    QLatin1String name = QLatin1String("I am the main window");
    mw->setWindowTitle(name);
    QTest::qWaitForWindowShown(mw);
    QVERIFY_EVENT(mw, 0, QAccessible::ObjectShow);

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(mw);
    QCOMPARE(interface->text(QAccessible::Name), name);
    QCOMPARE(interface->role(), QAccessible::Window);
    delete interface;
    delete mw;
    QTestAccessibility::clearEvents();
}

class CounterButton : public QPushButton {
    Q_OBJECT
public:
    CounterButton(const QString& name, QWidget* parent)
        : QPushButton(name, parent), clickCount(0)
    {
        connect(this, SIGNAL(clicked(bool)), SLOT(incClickCount()));
    }
    int clickCount;
public Q_SLOTS:
    void incClickCount() {
        ++clickCount;
    }
};

void tst_QAccessibility::buttonTest()
{
    QWidget window;
    window.setLayout(new QVBoxLayout);

    // Standard push button
    CounterButton pushButton("Ok", &window);

    // toggle button
    QPushButton toggleButton("Toggle", &window);
    toggleButton.setCheckable(true);

    // standard checkbox
    QCheckBox checkBox("Check me!", &window);

    // tristate checkbox
    QCheckBox tristate("Tristate!", &window);
    tristate.setTristate(true);

    // radiobutton
    QRadioButton radio("Radio me!", &window);

    // standard toolbutton
    QToolButton toolbutton(&window);
    toolbutton.setText("Tool");
    toolbutton.setMinimumSize(20,20);

    // standard toolbutton
    QToolButton toggletool(&window);
    toggletool.setCheckable(true);
    toggletool.setText("Toggle");
    toggletool.setMinimumSize(20,20);

    // test push button
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(&pushButton);
    QAccessibleActionInterface* actionInterface = interface->actionInterface();
    QVERIFY(actionInterface != 0);
    QCOMPARE(interface->role(), QAccessible::PushButton);

    // buttons only have a click action
    QCOMPARE(actionInterface->actionNames().size(), 2);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::pressAction() << QAccessibleActionInterface::setFocusAction());
    QCOMPARE(pushButton.clickCount, 0);
    actionInterface->doAction(QAccessibleActionInterface::pressAction());
    QTest::qWait(500);
    QCOMPARE(pushButton.clickCount, 1);
    delete interface;

    // test toggle button
    interface = QAccessible::queryAccessibleInterface(&toggleButton);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::CheckBox);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::checkAction() << QAccessibleActionInterface::setFocusAction());
    QCOMPARE(actionInterface->localizedActionDescription(QAccessibleActionInterface::checkAction()), QString("Checks the checkbox"));
    QVERIFY(!toggleButton.isChecked());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::checkAction());
    QTest::qWait(500);
    QVERIFY(toggleButton.isChecked());
    QCOMPARE(actionInterface->actionNames().at(0), QAccessibleActionInterface::uncheckAction());
    QVERIFY(interface->state().checked);
    delete interface;

    {
    // test menu push button
    QAction *foo = new QAction("Foo", 0);
    foo->setShortcut(QKeySequence("Ctrl+F"));
    QMenu *menu = new QMenu();
    menu->addAction(foo);
    QPushButton menuButton;
    menuButton.setMenu(menu);
    menuButton.show();
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&menuButton);
    QCOMPARE(interface->role(), QAccessible::ButtonMenu);
    QVERIFY(interface->state().hasPopup);
    QCOMPARE(interface->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction() << QAccessibleActionInterface::setFocusAction());
    // showing the menu enters a new event loop...
//    interface->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
//    QTest::qWait(500);
    delete interface;
    delete menu;
    }

    // test check box
    interface = QAccessible::queryAccessibleInterface(&checkBox);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::CheckBox);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::checkAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::checkAction());
    QTest::qWait(500);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::uncheckAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(interface->state().checked);
    QVERIFY(checkBox.isChecked());
    delete interface;

    // test radiobutton
    interface = QAccessible::queryAccessibleInterface(&radio);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::RadioButton);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::checkAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::checkAction());
    QTest::qWait(500);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::checkAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(interface->state().checked);
    QVERIFY(checkBox.isChecked());
    delete interface;

//    // test standard toolbutton
//    QVERIFY(QAccessible::queryAccessibleInterface(&toolbutton, &test));
//    QCOMPARE(test->role(), QAccessible::PushButton);
//    QCOMPARE(test->defaultAction(0), QAccessible::Press);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Press"));
//    QCOMPARE(test->state(), (int)QAccessible::Normal);
//    test->release();

//    // toggle tool button
//    QVERIFY(QAccessible::queryAccessibleInterface(&toggletool, &test));
//    QCOMPARE(test->role(), QAccessible::CheckBox);
//    QCOMPARE(test->defaultAction(0), QAccessible::Press);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Check"));
//    QCOMPARE(test->state(), (int)QAccessible::Normal);
//    QVERIFY(test->doAction(QAccessible::Press, 0));
//    QTest::qWait(500);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Uncheck"));
//    QCOMPARE(test->state(), (int)QAccessible::Checked);
//    test->release();

//    // test menu toolbutton
//    QVERIFY(QAccessible::queryAccessibleInterface(&menuToolButton, &test));
//    QCOMPARE(test->role(), QAccessible::ButtonMenu);
//    QCOMPARE(test->defaultAction(0), 1);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Open"));
//    QCOMPARE(test->state(), (int)QAccessible::HasPopup);
//    QCOMPARE(test->actionCount(0), 1);
//    QCOMPARE(test->actionText(QAccessible::Press, QAccessible::Name, 0), QString("Press"));
//    test->release();

//    // test split menu toolbutton
//    QVERIFY(QAccessible::queryAccessibleInterface(&splitToolButton, &test));
//    QCOMPARE(test->childCount(), 2);
//    QCOMPARE(test->role(), QAccessible::ButtonDropDown);
//    QCOMPARE(test->role(1), QAccessible::PushButton);
//    QCOMPARE(test->role(2), QAccessible::ButtonMenu);
//    QCOMPARE(test->defaultAction(0), QAccessible::Press);
//    QCOMPARE(test->defaultAction(1), QAccessible::Press);
//    QCOMPARE(test->defaultAction(2), QAccessible::Press);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Press"));
//    QCOMPARE(test->state(), (int)QAccessible::HasPopup);
//    QCOMPARE(test->actionCount(0), 1);
//    QCOMPARE(test->actionText(1, QAccessible::Name, 0), QString("Open"));
//    QCOMPARE(test->actionText(test->defaultAction(1), QAccessible::Name, 1), QString("Press"));
//    QCOMPARE(test->state(1), (int)QAccessible::Normal);
//    QCOMPARE(test->actionText(test->defaultAction(2), QAccessible::Name, 2), QString("Open"));
//    QCOMPARE(test->state(2), (int)QAccessible::HasPopup);
//    test->release();

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::scrollBarTest()
{
    QScrollBar *scrollBar  = new QScrollBar(Qt::Horizontal);
    QAccessibleInterface * const scrollBarInterface = QAccessible::queryAccessibleInterface(scrollBar);
    QVERIFY(scrollBarInterface);
    QVERIFY(scrollBarInterface->state().invisible);
    scrollBar->resize(200, 50);
    scrollBar->show();
    QVERIFY(!scrollBarInterface->state().invisible);
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(scrollBar, 0, QAccessible::ObjectShow)));
    QTestAccessibility::clearEvents();

    scrollBar->hide();
    QVERIFY(scrollBarInterface->state().invisible);
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(scrollBar, 0, QAccessible::ObjectHide)));
    QTestAccessibility::clearEvents();

    // Test that the left/right subcontrols are set to unavailable when the scrollBar is at the minimum/maximum.
    scrollBar->show();
    scrollBar->setMinimum(11);
    scrollBar->setMaximum(111);

    QAccessibleValueInterface *valueIface = scrollBarInterface->valueInterface();
    QVERIFY(valueIface != 0);
    QCOMPARE(valueIface->minimumValue().toInt(), scrollBar->minimum());
    QCOMPARE(valueIface->maximumValue().toInt(), scrollBar->maximum());
    scrollBar->setValue(50);
    QCOMPARE(valueIface->currentValue().toInt(), scrollBar->value());
    scrollBar->setValue(0);
    QCOMPARE(valueIface->currentValue().toInt(), scrollBar->value());
    scrollBar->setValue(100);
    QCOMPARE(valueIface->currentValue().toInt(), scrollBar->value());
    valueIface->setCurrentValue(77);
    QCOMPARE(77, scrollBar->value());

    const QRect scrollBarRect = scrollBarInterface->rect();
    QVERIFY(scrollBarRect.isValid());

    delete scrollBarInterface;
    delete scrollBar;

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tabTest()
{
    QTabBar *tabBar = new QTabBar();
    tabBar->show();

    QAccessibleInterface * const interface = QAccessible::queryAccessibleInterface(tabBar);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 2);

    // Test that the Invisible bit for the navigation buttons gets set
    // and cleared correctly.
    QAccessibleInterface *leftButton = interface->child(0);
    QCOMPARE(leftButton->role(), QAccessible::PushButton);
    QVERIFY(leftButton->state().invisible);
    delete leftButton;

    const int lots = 5;
    for (int i = 0; i < lots; ++i)
        tabBar->addTab("Foo");

    QAccessibleInterface *child1 = interface->child(0);
    QAccessibleInterface *child2 = interface->child(1);
    QVERIFY(child1);
    QCOMPARE(child1->role(), QAccessible::PageTab);
    QVERIFY(child2);
    QCOMPARE(child2->role(), QAccessible::PageTab);

    QVERIFY((child1->state().invisible) == false);
    tabBar->hide();

    QCoreApplication::processEvents();
    QTest::qWait(100);

    QVERIFY(child1->state().invisible);

    tabBar->show();
    tabBar->setCurrentIndex(0);

    // Test that sending a focus action to a tab does not select it.
//    child2->doAction(QAccessible::Focus, 2, QVariantList());
    QCOMPARE(tabBar->currentIndex(), 0);

    // Test that sending a press action to a tab selects it.
    QVERIFY(child2->actionInterface());
    QCOMPARE(child2->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());
    QCOMPARE(tabBar->currentIndex(), 0);
    child2->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
    QCOMPARE(tabBar->currentIndex(), 1);

    delete tabBar;
    delete interface;
    delete child1;
    delete child2;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tabWidgetTest()
{
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->show();

    // the interface for the tab is just a container for tabbar and stacked widget
    QAccessibleInterface * const interface = QAccessible::queryAccessibleInterface(tabWidget);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 2);
    QCOMPARE(interface->role(), QAccessible::Client);

    // Create pages, check navigation
    QLabel *label1 = new QLabel("Page 1", tabWidget);
    tabWidget->addTab(label1, "Tab 1");
    QLabel *label2 = new QLabel("Page 2", tabWidget);
    tabWidget->addTab(label2, "Tab 2");

    QCOMPARE(interface->childCount(), 2);

    QAccessibleInterface* tabBarInterface = 0;
    // there is no special logic to sort the children, so the contents will be 1, the tab bar 2
    tabBarInterface = interface->child(1);
    QVERIFY(tabBarInterface);
    QCOMPARE(tabBarInterface->childCount(), 4);
    QCOMPARE(tabBarInterface->role(), QAccessible::PageTabList);

    QAccessibleInterface* tabButton1Interface = tabBarInterface->child(0);
    QVERIFY(tabButton1Interface);
    QCOMPARE(tabButton1Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton1Interface->text(QAccessible::Name), QLatin1String("Tab 1"));

    QAccessibleInterface* tabButton2Interface = tabBarInterface->child(1);
    QVERIFY(tabButton1Interface);
    QCOMPARE(tabButton2Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton2Interface->text(QAccessible::Name), QLatin1String("Tab 2"));

    QAccessibleInterface* tabButtonLeft = tabBarInterface->child(2);
    QVERIFY(tabButtonLeft);
    QCOMPARE(tabButtonLeft->role(), QAccessible::PushButton);
    QCOMPARE(tabButtonLeft->text(QAccessible::Name), QLatin1String("Scroll Left"));

    QAccessibleInterface* tabButtonRight = tabBarInterface->child(3);
    QVERIFY(tabButtonRight);
    QCOMPARE(tabButtonRight->role(), QAccessible::PushButton);
    QCOMPARE(tabButtonRight->text(QAccessible::Name), QLatin1String("Scroll Right"));
    delete tabButton1Interface;
    delete tabButton2Interface;
    delete tabButtonLeft;
    delete tabButtonRight;

    QAccessibleInterface* stackWidgetInterface = interface->child(0);
    QVERIFY(stackWidgetInterface);
    QCOMPARE(stackWidgetInterface->childCount(), 2);
    QCOMPARE(stackWidgetInterface->role(), QAccessible::LayeredPane);

    QAccessibleInterface* stackChild1Interface = stackWidgetInterface->child(0);
    QVERIFY(stackChild1Interface);
#ifndef Q_CC_INTEL
    QCOMPARE(stackChild1Interface->childCount(), 0);
#endif
    QCOMPARE(stackChild1Interface->role(), QAccessible::StaticText);
    QCOMPARE(stackChild1Interface->text(QAccessible::Name), QLatin1String("Page 1"));
    QCOMPARE(label1, stackChild1Interface->object());

    // Navigation in stack widgets should be consistent
    QAccessibleInterface* parent = stackChild1Interface->parent();
    QVERIFY(parent);
#ifndef Q_CC_INTEL
    QCOMPARE(parent->childCount(), 2);
#endif
    QCOMPARE(parent->role(), QAccessible::LayeredPane);
    delete parent;

    QAccessibleInterface* stackChild2Interface = stackWidgetInterface->child(1);
    QVERIFY(stackChild2Interface);
    QCOMPARE(stackChild2Interface->childCount(), 0);
    QCOMPARE(stackChild2Interface->role(), QAccessible::StaticText);
    QCOMPARE(label2, stackChild2Interface->object());
    QCOMPARE(label2->text(), stackChild2Interface->text(QAccessible::Name));

    parent = stackChild2Interface->parent();
    QVERIFY(parent);
#ifndef Q_CC_INTEL
    QCOMPARE(parent->childCount(), 2);
#endif
    QCOMPARE(parent->role(), QAccessible::LayeredPane);
    delete parent;

    delete tabBarInterface;
    delete stackChild1Interface;
    delete stackChild2Interface;
    delete stackWidgetInterface;
    delete interface;
    delete tabWidget;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::menuTest()
{
    {
    QMainWindow mw;
    mw.resize(300, 200);
    QMenu *file = mw.menuBar()->addMenu("&File");
    QMenu *fileNew = file->addMenu("&New...");
    fileNew->menuAction()->setShortcut(tr("Ctrl+N"));
    fileNew->addAction("Text file");
    fileNew->addAction("Image file");
    file->addAction("&Open")->setShortcut(tr("Ctrl+O"));
    file->addAction("&Save")->setShortcut(tr("Ctrl+S"));
    file->addSeparator();
    file->addAction("E&xit")->setShortcut(tr("Alt+F4"));

    QMenu *edit = mw.menuBar()->addMenu("&Edit");
    edit->addAction("&Undo")->setShortcut(tr("Ctrl+Z"));
    edit->addAction("&Redo")->setShortcut(tr("Ctrl+Y"));
    edit->addSeparator();
    edit->addAction("Cu&t")->setShortcut(tr("Ctrl+X"));
    edit->addAction("&Copy")->setShortcut(tr("Ctrl+C"));
    edit->addAction("&Paste")->setShortcut(tr("Ctrl+V"));
    edit->addAction("&Delete")->setShortcut(tr("Del"));
    edit->addSeparator();
    edit->addAction("Pr&operties");

    mw.menuBar()->addSeparator();

    QMenu *help = mw.menuBar()->addMenu("&Help");
    help->addAction("&Contents");
    help->addAction("&About");

    mw.menuBar()->addAction("Action!");

    mw.show(); // triggers layout
    QTest::qWait(100);

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(mw.menuBar());
    QCOMPARE(verifyHierarchy(interface),  0);

    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 5);
    QCOMPARE(interface->role(), QAccessible::MenuBar);

    QAccessibleInterface *iFile = interface->child(0);
    QAccessibleInterface *iEdit = interface->child(1);
    QAccessibleInterface *iSeparator = interface->child(2);
    QAccessibleInterface *iHelp = interface->child(3);
    QAccessibleInterface *iAction = interface->child(4);

    QCOMPARE(iFile->role(), QAccessible::MenuItem);
    QCOMPARE(iEdit->role(), QAccessible::MenuItem);
    QCOMPARE(iSeparator->role(), QAccessible::Separator);
    QCOMPARE(iHelp->role(), QAccessible::MenuItem);
    QCOMPARE(iAction->role(), QAccessible::MenuItem);
#ifndef Q_WS_MAC
#ifdef Q_OS_WINCE
    if (!IsValidCEPlatform())
        QSKIP("Tests do not work on Mobile platforms due to native menus");
#endif
    QCOMPARE(mw.mapFromGlobal(interface->rect().topLeft()), mw.menuBar()->geometry().topLeft());
    QCOMPARE(interface->rect().size(), mw.menuBar()->size());

    QVERIFY(interface->rect().contains(iFile->rect()));
    QVERIFY(interface->rect().contains(iEdit->rect()));
    // QVERIFY(interface->rect().contains(childSeparator->rect())); //separator might be invisible
    QVERIFY(interface->rect().contains(iHelp->rect()));
    QVERIFY(interface->rect().contains(iAction->rect()));
#endif

    QCOMPARE(iFile->text(QAccessible::Name), QString("File"));
    QCOMPARE(iEdit->text(QAccessible::Name), QString("Edit"));
    QCOMPARE(iSeparator->text(QAccessible::Name), QString());
    QCOMPARE(iHelp->text(QAccessible::Name), QString("Help"));
    QCOMPARE(iAction->text(QAccessible::Name), QString("Action!"));

// TODO: Currently not working, task to fix is #100019.
#ifndef Q_OS_MAC
    QCOMPARE(iFile->text(QAccessible::Accelerator), tr("Alt+F"));
    QCOMPARE(iEdit->text(QAccessible::Accelerator), tr("Alt+E"));
    QCOMPARE(iSeparator->text(QAccessible::Accelerator), QString());
    QCOMPARE(iHelp->text(QAccessible::Accelerator), tr("Alt+H"));
    QCOMPARE(iAction->text(QAccessible::Accelerator), QString());
#endif

    QVERIFY(iFile->actionInterface());

    QCOMPARE(iFile->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    QCOMPARE(iSeparator->actionInterface()->actionNames(), QStringList());
    QCOMPARE(iHelp->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    QCOMPARE(iAction->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());

    bool menuFade = qApp->isEffectEnabled(Qt::UI_FadeMenu);
    int menuFadeDelay = 300;
    iFile->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(file->isVisible() && !edit->isVisible() && !help->isVisible());
    iEdit->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(!file->isVisible() && edit->isVisible() && !help->isVisible());
    iHelp->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(!file->isVisible() && !edit->isVisible() && help->isVisible());
    iAction->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(!file->isVisible() && !edit->isVisible() && !help->isVisible());

    QVERIFY(interface->actionInterface());
    QCOMPARE(interface->actionInterface()->actionNames(), QStringList());
    delete interface;
    interface = QAccessible::queryAccessibleInterface(file);
    QCOMPARE(interface->childCount(), 5);
    QCOMPARE(interface->role(), QAccessible::PopupMenu);

    QAccessibleInterface *iFileNew = interface->child(0);
    QAccessibleInterface *iFileOpen = interface->child(1);
    QAccessibleInterface *iFileSave = interface->child(2);
    QAccessibleInterface *iFileSeparator = interface->child(3);
    QAccessibleInterface *iFileExit = interface->child(4);

    QCOMPARE(iFileNew->role(), QAccessible::MenuItem);
    QCOMPARE(iFileOpen->role(), QAccessible::MenuItem);
    QCOMPARE(iFileSave->role(), QAccessible::MenuItem);
    QCOMPARE(iFileSeparator->role(), QAccessible::Separator);
    QCOMPARE(iFileExit->role(), QAccessible::MenuItem);
    QCOMPARE(iFileNew->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    QCOMPARE(iFileOpen->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());
    QCOMPARE(iFileSave->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());
    QCOMPARE(iFileSeparator->actionInterface()->actionNames(), QStringList());
    QCOMPARE(iFileExit->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());

    QAccessibleInterface *iface = 0;
    QAccessibleInterface *iface2 = 0;

    // traverse siblings with navigate(Sibling, ...)
    int entry;
    iface = interface->child(0);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    QAccessible::Role fileRoles[5] = {
        QAccessible::MenuItem,
        QAccessible::MenuItem,
        QAccessible::MenuItem,
        QAccessible::Separator,
        QAccessible::MenuItem
    };
    for (int child = 0; child < 5; ++child) {
        iface2 = interface->child(child);
        QVERIFY(iface2);
        QCOMPARE(iface2->role(), fileRoles[child]);
        delete iface2;
    }
    delete iface;

    // traverse menu items with navigate(Down, ...)
    iface = interface->child(0);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    for (int child = 0; child < 4; ++child) {
        entry = iface->navigate(QAccessible::Down, 1, &iface2);
        delete iface;
        iface = iface2;
        QCOMPARE(entry, 0);
        QVERIFY(iface);
        QCOMPARE(iface->role(), fileRoles[child + 1]);
    }
    delete iface;

    // traverse menu items with navigate(Up, ...)
    iface = interface->child(interface->childCount() - 1);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    for (int child = 3; child >= 0; --child) {
        entry = iface->navigate(QAccessible::Up, 1, &iface2);
        delete iface;
        iface = iface2;
        QCOMPARE(entry, 0);
        QVERIFY(iface);
        QCOMPARE(iface->role(), fileRoles[child]);
    }
    delete iface;

    // "New" item
    iface = interface->child(0);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    // "New" menu
    iface2 = iface->child(0);
    delete iface;
    iface = iface2;
    QCOMPARE(entry, 0);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::PopupMenu);

    // "Text file" menu item
    iface2 = iface->child(0);
    delete iface;
    iface = iface2;
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    delete iface;

    // move mouse pointer away, since that might influence the
    // subsequent tests
    QTest::mouseMove(&mw, QPoint(-1, -1));
    QTest::qWait(100);
    if (menuFade)
        QTest::qWait(menuFadeDelay);

    iFile->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    iFileNew->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());

    QVERIFY(file->isVisible());
    QVERIFY(fileNew->isVisible());
    QVERIFY(!edit->isVisible());
    QVERIFY(!help->isVisible());

    QTestAccessibility::clearEvents();
    mw.hide();

    delete iFile;
    delete iFileNew;
    delete iFileOpen;
    delete iFileSave;
    delete iFileSeparator;
    delete iFileExit;

    // Do not crash if the menu don't have a parent
    QMenu *menu = new QMenu;
    menu->addAction(QLatin1String("one"));
    menu->addAction(QLatin1String("two"));
    menu->addAction(QLatin1String("three"));
    iface = QAccessible::queryAccessibleInterface(menu);
    iface2 = iface->parent();
    QVERIFY(iface2);
    QCOMPARE(iface2->role(), QAccessible::Application);
    // caused a *crash*
    iface2->state();
    delete iface2;
    delete iface;
    delete menu;

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::spinBoxTest()
{
    QSpinBox * const spinBox = new QSpinBox();
    spinBox->setValue(3);
    spinBox->show();

    QAccessibleInterface * const interface = QAccessible::queryAccessibleInterface(spinBox);
    QVERIFY(interface);
    QCOMPARE(interface->role(), QAccessible::SpinBox);

    const QRect widgetRect = spinBox->geometry();
    const QRect accessibleRect = interface->rect();
    QCOMPARE(accessibleRect, widgetRect);
    QCOMPARE(interface->text(QAccessible::Value), QLatin1String("3"));

    // one child, the line edit
    const int numChildren = interface->childCount();
    QCOMPARE(numChildren, 1);
    QAccessibleInterface *lineEdit = interface->child(0);

    QCOMPARE(lineEdit->role(), QAccessible::EditableText);
    QCOMPARE(lineEdit->text(QAccessible::Value), QLatin1String("3"));
    delete lineEdit;

    QVERIFY(interface->valueInterface());
    QCOMPARE(interface->valueInterface()->currentValue().toInt(), 3);
    interface->valueInterface()->setCurrentValue(23);
    QCOMPARE(interface->valueInterface()->currentValue().toInt(), 23);
    QCOMPARE(spinBox->value(), 23);

    spinBox->setFocus();
    QTestAccessibility::clearEvents();
    QTest::keyPress(spinBox, Qt::Key_Up);
    QTest::qWait(200);
    EventList events = QTestAccessibility::events();
    QTestAccessibilityEvent expectedEvent(spinBox, 0, (int)QAccessible::ValueChanged);
    QVERIFY(events.contains(expectedEvent));
    delete spinBox;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::doubleSpinBoxTest()
{
    QDoubleSpinBox *doubleSpinBox = new QDoubleSpinBox;
    doubleSpinBox->show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(doubleSpinBox);
    QVERIFY(interface);

    const QRect widgetRect = doubleSpinBox->geometry();
    const QRect accessibleRect = interface->rect();
    QCOMPARE(accessibleRect, widgetRect);

    // Test that we get valid rects for all the spinbox child interfaces.
    const int numChildren = interface->childCount();
    for (int i = 0; i < numChildren; ++i) {
        QAccessibleInterface *childIface = interface->child(i);
        const QRect childRect = childIface->rect();
        QVERIFY(childRect.isValid());
        delete childIface;
    }

    delete doubleSpinBox;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::textEditTest()
{
    {
    QTextEdit edit;
    int startOffset;
    int endOffset;
    QString text = "hello world\nhow are you today?\n";
    edit.setText(text);
    edit.show();

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&edit);
    QCOMPARE(iface->text(QAccessible::Value), text);
    QCOMPARE(iface->textInterface()->textAtOffset(8, QAccessible2::WordBoundary, &startOffset, &endOffset), QString("world"));
    QCOMPARE(startOffset, 6);
    QCOMPARE(endOffset, 11);
    QCOMPARE(iface->textInterface()->textAtOffset(14, QAccessible2::LineBoundary, &startOffset, &endOffset), QString("how are you today?"));
    QCOMPARE(startOffset, 12);
    QCOMPARE(endOffset, 30);
    QCOMPARE(iface->textInterface()->characterCount(), 31);
    QFontMetrics fm(edit.font());
    QCOMPARE(iface->textInterface()->characterRect(0, QAccessible2::RelativeToParent).size(), QSize(fm.width("h"), fm.height()));
    QCOMPARE(iface->textInterface()->characterRect(5, QAccessible2::RelativeToParent).size(), QSize(fm.width(" "), fm.height()));
    QCOMPARE(iface->textInterface()->characterRect(6, QAccessible2::RelativeToParent).size(), QSize(fm.width("w"), fm.height()));
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::textBrowserTest()
{
    {
    QTextBrowser textBrowser;
    QString text = QLatin1String("Hello world\nhow are you today?\n");
    textBrowser.setText(text);
    textBrowser.show();

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&textBrowser);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::StaticText);
    QCOMPARE(iface->text(QAccessible::Value), text);
    int startOffset;
    int endOffset;
    QCOMPARE(iface->textInterface()->textAtOffset(8, QAccessible2::WordBoundary, &startOffset, &endOffset), QString("world"));
    QCOMPARE(startOffset, 6);
    QCOMPARE(endOffset, 11);
    QCOMPARE(iface->textInterface()->textAtOffset(14, QAccessible2::LineBoundary, &startOffset, &endOffset), QString("how are you today?"));
    QCOMPARE(startOffset, 12);
    QCOMPARE(endOffset, 30);
    QCOMPARE(iface->textInterface()->characterCount(), 31);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::mdiAreaTest()
{
    {
    QMdiArea mdiArea;
    mdiArea.resize(400,300);
    mdiArea.show();
    const int subWindowCount = 3;
    for (int i = 0; i < subWindowCount; ++i)
        mdiArea.addSubWindow(new QWidget, Qt::Dialog)->show();

    QList<QMdiSubWindow *> subWindows = mdiArea.subWindowList();
    QCOMPARE(subWindows.count(), subWindowCount);

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&mdiArea);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), subWindowCount);

    // Right, right, right, ...
    for (int i = 0; i < subWindowCount; ++i) {
        QAccessibleInterface *destination = 0;
        int index = interface->navigate(QAccessible::Right, i + 1, &destination);
        if (i == subWindowCount - 1) {
            QVERIFY(!destination);
            QCOMPARE(index, -1);
        } else {
            QVERIFY(destination);
            QCOMPARE(index, 0);
            QCOMPARE(destination->object(), (QObject*)subWindows.at(i + 1));
            delete destination;
        }
    }

    // Left, left, left, ...
    for (int i = subWindowCount; i > 0; --i) {
        QAccessibleInterface *destination = 0;
        int index = interface->navigate(QAccessible::Left, i, &destination);
        if (i == 1) {
            QVERIFY(!destination);
            QCOMPARE(index, -1);
        } else {
            QVERIFY(destination);
            QCOMPARE(index, 0);
            QCOMPARE(destination->object(), (QObject*)subWindows.at(i - 2));
            delete destination;
        }
    }
    // ### Add test for Up and Down.

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::mdiSubWindowTest()
{
    {
    QMdiArea mdiArea;
    mdiArea.show();
    qApp->setActiveWindow(&mdiArea);
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(150);
#endif

    bool isSubWindowsPlacedNextToEachOther = false;
    const int subWindowCount =  5;
    for (int i = 0; i < subWindowCount; ++i) {
        QMdiSubWindow *window = mdiArea.addSubWindow(new QPushButton("QAccessibilityTest"));
        window->show();
        // Parts of this test requires that the sub windows are placed next
        // to each other. In order to achieve that QMdiArea must have
        // a width which is larger than subWindow->width() * subWindowCount.
        if (i == 0) {
            int minimumWidth = window->width() * subWindowCount + 20;
            mdiArea.resize(mdiArea.size().expandedTo(QSize(minimumWidth, 0)));
#if defined(Q_OS_UNIX)
            QCoreApplication::processEvents();
            QTest::qWait(100);
#endif
            if (mdiArea.width() >= minimumWidth)
                isSubWindowsPlacedNextToEachOther = true;
        }
    }

    QList<QMdiSubWindow *> subWindows = mdiArea.subWindowList();
    QCOMPARE(subWindows.count(), subWindowCount);

    QMdiSubWindow *testWindow = subWindows.at(3);
    QVERIFY(testWindow);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(testWindow);

    // childCount
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 1);

    // setText / text
    QCOMPARE(interface->text(QAccessible::Name), QString());
    testWindow->setWindowTitle(QLatin1String("ReplaceMe"));
    QCOMPARE(interface->text(QAccessible::Name), QLatin1String("ReplaceMe"));
    interface->setText(QAccessible::Name, QLatin1String("TitleSetOnWindow"));
    QCOMPARE(interface->text(QAccessible::Name), QLatin1String("TitleSetOnWindow"));

    mdiArea.setActiveSubWindow(testWindow);

    // state
    QAccessible::State state;
    state.focusable = true;
    state.focused = true;
    state.movable = true;
    state.sizeable = true;

    QCOMPARE(interface->state(), state);
    const QRect originalGeometry = testWindow->geometry();
    testWindow->showMaximized();
    state.sizeable = false;
    state.movable = false;
    QCOMPARE(interface->state(), state);
    testWindow->showNormal();
    testWindow->move(-10, 0);
    QVERIFY(interface->state().offscreen);
    testWindow->setVisible(false);
    QVERIFY(interface->state().invisible);
    testWindow->setVisible(true);
    testWindow->setEnabled(false);
    QVERIFY(interface->state().unavailable);
    testWindow->setEnabled(true);
    qApp->setActiveWindow(&mdiArea);
    mdiArea.setActiveSubWindow(testWindow);
    testWindow->setFocus();
    QVERIFY(testWindow->isAncestorOf(qApp->focusWidget()));
    QVERIFY(interface->state().focused);
    testWindow->setGeometry(originalGeometry);

    if (isSubWindowsPlacedNextToEachOther) {
        // This part of the test can only be run if the sub windows are
        // placed next to each other.
        QAccessibleInterface *destination = interface->child(0);
        QVERIFY(destination);
        QCOMPARE(destination->object(), (QObject*)testWindow->widget());
        delete destination;
        QCOMPARE(interface->navigate(QAccessible::Left, 0, &destination), 0);
        QVERIFY(destination);
        QCOMPARE(destination->object(), (QObject*)subWindows.at(2));
        delete destination;
        QCOMPARE(interface->navigate(QAccessible::Right, 0, &destination), 0);
        QVERIFY(destination);
        QCOMPARE(destination->object(), (QObject*)subWindows.at(4));
        delete destination;
    }

    // rect
    const QPoint globalPos = testWindow->mapToGlobal(QPoint(0, 0));
    QCOMPARE(interface->rect(), QRect(globalPos, testWindow->size()));
    testWindow->hide();
    QCOMPARE(interface->rect(), QRect());
    QCOMPARE(childRect(interface), QRect());
    testWindow->showMinimized();
    QCOMPARE(childRect(interface), QRect());
    testWindow->showNormal();
    testWindow->widget()->hide();
    QCOMPARE(childRect(interface), QRect());
    testWindow->widget()->show();
    const QRect widgetGeometry = testWindow->contentsRect();
    const QPoint globalWidgetPos = QPoint(globalPos.x() + widgetGeometry.x(),
                                          globalPos.y() + widgetGeometry.y());
    QCOMPARE(childRect(interface), QRect(globalWidgetPos, widgetGeometry.size()));

    // childAt
    QCOMPARE(interface->childAt(-10, 0), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->childAt(globalPos.x(), globalPos.y()), static_cast<QAccessibleInterface*>(0));
    QAccessibleInterface *child = interface->childAt(globalWidgetPos.x(), globalWidgetPos.y());
    QCOMPARE(child->role(), QAccessible::PushButton);
    QCOMPARE(child->text(QAccessible::Name), QString("QAccessibilityTest"));
    delete child;
    testWindow->widget()->hide();
    QCOMPARE(interface->childAt(globalWidgetPos.x(), globalWidgetPos.y()), static_cast<QAccessibleInterface*>(0));

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::lineEditTest()
{
    QLineEdit *le = new QLineEdit;
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(le);
    QVERIFY(iface);
    le->show();

    QApplication::processEvents();
    QCOMPARE(iface->childCount(), 0);
    QVERIFY(iface->state().sizeable);
    QVERIFY(iface->state().movable);
    QCOMPARE(bool(iface->state().focusable), le->isActiveWindow());
    QVERIFY(iface->state().selectable);
    QVERIFY(iface->state().hasPopup);
    QCOMPARE(bool(iface->state().focused), le->hasFocus());

    QString secret(QLatin1String("secret"));
    le->setText(secret);
    le->setEchoMode(QLineEdit::Normal);
    QVERIFY(!(iface->state().passwordEdit));
    QCOMPARE(iface->text(QAccessible::Value), secret);
    le->setEchoMode(QLineEdit::NoEcho);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value).isEmpty());
    le->setEchoMode(QLineEdit::Password);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value).isEmpty());
    le->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value).isEmpty());
    le->setEchoMode(QLineEdit::Normal);
    QVERIFY(!(iface->state().passwordEdit));
    QCOMPARE(iface->text(QAccessible::Value), secret);

    QWidget *toplevel = new QWidget;
    le->setParent(toplevel);
    toplevel->show();
    QApplication::processEvents();
    QVERIFY(!(iface->state().sizeable));
    QVERIFY(!(iface->state().movable));
    QCOMPARE(bool(iface->state().focusable), le->isActiveWindow());
    QVERIFY(iface->state().selectable);
    QVERIFY(iface->state().hasPopup);
    QCOMPARE(bool(iface->state().focused), le->hasFocus());

    QLineEdit *le2 = new QLineEdit(toplevel);
    le2->show();
    QTest::qWait(100);
    le2->activateWindow();
    QTest::qWait(100);
    le->setFocus(Qt::TabFocusReason);
    QTestAccessibility::clearEvents();
    le2->setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(le2, 0, QAccessible::Focus)));

    le->setText(QLatin1String("500"));
    le->setValidator(new QIntValidator());
    iface->setText(QAccessible::Value, QLatin1String("This text is not a number"));
    QCOMPARE(le->text(), QLatin1String("500"));

    delete iface;
    delete le;
    delete le2;
    QTestAccessibility::clearEvents();

    // IA2
    QString cite = "I always pass on good advice. It is the only thing to do with it. It is never of any use to oneself. --Oscar Wilde";
    QLineEdit *le3 = new QLineEdit(cite, toplevel);
    iface = QAccessible::queryAccessibleInterface(le3);
    QAccessibleTextInterface* textIface = iface->textInterface();
    le3->deselect();
    le3->setCursorPosition(3);
    QCOMPARE(textIface->cursorPosition(), 3);
    QTRY_VERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(le3, 0, QAccessible::TextCaretMoved)));
    QCOMPARE(textIface->selectionCount(), 0);
    QTestAccessibility::clearEvents();

    int start, end;
    QCOMPARE(textIface->text(0, 8), QString::fromLatin1("I always"));
    QCOMPARE(textIface->textAtOffset(0, QAccessible2::CharBoundary,&start,&end), QString::fromLatin1("I"));
    QCOMPARE(start, 0);
    QCOMPARE(end, 1);
    QCOMPARE(textIface->textBeforeOffset(0, QAccessible2::CharBoundary,&start,&end), QString());
    QCOMPARE(textIface->textAfterOffset(0, QAccessible2::CharBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(start, 1);
    QCOMPARE(end, 2);

    QCOMPARE(textIface->textAtOffset(5, QAccessible2::CharBoundary,&start,&end), QString::fromLatin1("a"));
    QCOMPARE(start, 5);
    QCOMPARE(end, 6);
    QCOMPARE(textIface->textBeforeOffset(5, QAccessible2::CharBoundary,&start,&end), QString::fromLatin1("w"));
    QCOMPARE(textIface->textAfterOffset(5, QAccessible2::CharBoundary,&start,&end), QString::fromLatin1("y"));

    QCOMPARE(textIface->textAtOffset(5, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1("always"));
    QCOMPARE(start, 2);
    QCOMPARE(end, 8);

    QCOMPARE(textIface->textAtOffset(2, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1("always"));
    QCOMPARE(textIface->textAtOffset(7, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1("always"));
    QCOMPARE(textIface->textAtOffset(8, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(textIface->textAtOffset(25, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1("advice"));
    QCOMPARE(textIface->textAtOffset(92, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1("oneself"));

    QCOMPARE(textIface->textBeforeOffset(5, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(textIface->textAfterOffset(5, QAccessible2::WordBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(textIface->textAtOffset(5, QAccessible2::SentenceBoundary,&start,&end), QString::fromLatin1("I always pass on good advice. "));
    QCOMPARE(start, 0);
    QCOMPARE(end, 30);

    QCOMPARE(textIface->textBeforeOffset(40, QAccessible2::SentenceBoundary,&start,&end), QString::fromLatin1("I always pass on good advice. "));
    QCOMPARE(textIface->textAfterOffset(5, QAccessible2::SentenceBoundary,&start,&end), QString::fromLatin1("It is the only thing to do with it. "));

    QCOMPARE(textIface->textAtOffset(5, QAccessible2::ParagraphBoundary,&start,&end), cite);
    QCOMPARE(start, 0);
    QCOMPARE(end, cite.length());
    QCOMPARE(textIface->textAtOffset(5, QAccessible2::LineBoundary,&start,&end), cite);
    QCOMPARE(textIface->textAtOffset(5, QAccessible2::NoBoundary,&start,&end), cite);

    delete iface;
    delete toplevel;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::workspaceTest()
{
    {
    QWorkspace workspace;
    workspace.resize(400,300);
    workspace.show();
    const int subWindowCount =  3;
    for (int i = 0; i < subWindowCount; ++i) {
        QWidget *window = workspace.addWindow(new QWidget);
        if (i > 0)
            window->move(window->x() + 1, window->y());
        window->show();
        window->resize(70, window->height());
    }

    QWidgetList subWindows = workspace.windowList();
    QCOMPARE(subWindows.count(), subWindowCount);

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&workspace);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), subWindowCount);

    // Right, right, right, ...
    for (int i = 0; i < subWindowCount; ++i) {
        QAccessibleInterface *destination = 0;
        int index = interface->navigate(QAccessible::Right, i + 1, &destination);
        if (i == subWindowCount - 1) {
            QVERIFY(!destination);
            QCOMPARE(index, -1);
        } else {
            QVERIFY(destination);
            QCOMPARE(index, 0);
            QCOMPARE(destination->object(), (QObject*)subWindows.at(i + 1));
            delete destination;
        }
    }

    // Left, left, left, ...
    for (int i = subWindowCount; i > 0; --i) {
        QAccessibleInterface *destination = 0;
        int index = interface->navigate(QAccessible::Left, i, &destination);
        if (i == 1) {
            QVERIFY(!destination);
            QCOMPARE(index, -1);
        } else {
            QVERIFY(destination);
            QCOMPARE(index, 0);
            QCOMPARE(destination->object(), (QObject*)subWindows.at(i - 2));
            delete destination;
        }
    }
    // ### Add test for Up and Down.

    }
    QTestAccessibility::clearEvents();
}

bool accessibleInterfaceLeftOf(const QAccessibleInterface *a1, const QAccessibleInterface *a2)
{
    return a1->rect().x() < a2->rect().x();
}

bool accessibleInterfaceAbove(const QAccessibleInterface *a1, const QAccessibleInterface *a2)
{
    return a1->rect().y() < a2->rect().y();
}

void tst_QAccessibility::dialogButtonBoxTest()
{
    {
    QDialogButtonBox box(QDialogButtonBox::Reset |
                         QDialogButtonBox::Help |
                         QDialogButtonBox::Ok, Qt::Horizontal);


    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&box);
    QVERIFY(iface);
    box.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    QApplication::processEvents();
    QCOMPARE(iface->childCount(), 3);
    QCOMPARE(iface->role(), QAccessible::Grouping);
    QStringList actualOrder;
    QAccessibleInterface *child;
    child = iface->child(0);
    QCOMPARE(child->role(), QAccessible::PushButton);

    QVector<QAccessibleInterface *> buttons;
    for (int i = 0; i < iface->childCount(); ++i)
        buttons <<  iface->child(i);

    qSort(buttons.begin(), buttons.end(), accessibleInterfaceLeftOf);

    for (int i = 0; i < buttons.count(); ++i)
        actualOrder << buttons.at(i)->text(QAccessible::Name);

    QStringList expectedOrder;
    QDialogButtonBox::ButtonLayout btnlout =
        QDialogButtonBox::ButtonLayout(QApplication::style()->styleHint(QStyle::SH_DialogButtonLayout));
    switch (btnlout) {
    case QDialogButtonBox::WinLayout:
        expectedOrder << QDialogButtonBox::tr("Reset")
                      << QDialogButtonBox::tr("OK")
                      << QDialogButtonBox::tr("Help");
        break;
    case QDialogButtonBox::GnomeLayout:
    case QDialogButtonBox::KdeLayout:
    case QDialogButtonBox::MacLayout:
        expectedOrder << QDialogButtonBox::tr("Help")
                      << QDialogButtonBox::tr("Reset")
                      << QDialogButtonBox::tr("OK");
        break;
    }
    QCOMPARE(actualOrder, expectedOrder);
    delete iface;
    QApplication::processEvents();
    QTestAccessibility::clearEvents();
    }

    {
    QDialogButtonBox box(QDialogButtonBox::Reset |
                         QDialogButtonBox::Help |
                         QDialogButtonBox::Ok, Qt::Horizontal);


    // Test up and down navigation
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&box);
    QVERIFY(iface);
    box.setOrientation(Qt::Vertical);
    box.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    QApplication::processEvents();
    QStringList actualOrder;

    QVector<QAccessibleInterface *> buttons;
    for (int i = 0; i < iface->childCount(); ++i)
        buttons <<  iface->child(i);

    qSort(buttons.begin(), buttons.end(), accessibleInterfaceAbove);

    for (int i = 0; i < buttons.count(); ++i)
        actualOrder << buttons.at(i)->text(QAccessible::Name);

    QStringList expectedOrder;
    expectedOrder << QDialogButtonBox::tr("OK")
                  << QDialogButtonBox::tr("Reset")
                  << QDialogButtonBox::tr("Help");

    QCOMPARE(actualOrder, expectedOrder);
    delete iface;
    QApplication::processEvents();

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::dialTest()
{
    {
    QDial dial;
    dial.setMinimum(23);
    dial.setMaximum(121);
    dial.setValue(42);
    QCOMPARE(dial.value(), 42);
    dial.show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&dial);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 0);

    QCOMPARE(interface->text(QAccessible::Value), QString::number(dial.value()));
    QCOMPARE(interface->rect(), dial.geometry());

    QAccessibleValueInterface *valueIface = interface->valueInterface();
    QVERIFY(valueIface != 0);
    QCOMPARE(valueIface->minimumValue().toInt(), dial.minimum());
    QCOMPARE(valueIface->maximumValue().toInt(), dial.maximum());
    QCOMPARE(valueIface->currentValue().toInt(), 42);
    dial.setValue(50);
    QCOMPARE(valueIface->currentValue().toInt(), dial.value());
    dial.setValue(0);
    QCOMPARE(valueIface->currentValue().toInt(), dial.value());
    dial.setValue(100);
    QCOMPARE(valueIface->currentValue().toInt(), dial.value());
    valueIface->setCurrentValue(77);
    QCOMPARE(77, dial.value());
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::rubberBandTest()
{
    QRubberBand rubberBand(QRubberBand::Rectangle);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&rubberBand);
    QVERIFY(interface);
    QCOMPARE(interface->role(), QAccessible::Border);
    delete interface;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::abstractScrollAreaTest()
{
    {
    QAbstractScrollArea abstractScrollArea;

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&abstractScrollArea);
    QVERIFY(interface);
    QVERIFY(!interface->rect().isValid());
    QCOMPARE(interface->childAt(200, 200), static_cast<QAccessibleInterface*>(0));

    abstractScrollArea.resize(400, 400);
    abstractScrollArea.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif
    const QRect globalGeometry = QRect(abstractScrollArea.mapToGlobal(QPoint(0, 0)),
                                       abstractScrollArea.size());

    // Viewport.
    QCOMPARE(interface->childCount(), 1);
    QWidget *viewport = abstractScrollArea.viewport();
    QVERIFY(viewport);
    QVERIFY(verifyChild(viewport, interface, 0, globalGeometry));

    // Horizontal scrollBar.
    abstractScrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QCOMPARE(interface->childCount(), 2);
    QWidget *horizontalScrollBar = abstractScrollArea.horizontalScrollBar();
    QWidget *horizontalScrollBarContainer = horizontalScrollBar->parentWidget();
    QVERIFY(verifyChild(horizontalScrollBarContainer, interface, 1, globalGeometry));

    // Horizontal scrollBar widgets.
    QLabel *secondLeftLabel = new QLabel(QLatin1String("L2"));
    abstractScrollArea.addScrollBarWidget(secondLeftLabel, Qt::AlignLeft);
    QCOMPARE(interface->childCount(), 2);

    QLabel *firstLeftLabel = new QLabel(QLatin1String("L1"));
    abstractScrollArea.addScrollBarWidget(firstLeftLabel, Qt::AlignLeft);
    QCOMPARE(interface->childCount(), 2);

    QLabel *secondRightLabel = new QLabel(QLatin1String("R2"));
    abstractScrollArea.addScrollBarWidget(secondRightLabel, Qt::AlignRight);
    QCOMPARE(interface->childCount(), 2);

    QLabel *firstRightLabel = new QLabel(QLatin1String("R1"));
    abstractScrollArea.addScrollBarWidget(firstRightLabel, Qt::AlignRight);
    QCOMPARE(interface->childCount(), 2);

    // Vertical scrollBar.
    abstractScrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QCOMPARE(interface->childCount(), 3);
    QWidget *verticalScrollBar = abstractScrollArea.verticalScrollBar();
    QWidget *verticalScrollBarContainer = verticalScrollBar->parentWidget();
    QVERIFY(verifyChild(verticalScrollBarContainer, interface, 2, globalGeometry));

    // Vertical scrollBar widgets.
    QLabel *secondTopLabel = new QLabel(QLatin1String("T2"));
    abstractScrollArea.addScrollBarWidget(secondTopLabel, Qt::AlignTop);
    QCOMPARE(interface->childCount(), 3);

    QLabel *firstTopLabel = new QLabel(QLatin1String("T1"));
    abstractScrollArea.addScrollBarWidget(firstTopLabel, Qt::AlignTop);
    QCOMPARE(interface->childCount(), 3);

    QLabel *secondBottomLabel = new QLabel(QLatin1String("B2"));
    abstractScrollArea.addScrollBarWidget(secondBottomLabel, Qt::AlignBottom);
    QCOMPARE(interface->childCount(), 3);

    QLabel *firstBottomLabel = new QLabel(QLatin1String("B1"));
    abstractScrollArea.addScrollBarWidget(firstBottomLabel, Qt::AlignBottom);
    QCOMPARE(interface->childCount(), 3);

    // CornerWidget.
    abstractScrollArea.setCornerWidget(new QLabel(QLatin1String("C")));
    QCOMPARE(interface->childCount(), 4);
    QWidget *cornerWidget = abstractScrollArea.cornerWidget();
    QVERIFY(verifyChild(cornerWidget, interface, 3, globalGeometry));

    // Test navigate.
    QAccessibleInterface *target = 0;

    // viewport -> Up -> NOTHING
    const int viewportIndex = indexOfChild(interface, viewport);
    QVERIFY(viewportIndex != -1);
    QCOMPARE(interface->navigate(QAccessible::Up, viewportIndex, &target), -1);
    QVERIFY(!target);

    // viewport -> Left -> NOTHING
    QCOMPARE(interface->navigate(QAccessible::Left, viewportIndex, &target), -1);
    QVERIFY(!target);

    // viewport -> Down -> horizontalScrollBarContainer
    const int horizontalScrollBarContainerIndex = indexOfChild(interface, horizontalScrollBarContainer);
    QVERIFY(horizontalScrollBarContainerIndex != -1);
    QCOMPARE(interface->navigate(QAccessible::Down, viewportIndex + 1, &target), 0);
    QVERIFY(target);
    QCOMPARE(target->object(), static_cast<QObject *>(horizontalScrollBarContainer));
    delete target;
    target = 0;

    // horizontalScrollBarContainer -> Left -> NOTHING
    QCOMPARE(interface->navigate(QAccessible::Left, horizontalScrollBarContainerIndex + 1, &target), -1);
    QVERIFY(!target);

    // horizontalScrollBarContainer -> Down -> NOTHING
    QVERIFY(horizontalScrollBarContainerIndex != -1);
    QCOMPARE(interface->navigate(QAccessible::Down, horizontalScrollBarContainerIndex + 1, &target), -1);
    QVERIFY(!target);

    // horizontalScrollBarContainer -> Right -> cornerWidget
    const int cornerWidgetIndex = indexOfChild(interface, cornerWidget);
    QVERIFY(cornerWidgetIndex != -1);
    QCOMPARE(interface->navigate(QAccessible::Right, horizontalScrollBarContainerIndex + 1, &target), 0);
    QVERIFY(target);
    QCOMPARE(target->object(), static_cast<QObject *>(cornerWidget));
    delete target;
    target = 0;

    // cornerWidget -> Down -> NOTHING
    QCOMPARE(interface->navigate(QAccessible::Down, cornerWidgetIndex + 1, &target), -1);
    QVERIFY(!target);

    // cornerWidget -> Right -> NOTHING
    QVERIFY(cornerWidgetIndex != -1);
    QCOMPARE(interface->navigate(QAccessible::Right, cornerWidgetIndex + 1, &target), -1);
    QVERIFY(!target);

    // cornerWidget -> Up ->  verticalScrollBarContainer
    const int verticalScrollBarContainerIndex = indexOfChild(interface, verticalScrollBarContainer);
    QVERIFY(verticalScrollBarContainerIndex != -1);
    QCOMPARE(interface->navigate(QAccessible::Up, cornerWidgetIndex + 1, &target), 0);
    QVERIFY(target);
    QCOMPARE(target->object(), static_cast<QObject *>(verticalScrollBarContainer));
    delete target;
    target = 0;

    // verticalScrollBarContainer -> Right -> NOTHING
    QCOMPARE(interface->navigate(QAccessible::Right, verticalScrollBarContainerIndex + 1, &target), -1);
    QVERIFY(!target);

    // verticalScrollBarContainer -> Up -> NOTHING
    QCOMPARE(interface->navigate(QAccessible::Up, verticalScrollBarContainerIndex + 1, &target), -1);
    QVERIFY(!target);

    // verticalScrollBarContainer -> Left -> viewport
    QCOMPARE(interface->navigate(QAccessible::Left, verticalScrollBarContainerIndex + 1, &target), 0);
    QVERIFY(target);
    QCOMPARE(target->object(), static_cast<QObject *>(viewport));
    delete target;
    target = 0;

    QCOMPARE(verifyHierarchy(interface), 0);

    delete interface;
    }

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::scrollAreaTest()
{
    {
    QScrollArea scrollArea;
    scrollArea.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&scrollArea);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 1); // The viewport.
    delete interface;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::listTest()
{
    {
    QListWidget *listView = new QListWidget;
    listView->addItem("Oslo");
    listView->addItem("Berlin");
    listView->addItem("Brisbane");
    listView->resize(400,400);
    listView->show();
    QTest::qWait(1); // Need this for indexOfchild to work.
    QCoreApplication::processEvents();
    QTest::qWait(100);

    QAIPtr iface = QAIPtr(QAccessible::queryAccessibleInterface(listView));
    QCOMPARE(verifyHierarchy(iface.data()), 0);

    QCOMPARE((int)iface->role(), (int)QAccessible::List);
    QCOMPARE(iface->childCount(), 3);

    {
    QAIPtr child1 = QAIPtr(iface->child(0));
    QVERIFY(child1);
    QCOMPARE(iface->indexOfChild(child1.data()), 0);
    QCOMPARE(child1->text(QAccessible::Name), QString("Oslo"));
    QCOMPARE(child1->role(), QAccessible::ListItem);

    QAIPtr child2 = QAIPtr(iface->child(1));
    QVERIFY(child2);
    QCOMPARE(iface->indexOfChild(child2.data()), 1);
    QCOMPARE(child2->text(QAccessible::Name), QString("Berlin"));

    QAIPtr child3 = QAIPtr(iface->child(2));
    QVERIFY(child3);
    QCOMPARE(iface->indexOfChild(child3.data()), 2);
    QCOMPARE(child3->text(QAccessible::Name), QString("Brisbane"));
    }
    QTestAccessibility::clearEvents();

    // Check for events
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, 0, listView->visualItemRect(listView->item(1)).center());
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(listView, 2, QAccessible::Selection)));
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(listView, 2, QAccessible::Focus)));
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, 0, listView->visualItemRect(listView->item(2)).center());
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(listView, 3, QAccessible::Selection)));
    QVERIFY(QTestAccessibility::events().contains(QTestAccessibilityEvent(listView, 3, QAccessible::Focus)));

    listView->addItem("Munich");
    QCOMPARE(iface->childCount(), 4);

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 1);
    QCOMPARE(table2->rowCount(), 4);
    QAIPtr cell1 = QAIPtr(table2->cellAt(0,0));
    QVERIFY(cell1);
    QCOMPARE(cell1->text(QAccessible::Name), QString("Oslo"));

    QAIPtr cell4 = QAIPtr(table2->cellAt(3,0));
    QVERIFY(cell4);
    QCOMPARE(cell4->text(QAccessible::Name), QString("Munich"));
    QCOMPARE(cell4->role(), QAccessible::ListItem);

    QAccessibleTableCellInterface *cellInterface = cell4->tableCellInterface();
    QVERIFY(cellInterface);
    QCOMPARE(cellInterface->rowIndex(), 3);
    QCOMPARE(cellInterface->columnIndex(), 0);
    QCOMPARE(cellInterface->rowExtent(), 1);
    QCOMPARE(cellInterface->columnExtent(), 1);
    QCOMPARE(cellInterface->rowHeaderCells(), QList<QAccessibleInterface*>());
    QCOMPARE(cellInterface->columnHeaderCells(), QList<QAccessibleInterface*>());

    QCOMPARE(QAIPtr(cellInterface->table())->object(), listView);

    listView->clearSelection();
    QVERIFY(!(cell4->state().expandable));
    QVERIFY( (cell4->state().selectable));
    QVERIFY(!(cell4->state().selected));
    table2->selectRow(3);
    QCOMPARE(listView->selectedItems().size(), 1);
    QCOMPARE(listView->selectedItems().at(0)->text(), QLatin1String("Munich"));
    QVERIFY(cell4->state().selected);
    QVERIFY(cellInterface->isSelected());

    QVERIFY(table2->cellAt(-1, 0) == 0);
    QVERIFY(table2->cellAt(0, -1) == 0);
    QVERIFY(table2->cellAt(0, 1) == 0);
    QVERIFY(table2->cellAt(4, 0) == 0);

    delete listView;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::treeTest()
{
    QTreeWidget *treeView = new QTreeWidget;
    treeView->setColumnCount(2);
    QTreeWidgetItem *header = new QTreeWidgetItem;
    header->setText(0, "Artist");
    header->setText(1, "Work");
    treeView->setHeaderItem(header);

    QTreeWidgetItem *root1 = new QTreeWidgetItem;
    root1->setText(0, "Spain");
    treeView->addTopLevelItem(root1);

    QTreeWidgetItem *item1 = new QTreeWidgetItem;
    item1->setText(0, "Picasso");
    item1->setText(1, "Guernica");
    root1->addChild(item1);

    QTreeWidgetItem *item2 = new QTreeWidgetItem;
    item2->setText(0, "Tapies");
    item2->setText(1, "Ambrosia");
    root1->addChild(item2);

    QTreeWidgetItem *root2 = new QTreeWidgetItem;
    root2->setText(0, "Austria");
    treeView->addTopLevelItem(root2);

    QTreeWidgetItem *item3 = new QTreeWidgetItem;
    item3->setText(0, "Klimt");
    item3->setText(1, "The Kiss");
    root2->addChild(item3);

    treeView->resize(400,400);
    treeView->show();

    QCoreApplication::processEvents();
    QTest::qWait(100);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(treeView);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE((int)iface->role(), (int)QAccessible::Tree);
    // header and 2 rows (the others are not expanded, thus not visible)
    QCOMPARE(iface->childCount(), 6);

    QAccessibleInterface *header1 = 0;
    header1 = iface->child(0);
    QVERIFY(header1);
    QCOMPARE(iface->indexOfChild(header1), 0);
    QCOMPARE(header1->text(QAccessible::Name), QString("Artist"));
    QCOMPARE(header1->role(), QAccessible::ColumnHeader);
    delete header1;

    QAccessibleInterface *child1 = 0;
    child1 = iface->child(2);
    QVERIFY(child1);
    QCOMPARE(iface->indexOfChild(child1), 2);
    QCOMPARE(child1->text(QAccessible::Name), QString("Spain"));
    QCOMPARE(child1->role(), QAccessible::TreeItem);
    QVERIFY(!(child1->state().expanded));
    delete child1;

    QAccessibleInterface *child2 = 0;
    child2 = iface->child(4);
    QVERIFY(child2);
    QCOMPARE(iface->indexOfChild(child2), 4);
    QCOMPARE(child2->text(QAccessible::Name), QString("Austria"));
    delete child2;

    QTestAccessibility::clearEvents();

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 2);
    QCOMPARE(table2->rowCount(), 2);
    QAccessibleInterface *cell1;
    QVERIFY(cell1 = table2->cellAt(0,0));
    QCOMPARE(cell1->text(QAccessible::Name), QString("Spain"));
    QAccessibleInterface *cell2;
    QVERIFY(cell2 = table2->cellAt(1,0));
    QCOMPARE(cell2->text(QAccessible::Name), QString("Austria"));
    QCOMPARE(cell2->role(), QAccessible::TreeItem);
    QCOMPARE(cell2->tableCellInterface()->rowIndex(), 1);
    QCOMPARE(cell2->tableCellInterface()->columnIndex(), 0);
    QVERIFY(cell2->state().expandable);
    QCOMPARE(iface->indexOfChild(cell2), 4);
    QVERIFY(!(cell2->state().expanded));
    QCOMPARE(table2->columnDescription(1), QString("Work"));
    delete cell2;
    delete cell1;

    treeView->expandAll();

    // Need this for indexOfchild to work.
    QCoreApplication::processEvents();
    QTest::qWait(100);

    QCOMPARE(table2->columnCount(), 2);
    QCOMPARE(table2->rowCount(), 5);
    cell1 = table2->cellAt(1,0);
    QCOMPARE(cell1->text(QAccessible::Name), QString("Picasso"));
    QCOMPARE(iface->indexOfChild(cell1), 4); // 2 header + 2 for root item

    cell2 = table2->cellAt(4,0);
    QCOMPARE(cell2->text(QAccessible::Name), QString("Klimt"));
    QCOMPARE(cell2->role(), QAccessible::TreeItem);
    QCOMPARE(cell2->tableCellInterface()->rowIndex(), 4);
    QCOMPARE(cell2->tableCellInterface()->columnIndex(), 0);
    QVERIFY(!(cell2->state().expandable));
    QCOMPARE(iface->indexOfChild(cell2), 10);

    QCOMPARE(table2->columnDescription(0), QString("Artist"));
    QCOMPARE(table2->columnDescription(1), QString("Work"));

    delete iface;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tableTest()
{
    QTableWidget *tableView = new QTableWidget(3, 3);
    tableView->setColumnCount(3);
    QStringList hHeader;
    hHeader << "h1" << "h2" << "h3";
    tableView->setHorizontalHeaderLabels(hHeader);

    QStringList vHeader;
    vHeader << "v1" << "v2" << "v3";
    tableView->setVerticalHeaderLabels(vHeader);

    for (int i = 0; i<9; ++i) {
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setText(QString::number(i/3) + QString(".") + QString::number(i%3));
        tableView->setItem(i/3, i%3, item);
    }

    tableView->resize(600,600);
    tableView->show();
    QTest::qWait(1); // Need this for indexOfchild to work.
    QCoreApplication::processEvents();
    QTest::qWait(100);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(tableView);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE((int)iface->role(), (int)QAccessible::Table);
    // header and 2 rows (the others are not expanded, thus not visible)
    QCOMPARE(iface->childCount(), 9+3+3+1); // cell+headers+topleft button

    QAccessibleInterface *cornerButton = iface->child(0);
    QVERIFY(cornerButton);
    QCOMPARE(iface->indexOfChild(cornerButton), 0);
    QCOMPARE(cornerButton->role(), QAccessible::Pane);
    delete cornerButton;

    QAccessibleInterface *child1 = iface->child(2);
    QVERIFY(child1);
    QCOMPARE(iface->indexOfChild(child1), 2);
    QCOMPARE(child1->text(QAccessible::Name), QString("h2"));
    QCOMPARE(child1->role(), QAccessible::ColumnHeader);
    QVERIFY(!(child1->state().expanded));
    delete child1;

    QAccessibleInterface *child2 = iface->child(10);
    QVERIFY(child2);
    QCOMPARE(iface->indexOfChild(child2), 10);
    QCOMPARE(child2->text(QAccessible::Name), QString("1.1"));
    QAccessibleTableCellInterface *cell2Iface = child2->tableCellInterface();
    QCOMPARE(cell2Iface->rowIndex(), 1);
    QCOMPARE(cell2Iface->columnIndex(), 1);
    delete child2;

    QAccessibleInterface *child3 = iface->child(11);
    QCOMPARE(iface->indexOfChild(child3), 11);
    QCOMPARE(child3->text(QAccessible::Name), QString("1.2"));
    delete child3;

    QTestAccessibility::clearEvents();

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 3);
    QCOMPARE(table2->rowCount(), 3);
    QAccessibleInterface *cell1;
    QVERIFY(cell1 = table2->cellAt(0,0));
    QCOMPARE(cell1->text(QAccessible::Name), QString("0.0"));
    QCOMPARE(iface->indexOfChild(cell1), 5);

    QAccessibleInterface *cell2;
    QVERIFY(cell2 = table2->cellAt(0,1));
    QCOMPARE(cell2->text(QAccessible::Name), QString("0.1"));
    QCOMPARE(cell2->role(), QAccessible::Cell);
    QCOMPARE(cell2->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell2->tableCellInterface()->columnIndex(), 1);
    QCOMPARE(iface->indexOfChild(cell2), 6);
    delete cell2;

    QAccessibleInterface *cell3;
    QVERIFY(cell3 = table2->cellAt(1,2));
    QCOMPARE(cell3->text(QAccessible::Name), QString("1.2"));
    QCOMPARE(cell3->role(), QAccessible::Cell);
    QCOMPARE(cell3->tableCellInterface()->rowIndex(), 1);
    QCOMPARE(cell3->tableCellInterface()->columnIndex(), 2);
    QCOMPARE(iface->indexOfChild(cell3), 11);
    delete cell3;

    QCOMPARE(table2->columnDescription(0), QString("h1"));
    QCOMPARE(table2->columnDescription(1), QString("h2"));
    QCOMPARE(table2->columnDescription(2), QString("h3"));
    QCOMPARE(table2->rowDescription(0), QString("v1"));
    QCOMPARE(table2->rowDescription(1), QString("v2"));
    QCOMPARE(table2->rowDescription(2), QString("v3"));

    delete iface;

    delete tableView;

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::calendarWidgetTest()
{
#ifndef QT_NO_CALENDARWIDGET
    {
    QCalendarWidget calendarWidget;

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&calendarWidget);
    QVERIFY(interface);
    QCOMPARE(interface->role(), QAccessible::Table);
    QVERIFY(!interface->rect().isValid());
    QCOMPARE(interface->childAt(200, 200), static_cast<QAccessibleInterface*>(0));

    calendarWidget.resize(400, 300);
    calendarWidget.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    // 1 = navigationBar, 2 = view.
    QCOMPARE(interface->childCount(), 2);

    const QRect globalGeometry = QRect(calendarWidget.mapToGlobal(QPoint(0, 0)),
                                       calendarWidget.size());
    QCOMPARE(interface->rect(), globalGeometry);

    QWidget *navigationBar = 0;
    foreach (QObject *child, calendarWidget.children()) {
        if (child->objectName() == QLatin1String("qt_calendar_navigationbar")) {
            navigationBar = static_cast<QWidget *>(child);
            break;
        }
    }
    QVERIFY(navigationBar);
    QVERIFY(verifyChild(navigationBar, interface, 0, globalGeometry));

    QAbstractItemView *calendarView = 0;
    foreach (QObject *child, calendarWidget.children()) {
        if (child->objectName() == QLatin1String("qt_calendar_calendarview")) {
            calendarView = static_cast<QAbstractItemView *>(child);
            break;
        }
    }
    QVERIFY(calendarView);
    QVERIFY(verifyChild(calendarView, interface, 1, globalGeometry));

    // Hide navigation bar.
    calendarWidget.setNavigationBarVisible(false);
    QCOMPARE(interface->childCount(), 1);
    QVERIFY(!navigationBar->isVisible());

    QVERIFY(verifyChild(calendarView, interface, 0, globalGeometry));

    // Show navigation bar.
    calendarWidget.setNavigationBarVisible(true);
    QCOMPARE(interface->childCount(), 2);
    QVERIFY(navigationBar->isVisible());

    // Navigate to the navigation bar via Child.
    QAccessibleInterface *navigationBarInterface = interface->child(0);
    QVERIFY(navigationBarInterface);
    QCOMPARE(navigationBarInterface->object(), (QObject*)navigationBar);
    delete navigationBarInterface;
    navigationBarInterface = 0;

    // Navigate to the view via Child.
    QAccessibleInterface *calendarViewInterface = interface->child(1);
    QVERIFY(calendarViewInterface);
    QCOMPARE(calendarViewInterface->object(), (QObject*)calendarView);
    delete calendarViewInterface;
    calendarViewInterface = 0;

    QVERIFY(!interface->child(-1));

    // Navigate from navigation bar -> view (Down).
    QCOMPARE(interface->navigate(QAccessible::Down, 1, &calendarViewInterface), 0);
    QVERIFY(calendarViewInterface);
    QCOMPARE(calendarViewInterface->object(), (QObject*)calendarView);
    delete calendarViewInterface;
    calendarViewInterface = 0;

    // Navigate from view -> navigation bar (Up).
    QCOMPARE(interface->navigate(QAccessible::Up, 2, &navigationBarInterface), 0);
    QVERIFY(navigationBarInterface);
    QCOMPARE(navigationBarInterface->object(), (QObject*)navigationBar);
    delete navigationBarInterface;
    navigationBarInterface = 0;

    }
    QTestAccessibility::clearEvents();
#endif // QT_NO_CALENDARWIDGET
}

void tst_QAccessibility::dockWidgetTest()
{
#ifndef QT_NO_DOCKWIDGET
    // Set up a proper main window with two dock widgets
    QMainWindow *mw = new QMainWindow();
    QFrame *central = new QFrame(mw);
    mw->setCentralWidget(central);
    QMenuBar *mb = new QMenuBar(mw);
    mb->addAction(tr("&File"));
    mw->setMenuBar(mb);

    QDockWidget *dock1 = new QDockWidget(mw);
    mw->addDockWidget(Qt::LeftDockWidgetArea, dock1);
    QPushButton *pb1 = new QPushButton(tr("Push me"), dock1);
    dock1->setWidget(pb1);

    QDockWidget *dock2 = new QDockWidget(mw);
    mw->addDockWidget(Qt::BottomDockWidgetArea, dock2);
    QPushButton *pb2 = new QPushButton(tr("Push me"), dock2);
    dock2->setWidget(pb2);

    mw->resize(600,400);
    mw->show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    QAccessibleInterface *accMainWindow = QAccessible::queryAccessibleInterface(mw);
    // 4 children: menu bar, dock1, dock2, and central widget
    QCOMPARE(accMainWindow->childCount(), 4);
    QAccessibleInterface *accDock1 = 0;
    for (int i = 0; i < 4; ++i) {
        accDock1 = accMainWindow->child(i);
        if (accMainWindow->role() == QAccessible::Window) {
            if (accDock1 && qobject_cast<QDockWidget*>(accDock1->object()) == dock1) {
                break;
            } else {
                delete accDock1;
            }
        }
    }
    QVERIFY(accDock1);
    QCOMPARE(accDock1->role(), QAccessible::Window);

    QAccessibleInterface *dock1TitleBar = accDock1->child(0);
    QCOMPARE(dock1TitleBar->role(), QAccessible::TitleBar);
    QVERIFY(accDock1->rect().contains(dock1TitleBar->rect()));
    delete dock1TitleBar;

    QPoint globalPos = dock1->mapToGlobal(QPoint(0,0));
    globalPos.rx()+=5;  //### query style
    globalPos.ry()+=5;
    QAccessibleInterface *childAt = accDock1->childAt(globalPos.x(), globalPos.y());    //###
    QCOMPARE(childAt->role(), QAccessible::TitleBar);
    int index = accDock1->indexOfChild(childAt);
    delete childAt;
    QAccessibleInterface *accTitleBar = accDock1->child(index);

    QCOMPARE(accTitleBar->role(), QAccessible::TitleBar);
    QCOMPARE(accDock1->indexOfChild(accTitleBar), 0);
    QAccessibleInterface *acc;
    acc = accTitleBar->parent();
    QVERIFY(acc);
    QCOMPARE(acc->role(), QAccessible::Window);


    delete accTitleBar;
    delete accDock1;
    delete pb1;
    delete pb2;
    delete dock1;
    delete dock2;
    delete mw;
    QTestAccessibility::clearEvents();
#endif // QT_NO_DOCKWIDGET
}

void tst_QAccessibility::comboBoxTest()
{
#if defined(Q_OS_WINCE)
    if (!IsValidCEPlatform())
        QSKIP("Test skipped on Windows Mobile test hardware");
#endif
    { // not editable combobox
    QComboBox combo;
    combo.addItems(QStringList() << "one" << "two" << "three");
    combo.show();
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&combo);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE(iface->role(), QAccessible::ComboBox);
    QCOMPARE(iface->childCount(), 1);

#ifdef Q_OS_UNIX
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("one"));
#endif
    QCOMPARE(iface->text(QAccessible::Value), QLatin1String("one"));
    combo.setCurrentIndex(2);
#ifdef Q_OS_UNIX
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("three"));
#endif
    QCOMPARE(iface->text(QAccessible::Value), QLatin1String("three"));

    QAccessibleInterface *listIface = iface->child(0);
    QCOMPARE(listIface->role(), QAccessible::List);
    QCOMPARE(listIface->childCount(), 3);

    QVERIFY(!combo.view()->isVisible());
    QVERIFY(iface->actionInterface());
    QCOMPARE(iface->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    iface->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    QVERIFY(combo.view()->isVisible());

    delete iface;
    }

    { // editable combobox
    QComboBox editableCombo;
    editableCombo.show();
    editableCombo.setEditable(true);
    editableCombo.addItems(QStringList() << "foo" << "bar" << "baz");

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&editableCombo);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE(iface->role(), QAccessible::ComboBox);
    QCOMPARE(iface->childCount(), 2);

    QAccessibleInterface *listIface = iface->child(0);
    QCOMPARE(listIface->role(), QAccessible::List);
    QAccessibleInterface *editIface = iface->child(1);
    QCOMPARE(editIface->role(), QAccessible::EditableText);

    delete listIface;
    delete editIface;
    delete iface;
    }

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::labelTest()
{
    QString text = "Hello World";
    QLabel *label = new QLabel(text);
    label->show();

#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);

    QAccessibleInterface *acc_label = QAccessible::queryAccessibleInterface(label);
    QVERIFY(acc_label);

    QCOMPARE(acc_label->text(QAccessible::Name), text);

    delete acc_label;
    delete label;
    QTestAccessibility::clearEvents();

    QPixmap testPixmap(50, 50);
    testPixmap.fill();

    QLabel imageLabel;
    imageLabel.setPixmap(testPixmap);
    imageLabel.setToolTip("Test Description");

    acc_label = QAccessible::queryAccessibleInterface(&imageLabel);
    QVERIFY(acc_label);

    QAccessibleImageInterface *imageInterface = acc_label->imageInterface();
    QVERIFY(imageInterface);

    QCOMPARE(imageInterface->imageSize(), testPixmap.size());
    QCOMPARE(imageInterface->imageDescription(), QString::fromLatin1("Test Description"));
    QCOMPARE(imageInterface->imagePosition(QAccessible2::RelativeToParent), imageLabel.geometry());

    delete acc_label;

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::accelerators()
{
    QWidget *window = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(window);
    QLabel *label = new QLabel(tr("&Line edit"), window);
    QLineEdit *le = new QLineEdit(window);
    lay->addWidget(label);
    lay->addWidget(le);
    label->setBuddy(le);

    window->show();

    QAccessibleInterface *accLineEdit = QAccessible::queryAccessibleInterface(le);
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("L"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("L"));
    label->setText(tr("Q &"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q &&"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q && A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q &&&A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("A"));
    label->setText(tr("Q &&A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());

#if !defined(QT_NO_DEBUG) && !defined(Q_WS_MAC)
    QTest::ignoreMessage(QtWarningMsg, "QKeySequence::mnemonic: \"Q &A&B\" contains multiple occurrences of '&'");
#endif
    label->setText(tr("Q &A&B"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("A"));

#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);
    delete window;
    QTestAccessibility::clearEvents();
}

QTEST_MAIN(tst_QAccessibility)
#include "tst_qaccessibility.moc"
