/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QDir>
class KUrlRequester;
class KLineEdit;

class QRadioButton;
class QCheckBox;

#include <QWizard>

class ExportWizard : public QWizard
{
    Q_OBJECT
public:
    explicit ExportWizard(QWidget *parent);
    ~ExportWizard() override;
};

static const struct DefaultExportValues {
    const bool isExportingColumnNames = false;
    const bool isExportingLineNumbers = false;
    const bool isQuotingStrings = false;
    const bool isQuotingNumbers = false;

    const QChar noQuotingChar = u'\0';

    const QChar quoteStringCharForCopyPaste = u'"';
    const QChar quoteNumbersCharForCopyPaste = noQuotingChar;
    const QChar fieldDelimiterForCopyPaste = u'\t';
    const QChar lineDelimiterForCopyPaste = u'\n';

    const QChar quoteStringCharForWizard = u'"';
    const QChar quoteNumbersCharForWizard = u'"';
    const QString fieldDelimiterForWizard = QStringLiteral("\\t");
} defaultExportValues;

class ExportOutputPage : public QWizardPage
{
public:
    explicit ExportOutputPage(QWidget *parent = nullptr);

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
    explicit ExportFormatPage(QWidget *parent = nullptr);

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
