/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "connectionwizard.h"
#include "katesqlconstants.h"
#include "sqlmanager.h"

#include <KComboBox>
#include <KLineEdit>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPasswordLineEdit>
#include <KUrlRequester>

#include <QFormLayout>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <ktexteditor/codecompletionmodel.h>
#include <qlatin1stringview.h>

ConnectionWizard::ConnectionWizard(SQLManager *manager, Connection *conn, QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags)
    , m_manager(manager)
    , m_connection(conn)
{
    setWindowTitle(i18nc("@title:window", "Connection Wizard"));

    setPage(Page_Driver, new ConnectionDriverPage);
    setPage(Page_Standard_Server, new ConnectionStandardServerPage);
    setPage(Page_SQLite_Server, new ConnectionSQLiteServerPage);
    setPage(Page_Save, new ConnectionSavePage);
}

ConnectionWizard::~ConnectionWizard()
{
}

ConnectionDriverPage::ConnectionDriverPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18nc("@title Wizard page title", "Database Driver"));
    setSubTitle(i18nc("@title Wizard page subtitle", "Select the database driver"));

    auto *layout = new QFormLayout();

    driverComboBox = new KComboBox();
    driverComboBox->addItems(QSqlDatabase::drivers());

    layout->addRow(i18nc("@label:listbox", "Database driver:"), driverComboBox);

    setLayout(layout);

    registerField(QLatin1String(KateSQLConstants::Connection::Driver), driverComboBox, "currentText");
}

void ConnectionDriverPage::initializePage()
{
    auto *wiz = static_cast<ConnectionWizard *>(wizard());
    Connection *c = wiz->connection();

    if (!c->driver.isEmpty()) {
        driverComboBox->setCurrentItem(c->driver);
    }
}

int ConnectionDriverPage::nextId() const
{
    if (driverComboBox->currentText().contains(KateSQLConstants::Connection::QSQLite)) {
        return ConnectionWizard::Page_SQLite_Server;
    } else {
        return ConnectionWizard::Page_Standard_Server;
    }
}

ConnectionStandardServerPage::ConnectionStandardServerPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18nc("@title Wizard page title", "Connection Parameters"));
    setSubTitle(i18nc("@title Wizard page subtitle", "Please enter connection parameters"));

    auto *layout = new QFormLayout();

    hostnameLineEdit = new KLineEdit();
    usernameLineEdit = new KLineEdit();
    passwordLineEdit = new KPasswordLineEdit();
    databaseLineEdit = new KLineEdit();
    optionsLineEdit = new KLineEdit();
    portSpinBox = new QSpinBox();

    portSpinBox->setMaximum(65535);
    portSpinBox->setSpecialValueText(i18nc("@item Spinbox special value", "Default"));
    portSpinBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    layout->addRow(i18nc("@label:textbox", "Hostname:"), hostnameLineEdit);
    layout->addRow(i18nc("@label:textbox", "Username:"), usernameLineEdit);
    layout->addRow(i18nc("@label:textbox", "Password:"), passwordLineEdit);
    layout->addRow(i18nc("@label:spinbox", "Port:"), portSpinBox);
    layout->addRow(i18nc("@label:textbox", "Database name:"), databaseLineEdit);
    layout->addRow(i18nc("@label:textbox", "Connection options:"), optionsLineEdit);

    setLayout(layout);

    registerField(QLatin1String(KateSQLConstants::Connection::Hostname) + FieldIsRequired, hostnameLineEdit);
    registerField(QLatin1String(KateSQLConstants::Connection::Username), usernameLineEdit);
    registerField(QLatin1String(KateSQLConstants::Connection::Password), passwordLineEdit, "password", "passwordChanged");
    registerField(QLatin1String(KateSQLConstants::Connection::Database), databaseLineEdit);
    registerField(QLatin1String(KateSQLConstants::Connection::StdOptions), optionsLineEdit);
    registerField(QLatin1String(KateSQLConstants::Connection::Port), portSpinBox);
}

ConnectionStandardServerPage::~ConnectionStandardServerPage() = default;

void ConnectionStandardServerPage::initializePage()
{
    auto *wiz = static_cast<ConnectionWizard *>(wizard());
    Connection *c = wiz->connection();

    hostnameLineEdit->setText(QStringLiteral("localhost"));

    if (c->driver == field(QLatin1String(KateSQLConstants::Connection::Driver)).toString()) {
        hostnameLineEdit->setText(c->hostname);
        usernameLineEdit->setText(c->username);
        passwordLineEdit->setPassword(c->password);
        databaseLineEdit->setText(c->database);
        optionsLineEdit->setText(c->options);
        portSpinBox->setValue(c->port);
    }

    hostnameLineEdit->selectAll();
}

bool ConnectionStandardServerPage::validatePage()
{
    Connection c;

    c.driver = field(QLatin1String(KateSQLConstants::Connection::Driver)).toString();
    c.hostname = field(QLatin1String(KateSQLConstants::Connection::Hostname)).toString();
    c.username = field(QLatin1String(KateSQLConstants::Connection::Username)).toString();
    c.password = field(QLatin1String(KateSQLConstants::Connection::Password)).toString();
    c.database = field(QLatin1String(KateSQLConstants::Connection::Database)).toString();
    c.options = field(QLatin1String(KateSQLConstants::Connection::StdOptions)).toString();
    c.port = field(QLatin1String(KateSQLConstants::Connection::Port)).toInt();

    QSqlError e;

    auto *wiz = static_cast<ConnectionWizard *>(wizard());

    if (!wiz->manager()->testConnection(c, e)) {
        KMessageBox::error(this, i18n("Unable to connect to database.") + u'\n' + e.text());
        return false;
    }

    return true;
}

int ConnectionStandardServerPage::nextId() const
{
    return ConnectionWizard::Page_Save;
}

ConnectionSQLiteServerPage::ConnectionSQLiteServerPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18nc("@title Wizard page title", "Connection Parameters"));
    setSubTitle(
        i18nc("@title Wizard page subtitle", "Please enter the SQLite database file path.\nIf the file does not exist, a new database will be created."));

    auto *layout = new QFormLayout();

    pathUrlRequester = new KUrlRequester(this);
    optionsLineEdit = new KLineEdit();

    pathUrlRequester->setMode(KFile::File);
    pathUrlRequester->setNameFilters({i18n("Database files") + QLatin1String(" (*.db *.sqlite)"), i18n("All files") + QLatin1String(" (*)")});
    layout->addRow(i18nc("@label:textbox", "Path:"), pathUrlRequester);
    layout->addRow(i18nc("@label:textbox", "Connection options:"), optionsLineEdit);

    setLayout(layout);

    registerField(QLatin1String(KateSQLConstants::Connection::Path) + FieldIsRequired, pathUrlRequester->lineEdit());
    registerField(QLatin1String(KateSQLConstants::Connection::SqliteOptions), optionsLineEdit);
}

void ConnectionSQLiteServerPage::initializePage()
{
    auto *wiz = static_cast<ConnectionWizard *>(wizard());
    Connection *c = wiz->connection();

    if (c->driver == field(QLatin1String(KateSQLConstants::Connection::Driver)).toString()) {
        pathUrlRequester->lineEdit()->setText(c->database);
        optionsLineEdit->setText(c->options);
    }
}

bool ConnectionSQLiteServerPage::validatePage()
{
    Connection c;

    c.driver = field(QLatin1String(KateSQLConstants::Connection::Driver)).toString();
    c.database = field(QLatin1String(KateSQLConstants::Connection::Path)).toString();
    c.options = field(QLatin1String(KateSQLConstants::Connection::SqliteOptions)).toString();

    QSqlError e;

    auto *wiz = static_cast<ConnectionWizard *>(wizard());

    if (!wiz->manager()->testConnection(c, e)) {
        KMessageBox::error(this, xi18nc("@info", "Unable to connect to database.<nl/><message>%1</message>", e.text()));
        return false;
    }

    return true;
}

int ConnectionSQLiteServerPage::nextId() const
{
    return ConnectionWizard::Page_Save;
}

ConnectionSavePage::ConnectionSavePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(i18nc("@title Wizard page title", "Connection Name"));
    setSubTitle(i18nc("@title Wizard page subtitle", "Enter a unique connection name"));

    auto *layout = new QFormLayout();

    connectionNameLineEdit = new KLineEdit();

    layout->addRow(i18nc("@label:textbox", "Connection name:"), connectionNameLineEdit);

    setLayout(layout);

    registerField(QLatin1String(KateSQLConstants::Connection::ConnectionName) + FieldIsRequired, connectionNameLineEdit);
}

void ConnectionSavePage::initializePage()
{
    QString name;

    auto *wiz = static_cast<ConnectionWizard *>(wizard());
    Connection *c = wiz->connection();

    if (!c->name.isEmpty()) {
        name = c->name;
    } else if (field(QLatin1String(KateSQLConstants::Connection::Driver)).toString().contains(KateSQLConstants::Connection::QSQLite)) {
        const QLatin1String SQLiteTempName("SQLite");
        name = SQLiteTempName;

        for (int i = 1; QSqlDatabase::contains(name); i++) {
            name = QStringLiteral("%1%2").arg(SQLiteTempName).arg(i);
        }
    } else {
        name = QStringLiteral("%1 on %2")
                   .arg(field(QLatin1String(KateSQLConstants::Connection::Database)).toString(),
                        field(QLatin1String(KateSQLConstants::Connection::Hostname)).toString())
                   .simplified();

        for (int i = 1; QSqlDatabase::contains(name); i++) {
            name = QStringLiteral("%1 on %2 (%3)")
                       .arg(field(QLatin1String(KateSQLConstants::Connection::Database)).toString())
                       .arg(field(QLatin1String(KateSQLConstants::Connection::Hostname)).toString())
                       .arg(i)
                       .simplified();
        }
    }

    connectionNameLineEdit->setText(name);
    connectionNameLineEdit->selectAll();
}

bool ConnectionSavePage::validatePage()
{
    QString name = field(QLatin1String(KateSQLConstants::Connection::ConnectionName)).toString().simplified();

    auto *wiz = static_cast<ConnectionWizard *>(wizard());
    Connection *c = wiz->connection();

    c->name = name;
    c->driver = field(QLatin1String(KateSQLConstants::Connection::Driver)).toString();

    if (field(QLatin1String(KateSQLConstants::Connection::Driver)).toString().contains(KateSQLConstants::Connection::QSQLite)) {
        c->database = field(QLatin1String(KateSQLConstants::Connection::Path)).toString();
        c->options = field(QLatin1String(KateSQLConstants::Connection::SqliteOptions)).toString();
    } else {
        c->hostname = field(QLatin1String(KateSQLConstants::Connection::Hostname)).toString();
        c->username = field(QLatin1String(KateSQLConstants::Connection::Username)).toString();
        c->password = field(QLatin1String(KateSQLConstants::Connection::Password)).toString();
        c->database = field(QLatin1String(KateSQLConstants::Connection::Database)).toString();
        c->options = field(QLatin1String(KateSQLConstants::Connection::StdOptions)).toString();
        c->port = field(QLatin1String(KateSQLConstants::Connection::Port)).toInt();
    }

    return true;
}

int ConnectionSavePage::nextId() const
{
    return -1;
}
