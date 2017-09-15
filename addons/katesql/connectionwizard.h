/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef CONNECTIONWIZARD_H
#define CONNECTIONWIZARD_H

class SQLManager;
class KComboBox;
class KLineEdit;
class QSpinBox;
class KUrlRequester;

#include "connection.h"

#include <kwallet.h>
#include <qwizard.h>

class ConnectionWizard : public QWizard
{
  public:

    enum
    {
      Page_Driver,
      Page_Standard_Server,
      Page_SQLite_Server,
      Page_Save
    };

    ConnectionWizard(SQLManager *manager, Connection *conn, QWidget *parent=nullptr, Qt::WindowFlags flags = nullptr);
    ~ConnectionWizard() override;

    SQLManager *manager() { return m_manager; }
    Connection *connection() { return m_connection; }

  private:
    SQLManager *m_manager;
    Connection *m_connection;
};


class ConnectionDriverPage : public QWizardPage
{
  public:
    ConnectionDriverPage(QWidget *parent=nullptr);
    void initializePage() override;
    int nextId() const override;

  private:
    KComboBox *driverComboBox;
};

class ConnectionStandardServerPage : public QWizardPage
{
  public:
    ConnectionStandardServerPage(QWidget *parent=nullptr);
    ~ConnectionStandardServerPage() override;
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

  private:
    KLineEdit *hostnameLineEdit;
    KLineEdit *usernameLineEdit;
    KLineEdit *passwordLineEdit;
    KLineEdit *databaseLineEdit;
    KLineEdit *optionsLineEdit;
    QSpinBox  *portSpinBox;
};

class ConnectionSQLiteServerPage : public QWizardPage
{
  public:
    ConnectionSQLiteServerPage(QWidget *parent=nullptr);
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

  private:
//     KLineEdit *pathLineEdit;
    KUrlRequester *pathUrlRequester;

    KLineEdit *optionsLineEdit;
};

class ConnectionSavePage : public QWizardPage
{
  public:
    ConnectionSavePage(QWidget *parent=nullptr);
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

  private:
    KLineEdit *connectionNameLineEdit;
};

#endif // CONNECTIONWIZARD_H
