/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gitcommitdialog.h"

#include <QCoreApplication>
#include <QDebug>
#include <QInputMethodEvent>
#include <QSyntaxHighlighter>
#include <QVBoxLayout>

#include <KColorScheme>
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
        setFormat(badLength, text.length() - badLength, red);
    }

private:
    int badLength = 0;
    const QColor red = KColorScheme().foreground(KColorScheme::NegativeText).color();
};

static void changeTextColorToRed(QLineEdit *lineEdit, const QColor &red)
{
    if (!lineEdit)
        return;

    // Everything > 52 = red color
    QList<QInputMethodEvent::Attribute> attributes;
    if (lineEdit->text().length() > 52) {
        int start = 52 - lineEdit->cursorPosition();
        int len = lineEdit->text().length() - start;
        QInputMethodEvent::AttributeType type = QInputMethodEvent::TextFormat;

        QTextCharFormat fmt;
        fmt.setForeground(red);
        QVariant format = fmt;

        attributes.append(QInputMethodEvent::Attribute(type, start, len, format));
    }
    QInputMethodEvent event(QString(), attributes);
    QCoreApplication::sendEvent(lineEdit, &event);
}

GitCommitDialog::GitCommitDialog(const QString &lastCommit, const QFont &font, QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    setWindowTitle(i18n("Commit Changes"));

    ok.setText(i18n("Commit"));
    cancel.setText(i18n("Cancel"));

    m_le.setPlaceholderText(i18n("Write commit message..."));
    m_le.setFont(font);

    QFontMetrics fm(font);
    /** Add 8 because 4 + 4 margins on left / right */
    const int width = (fm.averageCharWidth() * 72) + 8;

    m_leLen.setText(QStringLiteral("0 / 52"));

    m_pe.setPlaceholderText(i18n("Extended commit description..."));
    m_pe.setFont(font);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(4, 4, 4, 4);
    setLayout(vlayout);

    QHBoxLayout *hLayoutLine = new QHBoxLayout;
    hLayoutLine->addStretch();
    hLayoutLine->addWidget(&m_leLen);

    vlayout->addLayout(hLayoutLine);
    vlayout->addWidget(&m_le);
    vlayout->addWidget(&m_pe);

    // set 72 chars wide plain text edit
    m_pe.resize(width, m_pe.height());
    resize(width, fm.averageCharWidth() * 52);

    // restore last message ?
    if (!lastCommit.isEmpty()) {
        auto msgs = lastCommit.split(QStringLiteral("[[\n\n]]"));
        if (!msgs.isEmpty()) {
            m_le.setText(msgs.at(0));
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
    connect(&m_le, &QLineEdit::textChanged, this, &GitCommitDialog::updateLineSizeLabel);

    updateLineSizeLabel();

    vlayout->addLayout(hLayout);

    auto hl = new BadLengthHighlighter(m_pe.document(), 72);
    Q_UNUSED(hl)
}

QString GitCommitDialog::subject() const
{
    return m_le.text();
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
    const QColor red = KColorScheme().foreground(KColorScheme::NegativeText).color();
    int len = m_le.text().length();
    if (len < 52) {
        m_leLen.setText(i18nc("Number of characters", "%1 / 52", QString::number(len)));
    } else {
        changeTextColorToRed(&m_le, red);
        m_leLen.setText(i18nc("Number of characters", "<span style=\"color:%1;\">%2</span> / 52", red.name(), QString::number(len)));
    }
}
