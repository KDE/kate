/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

class SQLManager;
class SchemaWidget;

#include <QWidget>

class SchemaBrowserWidget : public QWidget
{
public:
    SchemaBrowserWidget(QWidget *parent, SQLManager *manager);
    ~SchemaBrowserWidget() override;

    SchemaWidget *schemaWidget() const;

private:
    SchemaWidget *m_schemaWidget;
};
