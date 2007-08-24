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

// FIXME: There exists code duplication. I have planned to fix it, but the
//        important thing is that it is working perfectly :)

#include "pluginselector.h"
#include "pluginselector_p.h"

#include <QtGui/QPainter>
#include <QtGui/QFrame>
#include <QtGui/QStackedWidget>
#include <QtGui/QTreeWidget>
#include <QtGui/QSplitter>
#include <QtGui/QHeaderView>
#include <QtGui/QBoxLayout>
#include <QtCore/QList>
#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QLabel>
#include <QtGui/QBrush>

#include "kcmoduleinfo.h"
#include "kcmoduleloader.h"
#include "kcmoduleproxy.h"
#include <kapplication.h>
#include <klocalizedstring.h>
#include <ktabwidget.h>
#include <kcomponentdata.h>
#include <kplugininfo.h>
#include <kstandarddirs.h>
#include <kconfigbase.h>
#include <kiconloader.h>
#include <kcmodule.h>
#include <kconfiggroup.h>
#include <kicon.h>
#include <kstyle.h>
#include <kdialog.h>
#include <kurllabel.h>
#include <klineedit.h>
#include <kurl.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kglobalsettings.h>

static QString details = I18N_NOOP("Settings");
static QString about = I18N_NOOP("About");

KatePluginSelector::Private::Private(KatePluginSelector *parent)
    : QObject(parent)
    , parent(parent)
    , listView(0)
    , showIcons(false)
{
    pluginModel = new PluginModel(this);
    pluginDelegate = new PluginDelegate(this);

    pluginDelegate->setMinimumItemWidth(200);
    pluginDelegate->setLeftMargin(20);
    pluginDelegate->setRightMargin(20);
    pluginDelegate->setSeparatorPixels(8);

    QFont title(parent->font());
    title.setPointSize(title.pointSize() + 2);
    title.setWeight(QFont::Bold);

    QFontMetrics titleMetrics(title);
    QFontMetrics currentMetrics(parent->font());

    QStyleOptionButton opt;
    opt.fontMetrics = currentMetrics;
    opt.text = "foo"; // height() will be checked, and that does not depend on the string
    if (KGlobalSettings::showIconsOnPushButtons())
    {
        opt.iconSize = QSize(KIconLoader::global()->currentSize(K3Icon::Small), KIconLoader::global()->currentSize(K3Icon::Small));
    }
    opt.rect = pluginDelegate->aboutButtonRect(opt);

    pluginDelegate->setIconSize(pluginDelegate->getSeparatorPixels() + qMax(titleMetrics.height(), opt.rect.height()) + currentMetrics.height(),
                                pluginDelegate->getSeparatorPixels() + qMax(titleMetrics.height(), opt.rect.height()) + currentMetrics.height());

    QObject::connect(pluginModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(emitChanged()));
    QObject::connect(pluginDelegate, SIGNAL(configCommitted(QByteArray)), this, SIGNAL(configCommitted(QByteArray)));
}

KatePluginSelector::Private::~Private()
{
    delete pluginModel;
    delete pluginDelegate;
}

void KatePluginSelector::Private::checkIfShowIcons(const QList<KPluginInfo> &pluginInfoList)
{
    foreach (KPluginInfo pluginInfo, pluginInfoList)
    {
        if (!KIconLoader::global()->iconPath(pluginInfo.icon(), K3Icon::NoGroup, true).isNull())
        {
            showIcons = true;
            return;
        }
    }
}

void KatePluginSelector::Private::emitChanged()
{
    emit changed(true);
}


// =============================================================


KatePluginSelector::Private::DependenciesWidget::DependenciesWidget(QWidget *parent)
    : QWidget(parent)
    , addedByDependencies(0)
    , removedByDependencies(0)
{
    setVisible(false);

    details = new QLabel();

    QHBoxLayout *layout = new QHBoxLayout;

    QVBoxLayout *dataLayout = new QVBoxLayout;
    dataLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->setAlignment(Qt::AlignLeft);
    QLabel *label = new QLabel();
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setPixmap(KIconLoader::global()->loadIcon("dialog-information", K3Icon::Dialog));
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(label);
    KUrlLabel *link = new KUrlLabel();
    link->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    link->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    link->setGlowEnabled(false);
    link->setUnderline(false);
    link->setFloatEnabled(true);
    link->setUseCursor(false);
    link->setHighlightedColor(palette().color(QPalette::Link));
    link->setSelectedColor(palette().color(QPalette::Link));
    link->setText(i18n("Automatic changes have been performed due to plugin dependencies"));
    dataLayout->addWidget(link);
    dataLayout->addWidget(details);
    layout->addLayout(dataLayout);
    setLayout(layout);

    QObject::connect(link, SIGNAL(leftClickedUrl()), this, SLOT(showDependencyDetails()));
}

KatePluginSelector::Private::DependenciesWidget::~DependenciesWidget()
{
}

void KatePluginSelector::Private::DependenciesWidget::addDependency(const QString &dependency, const QString &pluginCausant, bool added)
{
    if (!isVisible())
        setVisible(true);

    struct FurtherInfo furtherInfo;
    furtherInfo.added = added;
    furtherInfo.pluginCausant = pluginCausant;

    if (dependencyMap.contains(dependency)) // The dependency moved from added to removed or vice-versa
    {
        if (added && removedByDependencies)
            removedByDependencies--;
        else if (addedByDependencies)
            addedByDependencies--;

        dependencyMap[dependency] = furtherInfo;
    }
    else
        dependencyMap.insert(dependency, furtherInfo);

    if (added)
        addedByDependencies++;
    else
        removedByDependencies++;

    updateDetails();
}

void KatePluginSelector::Private::DependenciesWidget::userOverrideDependency(const QString &dependency)
{
    if (dependencyMap.contains(dependency))
    {
        if (addedByDependencies && dependencyMap[dependency].added)
            addedByDependencies--;
        else if (removedByDependencies)
            removedByDependencies--;

        dependencyMap.remove(dependency);
    }

    updateDetails();
}

void KatePluginSelector::Private::DependenciesWidget::clearDependencies()
{
    addedByDependencies = 0;
    removedByDependencies = 0;
    dependencyMap.clear();
    updateDetails();
}

void KatePluginSelector::Private::DependenciesWidget::showDependencyDetails()
{
    QString message = i18n("Automatically changes have been performed in order to satisfy plugin dependencies:\n");
    foreach(const QString &dependency, dependencyMap.keys())
    {
        if (dependencyMap[dependency].added)
            message += i18n("\n    %1 plugin has been automatically checked because the dependency of %2 plugin", dependency, dependencyMap[dependency].pluginCausant);
        else
            message += i18n("\n    %1 plugin has been automatically unchecked becase its dependency on %2 plugin", dependency, dependencyMap[dependency].pluginCausant);
    }
    KMessageBox::information(0, message, i18n("Dependency Check"));

    addedByDependencies = 0;
    removedByDependencies = 0;
    updateDetails();
}

void KatePluginSelector::Private::DependenciesWidget::updateDetails()
{
    if (!dependencyMap.count())
    {
        setVisible(false);
        return;
    }

    QString message;

    if (addedByDependencies)
        message += i18np("%1 plugin added", "%1 plugins added", addedByDependencies);

    if (removedByDependencies && !message.isEmpty())
        message += i18n(", ");

    if (removedByDependencies)
        message += i18np("%1 plugin removed", "%1 plugins removed", removedByDependencies);

    if (!message.isEmpty())
        message += i18n(" since the last time you asked for details");

    if (message.isEmpty())
        details->setVisible(false);
    else
    {
        details->setVisible(true);
        details->setText(message);
    }
}


// =============================================================


KatePluginSelector::Private::QListViewSpecialized::QListViewSpecialized(QWidget *parent)
    : QListView(parent)
{
    setMouseTracking(true);
    setSpacing(0);
}

KatePluginSelector::Private::QListViewSpecialized::~QListViewSpecialized()
{
}

QStyleOptionViewItem KatePluginSelector::Private::QListViewSpecialized::viewOptions() const
{
    return QListView::viewOptions();
}


// =============================================================


KatePluginSelector::Private::PluginModel::PluginModel(KatePluginSelector::Private *parent)
    : QAbstractListModel()
    , parent(parent)
{
}

KatePluginSelector::Private::PluginModel::~PluginModel()
{
}

void KatePluginSelector::Private::PluginModel::appendPluginList(const KPluginInfo::List &pluginInfoList,
                                                             const QString &categoryName,
                                                             const QString &categoryKey,
                                                             const KConfigGroup &configGroup,
                                                             PluginLoadMethod pluginLoadMethod,
                                                             AddMethod addMethod)
{
    QString myCategoryKey = categoryKey.toLower();

    if (!pluginInfoByCategory.contains(categoryName))
    {
        pluginInfoByCategory.insert(categoryName, KPluginInfo::List());
    }

    KConfigGroup providedConfigGroup;
    int addedPlugins = 0;
    bool alternateColor = pluginCount.contains(categoryName) ? ((pluginCount[categoryName] % 2) != 0) : false;
    foreach (KPluginInfo pluginInfo, pluginInfoList)
    {
        if (!pluginInfo.isHidden() &&
             ((myCategoryKey.isEmpty()) ||
              (pluginInfo.category().toLower() == myCategoryKey)))
        {
            if ((pluginLoadMethod == ReadConfigFile) && !pluginInfo.config().isValid())
                pluginInfo.load(configGroup);
            else if (pluginLoadMethod == ReadConfigFile)
            {
                providedConfigGroup = pluginInfo.config();
                pluginInfo.load();
            }

            pluginInfoByCategory[categoryName].append(pluginInfo);

            struct AdditionalInfo pluginAdditionalInfo;

            if (pluginInfo.isPluginEnabled())
                pluginAdditionalInfo.itemChecked = Qt::Checked;
            else
                pluginAdditionalInfo.itemChecked = Qt::Unchecked;

            pluginAdditionalInfo.alternateColor = alternateColor;

            pluginAdditionalInfo.configGroup = pluginInfo.config().isValid() ? providedConfigGroup : configGroup;
            pluginAdditionalInfo.addMethod = addMethod;

            additionalInfo.insert(pluginInfo, pluginAdditionalInfo);

            addedPlugins++;
            alternateColor = !alternateColor;
        }
    }

    if (addedPlugins)
    {
        if (pluginCount.contains(categoryName))
        {
            pluginCount[categoryName] += addedPlugins;
        }
        else
        {
            pluginCount.insert(categoryName, addedPlugins);
        }
    }
    else if (!pluginInfoByCategory[categoryName].count())
    {
        pluginInfoByCategory.remove(categoryName);
    }
}

bool KatePluginSelector::Private::PluginModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !value.isValid() || index.row() >= rowCount())
        return false;

    switch (role)
    {
        case PluginDelegate::Checked:
            if (value.toBool())
                additionalInfo[*static_cast<KPluginInfo*>(index.internalPointer())].itemChecked = Qt::Checked;
            else
                additionalInfo[*static_cast<KPluginInfo*>(index.internalPointer())].itemChecked = Qt::Unchecked;
            break;
        default:
            return false;
    }

    emit dataChanged(index, index);

    return true;
}

QVariant KatePluginSelector::Private::PluginModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    if (index.internalPointer()) // Is a plugin item
    {
        KPluginInfo pluginInfo = *static_cast<KPluginInfo*>(index.internalPointer());

        switch (role)
        {
            case PluginDelegate::Name:
                return pluginInfo.name();
            case PluginDelegate::Comment:
                return pluginInfo.comment();
            case PluginDelegate::Icon:
                return pluginInfo.icon();
            case PluginDelegate::Author:
                return pluginInfo.author();
            case PluginDelegate::Email:
                return pluginInfo.email();
            case PluginDelegate::Category:
                return pluginInfo.category();
            case PluginDelegate::InternalName:
                return pluginInfo.pluginName();
            case PluginDelegate::Version:
                return pluginInfo.version();
            case PluginDelegate::Website:
                return pluginInfo.website();
            case PluginDelegate::License:
                return pluginInfo.license();
            case PluginDelegate::Checked:
                return additionalInfo.value(*static_cast<KPluginInfo*>(index.internalPointer())).itemChecked;
        }
    }
    else // Is a category
    {
        switch (role)
        {
            case PluginDelegate::Checked:
                return additionalInfo.value(*static_cast<KPluginInfo*>(index.internalPointer())).itemChecked;

            case Qt::DisplayRole:
                int currentPosition = 0;
                foreach (QString category, pluginInfoByCategory.keys())
                {
                    if (currentPosition == index.row())
                        return category;

                    currentPosition += pluginInfoByCategory[category].count() + 1;
                }
        }
    }

    return QVariant();
}

Qt::ItemFlags KatePluginSelector::Private::PluginModel::flags(const QModelIndex &index) const
{
    QModelIndex modelIndex = this->index(index.row(), index.column());

    if (modelIndex.internalPointer()) // Is a plugin item
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else // Is a category
        return Qt::ItemIsEnabled;
}

QModelIndex KatePluginSelector::Private::PluginModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    int currentPosition = 0;

    if ((row < 0) || (row >= rowCount()))
        return QModelIndex();

    foreach (QString category, pluginInfoByCategory.keys())
    {
        if (currentPosition == row)
            return createIndex(row, column, 0); // Is a category

        foreach (const KPluginInfo &pluginInfo, pluginInfoByCategory[category])
        {
            currentPosition++;

            if (currentPosition == row)
                return createIndex(row, column, const_cast<KPluginInfo *>(&pluginInfo)); // Is a plugin item
        }

        currentPosition++;
    }

    return QModelIndex();
}

int KatePluginSelector::Private::PluginModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    int retValue = pluginInfoByCategory.count(); // We have pluginInfoCategory.count() categories

    foreach (QString category, pluginInfoByCategory.keys())
    {
        if (pluginCount.contains(category))
            retValue += pluginCount[category];
    }

    return retValue;
}

QList<KService::Ptr> KatePluginSelector::Private::PluginModel::services(const QModelIndex &index) const
{
    if (index.internalPointer()) // Is a plugin item
    {
        const KPluginInfo pluginInfo = *static_cast<KPluginInfo*>(index.internalPointer());

        return pluginInfo.kcmServices();
    }

    return QList<KService::Ptr>(); // We were asked for a category
}

KConfigGroup KatePluginSelector::Private::PluginModel::configGroup(const QModelIndex &index) const
{
    return additionalInfo.value(*static_cast<KPluginInfo*>(index.internalPointer())).configGroup;
}

void KatePluginSelector::Private::PluginModel::setParentComponents(const QModelIndex &index, const QStringList &parentComponents)
{
    additionalInfo[*static_cast<KPluginInfo*>(index.internalPointer())].parentComponents = parentComponents;
}

QStringList KatePluginSelector::Private::PluginModel::parentComponents(const QModelIndex &index) const
{
    return additionalInfo.value(*static_cast<KPluginInfo*>(index.internalPointer())).parentComponents;
}

void KatePluginSelector::Private::PluginModel::updateDependencies(const QString &dependency, const QString &pluginCausant, CheckWhatDependencies whatDependencies, QStringList &dependenciesPushed)
{
    QModelIndex theIndex;
    if (whatDependencies == DependenciesINeed)
    {
        for (int i = 0; i < rowCount(); i++)
        {
            theIndex = index(i, 0);

            if (data(theIndex, PluginDelegate::InternalName).toString() == dependency)
            {
                const KPluginInfo pluginInfo(*static_cast<const KPluginInfo*>(theIndex.internalPointer()));

                if (!data(theIndex, PluginDelegate::Checked).toBool())
                {
                    parent->dependenciesWidget->addDependency(pluginInfo.name(), pluginCausant, true);

                    setData(theIndex, true, PluginDelegate::Checked);
                    dependenciesPushed.append(pluginInfo.name());
                }

                foreach(const QString &indirectDependency, pluginInfo.dependencies())
                {
                    updateDependencies(indirectDependency, pluginInfo.name(), whatDependencies, dependenciesPushed);
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < rowCount(); i++)
        {
            theIndex = index(i, 0);

            if (theIndex.internalPointer())
            {
                const KPluginInfo pluginInfo(*static_cast<const KPluginInfo*>(theIndex.internalPointer()));

                if (pluginInfo.dependencies().contains(dependency))
                {
                    if (data(theIndex, PluginDelegate::Checked).toBool())
                    {
                        parent->dependenciesWidget->addDependency(pluginInfo.name(), pluginCausant, false);

                        setData(theIndex, false, PluginDelegate::Checked);
                        dependenciesPushed.append(pluginInfo.name());
                    }

                    updateDependencies(pluginInfo.pluginName(), pluginCausant, whatDependencies, dependenciesPushed);
                }
            }
        }
    }
}

KatePluginSelector::Private::PluginModel::AddMethod KatePluginSelector::Private::PluginModel::addMethod(const KPluginInfo &pluginInfo) const
{
    return additionalInfo[pluginInfo].addMethod;
}

bool KatePluginSelector::Private::PluginModel::alternateColor(const KPluginInfo &pluginInfo) const
{
    return additionalInfo[pluginInfo].alternateColor;
}


// =============================================================

KatePluginSelector::KatePluginSelector(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    QObject::connect(d, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
    QObject::connect(d, SIGNAL(configCommitted(QByteArray)), this, SIGNAL(configCommitted(QByteArray)));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    setLayout(layout);

    d->listView = new Private::QListViewSpecialized();
    d->listView->setVerticalScrollMode(QListView::ScrollPerPixel);

    d->listView->setModel(d->pluginModel);
    d->listView->setItemDelegate(d->pluginDelegate);

    d->listView->viewport()->installEventFilter(d->pluginDelegate);
    d->listView->installEventFilter(d->pluginDelegate);

    d->dependenciesWidget = new Private::DependenciesWidget;

    layout->addWidget(d->listView);
    layout->addWidget(d->dependenciesWidget);
}

KatePluginSelector::~KatePluginSelector()
{
    delete d;
}

void KatePluginSelector::addPlugins(const QString &componentName,
                                 const QString &categoryName,
                                 const QString &categoryKey,
                                 KSharedConfig::Ptr config)
{
    QStringList desktopFileNames = KGlobal::dirs()->findAllResources("data",
        componentName + "/kpartplugins/*.desktop", KStandardDirs::Recursive);

    QList<KPluginInfo> pluginInfoList = KPluginInfo::fromFiles(desktopFileNames);

    if (pluginInfoList.isEmpty())
        return;

    Q_ASSERT(config);
    if (!config)
        config = KSharedConfig::openConfig(componentName);

    KConfigGroup *cfgGroup = new KConfigGroup(config, "KParts Plugins");
    kDebug( 702 ) << "cfgGroup = " << cfgGroup;

    d->checkIfShowIcons(pluginInfoList);

    d->pluginModel->appendPluginList(pluginInfoList, categoryName, categoryKey, *cfgGroup);
}

void KatePluginSelector::addPlugins(const KComponentData &instance,
                                 const QString &categoryName,
                                 const QString &categoryKey,
                                 const KSharedConfig::Ptr &config)
{
    addPlugins(instance.componentName(), categoryName, categoryKey, config);
}

void KatePluginSelector::addPlugins(const QList<KPluginInfo> &pluginInfoList,
                                 PluginLoadMethod pluginLoadMethod,
                                 const QString &categoryName,
                                 const QString &categoryKey,
                                 const KSharedConfig::Ptr &config)
{
    if (pluginInfoList.isEmpty())
        return;

    KConfigGroup *cfgGroup = new KConfigGroup(config ? config : KGlobal::config(), "Plugins");
    kDebug( 702 ) << "cfgGroup = " << cfgGroup;

    d->checkIfShowIcons(pluginInfoList);

    d->pluginModel->appendPluginList(pluginInfoList, categoryName, categoryKey, *cfgGroup, pluginLoadMethod, Private::PluginModel::ManuallyAdded);
}

void KatePluginSelector::load()
{
    QModelIndex currentIndex;
    for (int i = 0; i < d->pluginModel->rowCount(); i++)
    {
        currentIndex = d->pluginModel->index(i, 0);
        if (currentIndex.internalPointer())
        {
            KPluginInfo currentPlugin(*static_cast<KPluginInfo*>(currentIndex.internalPointer()));

            currentPlugin.load(d->pluginModel->configGroup(currentIndex));

            d->pluginModel->setData(currentIndex, currentPlugin.isPluginEnabled(), Private::PluginDelegate::Checked);
        }
    }

    emit changed(false);
}

void KatePluginSelector::save()
{
    QModelIndex currentIndex;
    KConfigGroup configGroup;
    for (int i = 0; i < d->pluginModel->rowCount(); i++)
    {
        currentIndex = d->pluginModel->index(i, 0);
        if (currentIndex.internalPointer())
        {
            KPluginInfo currentPlugin(*static_cast<KPluginInfo*>(currentIndex.internalPointer()));
            currentPlugin.setPluginEnabled(d->pluginModel->data(currentIndex, Private::PluginDelegate::Checked).toBool());

            configGroup = d->pluginModel->configGroup(currentIndex);

            currentPlugin.save(configGroup);

            configGroup.sync();
        }
    }

    d->dependenciesWidget->clearDependencies();
}

void KatePluginSelector::defaults()
{
    QModelIndex currentIndex;
    for (int i = 0; i < d->pluginModel->rowCount(); i++)
    {
        currentIndex = d->pluginModel->index(i, 0);
        if (currentIndex.internalPointer())
        {
            KPluginInfo currentPlugin(*static_cast<KPluginInfo*>(currentIndex.internalPointer()));
            currentPlugin.defaults();
            // Avoid emit the changed signal when possible. Probably all items are in their default value and nothing changed
            if (d->pluginModel->data(currentIndex, Private::PluginDelegate::Checked).toBool() != currentPlugin.isPluginEnabled())
            {
                d->pluginModel->setData(currentIndex, currentPlugin.isPluginEnabled(), Private::PluginDelegate::Checked);
            }
        }
    }
}

void KatePluginSelector::updatePluginsState()
{
    QModelIndex currentIndex;
    for (int i = 0; i < d->pluginModel->rowCount(); i++)
    {
        currentIndex = d->pluginModel->index(i, 0);
        if (currentIndex.internalPointer())
        {
            KPluginInfo currentPlugin(*static_cast<KPluginInfo*>(currentIndex.internalPointer()));

            // Only the items that were added "manually" will be updated, since the others
            // are not visible from the outside
            if (d->pluginModel->addMethod(currentPlugin) == Private::PluginModel::ManuallyAdded)
                currentPlugin.setPluginEnabled(d->pluginModel->data(currentIndex, Private::PluginDelegate::Checked).toBool());
        }
    }
}


// =============================================================


KatePluginSelector::Private::PluginDelegate::PluginDelegate(KatePluginSelector::Private *parent)
    : QItemDelegate(0)
    , focusedElement(CheckBoxFocused)
    , sunkenButton(false)
    , currentModuleProxyList(0)
    , configDialog(0)
    , parent(parent)
{
    iconLoader = new KIconLoader();
}

KatePluginSelector::Private::PluginDelegate::~PluginDelegate()
{
    qDeleteAll(configDialogs);
    qDeleteAll(aboutDialogs);

    delete iconLoader;
}

void KatePluginSelector::Private::PluginDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QStyleOptionViewItem optionCopy(option);
    const PluginModel *model = static_cast<const PluginModel*>(index.model());

    QRect theCheckRect = checkRect(index, optionCopy);
    QFontMetrics fontMetrics = painter->fontMetrics();

    QColor unselectedTextColor = optionCopy.palette.text().color();
    QPen currentPen = painter->pen();
    QPen linkPen = QPen(option.palette.color(QPalette::Link));

    QPixmap iconPixmap;

    if (parent->showIcons)
    {
        iconPixmap = icon(index, iconWidth, iconHeight);
    }

    QFont title(painter->font());
    QFont previousFont(painter->font());
    QFont configureFont(painter->font());

    title.setPointSize(title.pointSize() + 2);
    title.setWeight(QFont::Bold);

    if (index.internalPointer())
    {
        const KPluginInfo info(*static_cast<KPluginInfo*>(index.internalPointer()));

        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(optionCopy.rect, optionCopy.palette.color(QPalette::Highlight));
        }
        else
        {
            if (model->alternateColor(info))
                painter->fillRect(optionCopy.rect, optionCopy.palette.color(QPalette::AlternateBase));
            else
                painter->fillRect(optionCopy.rect, optionCopy.palette.color(QPalette::Base));
        }

        QString display;
        QString secondaryDisplay = fontMetrics.elidedText(comment(index), Qt::ElideRight, optionCopy.rect.width() - leftMargin - rightMargin - iconPixmap.width() - separatorPixels * 2 - theCheckRect.width());

        QPen prevPen(painter->pen());

        // Draw settings button

        if (model->services(index).size()) // has configuration dialog
        {
            QStyleOptionButton opt;

            opt.state |= QStyle::State_Enabled;
            if (KGlobalSettings::showIconsOnPushButtons())
            {
                opt.icon = iconLoader->loadIcon("configure", K3Icon::Small);
                opt.iconSize = QSize(iconLoader->currentSize(K3Icon::Small), iconLoader->currentSize(K3Icon::Small));
            }
            opt.text = details;
            opt.fontMetrics = option.fontMetrics;
            opt.direction = option.direction;
            opt.rect = option.rect;
            opt.rect = settingsButtonRect(opt);

            if (opt.rect.contains(relativeMousePosition))
            {
                opt.state |= QStyle::State_MouseOver;

                if (sunkenButton)
                {
                    opt.state |= QStyle::State_Sunken | QStyle::State_HasFocus;
                }
            }
            else if ((focusedElement == SettingsButtonFocused) && (option.state & QStyle::State_Selected) &&
                     (option.state & QStyle::State_HasFocus))
            {
                opt.state |= QStyle::State_HasFocus;
            }

            KApplication::style()->drawControl(QStyle::CE_PushButton, &opt, painter);
        }

        // Finish drawing settings button

        // Draw about button

        QStyleOptionButton opt;

        opt.state |= QStyle::State_Enabled;
        if (KGlobalSettings::showIconsOnPushButtons())
        {
            opt.icon = iconLoader->loadIcon("dialog-information", K3Icon::Small);
            opt.iconSize = QSize(iconLoader->currentSize(K3Icon::Small), iconLoader->currentSize(K3Icon::Small));
        }
        opt.text = about;
        opt.fontMetrics = option.fontMetrics;
        opt.direction = option.direction;
        opt.rect = option.rect;
        opt.rect = aboutButtonRect(opt);

        QStyleOptionButton opt2(opt);
        opt2.text = details;
        opt2.rect = option.rect;
        opt2.rect = settingsButtonRect(opt2);

        if (opt.rect.contains(relativeMousePosition))
        {
            opt.state |= QStyle::State_MouseOver;

            if (sunkenButton)
            {
                opt.state |= QStyle::State_Sunken | QStyle::State_HasFocus;
            }
        }
        else if ((focusedElement == AboutButtonFocused) && (option.state & QStyle::State_Selected) &&
                    (option.state & QStyle::State_HasFocus))
        {
            opt.state |= QStyle::State_HasFocus;
        }

        KApplication::style()->drawControl(QStyle::CE_PushButton, &opt, painter);

        // Finish drawing about button

        QStyleOptionViewItem otherOption(optionCopy);
        otherOption.font = title;
        otherOption.fontMetrics = QFontMetrics(title);

        display = otherOption.fontMetrics.elidedText(name(index), Qt::ElideRight, otherOption.rect.width() - leftMargin - rightMargin - iconPixmap.width() - separatorPixels * 3 - theCheckRect.width() - opt.rect.width() -
                                                                                  (dynamic_cast<const PluginModel *>(index.model())->services(index).count() ? opt2.rect.width()
                                                                                                                                                             : 0));


        if (option.state & QStyle::State_Selected)
        {
            painter->setPen(optionCopy.palette.color(QPalette::HighlightedText));
        }

        painter->setFont(title);
        painter->drawText(option.direction == Qt::LeftToRight ? leftMargin + separatorPixels * 2 + iconPixmap.width() + theCheckRect.width()
                                                              : option.rect.right() - rightMargin - separatorPixels * 2 - iconPixmap.width() - theCheckRect.width() - painter->fontMetrics().width(display), separatorPixels + optionCopy.rect.top(), painter->fontMetrics().width(display), painter->fontMetrics().height(), Qt::AlignLeft, display);

        painter->setFont(previousFont);

        painter->drawText(option.direction == Qt::LeftToRight ? leftMargin + separatorPixels * 2 + iconPixmap.width() + theCheckRect.width()
                                                              : option.rect.right() - rightMargin - separatorPixels * 2 - iconPixmap.width() - theCheckRect.width() - painter->fontMetrics().width(secondaryDisplay), optionCopy.rect.height() - separatorPixels - fontMetrics.height() + optionCopy.rect.top(), fontMetrics.width(secondaryDisplay), fontMetrics.height(), Qt::AlignLeft, secondaryDisplay);

        painter->drawPixmap(option.direction == Qt::LeftToRight ? leftMargin + separatorPixels + theCheckRect.width()
                                                                : option.rect.right() - rightMargin - separatorPixels - theCheckRect.width() - iconPixmap.width(), calculateVerticalCenter(optionCopy.rect, iconPixmap.height()) + optionCopy.rect.top(), iconPixmap);

        QStyleOptionButton optionCheck;

        optionCheck.direction = option.direction;
        optionCheck.rect = checkRect(index, optionCopy);
        optionCheck.state |= QStyle::State_Enabled;

        if (checkRect(index, optionCopy).contains(relativeMousePosition))
        {
            optionCheck.state |= QStyle::State_MouseOver;
        }
        else if ((focusedElement == CheckBoxFocused) && (option.state & QStyle::State_Selected) &&
                 (option.state & QStyle::State_HasFocus))
        {
            optionCheck.state |= QStyle::State_HasFocus;
        }

        optionCheck.state |= (((Qt::CheckState) index.model()->data(index, Checked).toInt()) == Qt::Checked) ?
                             QStyle::State_On : QStyle::State_Off;

        KApplication::style()->drawControl(QStyle::CE_CheckBox, &optionCheck, painter);
    }
    else // we are drawing a category
    {
        QString display = painter->fontMetrics().elidedText(index.model()->data(index, Qt::DisplayRole).toString(), Qt::ElideRight, optionCopy.rect.width() - leftMargin - rightMargin);

        QStyleOptionButton opt;

        opt.rect = QRect(leftMargin, separatorPixels + optionCopy.rect.top(), optionCopy.rect.width() - leftMargin - rightMargin, painter->fontMetrics().height());
        opt.palette = optionCopy.palette;
        opt.direction = optionCopy.direction;
        opt.text = display;

        QFont painterFont = painter->font();
        painterFont.setWeight(QFont::Bold);
        painterFont.setPointSize(painterFont.pointSize() + 2);
        QFontMetrics metrics(painterFont);
        painter->setFont(painterFont);

        opt.fontMetrics = painter->fontMetrics();

        QRect auxRect(optionCopy.rect.left() + leftMargin,
                      optionCopy.rect.bottom() - 2,
                      optionCopy.rect.width() - leftMargin - rightMargin,
                      2);

        QPainterPath path;
        path.addRect(auxRect);

        QLinearGradient gradient(optionCopy.rect.topLeft(),
                                 optionCopy.rect.bottomRight());

        gradient.setColorAt(0, option.direction == Qt::LeftToRight ? optionCopy.palette.text().color()
                                                                   : Qt::transparent);
        gradient.setColorAt(1, option.direction == Qt::LeftToRight ? Qt::transparent
                                                                   : optionCopy.palette.text().color());

        painter->setBrush(gradient);
        painter->fillPath(path, gradient);

        QRect auxRect2(optionCopy.rect.left() + leftMargin,
                       option.rect.top(),
                       optionCopy.rect.width() - leftMargin - rightMargin,
                       option.rect.height());

        painter->drawText(auxRect2, Qt::AlignVCenter | Qt::AlignLeft,
                          display);
    }

    painter->restore();
}

QSize KatePluginSelector::Private::PluginDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    QFont title(option.font);
    title.setPointSize(title.pointSize() + 2);
    title.setWeight(QFont::Bold);

    QFontMetrics titleMetrics(title);
    QFontMetrics currentMetrics(option.font);

    if (index.internalPointer())
    {
        QStyleOptionButton opt;
        opt.text = "foo"; // height() will be checked, and that does not depend on the string
        opt.fontMetrics = option.fontMetrics;
        if (KGlobalSettings::showIconsOnPushButtons())
        {
            opt.iconSize = QSize(iconLoader->currentSize(K3Icon::Small), iconLoader->currentSize(K3Icon::Small));
        }
        opt.rect = option.rect;
        opt.rect = aboutButtonRect(opt);

        return QSize(minimumItemWidth, qMax((separatorPixels * 2) + iconHeight, (separatorPixels * 3) + qMax(titleMetrics.height(), opt.rect.height()) + currentMetrics.height()));
    }

    return QSize(minimumItemWidth, separatorPixels + titleMetrics.height() + 2);
}

void KatePluginSelector::Private::PluginDelegate::setIconSize(int width, int height)
{
    this->iconWidth = width;
    this->iconHeight = height;
}

void KatePluginSelector::Private::PluginDelegate::setMinimumItemWidth(int minimumItemWidth)
{
    this->minimumItemWidth = minimumItemWidth;
}

void KatePluginSelector::Private::PluginDelegate::setLeftMargin(int leftMargin)
{
    this->leftMargin = leftMargin;
}

void KatePluginSelector::Private::PluginDelegate::setRightMargin(int rightMargin)
{
    this->rightMargin = rightMargin;
}

int KatePluginSelector::Private::PluginDelegate::getSeparatorPixels() const
{
    return separatorPixels;
}

void KatePluginSelector::Private::PluginDelegate::setSeparatorPixels(int separatorPixels)
{
    this->separatorPixels = separatorPixels;
}

QRect KatePluginSelector::Private::PluginDelegate::aboutButtonRect(const QStyleOptionButton &option) const
{
    QRect retRect;

    const QString &caption = option.text;
    const QFontMetrics &fontMetrics = option.fontMetrics;

    retRect.setHeight(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).height());
    retRect.setWidth(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).width());
    if (option.direction == Qt::LeftToRight)
    {
        retRect.setLeft(option.rect.right() - rightMargin - retRect.width());
    }
    else
    {
        retRect.setLeft(leftMargin);
    }
    retRect.setTop(option.rect.top() + separatorPixels);
    retRect.setHeight(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).height());
    retRect.setWidth(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).width());

    return retRect;
}

QRect KatePluginSelector::Private::PluginDelegate::settingsButtonRect(const QStyleOptionButton &option) const
{
    QRect retRect;

    const QString &caption = option.text;
    const QFontMetrics &fontMetrics = option.fontMetrics;

    QStyleOptionButton aboutOption(option);
    aboutOption.text = about;

    retRect.setHeight(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).height());
    retRect.setWidth(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).width());
    if (option.direction == Qt::LeftToRight)
    {
        retRect.setLeft(option.rect.right() - rightMargin - aboutButtonRect(aboutOption).width() - retRect.width() - separatorPixels);
    }
    else
    {
        retRect.setLeft(leftMargin + aboutButtonRect(aboutOption).width() + separatorPixels);
    }
    retRect.setTop(option.rect.top() + separatorPixels);
    retRect.setHeight(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).height());
    retRect.setWidth(KApplication::style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(fontMetrics.width(caption) + option.iconSize.width(), qMax(fontMetrics.height(), option.iconSize.height()))).width());

    return retRect;
}

bool KatePluginSelector::Private::PluginDelegate::eventFilter(QObject *watched, QEvent *event)
{
    if ((event->type() == QEvent::KeyRelease))
    {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);

        if (keyEvent && keyEvent->key() == Qt::Key_Space)
        {
            sunkenButton = false;

            QWidget *viewport = qobject_cast<QWidget*>(watched);

            if (viewport)
            {
                QModelIndex currentIndex;

                QListViewSpecialized *listView = dynamic_cast<QListViewSpecialized*>(viewport->parent());
                if (!listView) // the keyboard event comes directly from the view, not the viewport
                    listView = dynamic_cast<QListViewSpecialized*>(viewport);

                currentIndex = listView->currentIndex();

                if (listView && currentIndex.isValid())
                {
                    QStyleOptionViewItem optionViewItem(listView->viewOptions());
                    optionViewItem.rect = listView->visualRect(currentIndex);

                    if (currentIndex.internalPointer()) {
                        updateCheckState(currentIndex, optionViewItem,
                                         viewport->mapFromGlobal(QCursor::pos()), listView, KeyboardEvent);

                        return false;
                    }
                }
            }

            return false;
        }

        return false;
    }
    else if (event->type() == QEvent::KeyPress ||
             event->type() == QEvent::MouseButtonRelease)
    {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);

        if (keyEvent && (keyEvent->key() != Qt::Key_Space) &&
                        (keyEvent->key() != Qt::Key_Tab) &&
                        (keyEvent->key() != Qt::Key_Up) &&
                        (keyEvent->key() != Qt::Key_Down) &&
                        (keyEvent->key() != Qt::Key_Backtab))
        {
            return false;
        }

        EventReceived eventReceived;
        if (event->type() == QEvent::MouseButtonRelease)
        {
            eventReceived = MouseEvent;
            sunkenButton = false;
        }
        else
        {
            eventReceived = KeyboardEvent;
        }

        QWidget *viewport = qobject_cast<QWidget*>(watched);

        if (viewport)
        {
            QModelIndex currentIndex;

            QListViewSpecialized *listView = dynamic_cast<QListViewSpecialized*>(viewport->parent());
            if (!listView) // the keyboard event comes directly from the view, not the viewport
                listView = dynamic_cast<QListViewSpecialized*>(viewport);

            if ((eventReceived == MouseEvent) && listView)
            {
                currentIndex = listView->indexAt(viewport->mapFromGlobal(QCursor::pos()));

                focusedElement = CheckBoxFocused;
            }
            else if ((eventReceived == KeyboardEvent) && listView)
            {
                currentIndex = listView->currentIndex();
            }

            if (keyEvent && keyEvent->key() == Qt::Key_Up)
            {
                if (currentIndex.row() && listView->model()->index(currentIndex.row() - 1, 0).internalPointer())
                    listView->setCurrentIndex(listView->model()->index(currentIndex.row() - 1, 0));
                else if (currentIndex.row() > 2)
                    listView->setCurrentIndex(listView->model()->index(currentIndex.row() - 2, 0));
                else
                    listView->setCurrentIndex(QModelIndex());

                focusedElement = CheckBoxFocused;

                return true;
            }
            else if (keyEvent && keyEvent->key() == Qt::Key_Down)
            {
                if ((currentIndex.row() < listView->model()->rowCount()) && listView->model()->index(currentIndex.row() + 1, 0).internalPointer())
                    listView->setCurrentIndex(listView->model()->index(currentIndex.row() + 1, 0));
                else if (currentIndex.row() + 1 < listView->model()->rowCount())
                    listView->setCurrentIndex(listView->model()->index(currentIndex.row() + 2, 0));
                else
                    listView->setCurrentIndex(QModelIndex());

                focusedElement = CheckBoxFocused;

                return true;
            }
            else if (keyEvent && keyEvent->key() == Qt::Key_Space)
            {
                sunkenButton = true;
                listView->update(listView->currentIndex());

                return true;
            }
            else if (keyEvent && (keyEvent->key() == Qt::Key_Tab))
            {
                if ((focusedElement == CheckBoxFocused) &&
                    (!(dynamic_cast<PluginModel*>(listView->model())->services(currentIndex).count())))
                {
                    focusedElement = (FocusedElement) ((focusedElement + 1) % 3);
                }

                focusedElement = (FocusedElement) ((focusedElement + 1) % 3);

                if (!focusedElement)
                {
                    if ((currentIndex.row() < listView->model()->rowCount()) && listView->model()->index(currentIndex.row() + 1, 0).internalPointer())
                        listView->setCurrentIndex(listView->model()->index(currentIndex.row() + 1, 0));
                    else if (currentIndex.row() + 1 < listView->model()->rowCount())
                        listView->setCurrentIndex(listView->model()->index(currentIndex.row() + 2, 0));
                    else
                    {
                        listView->setCurrentIndex(QModelIndex());
                        return false;
                    }
                }

                listView->update(listView->currentIndex());

                return true;
            }
            else if (keyEvent && (keyEvent->key() == Qt::Key_Backtab))
            {
                if ((focusedElement == AboutButtonFocused) &&
                    (!(dynamic_cast<PluginModel*>(listView->model())->services(currentIndex).count())))
                {
                    focusedElement = (FocusedElement) (focusedElement - 1);
                }

                focusedElement = (FocusedElement) (focusedElement - 1);

                if (focusedElement == -1)
                {
                    focusedElement = AboutButtonFocused;

                    if (currentIndex.row() && listView->model()->index(currentIndex.row() - 1, 0).internalPointer())
                        listView->setCurrentIndex(listView->model()->index(currentIndex.row() - 1, 0));
                    else if (currentIndex.row() > 2)
                        listView->setCurrentIndex(listView->model()->index(currentIndex.row() - 2, 0));
                    else
                    {
                        listView->setCurrentIndex(QModelIndex());
                        return false;
                    }
                }

                listView->update(listView->currentIndex());

                return true;
            }

            if (listView && currentIndex.isValid())
            {
                QStyleOptionViewItem optionViewItem(listView->viewOptions());
                optionViewItem.rect = listView->visualRect(currentIndex);

                if (currentIndex.internalPointer()) {
                    updateCheckState(currentIndex, optionViewItem,
                                     viewport->mapFromGlobal(QCursor::pos()), listView, eventReceived);
                }
            }

            return true;
        }

        return false;
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        sunkenButton = true;

        if (QWidget *viewport = qobject_cast<QWidget*>(watched))
        {
            QListViewSpecialized *listView = dynamic_cast<QListViewSpecialized*>(viewport->parent());
            if (!listView) // the keyboard event comes directly from the view, not the viewport
                listView = dynamic_cast<QListViewSpecialized*>(viewport);

            if (!listView)
                return false;

            listView->update(listView->currentIndex());

            QModelIndex currentIndex = listView->indexAt(viewport->mapFromGlobal(QCursor::pos()));

            if (!currentIndex.isValid() || !currentIndex.internalPointer())
            {
                return false;
            }

            QStyleOptionButton opt;
            opt.text = about;
            opt.fontMetrics = listView->viewOptions().fontMetrics;
            opt.direction = listView->layoutDirection();
            if (KGlobalSettings::showIconsOnPushButtons())
            {
                opt.iconSize = QSize(KIconLoader::global()->currentSize(K3Icon::Small), KIconLoader::global()->currentSize(K3Icon::Small));
            }
            opt.rect = listView->visualRect(currentIndex);
            opt.rect = aboutButtonRect(opt);

            QStyleOptionButton opt2(opt);
            opt2.text = details;
            opt2.rect = listView->visualRect(currentIndex);
            opt2.rect = settingsButtonRect(opt2);

            QStyleOptionViewItem otherOpt;
            otherOpt.fontMetrics = listView->viewOptions().fontMetrics;
            otherOpt.direction = listView->layoutDirection();
            otherOpt.rect = listView->visualRect(currentIndex);

            if (opt.rect.contains(viewport->mapFromGlobal(QCursor::pos())) ||
                checkRect(currentIndex, otherOpt).contains(viewport->mapFromGlobal(QCursor::pos())))
            {
                listView->update(currentIndex);

                return true;
            }

            if ((dynamic_cast<const PluginModel *>(currentIndex.model()))->services(currentIndex).count() &&
                opt2.rect.contains(viewport->mapFromGlobal(QCursor::pos())))
            {
                listView->update(currentIndex);

                return true;
            }
        }

        return false;
    }
    else if (event->type() == QEvent::MouseMove)
    {
        if (QWidget *viewport = qobject_cast<QWidget*>(watched))
        {
            relativeMousePosition = viewport->mapFromGlobal(QCursor::pos());
            viewport->update();

            QListViewSpecialized *listView = dynamic_cast<QListViewSpecialized*>(viewport->parent());
            if (!listView) // the keyboard event comes directly from the view, not the viewport
                listView = dynamic_cast<QListViewSpecialized*>(viewport);

            if (!listView)
                return false;

            QModelIndex currentIndex = listView->indexAt(viewport->mapFromGlobal(QCursor::pos()));

            if (!currentIndex.isValid() || !currentIndex.internalPointer())
            {
                return false;
            }

            QStyleOptionButton opt;
            opt.text = about;
            opt.fontMetrics = listView->viewOptions().fontMetrics;
            opt.direction = listView->layoutDirection();
            if (KGlobalSettings::showIconsOnPushButtons())
            {
                opt.iconSize = QSize(KIconLoader::global()->currentSize(K3Icon::Small), KIconLoader::global()->currentSize(K3Icon::Small));
            }
            opt.rect = listView->visualRect(currentIndex);
            opt.rect = aboutButtonRect(opt);

            QStyleOptionButton opt2(opt);
            opt2.text = details;
            opt2.rect = listView->visualRect(currentIndex);
            opt2.rect = settingsButtonRect(opt2);

            QStyleOptionViewItem otherOpt;
            otherOpt.fontMetrics = listView->viewOptions().fontMetrics;
            otherOpt.direction = listView->layoutDirection();
            otherOpt.rect = listView->visualRect(currentIndex);

            if (opt.rect.contains(viewport->mapFromGlobal(QCursor::pos())) ||
                checkRect(currentIndex, otherOpt).contains(viewport->mapFromGlobal(QCursor::pos())))
            {
                listView->update(currentIndex);

                return true;
            }

            if ((dynamic_cast<const PluginModel *>(currentIndex.model()))->services(currentIndex).count() &&
                opt2.rect.contains(viewport->mapFromGlobal(QCursor::pos())))
            {
                listView->update(currentIndex);

                return true;
            }

            return false;
        }
    }
    else if (event->type() == QEvent::Leave)
    {
        QWidget *viewport = qobject_cast<QWidget*>(watched);
        if (viewport)
        {
            relativeMousePosition = QPoint(0, 0);
            viewport->update();

            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonDblClick)
    {
        return true;
    }

    return false;
}

void KatePluginSelector::Private::PluginDelegate::slotDefaultClicked()
{
    if (!currentModuleProxyList)
        return;

    QList<KCModuleProxy*>::iterator it;
    for (it = currentModuleProxyList->begin(); it != currentModuleProxyList->end(); ++it)
    {
        (*it)->defaults();
    }
}


void KatePluginSelector::Private::PluginDelegate::processUrl(const QString &url) const
{
    new KRun(KUrl(url), parent->parent);
}

QRect KatePluginSelector::Private::PluginDelegate::checkRect(const QModelIndex &index, const QStyleOptionViewItem &option) const
{
    QSize canvasSize = sizeHint(option, index);
    QRect checkDimensions = KApplication::style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option);

    QRect retRect;
    retRect.setTopLeft(QPoint(option.direction == Qt::LeftToRight ? option.rect.left() + leftMargin
                                                                  : option.rect.right() - rightMargin - checkDimensions.width(),
                       ((canvasSize.height() / 2) - (checkDimensions.height() / 2)) + option.rect.top()));
    retRect.setBottomRight(QPoint(option.direction == Qt::LeftToRight ? option.rect.left() + leftMargin + checkDimensions.width()
                                                                      : option.rect.right() - rightMargin,
                           ((canvasSize.height() / 2) - (checkDimensions.height() / 2)) + option.rect.top() + checkDimensions.height()));

    return retRect;
}

void KatePluginSelector::Private::PluginDelegate::updateCheckState(const QModelIndex &index, const QStyleOptionViewItem &option,
                                                                const QPoint &cursorPos, QListView *listView, EventReceived eventReceived)
{
    if (!index.isValid())
        return;

    PluginModel *model = static_cast<PluginModel*>(listView->model());

    if (!index.internalPointer())
    {
        return;
    }

    const KPluginInfo pluginInfo(*static_cast<KPluginInfo*>(index.internalPointer()));

    if ((checkRect(index, option).contains(cursorPos) && (eventReceived == MouseEvent)) ||
        ((focusedElement == CheckBoxFocused) && (eventReceived == KeyboardEvent)))
    {
        listView->model()->setData(index, !listView->model()->data(index, Checked).toBool(), Checked);

        parent->dependenciesWidget->userOverrideDependency(pluginInfo.name());

        if (listView->model()->data(index, Checked).toBool()) // Item was checked
            checkDependencies(model, pluginInfo, DependenciesINeed);
        else
            checkDependencies(model, pluginInfo, DependenciesNeedMe);
    }

    // Option button for the about button
    QStyleOptionButton opt;
    opt.text = about;
    if (KGlobalSettings::showIconsOnPushButtons())
    {
        opt.iconSize = QSize(iconLoader->currentSize(K3Icon::Small), iconLoader->currentSize(K3Icon::Small));
    }
    opt.fontMetrics = option.fontMetrics;
    opt.direction = option.direction;
    opt.rect = option.rect;
    opt.rect = aboutButtonRect(opt);

    // Option button for the settings button
    QStyleOptionButton opt2(opt);
    opt2.text = details;
    opt2.rect = option.rect;
    opt2.rect = settingsButtonRect(opt2);

    if ((dynamic_cast<const PluginModel*>(index.model())->services(index).count()) &&
        ((opt2.rect.contains(cursorPos) && (eventReceived == MouseEvent)) ||
         ((focusedElement == SettingsButtonFocused) && (eventReceived == KeyboardEvent))))
    {
        KDialog *configDialog;

        if (!configDialogs.contains(index.row()))
        {
            QList<KService::Ptr> services = model->services(index);

            configDialog = new KDialog(parent->parent);
            configDialog->setLayoutDirection(listView->layoutDirection());
            configDialog->setWindowTitle(pluginInfo.name());
            KTabWidget *newTabWidget = new KTabWidget(configDialog);

            tabWidgets.insert(index.row(), newTabWidget);

            foreach(KService::Ptr servicePtr, services)
            {
                if(!servicePtr->noDisplay())
                {
                    KCModuleInfo moduleinfo(servicePtr);
                    model->setParentComponents(index, moduleinfo.service()->property("X-KDE-ParentComponents").toStringList());
                    KCModuleProxy *currentModuleProxy = new KCModuleProxy(moduleinfo, newTabWidget);
                    if (currentModuleProxy->realModule())
                    {
                        newTabWidget->addTab(currentModuleProxy, servicePtr->name());
                    }

                    if (!modulesDialogs.contains(index.row()))
                        modulesDialogs.insert(index.row(), QList<KCModuleProxy*>() << currentModuleProxy);
                    else
                    {
                        modulesDialogs[index.row()].append(currentModuleProxy);
                    }
                }
            }

            configDialog->setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Default);

            configDialog->setMainWidget(newTabWidget);
        }
        else
        {
            configDialog = configDialogs[index.row()];
        }

        currentModuleProxyList = modulesDialogs.contains(index.row()) ? &modulesDialogs[index.row()] : 0;

        QObject::connect(configDialog, SIGNAL(defaultClicked()), this, SLOT(slotDefaultClicked()));

        if (configDialog->exec() == QDialog::Accepted)
        {
            foreach (KCModuleProxy *moduleProxy, modulesDialogs[index.row()])
            {
                moduleProxy->save();
                foreach (const QString &parentComponent, model->parentComponents(index))
                {
                    emit configCommitted(parentComponent.toLatin1());
                }
            }
        }
        else
        {
            foreach (KCModuleProxy *moduleProxy, modulesDialogs[index.row()])
            {
                moduleProxy->load();
            }
        }

        // Since the dialog is cached and the last tab selected will be kept selected, when closing the
        // dialog we set the selected tab to the first one again
        if (tabWidgets.contains(index.row()))
            tabWidgets[index.row()]->setCurrentIndex(0);

        QObject::disconnect(configDialog, SIGNAL(defaultClicked()), this, SLOT(slotDefaultClicked()));
    }
    else if ((opt.rect.contains(cursorPos) && (eventReceived == MouseEvent)) ||
             ((focusedElement == AboutButtonFocused) && (eventReceived == KeyboardEvent)))
    {
        KDialog *aboutDialog;

        if (!aboutDialogs.contains(index.row()))
        {
            aboutDialog = new KDialog(parent->parent);
            aboutDialog->setLayoutDirection(listView->layoutDirection());
            aboutDialog->setWindowTitle(i18n("About %1 plugin", pluginInfo.name()));
            aboutDialog->setButtons(KDialog::Close);

            QWidget *aboutWidget = new QWidget(aboutDialog);
            QVBoxLayout *layout = new QVBoxLayout;
            layout->setSpacing(0);
            aboutWidget->setLayout(layout);

            if (!pluginInfo.comment().isEmpty())
            {
                QLabel *description = new QLabel(i18n("Description:\n\t%1", pluginInfo.comment()), aboutWidget);
                layout->addWidget(description);
                layout->addSpacing(20);
            }

            if (!pluginInfo.author().isEmpty())
            {
                QLabel *author = new QLabel(i18n("Author:\n\t%1", pluginInfo.author()), aboutWidget);
                layout->addWidget(author);
                layout->addSpacing(20);
            }

            if (!pluginInfo.email().isEmpty())
            {
                QLabel *authorEmail = new QLabel(i18n("E-Mail:"), aboutWidget);
                KUrlLabel *sendEmail = new KUrlLabel("mailto:" + pluginInfo.email(), '\t' + pluginInfo.email());

                sendEmail->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                sendEmail->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                sendEmail->setGlowEnabled(false);
                sendEmail->setUnderline(false);
                sendEmail->setFloatEnabled(true);
                sendEmail->setUseCursor(false);
                sendEmail->setHighlightedColor(option.palette.color(QPalette::Link));
                sendEmail->setSelectedColor(option.palette.color(QPalette::Link));

                QObject::connect(sendEmail, SIGNAL(leftClickedUrl(QString)), this, SLOT(processUrl(QString)));

                layout->addWidget(authorEmail);
                layout->addWidget(sendEmail);
                layout->addSpacing(20);
            }

            if (!pluginInfo.website().isEmpty())
            {
                QLabel *website = new QLabel(i18n("Website:"), aboutWidget);
                KUrlLabel *visitWebsite = new KUrlLabel(pluginInfo.website(), '\t' + pluginInfo.website());

                visitWebsite->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                visitWebsite->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                visitWebsite->setGlowEnabled(false);
                visitWebsite->setUnderline(false);
                visitWebsite->setFloatEnabled(true);
                visitWebsite->setUseCursor(false);
                visitWebsite->setHighlightedColor(option.palette.color(QPalette::Link));
                visitWebsite->setSelectedColor(option.palette.color(QPalette::Link));

                QObject::connect(visitWebsite, SIGNAL(leftClickedUrl(QString)), this, SLOT(processUrl(QString)));

                layout->addWidget(website);
                layout->addWidget(visitWebsite);
                layout->addSpacing(20);
            }

            if (!pluginInfo.version().isEmpty())
            {
                QLabel *version = new QLabel(i18n("Version:\n\t%1", pluginInfo.version()), aboutWidget);

                layout->addWidget(version);
                layout->addSpacing(20);
            }

            if (!pluginInfo.license().isEmpty())
            {
                QLabel *license = new QLabel(i18n("License:\n\t%1", pluginInfo.license()), aboutWidget);

                layout->addWidget(license);
                layout->addSpacing(20);
            }

            layout->insertStretch(-1);

            aboutDialog->setMainWidget(aboutWidget);

            aboutDialogs.insert(index.row(), aboutDialog);
        }
        else
        {
            aboutDialog = aboutDialogs[index.row()];
        }

        aboutDialog->exec();
    }
}

void KatePluginSelector::Private::PluginDelegate::checkDependencies(PluginModel *model,
                                                                 const KPluginInfo &info,
                                                                 CheckWhatDependencies whatDependencies)
{
    QStringList dependenciesPushed;

    if (whatDependencies == DependenciesINeed)
    {
        foreach(const QString &dependency, info.dependencies())
        {
            model->updateDependencies(dependency, info.name(), whatDependencies, dependenciesPushed);
        }
    }
    else
    {
        model->updateDependencies(info.pluginName(), info.name(), whatDependencies, dependenciesPushed);
    }
}

QString KatePluginSelector::Private::PluginDelegate::name(const QModelIndex &index) const
{
    return index.model()->data(index, Name).toString();
}

QString KatePluginSelector::Private::PluginDelegate::comment(const QModelIndex &index) const
{
    return index.model()->data(index, Comment).toString();
}

QPixmap KatePluginSelector::Private::PluginDelegate::icon(const QModelIndex &index, int width, int height) const
{
    return KIcon(index.model()->data(index, Icon).toString(), iconLoader).pixmap(width, height);
}

QString KatePluginSelector::Private::PluginDelegate::author(const QModelIndex &index) const
{
    return index.model()->data(index, Author).toString();
}

QString KatePluginSelector::Private::PluginDelegate::email(const QModelIndex &index) const
{
    return index.model()->data(index, Email).toString();
}

QString KatePluginSelector::Private::PluginDelegate::category(const QModelIndex &index) const
{
    return index.model()->data(index, Category).toString();
}

QString KatePluginSelector::Private::PluginDelegate::internalName(const QModelIndex &index) const
{
    return index.model()->data(index, InternalName).toString();
}

QString KatePluginSelector::Private::PluginDelegate::version(const QModelIndex &index) const
{
    return index.model()->data(index, Version).toString();
}

QString KatePluginSelector::Private::PluginDelegate::website(const QModelIndex &index) const
{
    return index.model()->data(index, Website).toString();
}

QString KatePluginSelector::Private::PluginDelegate::license(const QModelIndex &index) const
{
    return index.model()->data(index, License).toString();
}

int KatePluginSelector::Private::PluginDelegate::calculateVerticalCenter(const QRect &rect, int pixmapHeight) const
{
    return (rect.height() / 2) - (pixmapHeight / 2);
}


#include "pluginselector_p.moc"
#include "pluginselector.moc"
