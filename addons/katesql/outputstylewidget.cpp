/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "outputstylewidget.h"
#include "katesqlconstants.h"

#include <KSharedConfig>
#include <QCheckBox>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>

#include <KColorButton>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QMetaEnum>

OutputStyleWidget::OutputStyleWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    QMetaEnum metaEnumForColumns = QMetaEnum::fromType<ColumnsOrder>();

    setColumnCount(metaEnumForColumns.keyCount());
    setRootIsDecorated(false);

    QStringList headerLabels;

    headerLabels << i18nc("@title:column", "Context") << QString() << QString() << QString() << QString() << i18nc("@title:column", "Text Color")
                 << i18nc("@title:column", "Background Color");

    setHeaderLabels(headerLabels);

    headerItem()->setIcon(ColumnsOrder::BoldCheckBox, QIcon::fromTheme(QIcon::ThemeIcon::FormatTextBold));
    headerItem()->setIcon(ColumnsOrder::ItalicCheckBox, QIcon::fromTheme(QIcon::ThemeIcon::FormatTextItalic));
    headerItem()->setIcon(ColumnsOrder::UnderlineCheckBox, QIcon::fromTheme(QIcon::ThemeIcon::FormatTextUnderline));
    headerItem()->setIcon(ColumnsOrder::StrikeOutCheckBox, QIcon::fromTheme(QIcon::ThemeIcon::FormatTextStrikethrough));

    addContext(KateSQLConstants::Config::TypesToStyle::Text, i18nc("@item:intable", "Text"));
    addContext(KateSQLConstants::Config::TypesToStyle::Number, i18nc("@item:intable", "Number"));
    addContext(KateSQLConstants::Config::TypesToStyle::Bool, i18nc("@item:intable", "Bool"));
    addContext(KateSQLConstants::Config::TypesToStyle::DateTime, i18nc("@item:intable", "Date & Time"));
    addContext(KateSQLConstants::Config::TypesToStyle::Null, i18nc("@item:intable", "NULL"));
    addContext(KateSQLConstants::Config::TypesToStyle::Blob, i18nc("@item:intable", "BLOB"));

    for (int i = 0; i < columnCount(); ++i) {
        resizeColumnToContents(i);
    }

    updatePreviews();
}

OutputStyleWidget::~OutputStyleWidget() = default;

QTreeWidgetItem *OutputStyleWidget::addContext(const QString &key, const QString &name)
{
    auto *item = new QTreeWidgetItem(this);

    item->setText(ColumnsOrder::TypeLabel, name);
    item->setData(ColumnsOrder::TypeLabel, Qt::UserRole, key);

    auto *boldCheckBox = new QCheckBox(this);
    auto *italicCheckBox = new QCheckBox(this);
    auto *underlineCheckBox = new QCheckBox(this);
    auto *strikeOutCheckBox = new QCheckBox(this);
    auto *foregroundColorButton = new KColorButton(this);
    auto *backgroundColorButton = new KColorButton(this);

    foregroundColorButton->setDefaultColor(palette().text().color());
    backgroundColorButton->setDefaultColor(palette().base().color());

    setItemWidget(item, ColumnsOrder::BoldCheckBox, boldCheckBox);
    setItemWidget(item, ColumnsOrder::ItalicCheckBox, italicCheckBox);
    setItemWidget(item, ColumnsOrder::UnderlineCheckBox, underlineCheckBox);
    setItemWidget(item, ColumnsOrder::StrikeOutCheckBox, strikeOutCheckBox);
    setItemWidget(item, ColumnsOrder::ForegroundColorButton, foregroundColorButton);
    setItemWidget(item, ColumnsOrder::BackgroundColorButton, backgroundColorButton);

    readConfig(item);

    connect(boldCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(italicCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(underlineCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(strikeOutCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(foregroundColorButton, &KColorButton::changed, this, &OutputStyleWidget::slotChanged);
    connect(backgroundColorButton, &KColorButton::changed, this, &OutputStyleWidget::slotChanged);

    return item;
}

void OutputStyleWidget::readConfig(QTreeWidgetItem *item)
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    KConfigGroup g = config.group(KateSQLConstants::Config::OutputCustomizationGroup).group(item->data(ColumnsOrder::TypeLabel, Qt::UserRole).toString());

    auto *boldCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::BoldCheckBox));
    auto *italicCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::ItalicCheckBox));
    auto *underlineCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::UnderlineCheckBox));
    auto *strikeOutCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::StrikeOutCheckBox));
    auto *foregroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ForegroundColorButton));
    auto *backgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::BackgroundColorButton));

    const QFont font = g.readEntry(KateSQLConstants::Config::Style::Font, QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    boldCheckBox->setChecked(font.bold());
    italicCheckBox->setChecked(font.italic());
    underlineCheckBox->setChecked(font.underline());
    strikeOutCheckBox->setChecked(font.strikeOut());

    foregroundColorButton->setColor(g.readEntry(KateSQLConstants::Config::Style::ForegroundColor, foregroundColorButton->defaultColor()));
    backgroundColorButton->setColor(g.readEntry(KateSQLConstants::Config::Style::BackgroundColor, backgroundColorButton->defaultColor()));
}

void OutputStyleWidget::writeConfig(QTreeWidgetItem *item)
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    KConfigGroup g = config.group(KateSQLConstants::Config::OutputCustomizationGroup).group(item->data(ColumnsOrder::TypeLabel, Qt::UserRole).toString());

    auto *boldCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::BoldCheckBox));
    auto *italicCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::ItalicCheckBox));
    auto *underlineCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::UnderlineCheckBox));
    auto *strikeOutCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::StrikeOutCheckBox));
    auto *foregroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ForegroundColorButton));
    auto *backgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::BackgroundColorButton));

    QFont f(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    f.setBold(boldCheckBox->isChecked());
    f.setItalic(italicCheckBox->isChecked());
    f.setUnderline(underlineCheckBox->isChecked());
    f.setStrikeOut(strikeOutCheckBox->isChecked());

    g.writeEntry(KateSQLConstants::Config::Style::Font, f);
    g.writeEntry(KateSQLConstants::Config::Style::ForegroundColor, foregroundColorButton->color());
    g.writeEntry(KateSQLConstants::Config::Style::BackgroundColor, backgroundColorButton->color());
}

void OutputStyleWidget::readConfig()
{
    QTreeWidgetItem *root = invisibleRootItem();

    for (int i = 0; i < root->childCount(); ++i) {
        readConfig(root->child(i));
    }
}

void OutputStyleWidget::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    config.deleteGroup(KateSQLConstants::Config::OutputCustomizationGroup);

    QTreeWidgetItem *root = invisibleRootItem();

    for (int i = 0; i < root->childCount(); ++i) {
        writeConfig(root->child(i));
    }
}

void OutputStyleWidget::updatePreviews()
{
    QTreeWidgetItem *root = invisibleRootItem();

    QFont systemFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *item = root->child(i);

        const QCheckBox *boldCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::BoldCheckBox));
        const QCheckBox *italicCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::ItalicCheckBox));
        const QCheckBox *underlineCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::UnderlineCheckBox));
        const QCheckBox *strikeOutCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::StrikeOutCheckBox));
        const KColorButton *foregroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ForegroundColorButton));
        const KColorButton *backgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::BackgroundColorButton));

        QFont f(systemFont);

        f.setBold(boldCheckBox->isChecked());
        f.setItalic(italicCheckBox->isChecked());
        f.setUnderline(underlineCheckBox->isChecked());
        f.setStrikeOut(strikeOutCheckBox->isChecked());

        item->setFont(ColumnsOrder::TypeLabel, f);
        item->setForeground(ColumnsOrder::TypeLabel, foregroundColorButton->color());
        item->setBackground(ColumnsOrder::TypeLabel, backgroundColorButton->color());
    }
}

void OutputStyleWidget::slotChanged()
{
    updatePreviews();

    Q_EMIT changed();
}

#include "moc_outputstylewidget.cpp"
