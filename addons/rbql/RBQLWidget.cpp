/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "RBQLWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/View>

RBQLWidget::RBQLWidget(KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
{
    new QVBoxLayout(this);
    layout()->setContentsMargins({});
    layout()->addWidget(&m_tabWidget);

    m_tabWidget.addTab(new RBQLTab(mainWindow, this), QStringLiteral("1"));
    m_tabWidget.setTabsClosable(true);
    connect(&m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int idx) {
        if (auto w = m_tabWidget.widget(idx)) {
            w->deleteLater();
        }
        m_tabWidget.removeTab(idx);
    });
}

void RBQLWidget::newTab()
{
    int i = m_tabWidget.addTab(new RBQLTab(m_mainWindow, this), QString::number(m_tabWidget.count() + 1));
    m_tabWidget.setCurrentIndex(i);
}

RBQLTab::RBQLTab(KTextEditor::MainWindow *mainWindow, RBQLWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
{
    auto l = new QVBoxLayout(this);

    m_queryLineEdit = new QLineEdit(this);
    m_queryLineEdit->setPlaceholderText(i18n("Enter RBQL query"));
    m_newTabBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("tab-new")), QString());
    m_newTabBtn->setToolTip(i18n("Add new tab"));
    m_execBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("media-playback-start")), QString());
    m_execBtn->setToolTip(i18n("Execute query"));
    connect(m_newTabBtn, &QAbstractButton::clicked, parent, &RBQLWidget::newTab);
    connect(m_execBtn, &QAbstractButton::clicked, this, &RBQLTab::exec);

    m_execBtn->setDisabled(true);
    connect(m_queryLineEdit, &QLineEdit::textChanged, this, [this] {
        m_execBtn->setEnabled(!m_queryLineEdit->text().isEmpty());
    });

    auto hl = new QHBoxLayout();
    hl->addWidget(m_queryLineEdit);
    hl->addWidget(m_execBtn);
    hl->addWidget(m_newTabBtn);
    l->addLayout(hl);

    m_hasHeader = new QCheckBox(i18n("File has header"));
    hl = new QHBoxLayout();
    hl->addWidget(m_hasHeader);
    l->addLayout(hl);

    m_tableView = new QTableView(this);
    m_tableView->setVisible(false);
    m_errorLabel = new QLabel(this);
    m_errorLabel->setVisible(false);
    m_errorLabel->setWordWrap(true);
    l->addWidget(m_errorLabel);
    l->addWidget(m_tableView);

    connect(&m_queryExecutionFuture, &QFutureWatcher<QStandardItemModel *>::finished, this, &RBQLTab::onQueryExecuted);

    l->addStretch();
}

RBQLTab::~RBQLTab()
{
    m_queryExecutionFuture.disconnect(this);
    if (m_queryExecutionFuture.isRunning()) {
        m_queryExecutionFuture.cancel();
        m_queryExecutionFuture.waitForFinished();
    }
    delete m_tableView->model();
}

void RBQLTab::exec()
{
    if (m_queryLineEdit->text().isEmpty()) {
        return;
    }

    if (m_queryExecutionFuture.isRunning()) {
        return;
    }

    QString sep = getSeparatorForDocument();
    if (sep.isEmpty()) {
        reportError(i18n("Failed to get separator for current document. Not a CSV/TSV doc?"));
        return;
    }

    m_errorLabel->setVisible(false);

    auto d = m_mainWindow->activeView()->document();
    QStringList lines = d->textLines(d->documentRange());
    if (lines.empty()) {
        reportError(i18n("Document is empty"));
        return;
    }

    initEngine();

    // delete old model
    delete m_tableView->model();
    m_tableView->setModel(nullptr);

    bool includeHeader = m_hasHeader->isChecked();

    QFuture<QStandardItemModel *> future = QtConcurrent::run(&RBQLTab::execQuery, this, std::move(sep), std::move(lines), includeHeader);
    m_queryExecutionFuture.setFuture(future);
}

QStandardItemModel *RBQLTab::execQuery(const QString &sep, QStringList lines, bool includeHeader)
{
    if (lines.isEmpty()) {
        return nullptr;
    }
    // csv data:
    /*
    name,city,popu
    waqar,islam,1000
    ammar,islam,1200
    abdul,lahore,600
    */
    // test queries:
    // SELECT a.name,a.city WHERE a.popu <= 1000
    // SELECT a1,a2 WHERE a3 <= 1000
    // SELECT NR,*

    int row = 0;
    QStringList header;
    if (includeHeader) {
        row = 1;
        header = lines.at(0).split(sep, Qt::SkipEmptyParts);
    }

    QJSValue inputRecords = m_engine->newArray();
    int recordIdx = 0;
    for (; row < lines.size(); ++row) {
        if (QStringView(lines.at(row)).trimmed().isEmpty()) {
            continue;
        }
        auto line = lines.at(row).split(sep, Qt::SkipEmptyParts);
        auto recordColumn = m_engine->newArray(line.size());
        for (int j = 0; j < line.size(); ++j) {
            recordColumn.setProperty(j, line[j]);
        }
        inputRecords.setProperty(recordIdx++, recordColumn);
    }
    // clear
    lines = {};
    // qDebug() << "InputRecords:" << inputRecords.property(QStringLiteral("length")).toInt();

    auto rbql = m_engine->globalObject().property(QStringLiteral("rbql"));
    auto queryFn = rbql.property(QStringLiteral("query_table"));
    QJSValueList args;
    args << QJSValue(m_queryLineEdit->text());
    args << inputRecords;

    QJSValue output = m_engine->newArray();
    args << output;

    QJSValue outputWarnings = m_engine->newArray();
    args << outputWarnings;

    args << QJSValue(QJSValue::NullValue); // join_table

    QJSValue inputColumnNames = [includeHeader, &header, this]() {
        if (includeHeader) {
            QJSValue inputColumnNames = m_engine->newArray();
            for (int i = 0; i < header.size(); ++i) {
                inputColumnNames.setProperty(i, header.at(i));
            }
            return inputColumnNames;
        }
        return QJSValue(QJSValue::NullValue);
    }();
    // qDebug() << "include Header" << header << inputColumnNames.property(QStringLiteral("length")).toInt();
    args << inputColumnNames; // input_column_names

    args << QJSValue(QJSValue::NullValue); // join_column_names

    QJSValue outputColumnNames = m_engine->newArray();
    args << outputColumnNames;

    auto result = queryFn.call(args);
    if (result.isError()) {
        QMetaObject::invokeMethod(this, [err = result.toString(), this] {
            reportError(err);
        });
        return nullptr;
    }

    QString warnings = outputWarnings.toString();
    if (!warnings.isEmpty()) {
        QMetaObject::invokeMethod(this, [err = result.toString(), this] {
            reportError(err);
        });
    }

    auto model = new QStandardItemModel();

    const int length = output.property(QStringLiteral("length")).toInt();
    // qDebug() << "OutputLen:" << length;
    for (int i = 0; i < length; ++i) {
        auto cols = QList<QStandardItem *>();
        auto colArray = output.property(i);
        int colCount = colArray.property(QStringLiteral("length")).toInt();
        cols.resize(colCount);

        for (int c = 0; c < colCount; ++c) {
            cols[c] = new QStandardItem(colArray.property(c).toString());
        }

        model->appendRow(cols);
        // qDebug() << i << "-" << output.property(i).toString() << output.property(i).isArray();
    }

    if (outputColumnNames.isArray()) {
        const int headerColumnsLength = outputColumnNames.property(QStringLiteral("length")).toInt();
        QStringList headerLabels;
        for (int i = 0; i < headerColumnsLength; ++i) {
            headerLabels.append(outputColumnNames.property(i).toString());
        }
        model->setHorizontalHeaderLabels(headerLabels);
    }

    return model;
}

void RBQLTab::onQueryExecuted()
{
    auto model = m_queryExecutionFuture.result();
    if (!model) {
        return;
    }
    m_tableView->setModel(model);
    m_tableView->setVisible(true);
}

void RBQLTab::initEngine()
{
    if (m_engine) {
        return;
    }

    QFile file(QStringLiteral(":/rbql/rbql.js"));
    if (!file.open(QFile::ReadOnly)) {
        qWarning("Failed to open :/rbql/rbql.js");
        reportError(QStringLiteral("Failed to open :/rbql/rbql.js"));
        return;
    }
    m_engine = std::make_unique<QJSEngine>();
    m_engine->installExtensions(QJSEngine::ConsoleExtension);
    auto res = m_engine->evaluate(QString::fromUtf8(file.readAll()));
    if (res.isError()) {
        qWarning("Failed to init engine %ls", qUtf16Printable(res.toString()));
        reportError(QStringLiteral("Failed to init engine: %1").arg(res.toString()));
    }
}

void RBQLTab::reportError(const QString &error)
{
    if (m_errorLabel->isHidden()) {
        m_errorLabel->setVisible(true);
    }
    m_errorLabel->setText(error);
}

QString RBQLTab::getSeparatorForDocument() const
{
    auto v = m_mainWindow->activeView();
    if (!v) {
        return {};
    }
    QString d = v->document()->highlightingMode().toLower();
    if (d == u"csv") {
        return QStringLiteral(",");
    } else if (d == QLatin1String("tsv")) {
        return QStringLiteral("\t");
    } else if ("csv (pipe)") {
        return QStringLiteral("|");
    } else if ("csv (semicolon)") {
        return QStringLiteral(";");
    }
    return {};
}
