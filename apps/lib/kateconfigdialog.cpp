/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

   For the addScrollablePage original
   SPDX-FileCopyrightText: 2003 Benjamin C Meyer <ben+kdelibs at meyerhome dot net>
   SPDX-FileCopyrightText: 2003 Waldo Bastian <bastian@kde.org>
   SPDX-FileCopyrightText: 2004 Michael Brade <brade@kde.org>
   SPDX-FileCopyrightText: 2021 Ahmad Samir <a.samirh78@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateconfigdialog.h"

#include "kateapp.h"
#include "kateconfigplugindialogpage.h"
#include "katedebug.h"
#include "katedocmanager.h"
#include "katemainwindow.h"
#include "katepluginmanager.h"
#include "katequickopenmodel.h"
#include "katesessionmanager.h"
#include "kateviewmanager.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluralHandlingSpinBox>
#include <KSharedConfig>
#include <kwidgetsaddons_version.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

KateConfigDialog::KateConfigDialog(KateMainWindow *parent)
    : KPageDialog(parent)
    , m_mainWindow(parent)
    , m_searchLineEdit(new QLineEdit(this))
    , m_searchTimer(new QTimer(this))
{
    setWindowTitle(i18n("Configure"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("configure")));
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Help);

    m_searchLineEdit->setPlaceholderText(i18n("Search..."));
    setFocusProxy(m_searchLineEdit);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(400);
    m_searchTimer->callOnTimeout(this, &KateConfigDialog::onSearchTextChanged);
    connect(m_searchLineEdit, &QLineEdit::textChanged, m_searchTimer, qOverload<>(&QTimer::start));

    if (auto layout = qobject_cast<QVBoxLayout *>(this->layout())) {
        layout->insertWidget(0, m_searchLineEdit);
    } else {
        m_searchLineEdit->hide();
        qWarning() << Q_FUNC_INFO << "Failed to get layout! Search will be disabled";
    }

    // we may have a lot of pages on Kate, we want small icons for the list
    if (KateApp::isKate()) {
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 94, 0)
        setFaceType(KPageDialog::FlatList);
#else
        setFaceType(KPageDialog::List);
#endif
    }
    else {
        setFaceType(KPageDialog::List);
    }

    const auto listViews = findChildren<QListView *>();
    for (auto *lv : listViews) {
        if (qstrcmp(lv->metaObject()->className(), "KDEPrivate::KPageListView") == 0) {
            m_sideBar = lv;
            break;
        }
    }
    if (!m_sideBar) {
        qWarning() << "Unable to find config dialog sidebar listview!!";
    }

    // first: add the KTextEditor config pages
    // rational: most people want to alter e.g. the fonts, the colors or some other editor stuff first
    addEditorPages();

    // second: add out own config pages
    // this includes all plugin config pages, added to the bottom
    addBehaviorPage();
    addSessionPage();
    addFeedbackPage();

    // no plugins for KWrite
    if (KateApp::isKate()) {
        addPluginsPage();
        addPluginPages();
    }

    // ensure no stray signals already set this!
    m_dataChanged = false;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);

    // handle dialog actions
    connect(this, &KateConfigDialog::accepted, this, &KateConfigDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KateConfigDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &KateConfigDialog::slotHelp);

    // set focus on next iteration of event loop, doesn't work directly for some reason
    QMetaObject::invokeMethod(
        this,
        [this] {
            m_searchLineEdit->setFocus();
        },
        Qt::QueuedConnection);
}

template<typename WidgetType>
QVector<QWidget *> hasMatchingText(const QString &text, QWidget *page)
{
    QVector<QWidget *> ret;
    const auto widgets = page->findChildren<WidgetType *>();
    for (auto label : widgets) {
        if (label->text().contains(text, Qt::CaseInsensitive)) {
            ret << label;
        }
    }
    return ret;
}

template<>
QVector<QWidget *> hasMatchingText<QComboBox>(const QString &text, QWidget *page)
{
    QVector<QWidget *> ret;
    const auto comboxBoxes = page->findChildren<QComboBox *>();
    for (auto cb : comboxBoxes) {
        if (cb->findText(text, Qt::MatchFlag::MatchContains) != -1) {
            ret << cb;
        }
    }
    return ret;
}

template<typename...>
struct FindChildrenHelper {
    static QVector<QWidget *> hasMatchingTextForTypes(const QString &, QWidget *)
    {
        return {};
    }
};

template<typename First, typename... Rest>
struct FindChildrenHelper<First, Rest...> {
    static QVector<QWidget *> hasMatchingTextForTypes(const QString &text, QWidget *page)
    {
        return hasMatchingText<First>(text, page) << FindChildrenHelper<Rest...>::hasMatchingTextForTypes(text, page);
    }
};

class SearchMatchOverlay : public QWidget
{
public:
    SearchMatchOverlay(QWidget *parent, int tabIdx = -1)
        : QWidget(parent)
        , m_tabIdx(tabIdx)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        resize_impl();
        parent->installEventFilter(this);

        show();
        raise();
    }

    void doResize()
    {
        // trigger it delayed just to be safe as we will be calling
        // it in response to Resize sometimes
        QMetaObject::invokeMethod(this, &SearchMatchOverlay::resize_impl, Qt::QueuedConnection);
    }

    void resize_impl()
    {
        if (m_tabIdx >= 0) {
            auto tabBar = qobject_cast<QTabBar *>(parentWidget());
            if (!tabBar) {
                setVisible(false);
                return;
            }
            const QRect r = tabBar->tabRect(m_tabIdx);
            if (geometry() != r) {
                setGeometry(r);
            }
            return;
        }

        if (parentWidget() && size() != parentWidget()->size()) {
            resize(parentWidget()->size());
        }
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (parentWidget() && o == parentWidget() && (e->type() == QEvent::Resize || e->type() == QEvent::Show)) {
            doResize();
        }
        return QWidget::eventFilter(o, e);
    }

protected:
    void paintEvent(QPaintEvent *e) override
    {
        QPainter p(this);
        p.setClipRegion(e->region());
        QColor c = palette().brush(QPalette::Active, QPalette::Highlight).color();
        c.setAlpha(110);
        p.fillRect(rect(), c);
    }

private:
    int m_tabIdx = -1;
};

void KateConfigDialog::onSearchTextChanged()
{
    if (!m_sideBar) {
        qWarning() << Q_FUNC_INFO << "No config dialog sidebar, search will not continue";
        return;
    }

    const QString text = m_searchLineEdit->text();
    QSet<QString> pagesToHide;
    QVector<QWidget *> matchedWidgets;
    if (!text.isEmpty()) {
        for (auto item : std::as_const(m_allPages)) {
            const auto matchingWidgets = FindChildrenHelper<QLabel, QAbstractButton, QComboBox>::hasMatchingTextForTypes(text, item->widget());
            if (matchingWidgets.isEmpty()) {
                pagesToHide << item->name();
            }
            matchedWidgets << matchingWidgets;
        }
    }

    if (auto model = m_sideBar->model()) {
        QModelIndex current;
        for (int i = 0; i < model->rowCount(); ++i) {
            const auto itemName = model->index(i, 0).data().toString();
            m_sideBar->setRowHidden(i, pagesToHide.contains(itemName) && !itemName.contains(text, Qt::CaseInsensitive));
            if (!text.isEmpty() && !m_sideBar->isRowHidden(i) && !current.isValid()) {
                current = model->index(i, 0);
            }
        }
        if (current.isValid()) {
            m_sideBar->setCurrentIndex(current);
        }
    }

    qDeleteAll(m_searchMatchOverlays);
    m_searchMatchOverlays.clear();

    using TabWidgetAndPage = QPair<QTabWidget *, QWidget *>;
    auto tabWidgetParent = [](QWidget *w) {
        // Finds if @p w is in a QTabWidget and returns
        // The QTabWidget + the widget in the stack where
        // @p w lives
        auto parent = w->parentWidget();
        TabWidgetAndPage p = {nullptr, nullptr};
        if (auto tw = qobject_cast<QTabWidget *>(parent)) {
            p.first = tw;
        }
        QVarLengthArray<QWidget *, 8> parentChain;
        while (parent) {
            if (!p.first) {
                if (auto tw = qobject_cast<QTabWidget *>(parent)) {
                    if (parentChain.size() >= 3) {
                        // last == QTabWidget
                        // second last == QStackedWidget of QTabWidget
                        // third last => the widget we want
                        p.second = parentChain.value((parentChain.size() - 1) - 2);
                    }
                    p.first = tw;
                    break;
                }
            }
            parent = parent->parentWidget();
            parentChain << parent;
        }
        return p;
    };

    for (auto w : std::as_const(matchedWidgets)) {
        if (w) {
            m_searchMatchOverlays << new SearchMatchOverlay(w);

            if (!w->isVisible()) {
                const auto [tabWidget, page] = tabWidgetParent(w);
                if (!tabWidget && !page) {
                    continue;
                }
                const int idx = tabWidget->indexOf(page);
                if (idx < 0) {
                    //                     qDebug() << page << tabWidget << "not found" << w;
                    continue;
                }
                m_searchMatchOverlays << new SearchMatchOverlay(tabWidget->tabBar(), idx);
            }
        }
    }
}

QSize KateConfigDialog::sizeHint() const
{
    // start with a bit enlarged default size hint to minimize changes of useless scrollbars
    QSize size = KPageDialog::sizeHint() * 1.3;

    // enlarge it to half of the main window size, if that is larger
    size = size.expandedTo(m_mainWindow->size() * 0.5);

    // return bounded size to available real screen space
    return size.boundedTo(screen()->availableSize() * 0.9);
}

void KateConfigDialog::addBehaviorPage()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");

    QFrame *generalFrame = new QFrame;
    KPageWidgetItem *item = addScrollablePage(generalFrame, i18n("Behavior"));
    m_allPages.insert(item);
    item->setHeader(i18n("Behavior Options"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-system-windows-behavior")));

    QVBoxLayout *layout = new QVBoxLayout(generalFrame);
    layout->setContentsMargins(0, 0, 0, 0);

    // GROUP with the one below: "Behavior"
    QGroupBox *buttonGroup = new QGroupBox(i18n("&Behavior"), generalFrame);
    QVBoxLayout *vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    // shall we try to behave like some SDI application
    m_sdiMode = new QCheckBox(i18n("Open each document in its own window"), buttonGroup);
    m_sdiMode->setChecked(cgGeneral.readEntry("SDI Mode", false));
    m_sdiMode->setToolTip(
        i18n("If enabled, each document will be opened in its own window. "
             "If not enabled, each document will be opened in a new tab in the current window."));
    connect(m_sdiMode, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_sdiMode);

    QHBoxLayout *hlayout = nullptr;
    QLabel *label = nullptr;

    if (KateApp::isKate()) {
        hlayout = new QHBoxLayout;
        label = new QLabel(i18n("&Switch to output view upon message type:"), buttonGroup);
        hlayout->addWidget(label);
        m_messageTypes = new QComboBox(buttonGroup);
        hlayout->addWidget(m_messageTypes);
        label->setBuddy(m_messageTypes);
        m_messageTypes->addItems({i18n("Never"), i18n("Error"), i18n("Warning"), i18n("Info"), i18n("Log")});
        m_messageTypes->setCurrentIndex(cgGeneral.readEntry("Show output view for message type", 1));
        connect(m_messageTypes, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);
        vbox->addLayout(hlayout);

        hlayout = new QHBoxLayout;
        label = new QLabel(i18n("&Limit output view history:"), buttonGroup);
        hlayout->addWidget(label);
        m_outputHistoryLimit = new QSpinBox(buttonGroup);
        hlayout->addWidget(m_outputHistoryLimit);
        label->setBuddy(m_outputHistoryLimit);
        m_outputHistoryLimit->setRange(-1, 10000);
        m_outputHistoryLimit->setSpecialValueText(i18n("Unlimited"));
        m_outputHistoryLimit->setValue(cgGeneral.readEntry("Output History Limit", 100));
        connect(m_outputHistoryLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);
        vbox->addLayout(hlayout);
    }

    // modified files notification
    m_modNotifications = new QCheckBox(i18n("Use a separate &dialog for handling externally modified files"), buttonGroup);
    m_modNotifications->setChecked(m_mainWindow->modNotificationEnabled());
    m_modNotifications->setToolTip(
        i18n("If enabled, a modal dialog will be used to show all of the modified files. "
             "If not enabled, you will be individually asked what to do for each modified file "
             "only when that file's view receives focus."));
    connect(m_modNotifications, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    vbox->addWidget(m_modNotifications);
    buttonGroup->setLayout(vbox);

    if (KateApp::isKate()) {
        QGroupBox *buttonGroup = new QGroupBox(i18n("&Sidebars"), generalFrame);
        layout->addWidget(buttonGroup);
        QVBoxLayout *vbox = new QVBoxLayout;
        m_syncSectionSizeWithSidebarTabs = new QCheckBox(i18n("Sync section size with tab positions"), buttonGroup);
        m_syncSectionSizeWithSidebarTabs->setChecked(cgGeneral.readEntry("Sync section size with tab positions", false));
        m_syncSectionSizeWithSidebarTabs->setToolTip(
            i18n("When enabled the section size will be determined by the position of the tabs.\n"
                 "This option does not affect the bottom sidebar."));
        connect(m_syncSectionSizeWithSidebarTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
        vbox->addWidget(m_syncSectionSizeWithSidebarTabs);

        m_showTextForLeftRightSidebars = new QCheckBox(i18n("Show text for left and right sidebar buttons"), buttonGroup);
        m_showTextForLeftRightSidebars->setChecked(cgGeneral.readEntry("Show text for left and right sidebar", false));
        connect(m_showTextForLeftRightSidebars, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
        vbox->addWidget(m_showTextForLeftRightSidebars);

        label = new QLabel(i18n("Icon size for left and right sidebar buttons"), buttonGroup);
        m_leftRightSidebarsIconSize = new QSpinBox(buttonGroup);
        m_leftRightSidebarsIconSize->setMinimum(16);
        m_leftRightSidebarsIconSize->setMaximum(48);
        m_leftRightSidebarsIconSize->setValue(cgGeneral.readEntry("Icon size for left and right sidebar buttons", 32));
        connect(m_leftRightSidebarsIconSize, &QSpinBox::textChanged, this, &KateConfigDialog::slotChanged);
        hlayout = new QHBoxLayout;
        hlayout->addWidget(label);
        hlayout->addWidget(m_leftRightSidebarsIconSize);
        vbox->addLayout(hlayout);

        connect(m_showTextForLeftRightSidebars, &QCheckBox::toggled, this, [l = QPointer(label), this](bool v) {
            m_leftRightSidebarsIconSize->setEnabled(!v);
            l->setEnabled(!v);
        });
        buttonGroup->setLayout(vbox);
    }

    // tabbar => we allow to configure some limit on number of tabs to show
    buttonGroup = new QGroupBox(i18n("&Tabs"), generalFrame);
    vbox = new QVBoxLayout;
    buttonGroup->setLayout(vbox);
    hlayout = new QHBoxLayout;
    label = new QLabel(i18n("&Limit number of tabs:"), buttonGroup);
    hlayout->addWidget(label);
    m_tabLimit = new QSpinBox(buttonGroup);
    hlayout->addWidget(m_tabLimit);
    label->setBuddy(m_tabLimit);
    m_tabLimit->setRange(0, 256);
    m_tabLimit->setSpecialValueText(i18n("Unlimited"));
    m_tabLimit->setValue(cgGeneral.readEntry("Tabbar Tab Limit", 0));
    connect(m_tabLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);
    vbox->addLayout(hlayout);
    label =
        new QLabel(i18n("A high limit can increase the window size, please enable 'Allow tab scrolling' to prevent it. Unlimited tabs are always scrollable."));
    label->setWordWrap(true);
    vbox->addWidget(label);

    m_autoHideTabs = new QCheckBox(i18n("&Auto hide tabs"), buttonGroup);
    m_autoHideTabs->setChecked(cgGeneral.readEntry("Auto Hide Tabs", KateApp::isKWrite()));
    m_autoHideTabs->setToolTip(i18n("When checked tabs will be hidden if only one document is open."));
    connect(m_autoHideTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_autoHideTabs);

    m_showTabCloseButton = new QCheckBox(i18n("&Show close button"), buttonGroup);
    m_showTabCloseButton->setChecked(cgGeneral.readEntry("Show Tabs Close Button", true));
    m_showTabCloseButton->setToolTip(i18n("When checked each tab will display a close button."));
    connect(m_showTabCloseButton, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_showTabCloseButton);

    m_expandTabs = new QCheckBox(i18n("&Expand tabs"), buttonGroup);
    m_expandTabs->setChecked(cgGeneral.readEntry("Expand Tabs", false));
    m_expandTabs->setToolTip(i18n("When checked tabs take as much size as possible."));
    connect(m_expandTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_expandTabs);

    m_tabDoubleClickNewDocument = new QCheckBox(i18n("&Double click opens a new document"), buttonGroup);
    m_tabDoubleClickNewDocument->setChecked(cgGeneral.readEntry("Tab Double Click New Document", true));
    m_tabDoubleClickNewDocument->setToolTip(i18n("When checked double click opens a new document."));
    connect(m_tabDoubleClickNewDocument, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabDoubleClickNewDocument);

    m_tabMiddleClickCloseDocument = new QCheckBox(i18n("&Middle click closes a document"), buttonGroup);
    m_tabMiddleClickCloseDocument->setChecked(cgGeneral.readEntry("Tab Middle Click Close Document", true));
    m_tabMiddleClickCloseDocument->setToolTip(i18n("When checked middle click closes a document."));
    connect(m_tabMiddleClickCloseDocument, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabMiddleClickCloseDocument);

    m_tabsScrollable = new QCheckBox(i18n("Allow tab scrolling"), this);
    m_tabsScrollable->setChecked(cgGeneral.readEntry("Allow Tab Scrolling", true));
    m_tabsScrollable->setToolTip(i18n("When checked this will allow scrolling in tab bar when number of tabs is large."));
    connect(m_tabsScrollable, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabsScrollable);

    m_tabsElided = new QCheckBox(i18n("Elide tab text"), this);
    m_tabsElided->setChecked(cgGeneral.readEntry("Elide Tab Text", false));
    m_tabsElided->setToolTip(i18n("When checked tab text might be elided if its too long."));
    connect(m_tabsElided, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabsElided);

    layout->addWidget(buttonGroup);

    buttonGroup = new QGroupBox(i18n("&Mouse"), generalFrame);
    vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    hlayout = new QHBoxLayout;
    label = new QLabel(i18n("&Backward button pressed:"), buttonGroup);
    hlayout->addWidget(label);
    m_mouseBackActions = new QComboBox(buttonGroup);
    hlayout->addWidget(m_mouseBackActions);
    label->setBuddy(m_mouseBackActions);
    m_mouseBackActions->addItems({i18n("Previous tab"), i18n("History back")});
    m_mouseBackActions->setCurrentIndex(cgGeneral.readEntry("Mouse back button action", 0));
    connect(m_mouseBackActions, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);
    vbox->addLayout(hlayout);

    hlayout = new QHBoxLayout;
    label = new QLabel(i18n("&Forward button pressed:"), buttonGroup);
    hlayout->addWidget(label);
    m_mouseForwardActions = new QComboBox(buttonGroup);
    hlayout->addWidget(m_mouseForwardActions);
    label->setBuddy(m_mouseForwardActions);
    m_mouseForwardActions->addItems({i18n("Next tab"), i18n("History forward")});
    m_mouseForwardActions->setCurrentIndex(cgGeneral.readEntry("Mouse forward button action", 0));
    connect(m_mouseForwardActions, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);
    vbox->addLayout(hlayout);

    buttonGroup->setLayout(vbox);

    /** DIFF **/
    buttonGroup = new QGroupBox(i18n("Diff"), generalFrame);
    vbox = new QVBoxLayout(buttonGroup);
    hlayout = new QHBoxLayout;
    vbox->addLayout(hlayout);
    layout->addWidget(buttonGroup);
    m_diffStyle = new QComboBox;
    m_diffStyle->addItem(i18n("Side By Side"));
    m_diffStyle->addItem(i18n("Unified"));
    m_diffStyle->addItem(i18n("Raw"));
    hlayout->addWidget(new QLabel(i18n("Diff Style:")));
    hlayout->addWidget(m_diffStyle);
    m_diffStyle->setCurrentIndex(cgGeneral.readEntry("Diff Show Style", 0));
    connect(m_diffStyle, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);

    buttonGroup = new QGroupBox(i18n("Navigation Bar"), generalFrame);
    vbox = new QVBoxLayout(buttonGroup);
    hlayout = new QHBoxLayout;
    vbox->addLayout(hlayout);
    layout->addWidget(buttonGroup);
    m_urlBarShowSymbols = new QCheckBox(i18n("Show current symbol in navigation bar"));
    hlayout->addWidget(m_urlBarShowSymbols);
    m_urlBarShowSymbols->setChecked(cgGeneral.readEntry("Show Symbol In Navigation Bar", true));
    connect(m_urlBarShowSymbols, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    layout->addStretch(1); // :-] works correct without autoadd
}

void KateConfigDialog::addSessionPage()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");

    QWidget *sessionsPage = new QWidget();
    auto item = addScrollablePage(sessionsPage, i18n("Session"));
    m_allPages.insert(item);
    item->setHeader(i18n("Session Management"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));

    sessionConfigUi.setupUi(sessionsPage);

    // save meta infos
    sessionConfigUi.saveMetaInfos->setChecked(KateApp::self()->documentManager()->getSaveMetaInfos());
    connect(sessionConfigUi.saveMetaInfos, &QGroupBox::toggled, this, &KateConfigDialog::slotChanged);

    // meta infos days
    sessionConfigUi.daysMetaInfos->setMaximum(180);
    sessionConfigUi.daysMetaInfos->setSpecialValueText(i18nc("The special case of 'Delete unused meta-information after'", "(never)"));
    sessionConfigUi.daysMetaInfos->setSuffix(ki18ncp("The suffix of 'Delete unused meta-information after'", " day", " days"));
    sessionConfigUi.daysMetaInfos->setValue(KateApp::self()->documentManager()->getDaysMetaInfos());
    connect(sessionConfigUi.daysMetaInfos,
            static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged),
            this,
            &KateConfigDialog::slotChanged);

    // restore view  config
    sessionConfigUi.restoreVC->setChecked(cgGeneral.readEntry("Restore Window Configuration", true));
    connect(sessionConfigUi.restoreVC, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    sessionConfigUi.spinBoxRecentFilesCount->setValue(recentFilesMaxCount());
    connect(sessionConfigUi.spinBoxRecentFilesCount, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);

    QString sesStart(cgGeneral.readEntry("Startup Session", "manual"));
    if (sesStart == QLatin1String("new")) {
        sessionConfigUi.startNewSessionRadioButton->setChecked(true);
    } else if (sesStart == QLatin1String("last")) {
        sessionConfigUi.loadLastUserSessionRadioButton->setChecked(true);
    } else {
        sessionConfigUi.manuallyChooseSessionRadioButton->setChecked(true);
    }

    connect(sessionConfigUi.startNewSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi.loadLastUserSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi.manuallyChooseSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);

    // New main windows open always a new document if none there
    sessionConfigUi.showWelcomeViewForNewWindow->setChecked(cgGeneral.readEntry("Show welcome view for new window", true));
    connect(sessionConfigUi.showWelcomeViewForNewWindow, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    // When a window is closed, close all documents only visible in that window, too
    sessionConfigUi.winClosesDocuments->setChecked(cgGeneral.readEntry("Close documents with window", true));
    sessionConfigUi.winClosesDocuments->setToolTip(i18n("When a window is closed the documents opened only in this window are closed as well."));
    connect(sessionConfigUi.winClosesDocuments, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    // Closing last file closes Kate
    sessionConfigUi.modCloseAfterLast->setChecked(m_mainWindow->modCloseAfterLast());
    connect(sessionConfigUi.modCloseAfterLast, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    // stash unsave changes
    sessionConfigUi.stashNewUnsavedFiles->setChecked(KateApp::self()->stashManager()->stashNewUnsavedFiles());
    sessionConfigUi.stashUnsavedFilesChanges->setChecked(KateApp::self()->stashManager()->stashUnsavedChanges());
    connect(sessionConfigUi.stashNewUnsavedFiles, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi.stashUnsavedFilesChanges, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);

    // simplify the session page for KWrite
    if (KateApp::isKWrite()) {
        sessionConfigUi.gbAppStartup->hide();
        sessionConfigUi.restoreVC->hide();
        sessionConfigUi.label_4->hide();
        sessionConfigUi.stashNewUnsavedFiles->hide();
        sessionConfigUi.stashUnsavedFilesChanges->hide();
        sessionConfigUi.label->hide();
    }
}

void KateConfigDialog::addPluginsPage()
{
    QFrame *page = new QFrame(this);
    QVBoxLayout *vlayout = new QVBoxLayout(page);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    m_configPluginPage = new KateConfigPluginPage(page, this);
    vlayout->addWidget(m_configPluginPage);
    connect(m_configPluginPage, &KateConfigPluginPage::changed, this, &KateConfigDialog::slotChanged);

    auto item = addScrollablePage(page, i18n("Plugins"));
    m_allPages.insert(item);
    item->setHeader(i18n("Plugin Manager"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-plugin")));
}

void KateConfigDialog::addFeedbackPage()
{
#ifdef WITH_KUSERFEEDBACK
    // KUserFeedback Config
    auto page = new QFrame(this);
    auto vlayout = new QVBoxLayout(page);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    m_userFeedbackWidget = new KUserFeedback::FeedbackConfigWidget(page);
    m_userFeedbackWidget->setFeedbackProvider(&KateApp::self()->userFeedbackProvider());
    connect(m_userFeedbackWidget, &KUserFeedback::FeedbackConfigWidget::configurationChanged, this, &KateConfigDialog::slotChanged);
    vlayout->addWidget(m_userFeedbackWidget);

    auto item = addScrollablePage(page, i18n("User Feedback"));
    m_allPages.insert(item);
    item->setHeader(i18n("User Feedback"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-locale")));
#endif
}

void KateConfigDialog::addPluginPages()
{
    const KatePluginList &pluginList(KateApp::self()->pluginManager()->pluginList());
    for (const KatePluginInfo &plugin : pluginList) {
        if (plugin.plugin) {
            addPluginPage(plugin.plugin);
        }
    }
}

void KateConfigDialog::addEditorPages()
{
    for (int i = 0; i < KTextEditor::Editor::instance()->configPages(); ++i) {
        KTextEditor::ConfigPage *page = KTextEditor::Editor::instance()->configPage(i, this);
        connect(page, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
        m_editorPages.push_back(page);
        KPageWidgetItem *item = addScrollablePage(page, page->name());
        item->setHeader(page->fullName());
        item->setIcon(page->icon());
        m_allPages.insert(item);
    }
}

void KateConfigDialog::addPluginPage(KTextEditor::Plugin *plugin)
{
    for (int i = 0; i < plugin->configPages(); i++) {
        KTextEditor::ConfigPage *cp = plugin->configPage(i, this);
        KPageWidgetItem *item = addScrollablePage(cp, cp->name());
        item->setHeader(cp->fullName());
        item->setIcon(cp->icon());

        PluginPageListItem info;
        info.plugin = plugin;
        info.pluginPage = cp;
        info.idInPlugin = i;
        info.pageWidgetItem = item;
        connect(info.pluginPage, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
        m_pluginPages.insert(item, info);
        m_allPages.insert(item);
    }
}

void KateConfigDialog::removePluginPage(KTextEditor::Plugin *plugin)
{
    QList<KPageWidgetItem *> remove;
    for (QHash<KPageWidgetItem *, PluginPageListItem>::const_iterator it = m_pluginPages.constBegin(); it != m_pluginPages.constEnd(); ++it) {
        const PluginPageListItem &pli = it.value();

        if (pli.plugin == plugin) {
            remove.append(it.key());
        }
    }

    qCDebug(LOG_KATE) << remove.count();
    while (!remove.isEmpty()) {
        KPageWidgetItem *wItem = remove.takeLast();
        PluginPageListItem item = m_pluginPages.take(wItem);
        m_allPages.remove(wItem);
        delete item.pluginPage;
        removePage(wItem);
    }
}

void KateConfigDialog::slotApply()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    // if data changed apply the kate app stuff
    if (m_dataChanged) {
        // apply plugin load state changes
        if (m_configPluginPage) {
            m_configPluginPage->slotApply();
        }

        KConfigGroup cg(config, "General");

        cg.writeEntry("SDI Mode", m_sdiMode->isChecked());

        // only there for kate
        if (m_syncSectionSizeWithSidebarTabs) {
            cg.writeEntry("Sync section size with tab positions", m_syncSectionSizeWithSidebarTabs->isChecked());
        }
        if (m_showTextForLeftRightSidebars) {
            cg.writeEntry("Show text for left and right sidebar", m_showTextForLeftRightSidebars->isChecked());
        }
        if (m_leftRightSidebarsIconSize) {
            cg.writeEntry("Icon size for left and right sidebar buttons", m_leftRightSidebarsIconSize->value());
        }

        cg.writeEntry("Restore Window Configuration", sessionConfigUi.restoreVC->isChecked());

        cg.writeEntry("Recent File List Entry Count", sessionConfigUi.spinBoxRecentFilesCount->value());

        if (sessionConfigUi.startNewSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "new");
        } else if (sessionConfigUi.loadLastUserSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "last");
        } else {
            cg.writeEntry("Startup Session", "manual");
        }

        cg.writeEntry("Save Meta Infos", sessionConfigUi.saveMetaInfos->isChecked());
        KateApp::self()->documentManager()->setSaveMetaInfos(sessionConfigUi.saveMetaInfos->isChecked());

        cg.writeEntry("Days Meta Infos", sessionConfigUi.daysMetaInfos->value());
        KateApp::self()->documentManager()->setDaysMetaInfos(sessionConfigUi.daysMetaInfos->value());

        cg.writeEntry("Show welcome view for new window", sessionConfigUi.showWelcomeViewForNewWindow->isChecked());

        cg.writeEntry("Close documents with window", sessionConfigUi.winClosesDocuments->isChecked());

        cg.writeEntry("Close After Last", sessionConfigUi.modCloseAfterLast->isChecked());
        m_mainWindow->setModCloseAfterLast(sessionConfigUi.modCloseAfterLast->isChecked());

        if (m_messageTypes && m_outputHistoryLimit) {
            cg.writeEntry("Show output view for message type", m_messageTypes->currentIndex());
            cg.writeEntry("Output History Limit", m_outputHistoryLimit->value());
        }

        cg.writeEntry("Mouse back button action", m_mouseBackActions->currentIndex());
        cg.writeEntry("Mouse forward button action", m_mouseForwardActions->currentIndex());

        cg.writeEntry("Stash unsaved file changes", sessionConfigUi.stashUnsavedFilesChanges->isChecked());
        KateApp::self()->stashManager()->setStashUnsavedChanges(sessionConfigUi.stashUnsavedFilesChanges->isChecked());
        cg.writeEntry("Stash new unsaved files", sessionConfigUi.stashNewUnsavedFiles->isChecked());
        KateApp::self()->stashManager()->setStashNewUnsavedFiles(sessionConfigUi.stashNewUnsavedFiles->isChecked());

        cg.writeEntry("Modified Notification", m_modNotifications->isChecked());
        m_mainWindow->setModNotificationEnabled(m_modNotifications->isChecked());

        cg.writeEntry("Tabbar Tab Limit", m_tabLimit->value());

        cg.writeEntry("Auto Hide Tabs", m_autoHideTabs->isChecked());

        cg.writeEntry("Show Tabs Close Button", m_showTabCloseButton->isChecked());

        cg.writeEntry("Expand Tabs", m_expandTabs->isChecked());

        cg.writeEntry("Tab Double Click New Document", m_tabDoubleClickNewDocument->isChecked());
        cg.writeEntry("Tab Middle Click Close Document", m_tabMiddleClickCloseDocument->isChecked());

        cg.writeEntry("Allow Tab Scrolling", m_tabsScrollable->isChecked());
        cg.writeEntry("Elide Tab Text", m_tabsElided->isChecked());

        cg.writeEntry("Diff Show Style", m_diffStyle->currentIndex());

        cg.writeEntry("Show Symbol In Navigation Bar", m_urlBarShowSymbols->isChecked());

        // patch document modified warn state
        const QList<KTextEditor::Document *> &docs = KateApp::self()->documentManager()->documentList();
        for (KTextEditor::Document *doc : docs) {
            if (auto modIface = qobject_cast<KTextEditor::ModificationInterface *>(doc)) {
                modIface->setModifiedOnDiskWarning(!m_modNotifications->isChecked());
            }
        }

        m_mainWindow->saveOptions();

        // save plugin config !!
        KateSessionManager *sessionmanager = KateApp::self()->sessionManager();
        KConfig *sessionConfig = sessionmanager->activeSession()->config();
        KateApp::self()->pluginManager()->writeConfig(sessionConfig);

#ifdef WITH_KUSERFEEDBACK
        // set current active mode + write back the config for future starts
        KateApp::self()->userFeedbackProvider().setTelemetryMode(m_userFeedbackWidget->telemetryMode());
        KateApp::self()->userFeedbackProvider().setSurveyInterval(m_userFeedbackWidget->surveyInterval());
#endif
    }

    for (const PluginPageListItem &plugin : qAsConst(m_pluginPages)) {
        if (plugin.pluginPage) {
            plugin.pluginPage->apply();
        }
    }

    // apply ktexteditor pages
    for (KTextEditor::ConfigPage *page : qAsConst(m_editorPages)) {
        page->apply();
    }

    config->sync();

    // emit config change
    if (m_dataChanged) {
        KateApp::self()->configurationChanged();
    }

    m_dataChanged = false;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void KateConfigDialog::slotChanged()
{
    m_dataChanged = true;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void KateConfigDialog::showAppPluginPage(KTextEditor::Plugin *p, int id)
{
    for (const PluginPageListItem &plugin : qAsConst(m_pluginPages)) {
        if ((plugin.plugin == p) && (id == plugin.idInPlugin)) {
            setCurrentPage(plugin.pageWidgetItem);
            break;
        }
    }
}

void KateConfigDialog::slotHelp()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/")));
}

int KateConfigDialog::recentFilesMaxCount()
{
    int maxItems = KConfigGroup(KSharedConfig::openConfig(), "General").readEntry("Recent File List Entry Count", 10);
    return maxItems;
}

bool KateConfigDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->modifiers().testFlag(Qt::NoModifier) && keyEvent->key() == Qt::Key_Escape) {
            event->accept();
            return true;
        }
    }
    return KPageDialog::event(event);
}

void KateConfigDialog::closeEvent(QCloseEvent *event)
{
    if (!m_dataChanged) {
        event->accept();
        return;
    }

#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    const auto response = KMessageBox::warningTwoActionsCancel(this,
#else
    const auto response = KMessageBox::warningYesNoCancel(this,
#endif
                                                               i18n("You have unsaved changes. Do you want to apply the changes or discard them?"),
                                                               i18n("Warning"),
                                                               KStandardGuiItem::save(),
                                                               KStandardGuiItem::discard(),
                                                               KStandardGuiItem::cancel());
    switch (response) {
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    case KMessageBox::PrimaryAction:
#else
    case KMessageBox::Yes:
#endif
        slotApply();
        Q_FALLTHROUGH();
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    case KMessageBox::SecondaryAction:
#else
    case KMessageBox::No:
#endif
        event->accept();
        break;
    case KMessageBox::Cancel:
        event->ignore();
        break;
    default:
        break;
    }
}

KPageWidgetItem *KateConfigDialog::addScrollablePage(QWidget *page, const QString &itemName)
{
    // inspired by KPageWidgetItem *KConfigDialogPrivate::addPageInternal(QWidget *page, const QString &itemName, const QString &pixmapName, const QString
    // &header)
    QWidget *frame = new QWidget;
    QVBoxLayout *boxLayout = new QVBoxLayout(frame);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    boxLayout->addWidget(scroll);
    return addPage(frame, itemName);
}
