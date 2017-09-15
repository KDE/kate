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

#ifndef EXPORTWIZARD_H
#define EXPORTWIZARD_H


class KUrlRequester;
class KLineEdit;

class QRadioButton;
class QCheckBox;

#include <qwizard.h>

class ExportWizard : public QWizard
{
  public:
    ExportWizard(QWidget *parent);
    ~ExportWizard() override;
};


class ExportOutputPage : public QWizardPage
{
  public:
    ExportOutputPage(QWidget *parent=nullptr);

    void initializePage() override;
    bool validatePage() override;

  private:
    QRadioButton *documentRadioButton;
    QRadioButton *clipboardRadioButton;
    QRadioButton *fileRadioButton;
    KUrlRequester *fileUrl;
};

class ExportFormatPage : public QWizardPage
{
  public:
    ExportFormatPage(QWidget *parent=nullptr);

    void initializePage() override;
    bool validatePage() override;

  private:
    QCheckBox *exportColumnNamesCheckBox;
    QCheckBox *exportLineNumbersCheckBox;
    QCheckBox *quoteStringsCheckBox;
    QCheckBox *quoteNumbersCheckBox;
    KLineEdit *quoteStringsLine;
    KLineEdit *quoteNumbersLine;
    KLineEdit *fieldDelimiterLine;
};

#endif // EXPORTWIZARD_H
