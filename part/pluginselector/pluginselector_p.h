/**
  * This file is part of the KDE project
  * Copyright (C) 2007, 2006 Rafael Fernández López <ereslibre@gmail.com>
  * Copyright (C) 2002-2003 Matthias Kretz <kretz@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KATEPLUGINSELECTOR_P_H
#define KATEPLUGINSELECTOR_P_H

#include <QListView>
#include <QtGui/QWidget>
#include <QtGui/QTreeWidget>
#include <QtGui/QMenu>
#include <QtGui/QItemDelegate>
#include <QAbstractListModel>

#include <kservice.h>
#include <kconfigbase.h>
#include <klocale.h>
#include <kconfiggroup.h>
#include <kplugininfo.h>

class KCModuleProxy;
class KIconLoader;
class KTabWidget;
class KDialog;
class QLabel;

class KatePluginSelector::Private
    : public QObject
{
    Q_OBJECT

public:
    enum CheckWhatDependencies
    {
        /// If an item was checked, check all dependencies of that item
        DependenciesINeed = 0,
        /// If an item was unchecked, uncheck all items that depends on that item
        DependenciesNeedMe
    };

    Private(KatePluginSelector *parent);
    ~Private();

    void checkIfShowIcons(const QList<KPluginInfo> &pluginInfoList);

Q_SIGNALS:
    void changed(bool hasChanged);
    void configCommitted(const QByteArray &componentName);

private Q_SLOTS:
    void emitChanged();

public:
    class PluginModel;
    class PluginDelegate;
    class QListViewSpecialized;
    class DependenciesWidget;
    KatePluginSelector *parent;
    PluginModel *pluginModel;
    PluginDelegate *pluginDelegate;
    QListViewSpecialized *listView;
    DependenciesWidget *dependenciesWidget;
    bool showIcons;
};


// =============================================================


/**
 * This widget will inform the user about changes that happened automatically
 * due to plugin dependencies.
 */
class KatePluginSelector::Private::DependenciesWidget
    : public QWidget
{
    Q_OBJECT

public:
    DependenciesWidget(QWidget *parent = 0);
    ~DependenciesWidget();

    void addDependency(const QString &dependency, const QString &pluginCausant, bool added);
    void userOverrideDependency(const QString &dependency);

    void clearDependencies();

private Q_SLOTS:
    void showDependencyDetails();

private:
    struct FurtherInfo
    {
        bool added;
        QString pluginCausant;
    };

    void updateDetails();

    QLabel *details;
    QMap<QString, struct FurtherInfo> dependencyMap;
    int addedByDependencies;
    int removedByDependencies;
};


// =============================================================


/**
 * Ah, we need viewOptions() as public...
 */
class KatePluginSelector::Private::QListViewSpecialized
    : public QListView
{
public:
    QListViewSpecialized(QWidget *parent = 0);
    ~QListViewSpecialized();

    QStyleOptionViewItem viewOptions() const;
};


// =============================================================


class KatePluginSelector::Private::PluginModel
    : public QAbstractListModel
{
public:
    enum AddMethod
    {
        AutomaticallyAdded = 0,
        ManuallyAdded
    };

    struct AdditionalInfo
    {
        int itemChecked;
        KConfigGroup configGroup;
        QStringList parentComponents;
        AddMethod addMethod; // If the plugin was added with the method
                             // addPlugins(const QList<KPluginInfo> &pluginInfoList ...
                             // Mainly for only updating the plugins that were manually
                             // added when calling to updatePluginsState()
        bool alternateColor;
    };

    PluginModel(KatePluginSelector::Private *parent);
    ~PluginModel();

    void appendPluginList(const KPluginInfo::List &pluginInfoList,
                          const QString &categoryName,
                          const QString &categoryKey,
                          const KConfigGroup &configGroup,
                          PluginLoadMethod pluginLoadMethod = ReadConfigFile,
                          AddMethod addMethod = AutomaticallyAdded);

    // Reimplemented from QAbstractItemModel

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::CheckStateRole);

    QVariant data(const QModelIndex &index, int role) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QList<KService::Ptr> services(const QModelIndex &index) const;

    KConfigGroup configGroup(const QModelIndex &index) const;

    void setParentComponents(const QModelIndex &index, const QStringList &parentComponents);

    QStringList parentComponents(const QModelIndex &index) const;

    void updateDependencies(const QString &dependency, const QString &pluginCausant, CheckWhatDependencies whatDependencies, QStringList &dependenciesPushed);

    // Own methods

    AddMethod addMethod(const KPluginInfo &pluginInfo) const;

    bool alternateColor(const KPluginInfo &pluginInfo) const;

private:
    QMap<QString, KPluginInfo::List> pluginInfoByCategory;
    QHash<KPluginInfo, KCModuleProxy *> moduleProxies;
    QMap<QString, int> pluginCount;
    QHash<KPluginInfo, struct AdditionalInfo> additionalInfo;
    KatePluginSelector::Private *parent;
};


// =============================================================


class KatePluginSelector::Private::PluginDelegate
    : public QItemDelegate
{
    Q_OBJECT

public:
    enum Roles
    {
        Name = 33,
        Comment,
        Icon,
        Author,
        Email,
        Category,
        InternalName,
        Version,
        Website,
        License,
        Checked
    };

    PluginDelegate(KatePluginSelector::Private *parent);
    ~PluginDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setIconSize(int width, int height);

    void setMinimumItemWidth(int minimumItemWidth);

    void setLeftMargin(int leftMargin);

    void setRightMargin(int rightMargin);

    int getSeparatorPixels() const;

    void setSeparatorPixels(int separatorPixels);

    QRect aboutButtonRect(const QStyleOptionButton &option) const;
    QRect settingsButtonRect(const QStyleOptionButton &option) const;

Q_SIGNALS:
    void configCommitted(const QByteArray &componentName);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

private Q_SLOTS:
    void slotDefaultClicked();
    void processUrl(const QString &url) const;

private:
    enum EventReceived
    {
        MouseEvent = 0,
        KeyboardEvent
    };

    enum FocusedElement
    {
        CheckBoxFocused = 0,
        SettingsButtonFocused,
        AboutButtonFocused
    };

    QRect checkRect(const QModelIndex &index, const QStyleOptionViewItem &option) const;

    void updateCheckState(const QModelIndex &index, const QStyleOptionViewItem &option,
                          const QPoint &cursorPos, QListView *listView, EventReceived eventReceived);

    void checkDependencies(PluginModel *model,
                           const KPluginInfo &info,
                           CheckWhatDependencies whatDependencies);

    QString name(const QModelIndex &index) const;
    QString comment(const QModelIndex &index) const;
    QPixmap icon(const QModelIndex &index, int width, int height) const;
    QString author(const QModelIndex &index) const;
    QString email(const QModelIndex &index) const;
    QString category(const QModelIndex &index) const;
    QString internalName(const QModelIndex &index) const;
    QString version(const QModelIndex &index) const;
    QString website(const QModelIndex &index) const;
    QString license(const QModelIndex &index) const;
    int calculateVerticalCenter(const QRect &rect, int pixmapHeight) const;

    int iconWidth;
    int iconHeight;
    int minimumItemWidth;
    int leftMargin;
    int rightMargin;
    int separatorPixels;
    FocusedElement focusedElement; // whether is focused the check or the link
    bool sunkenButton;
    KIconLoader *iconLoader;
    QPoint relativeMousePosition;
    QList<KCModuleProxy*> *currentModuleProxyList;
    QHash<int /* row */, KTabWidget*> tabWidgets;
    QHash<int /* row */, KDialog*> configDialogs;
    QHash<int /* row */, KDialog*> aboutDialogs;
    QHash<int /* row */, QList<KCModuleProxy*> > modulesDialogs;
    KDialog *configDialog; // For enabling/disabling default button
    KatePluginSelector::Private *parent;
};

#endif // KPLUGINSELECTOR_P_H
