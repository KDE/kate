/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "outputstylewidget.h"
#include "katesqlconstants.h"

#include <KSharedConfig>
#include <QApplication>
#include <QCheckBox>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QStyleHints>

#include <KColorButton>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QMetaEnum>

OutputStyleWidget::OutputStyleWidget(QWidget *parent)
    : QTreeWidget(parent)
    , m_useSystemDefaults(false)
{
    updateDefaultStyle();

    QMetaEnum metaEnumForColumns = QMetaEnum::fromType<ColumnsOrder>();

    setColumnCount(metaEnumForColumns.keyCount());
    setRootIsDecorated(false);

    QStringList headerLabels;

    headerLabels << i18nc("@title:column", "Context") << QString() << QString() << QString() << QString() << i18nc("@title:column", "Text Color")
                 << i18nc("@title:column", "Background Color") << i18nc("@title:column", "Changed Text Color")
                 << i18nc("@title:column", "Changed Background Color");

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

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &OutputStyleWidget::updateDefaultStyle);

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

    foregroundColorButton->setDefaultColor(m_defaultStyle.foreground.color());
    backgroundColorButton->setDefaultColor(m_defaultStyle.background.color());

    setItemWidget(item, ColumnsOrder::BoldCheckBox, boldCheckBox);
    setItemWidget(item, ColumnsOrder::ItalicCheckBox, italicCheckBox);
    setItemWidget(item, ColumnsOrder::UnderlineCheckBox, underlineCheckBox);
    setItemWidget(item, ColumnsOrder::StrikeOutCheckBox, strikeOutCheckBox);
    setItemWidget(item, ColumnsOrder::ForegroundColorButton, foregroundColorButton);
    setItemWidget(item, ColumnsOrder::BackgroundColorButton, backgroundColorButton);

    auto *changedForegroundColorButton = new KColorButton(this);
    auto *changedBackgroundColorButton = new KColorButton(this);

    changedForegroundColorButton->setDefaultColor(m_defaultStyle.changedForeground.color());
    changedBackgroundColorButton->setDefaultColor(m_defaultStyle.changedBackground.color());

    setItemWidget(item, ColumnsOrder::ChangedForegroundColorButton, changedForegroundColorButton);
    setItemWidget(item, ColumnsOrder::ChangedBackgroundColorButton, changedBackgroundColorButton);

    readConfig(item);

    connect(boldCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(italicCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(underlineCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(strikeOutCheckBox, &QCheckBox::toggled, this, &OutputStyleWidget::slotChanged);
    connect(foregroundColorButton, &KColorButton::changed, this, &OutputStyleWidget::slotChanged);
    connect(backgroundColorButton, &KColorButton::changed, this, &OutputStyleWidget::slotChanged);
    connect(changedForegroundColorButton, &KColorButton::changed, this, &OutputStyleWidget::slotChanged);
    connect(changedBackgroundColorButton, &KColorButton::changed, this, &OutputStyleWidget::slotChanged);

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
    auto *changedForegroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedForegroundColorButton));
    auto *changedBackgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedBackgroundColorButton));

    const QFont font = g.readEntry(KateSQLConstants::Config::Style::Font, m_defaultStyle.font);

    boldCheckBox->setChecked(font.bold());
    italicCheckBox->setChecked(font.italic());
    underlineCheckBox->setChecked(font.underline());
    strikeOutCheckBox->setChecked(font.strikeOut());

    foregroundColorButton->setColor(g.readEntry(KateSQLConstants::Config::Style::ForegroundColor, m_defaultStyle.foreground.color()));
    backgroundColorButton->setColor(g.readEntry(KateSQLConstants::Config::Style::BackgroundColor, m_defaultStyle.background.color()));
    changedForegroundColorButton->setColor(g.readEntry(KateSQLConstants::Config::Style::ChangedForegroundColor, m_defaultStyle.changedForeground.color()));
    changedBackgroundColorButton->setColor(g.readEntry(KateSQLConstants::Config::Style::ChangedBackgroundColor, m_defaultStyle.changedBackground.color()));
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
    auto *changedForegroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedForegroundColorButton));
    auto *changedBackgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedBackgroundColorButton));

    QFont f(m_defaultStyle.font);

    f.setBold(boldCheckBox->isChecked());
    f.setItalic(italicCheckBox->isChecked());
    f.setUnderline(underlineCheckBox->isChecked());
    f.setStrikeOut(strikeOutCheckBox->isChecked());

    g.writeEntry(KateSQLConstants::Config::Style::Font, f);
    g.writeEntry(KateSQLConstants::Config::Style::ForegroundColor, foregroundColorButton->color());
    g.writeEntry(KateSQLConstants::Config::Style::BackgroundColor, backgroundColorButton->color());
    g.writeEntry(KateSQLConstants::Config::Style::ChangedForegroundColor, changedForegroundColorButton->color());
    g.writeEntry(KateSQLConstants::Config::Style::ChangedBackgroundColor, changedBackgroundColorButton->color());
}

void OutputStyleWidget::readConfig()
{
    updateDefaultStyle();

    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    m_useSystemDefaults = config.readEntry(KateSQLConstants::Config::UseSystemDefaults, KateSQLConstants::Config::DefaultValues::UseSystemDefaults);

    setItemsEnabled(!m_useSystemDefaults);

    QTreeWidgetItem *root = invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        readConfig(root->child(i));
    }

    updatePreviews();
}

void OutputStyleWidget::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    config.writeEntry(KateSQLConstants::Config::UseSystemDefaults, m_useSystemDefaults);

    QTreeWidgetItem *root = invisibleRootItem();

    for (int i = 0; i < root->childCount(); ++i) {
        writeConfig(root->child(i));
    }
}

bool OutputStyleWidget::useSystemDefaults() const
{
    return m_useSystemDefaults;
}

void OutputStyleWidget::setUseSystemDefaults(bool useSystemDefaults)
{
    if (m_useSystemDefaults == useSystemDefaults) {
        return;
    }

    m_useSystemDefaults = useSystemDefaults;

    setItemsEnabled(!m_useSystemDefaults);
    updatePreviews();
    Q_EMIT changed();
}

void OutputStyleWidget::resetToSystemDefaults()
{
    updateDefaultStyle();

    setTableToCurrentDefaults();
    updatePreviews();
    Q_EMIT changed();
}

void OutputStyleWidget::setTableToCurrentDefaults()
{
    QTreeWidgetItem *root = invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *item = root->child(i);

        auto *boldCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::BoldCheckBox));
        auto *italicCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::ItalicCheckBox));
        auto *underlineCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::UnderlineCheckBox));
        auto *strikeOutCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::StrikeOutCheckBox));
        auto *foregroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ForegroundColorButton));
        auto *backgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::BackgroundColorButton));
        auto *changedForegroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedForegroundColorButton));
        auto *changedBackgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedBackgroundColorButton));

        boldCheckBox->setChecked(false);
        italicCheckBox->setChecked(false);
        underlineCheckBox->setChecked(false);
        strikeOutCheckBox->setChecked(false);
        foregroundColorButton->setColor(m_defaultStyle.foreground.color());
        backgroundColorButton->setColor(m_defaultStyle.background.color());
        changedForegroundColorButton->setColor(m_defaultStyle.changedForeground.color());
        changedBackgroundColorButton->setColor(m_defaultStyle.changedBackground.color());
    }
}

void OutputStyleWidget::updateDefaultStyle()
{
    OutputStyle::updateDefault(m_defaultStyle);

    QTreeWidgetItem *root = invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *item = root->child(i);
        auto *fgButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ForegroundColorButton));
        auto *bgButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::BackgroundColorButton));
        if (fgButton) {
            fgButton->setDefaultColor(m_defaultStyle.foreground.color());
        }
        if (bgButton) {
            bgButton->setDefaultColor(m_defaultStyle.background.color());
        }
        if (m_useSystemDefaults) {
            if (fgButton) {
                fgButton->setColor(m_defaultStyle.foreground.color());
            }
            if (bgButton) {
                bgButton->setColor(m_defaultStyle.background.color());
            }
        }
        auto *changedFgButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedForegroundColorButton));
        auto *changedBgButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ChangedBackgroundColorButton));
        if (changedFgButton) {
            changedFgButton->setDefaultColor(m_defaultStyle.changedForeground.color());
        }
        if (changedBgButton) {
            changedBgButton->setDefaultColor(m_defaultStyle.changedBackground.color());
        }
        if (m_useSystemDefaults) {
            if (changedFgButton) {
                changedFgButton->setColor(m_defaultStyle.changedForeground.color());
            }
            if (changedBgButton) {
                changedBgButton->setColor(m_defaultStyle.changedBackground.color());
            }
        }
    }

    if (m_useSystemDefaults) {
        updatePreviews();
    }
}

void OutputStyleWidget::updatePreviews()
{
    QTreeWidgetItem *root = invisibleRootItem();

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *item = root->child(i);

        const QCheckBox *boldCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::BoldCheckBox));
        const QCheckBox *italicCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::ItalicCheckBox));
        const QCheckBox *underlineCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::UnderlineCheckBox));
        const QCheckBox *strikeOutCheckBox = static_cast<QCheckBox *>(itemWidget(item, ColumnsOrder::StrikeOutCheckBox));
        const KColorButton *foregroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::ForegroundColorButton));
        const KColorButton *backgroundColorButton = static_cast<KColorButton *>(itemWidget(item, ColumnsOrder::BackgroundColorButton));

        QFont f(m_defaultStyle.font);

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

void OutputStyleWidget::setItemsEnabled(bool enabled)
{
    QTreeWidgetItem *root = invisibleRootItem();

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *item = root->child(i);

        for (int col = ColumnsOrder::BoldCheckBox; col <= ColumnsOrder::ChangedBackgroundColorButton; ++col) {
            if (auto *widget = itemWidget(item, col)) {
                widget->setEnabled(enabled);
            }
        }
    }
}

#include "moc_outputstylewidget.cpp"
