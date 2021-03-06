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

#ifndef QPLATFORMDIALOGHELPER_H
#define QPLATFORMDIALOGHELPER_H

#include <QtCore/QtGlobal>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QDir>
#include <QtGui/QRgb>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QString;
class QColor;
class QFont;
class QWindow;
class QVariant;
class QUrl;
class QColorDialogOptionsPrivate;
class QFontDialogOptionsPrivate;
class QFileDialogOptionsPrivate;

class Q_WIDGETS_EXPORT QPlatformDialogHelper : public QObject
{
    Q_OBJECT
public:
    enum StyleHint {
        SnapToDefaultButton
    };
    enum DialogCode { Rejected, Accepted };
    enum ShowFlag {
        ShowModal    = 0x00000001
    };

    Q_DECLARE_FLAGS(ShowFlags, ShowFlag)

    QPlatformDialogHelper();
    virtual ~QPlatformDialogHelper();

    virtual QVariant styleHint(StyleHint hint) const;

    virtual void platformNativeDialogModalHelp() = 0;
    virtual void _q_platformRunNativeAppModalPanel() = 0;

    virtual void deleteNativeDialog_sys() = 0;
    virtual bool show_sys(ShowFlags showFlags,
                          Qt::WindowFlags windowFlags,
                          QWindow *parent) = 0;
    virtual void hide_sys() = 0;

    virtual DialogCode dialogResultCode_sys() = 0;

    static QVariant defaultStyleHint(QPlatformDialogHelper::StyleHint hint);

Q_SIGNALS:
    void launchNativeAppModalPanel();
    void accept();
    void reject();

protected Q_SLOTS:
    void emitLaunchNativeAppModalPanel();
};

class Q_WIDGETS_EXPORT QColorDialogOptions
{
public:
    enum ColorDialogOption {
        ShowAlphaChannel    = 0x00000001,
        NoButtons           = 0x00000002,
        DontUseNativeDialog = 0x00000004
    };

    Q_DECLARE_FLAGS(ColorDialogOptions, ColorDialogOption)

    QColorDialogOptions();
    QColorDialogOptions(const QColorDialogOptions &rhs);
    QColorDialogOptions &operator=(const QColorDialogOptions &rhs);
    ~QColorDialogOptions();

    QString windowTitle() const;
    void setWindowTitle(const QString &);

    void setOption(ColorDialogOption option, bool on = true);
    bool testOption(ColorDialogOption option) const;
    void setOptions(ColorDialogOptions options);
    ColorDialogOptions options() const;

    static int customColorCount();
    static QRgb customColor(int index);
    static QRgb *customColors();
    static void setCustomColor(int index, QRgb color);

    static QRgb *standardColors();
    static QRgb standardColor(int index);
    static void setStandardColor(int index, QRgb color);

private:
    QSharedDataPointer<QColorDialogOptionsPrivate> d;
};

class Q_WIDGETS_EXPORT QPlatformColorDialogHelper : public QPlatformDialogHelper
{
    Q_OBJECT
public:
    const QSharedPointer<QColorDialogOptions> &options() const;
    void setOptions(const QSharedPointer<QColorDialogOptions> &options);

    virtual void setCurrentColor_sys(const QColor &) = 0;
    virtual QColor currentColor_sys() const = 0;

Q_SIGNALS:
    void currentColorChanged(const QColor &color);
    void colorSelected(const QColor &color);

private:
    QSharedPointer<QColorDialogOptions> m_options;
};

class Q_WIDGETS_EXPORT QFontDialogOptions
{
public:
    enum FontDialogOption {
        NoButtons           = 0x00000001,
        DontUseNativeDialog = 0x00000002
    };

    Q_DECLARE_FLAGS(FontDialogOptions, FontDialogOption)

    QFontDialogOptions();
    QFontDialogOptions(const QFontDialogOptions &rhs);
    QFontDialogOptions &operator=(const QFontDialogOptions &rhs);
    ~QFontDialogOptions();

    QString windowTitle() const;
    void setWindowTitle(const QString &);

    void setOption(FontDialogOption option, bool on = true);
    bool testOption(FontDialogOption option) const;
    void setOptions(FontDialogOptions options);
    FontDialogOptions options() const;

private:
    QSharedDataPointer<QFontDialogOptionsPrivate> d;
};

class Q_WIDGETS_EXPORT QPlatformFontDialogHelper : public QPlatformDialogHelper
{
    Q_OBJECT
public:
    virtual void setCurrentFont_sys(const QFont &) = 0;
    virtual QFont currentFont_sys() const = 0;

    const QSharedPointer<QFontDialogOptions> &options() const;
    void setOptions(const QSharedPointer<QFontDialogOptions> &options);

Q_SIGNALS:
    void currentFontChanged(const QFont &font);
    void fontSelected(const QFont &font);

private:
    QSharedPointer<QFontDialogOptions> m_options;
};

class Q_WIDGETS_EXPORT QFileDialogOptions
{
public:
    enum ViewMode { Detail, List };
    enum FileMode { AnyFile, ExistingFile, Directory, ExistingFiles, DirectoryOnly };
    enum AcceptMode { AcceptOpen, AcceptSave };
    enum DialogLabel { LookIn, FileName, FileType, Accept, Reject, DialogLabelCount };

    enum FileDialogOption
    {
        ShowDirsOnly          = 0x00000001,
        DontResolveSymlinks   = 0x00000002,
        DontConfirmOverwrite  = 0x00000004,
        DontUseSheet          = 0x00000008,
        DontUseNativeDialog   = 0x00000010,
        ReadOnly              = 0x00000020,
        HideNameFilterDetails = 0x00000040
    };
    Q_DECLARE_FLAGS(FileDialogOptions, FileDialogOption)

    QFileDialogOptions();
    QFileDialogOptions(const QFileDialogOptions &rhs);
    QFileDialogOptions &operator=(const QFileDialogOptions &rhs);
    ~QFileDialogOptions();

    QString windowTitle() const;
    void setWindowTitle(const QString &);

    void setOption(FileDialogOption option, bool on = true);
    bool testOption(FileDialogOption option) const;
    void setOptions(FileDialogOptions options);
    FileDialogOptions options() const;

    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void setViewMode(ViewMode mode);
    ViewMode viewMode() const;

    void setFileMode(FileMode mode);
    FileMode fileMode() const;

    void setAcceptMode(AcceptMode mode);
    AcceptMode acceptMode() const;

    void setSidebarUrls(const QList<QUrl> &urls);
    QList<QUrl> sidebarUrls() const;

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setDefaultSuffix(const QString &suffix);
    QString defaultSuffix() const;

    void setHistory(const QStringList &paths);
    QStringList history() const;

    void setLabelText(DialogLabel label, const QString &text);
    QString labelText(DialogLabel label) const;
    bool isLabelExplicitlySet(DialogLabel label);

    QString initialDirectory() const;
    void setInitialDirectory(const QString &);

    QString initiallySelectedNameFilter() const;
    void setInitiallySelectedNameFilter(const QString &);

    QStringList initiallySelectedFiles() const;
    void setInitiallySelectedFiles(const QStringList &);

private:
    QSharedDataPointer<QFileDialogOptionsPrivate> d;
};

class Q_WIDGETS_EXPORT QPlatformFileDialogHelper : public QPlatformDialogHelper
{
    Q_OBJECT
public:
    virtual bool defaultNameFilterDisables() const = 0;
    virtual void setDirectory_sys(const QString &directory) = 0;
    virtual QString directory_sys() const = 0;
    virtual void selectFile_sys(const QString &filename) = 0;
    virtual QStringList selectedFiles_sys() const = 0;
    virtual void setFilter_sys() = 0;
    virtual void selectNameFilter_sys(const QString &filter) = 0;
    virtual QString selectedNameFilter_sys() const = 0;

    const QSharedPointer<QFileDialogOptions> &options() const;
    void setOptions(const QSharedPointer<QFileDialogOptions> &options);

Q_SIGNALS:
    void fileSelected(const QString &file);
    void filesSelected(const QStringList &files);
    void currentChanged(const QString &path);
    void directoryEntered(const QString &directory);
    void filterSelected(const QString &filter);

private:
    QSharedPointer<QFileDialogOptions> m_options;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMDIALOGHELPER_H
