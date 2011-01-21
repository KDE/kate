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

#ifndef DATAOUTPUTWIDGET_H
#define DATAOUTPUTWIDGET_H

class QTextStream;
class QVBoxLayout;
class QSqlQuery;
class DataOutputModel;
class DataOutputView;

#include <qwidget.h>

class DataOutputWidget : public QWidget
{
  Q_OBJECT

  public:
    enum Option {
      NoOptions = 0x0,
      ExportColumnNames = 0x1,
      ExportLineNumbers = 0x2
    };

    Q_DECLARE_FLAGS(Options, Option)

    DataOutputWidget(QWidget *parent);
    ~DataOutputWidget();

    void exportData(QTextStream &stream,
                    const QChar stringsQuoteChar = '\0',
                    const QChar numbersQuoteChar = '\0',
                    const QString fieldDelimiter = "\t",
                    const Options opt = NoOptions);

    DataOutputModel *model() const { return m_model; }
    DataOutputView  *view()  const { return m_view; }

  public slots:
    void showQueryResultSets(QSqlQuery &query);
    void resizeColumnsToContents();
    void resizeRowsToContents();
    void clearResults();

    void slotToggleLocale();
    void slotCopySelected();
    void slotExport();

  private:
    QVBoxLayout *m_dataLayout;

    /// TODO: manage multiple views for query with multiple resultsets
    DataOutputModel *m_model;
    DataOutputView *m_view;

    bool m_isEmpty;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DataOutputWidget::Options)

#endif // DATAOUTPUTWIDGET_H
