/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gitcommitdialog.h"

#include <QDebug>
#include <QSyntaxHighlighter>
#include <QVBoxLayout>

#include <KLocalizedString>

class BadLengthHighlighter : public QSyntaxHighlighter
{
public:
    explicit BadLengthHighlighter(QTextDocument *doc, int badLen)
        : QSyntaxHighlighter(doc)
        , badLength(badLen)
    {
    }

    void highlightBlock(const QString &text) override
    {
        if (text.length() < badLength) {
            return;
        }
        setFormat(badLength, text.length() - badLength, Qt::red);
    }

private:
    int badLength = 0;
};

class SingleLineEdit : public QPlainTextEdit
{
public:
    explicit SingleLineEdit(const QFont &font, QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
        , m_hl(new BadLengthHighlighter(document(), 52))
    {
        // create a temporary line edit to figure out the correct size
        QLineEdit le;
        le.setFont(font);
        le.setText(QStringLiteral("TEMP"));
        setFont(font);
        setFixedHeight(le.sizeHint().height());
        setLineWrapMode(QPlainTextEdit::NoWrap);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void setText(const QString &text)
    {
        setPlainText(text);
    }

    int textLength()
    {
        return toPlainText().length();
    }

private:
    BadLengthHighlighter *m_hl;
};

GitCommitDialog::GitCommitDialog(const QString &lastCommit, const QFont &font, QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_le(new SingleLineEdit(font))
    , m_hl(new BadLengthHighlighter(m_pe.document(), 72))
{
    setWindowTitle(i18n("Commit Changes"));

    ok.setText(i18n("Commit"));
    cancel.setText(i18n("Cancel"));

    m_le->setPlaceholderText(i18n("Write commit message..."));
    m_le->setFont(font);

    QFontMetrics fm(font);
    const int width = fm.averageCharWidth() * 72;

    m_leLen.setText(QStringLiteral("0 / 52"));

    m_pe.setPlaceholderText(i18n("Extended commit description..."));
    m_pe.setFont(font);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(0, 0, 0, 0);
    setLayout(vlayout);

    QHBoxLayout *hLayoutLine = new QHBoxLayout;
    hLayoutLine->addStretch();
    hLayoutLine->addWidget(&m_leLen);

    vlayout->addLayout(hLayoutLine);
    vlayout->addWidget(m_le);
    vlayout->addWidget(&m_pe);

    // set 72 chars wide plain text edit
    m_pe.resize(width, m_pe.height());
    resize(width, fm.averageCharWidth() * 52);

    // restore last message ?
    if (!lastCommit.isEmpty()) {
        auto msgs = lastCommit.split(QStringLiteral("[[\n\n]]"));
        if (!msgs.isEmpty()) {
            m_le->setText(msgs.at(0));
            if (msgs.length() > 1) {
                m_pe.setPlainText(msgs.at(1));
            }
        }
    }

    m_cbSignOff.setChecked(false);
    m_cbSignOff.setText(i18n("Sign off"));
    vlayout->addWidget(&m_cbSignOff);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch();
    hLayout->addWidget(&ok);
    hLayout->addWidget(&cancel);

    connect(&ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(&cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_le, &QPlainTextEdit::textChanged, this, &GitCommitDialog::updateLineSizeLabel);

    updateLineSizeLabel();

    vlayout->addLayout(hLayout);
}

QString GitCommitDialog::subject() const
{
    return m_le->toPlainText();
}

QString GitCommitDialog::description() const
{
    return m_pe.toPlainText();
}

bool GitCommitDialog::signoff() const
{
    return m_cbSignOff.isChecked();
}

void GitCommitDialog::updateLineSizeLabel()
{
    int len = m_le->textLength();
    if (len < 52) {
        m_leLen.setText(QStringLiteral("%1 / 52").arg(QString::number(len)));
    } else {
        m_leLen.setText(QStringLiteral("<span style=\"color:red;\">%1</span> / 52").arg(QString::number(len)));
    }
}
