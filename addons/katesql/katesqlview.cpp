/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katesqlview.h"
#include "connectionmodel.h"
#include "connectionwizard.h"
#include "dataoutput/dataoutputmodelinterface.h"
#include "dataoutput/models/dataoutputeditablemodel.h"
#include "dataoutputwidget.h"
#include "katesqlconstants.h"
#include "outputwidget.h"
#include "schemabrowserwidget.h"
#include "schemawidget.h"
#include "sqlmanager.h"
#include "textoutputwidget.h"

#include <KActionCollection>
#include <KComboBox>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QAction>
#include <QActionGroup>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QSqlQuery>
#include <QString>
#include <QStyleHints>
#include <QWidgetAction>

KateSQLView::KateSQLView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw)
    : QObject(mw)
    , m_manager(new SQLManager(this))
    , m_mainWindow(mw)
{
    KXMLGUIClient::setComponentName(QStringLiteral("katesql"), i18n("SQL"));
    setXMLFile(QStringLiteral("ui.rc"));

    m_outputToolView = mw->createToolView(plugin,
                                          QStringLiteral("kate_private_plugin_katesql_output"),
                                          KTextEditor::MainWindow::Bottom,
                                          QIcon::fromTheme(QStringLiteral("server-database")), // TODO better Icon from QIcon::ThemeIcon::...
                                          i18nc("@title:window", "SQL"));

    m_schemaBrowserToolView = mw->createToolView(plugin,
                                                 QStringLiteral("kate_private_plugin_katesql_schemabrowser"),
                                                 KTextEditor::MainWindow::Left,
                                                 QIcon::fromTheme(QStringLiteral("server-database")), // TODO better Icon from QIcon::ThemeIcon::...
                                                 i18nc("@title:window", "SQL Schema"));

    m_outputWidget = new KateSQLOutputWidget(m_outputToolView);

    m_schemaBrowserWidget = new SchemaBrowserWidget(m_schemaBrowserToolView, m_manager);

    m_connectionsComboBox = new KComboBox(false);
    m_connectionsComboBox->setEditable(false);
    m_connectionsComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_connectionsComboBox->setModel(m_manager->connectionModel());

    setupActions();

    m_mainWindow->guiFactory()->addClient(this);

    // Register DataOutputWidget as a separate KXMLGUIClient so its shortcuts
    // (Ctrl+C, Ctrl+V, Ctrl+S, Ctrl+Z, etc.) are scoped to the tool view only
    // and don't conflict with katepart editor shortcuts.
    // addClient() calls beginXMLPlug(mainWindow) which adds the main window as
    // an associated widget, making shortcuts global. We must clear it and re-add
    // only the data output widget to keep shortcuts scoped.
    auto *dataOutput = m_outputWidget->dataOutputWidget();
    m_mainWindow->guiFactory()->addClient(dataOutput);
    dataOutput->actionCollection()->clearAssociatedWidgets();
    dataOutput->actionCollection()->addAssociatedWidget(dataOutput);

    QMenu *sqlMenu = static_cast<QMenu *>(factory()->container(MenuSQL, this));

    m_connectionsGroup = new QActionGroup(sqlMenu);
    m_connectionsGroup->setExclusive(true);

    connect(sqlMenu, &QMenu::aboutToShow, this, &KateSQLView::slotSQLMenuAboutToShow);
    connect(m_connectionsGroup, &QActionGroup::triggered, this, &KateSQLView::slotConnectionSelectedFromMenu);
    connect(m_manager, &SQLManager::error, this, &KateSQLView::slotError);
    connect(m_manager, &SQLManager::success, this, &KateSQLView::slotSuccess);
    connect(m_manager, &SQLManager::queryActivated, this, &KateSQLView::slotQueryActivated);
    connect(m_manager, &SQLManager::editableQueryActivated, this, &KateSQLView::slotEditableQueryActivated);
    connect(m_manager, &SQLManager::editableRelationalQueryActivated, this, &KateSQLView::slotEditableRelationalQueryActivated);
    connect(m_manager, &SQLManager::connectionCreated, this, &KateSQLView::slotConnectionCreated);
    connect(m_manager, &SQLManager::connectionAboutToBeClosed, this, &KateSQLView::slotConnectionAboutToBeClosed);
    connect(m_connectionsComboBox, &QComboBox::currentIndexChanged, this, &KateSQLView::slotConnectionChanged);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &KateSQLView::slotGlobalSettingsChanged);

    connect(m_outputWidget->dataOutputWidget(),
            &DataOutputWidget::displayColumnMapChanged,
            m_schemaBrowserWidget->schemaWidget(),
            &SchemaWidget::reloadDisplayColumnMap);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, [this]() {
        updateRunActionEnabled();
        updateViewEventFilter();
    });

    // Install event filter on the initial view (if any)
    updateViewEventFilter();
    updateRunActionEnabled();
    updateCachedConfig();

    stateChanged(StateHasConnectionSelected, KXMLGUIClient::StateReverse);
}

KateSQLView::~KateSQLView()
{
    if (m_activeView) {
        m_activeView->removeEventFilter(this);
        if (auto *proxy = m_activeView->focusProxy()) {
            proxy->removeEventFilter(this);
        }
    }

    m_mainWindow->guiFactory()->removeClient(m_outputWidget->dataOutputWidget());
    m_mainWindow->guiFactory()->removeClient(this);

    delete m_outputToolView;
    delete m_schemaBrowserToolView;

    delete m_manager;
}

void KateSQLView::setupActions()
{
    QAction *action;
    KActionCollection *collection = actionCollection();

    action = collection->addAction(QStringLiteral("connection_create"));
    action->setText(i18nc("@action:inmenu", "Add Connection..."));
    action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
    connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionCreate);

    action = collection->addAction(QStringLiteral("connection_remove"));
    action->setText(i18nc("@action:inmenu", "Remove Connection"));
    action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
    connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionRemove);

    action = collection->addAction(QStringLiteral("connection_edit"));
    action->setText(i18nc("@action:inmenu", "Edit Connection..."));
    action->setIcon(QIcon::fromTheme(QStringLiteral("configure"))); // TODO better Icon from QIcon::ThemeIcon::...
    connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionEdit);

    action = collection->addAction(QStringLiteral("connection_reconnect"));
    action->setText(i18nc("@action:inmenu", "Reconnect"));
    action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh));
    connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionReconnect);

    auto *wa = new QWidgetAction(this);
    collection->addAction(QStringLiteral("connection_chooser"), wa);
    wa->setText(i18nc("@action:intoolbar", "Connection"));
    wa->setDefaultWidget(m_connectionsComboBox);

    action = collection->addAction(ActionQueryRun);
    action->setText(i18nc("@action:inmenu", "Run Query"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("quickopen"))); // TODO better Icon from QIcon::ThemeIcon::...
    KActionCollection::setDefaultShortcuts(action, {Qt::CTRL | Qt::Key_Return, Qt::CTRL | Qt::Key_Enter});
    connect(action, &QAction::triggered, this, &KateSQLView::slotRunQuery);

    /// TODO: stop sql query
    //   action = collection->addAction("sql_stop");
    //   action->setText( i18n("Stop query") );
    //   action->setIcon( KIcon("process-stop") );
    //   action->setShortcut( QKeySequence(Qt::ALT | Qt::Key_F5) );
    //   connect( action , SIGNAL(triggered()) , this , SLOT(stopQuery()));
}

void KateSQLView::slotSQLMenuAboutToShow()
{
    qDeleteAll(m_connectionsGroup->actions());

    QMenu *sqlMenu = static_cast<QMenu *>(factory()->container(MenuSQL, this));
    QAction *before = action(ActionQueryRun);
    QAbstractItemModel *model = m_manager->connectionModel();

    int rows = model->rowCount(QModelIndex());

    for (int row = 0; row < rows; row++) {
        QModelIndex index = model->index(row, 0, QModelIndex());

        Q_ASSERT(index.isValid());

        QString connectionName = index.data(Qt::DisplayRole).toString();

        auto *act = new QAction(connectionName, m_connectionsGroup);
        act->setCheckable(true);

        if (m_connectionsComboBox->currentText() == connectionName) {
            act->setChecked(true);
        }

        sqlMenu->insertAction(before, act);
    }

    sqlMenu->insertSeparator(before);
}

void KateSQLView::slotConnectionSelectedFromMenu(QAction *action)
{
    m_connectionsComboBox->setCurrentItem(action->text());
}

void KateSQLView::slotConnectionChanged(int index)
{
    if (index >= 0) {
        const QString connection = m_connectionsComboBox->itemText(index);
        stateChanged(StateHasConnectionSelected, (connection.isEmpty()) ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse);

        m_schemaBrowserWidget->schemaWidget()->buildTree(connection);
    }

    updateRunActionEnabled();
}

void KateSQLView::slotGlobalSettingsChanged()
{
    m_outputWidget->dataOutputWidget()->readConfig();

    updateCachedConfig();
}

void KateSQLView::readSessionConfig(KConfigGroup const &group)
{
    m_manager->loadConnections(group);

    QString lastConnection = group.readEntry(KateSQLConstants::Config::LastUsed);

    if (m_connectionsComboBox->contains(lastConnection)) {
        m_connectionsComboBox->setCurrentItem(lastConnection);
    }
}

void KateSQLView::writeSessionConfig(KConfigGroup &group)
{
    group.deleteGroup(QLatin1String());

    KConfigGroup globalConfig(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    bool saveConnections = globalConfig.readEntry(KateSQLConstants::Config::SaveConnections, KateSQLConstants::Config::DefaultValues::SaveConnections);

    if (saveConnections) {
        m_manager->saveConnections(&group);
        group.writeEntry(KateSQLConstants::Config::LastUsed, m_connectionsComboBox->currentText());
    }
}

void KateSQLView::slotConnectionCreate()
{
    Connection c;

    ConnectionWizard wizard(m_manager, &c);

    if (wizard.exec() != QDialog::Accepted) {
        return;
    }

    for (int i = 1; QSqlDatabase::contains(c.name); i++) {
        c.name = QStringLiteral("%1 (%2)").arg(c.name).arg(i);
    }

    m_manager->createConnection(c);

    if (m_manager->storeCredentials(c) != KWalletConnectionResponse::Success) {
        qDebug("Connection credentials not saved");
    }
}

void KateSQLView::slotConnectionEdit()
{
    int i = m_connectionsComboBox->currentIndex();

    if (i == -1) {
        return;
    }

    ConnectionModel *model = m_manager->connectionModel();
    auto c = model->data(model->index(i), Qt::UserRole).value<Connection>();

    QString previousName = c.name;

    ConnectionWizard wizard(m_manager, &c);

    if (wizard.exec() != QDialog::Accepted) {
        return;
    }

    m_manager->removeConnection(previousName);
    m_manager->createConnection(c);

    if (m_manager->storeCredentials(c) != KWalletConnectionResponse::Success) {
        qDebug("Connection credentials not saved");
    }
}

void KateSQLView::slotConnectionRemove()
{
    QString connection = m_connectionsComboBox->currentText();

    if (!connection.isEmpty()) {
        m_manager->removeConnection(connection);
    }
}

void KateSQLView::slotConnectionReconnect()
{
    QString connection = m_connectionsComboBox->currentText();

    if (!connection.isEmpty()) {
        m_manager->reopenConnection(connection);
    }
}

void KateSQLView::slotConnectionAboutToBeClosed(const QString &name)
{
    /// must delete the QSqlQuery object inside the model before closing connection

    if (name == m_currentResultsetConnection) {
        m_outputWidget->dataOutputWidget()->clearResults();
    }
}

void KateSQLView::slotRunQuery()
{
    /// TODO:
    /// bind parameters dialog?

    QString connection = m_connectionsComboBox->currentText();

    if (connection.isEmpty()) {
        slotConnectionCreate();
        return;
    }

    KTextEditor::View *view = m_mainWindow->activeView();

    if (!view) {
        return;
    }

    QString text = view->selection() ? view->selectionText() : view->document()->text();
    text = text.trimmed();

    if (!text.isEmpty()) {
        runMultiStatementText(text, connection);
        view->setFocus();
    }
}

void KateSQLView::slotError(const QString &message)
{
    m_outputWidget->textOutputWidget()->showErrorMessage(message);
    m_outputWidget->setCurrentWidget(m_outputWidget->textOutputWidget());
    m_mainWindow->showToolView(m_outputToolView);
}

void KateSQLView::slotSuccess(const QString &message)
{
    m_outputWidget->textOutputWidget()->showSuccessMessage(message);
    m_outputWidget->setCurrentWidget(m_outputWidget->textOutputWidget());
    m_mainWindow->showToolView(m_outputToolView);
}

void KateSQLView::slotQueryActivated(QSqlQuery &query, const QString &connection)
{
    if (query.isSelect()) {
        m_currentResultsetConnection = connection;

        m_outputWidget->dataOutputWidget()->showQueryResultSets(query, connection);
        m_outputWidget->setCurrentWidget(m_outputWidget->dataOutputWidget());
        m_mainWindow->showToolView(m_outputToolView);
    }
}

void KateSQLView::slotEditableQueryActivated(DataOutputEditableModel *model, const QString &connection, const QMap<QString, QString> &displayColumns)
{
    if (!model) {
        return;
    }

    m_currentResultsetConnection = connection;

    m_outputWidget->dataOutputWidget()->showEditableTable(model, connection, displayColumns);
    m_outputWidget->setCurrentWidget(m_outputWidget->dataOutputWidget());
    m_mainWindow->showToolView(m_outputToolView);
}

void KateSQLView::slotEditableRelationalQueryActivated(DataOutputModelInterface *model, const QString &connection, const QMap<QString, QString> &displayColumns)
{
    if (!model) {
        return;
    }

    m_currentResultsetConnection = connection;

    m_outputWidget->dataOutputWidget()->showEditableTable(model, connection, displayColumns);
    m_outputWidget->setCurrentWidget(m_outputWidget->dataOutputWidget());
    m_mainWindow->showToolView(m_outputToolView);
}

void KateSQLView::slotConnectionCreated(const QString &name)
{
    m_connectionsComboBox->setCurrentItem(name);

    m_schemaBrowserWidget->schemaWidget()->buildTree(name);
}

void KateSQLView::updateRunActionEnabled()
{
    auto *runAction = actionCollection()->action(ActionQueryRun);
    if (!runAction) {
        return;
    }

    const bool hasConnection = m_connectionsComboBox->currentIndex() >= 0;
    runAction->setEnabled(hasConnection);
}

void KateSQLView::updateViewEventFilter()
{
    // Remove filter from previous view
    if (m_activeView) {
        m_activeView->removeEventFilter(this);
        if (auto *proxy = m_activeView->focusProxy()) {
            proxy->removeEventFilter(this);
        }
        m_activeView = nullptr;
    }

    auto *view = m_mainWindow->activeView();
    if (!view) {
        return;
    }

    // NOTE: We install on the focus proxy (not the view) because Qt delivers
    // ShortcutOverride events to the focus widget, which is the proxy.
    // If the proxy were to change mid-lifetime (it doesn't in KatePart —
    // KateViewInternal is created once), the filter would become stale until
    // the next view change reinstalls it.
    if (auto *proxy = view->focusProxy()) {
        proxy->installEventFilter(this);
    } else {
        view->installEventFilter(this);
    }
    m_activeView = view;
}

bool KateSQLView::eventFilter(QObject *obj, QEvent *event)
{
    // If our tracked view was deleted, bail immediately.
    // QPointer nulls on destruction, but events can still arrive
    // during the destruction sequence.
    if (!m_activeView) {
        return QObject::eventFilter(obj, event);
    }

    if (event->type() != QEvent::ShortcutOverride && event->type() != QEvent::KeyPress) {
        return QObject::eventFilter(obj, event);
    }

    auto *view = m_mainWindow->activeView();
    if (!view) {
        return QObject::eventFilter(obj, event);
    }

    if (!m_enableRunOutsideSqlFiles) {
        return QObject::eventFilter(obj, event);
    }

    auto *ke = static_cast<QKeyEvent *>(event);

    const QKeySequence eventShortCut(ke->keyCombination());

    auto *action = actionCollection()->action(ActionQueryRun);
    if (!action || !action->isEnabled()) {
        return QObject::eventFilter(obj, event);
    }

    const QList<QKeySequence> actions = action->shortcuts();

    bool isShortcutForAction = false;
    for (const QKeySequence &a : actions) {
        if (a == eventShortCut) {
            isShortcutForAction = true;
            break;
        }
    }

    if (!isShortcutForAction) {
        return QObject::eventFilter(obj, event);
    }

    if (event->type() == QEvent::ShortcutOverride) {
        // Accept to suppress any other action bound to Ctrl+Return
        event->accept();
        return true;
    }

    event->accept();
    action->trigger();

    return true;
}

void KateSQLView::runMultiStatementText(const QString &text, const QString &connection)
{
    QStringList queries;
    if (m_blankLineBreaksStatements) {
        queries = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    } else {
        queries = {text};
    }

    for (const auto &query : std::as_const(queries)) {
        const auto trimmedQuery = query.trimmed();
        if (!trimmedQuery.isEmpty()) {
            m_manager->runQuery(query, connection);
        }
    }
}

void KateSQLView::updateCachedConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    m_blankLineBreaksStatements =
        config.readEntry(KateSQLConstants::Config::TreatBlankLineAsStatementBreak, KateSQLConstants::Config::DefaultValues::TreatBlankLineAsStatementBreak);
}

// END KateSQLView

#include "moc_katesqlview.cpp"
