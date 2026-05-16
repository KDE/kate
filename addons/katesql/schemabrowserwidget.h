/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

class SQLManager;
class SchemaWidget;

class QLineEdit;

#include <QWidget>

class SchemaBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    SchemaBrowserWidget(QWidget *parent, SQLManager *manager);
    ~SchemaBrowserWidget() override;

    SchemaWidget *schemaWidget() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void searchFieldOpen();
    void searchFieldCommit();
    void searchFieldClose();

private:
    SchemaWidget *m_schemaWidget;
    QLineEdit *m_searchInput;
};
