/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputstylehelper.h"
#include "katesqlconstants.h"
#include "outputstyle.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QApplication>
#include <QFontDatabase>
#include <QLatin1String>
#include <QLocale>
#include <QPalette>
#include <QtContainerFwd>

DataOutputStyleHelper::DataOutputStyleHelper()
    : m_styles({
          {KateSQLConstants::Config::TypesToStyle::Text, new OutputStyle()},
          {KateSQLConstants::Config::TypesToStyle::Number, new OutputStyle()},
          {KateSQLConstants::Config::TypesToStyle::Null, new OutputStyle()},
          {KateSQLConstants::Config::TypesToStyle::Blob, new OutputStyle()},
          {KateSQLConstants::Config::TypesToStyle::DateTime, new OutputStyle()},
          {KateSQLConstants::Config::TypesToStyle::Bool, new OutputStyle()},
      })
    , m_useSystemLocale(false)
{
    updateDefaultStyle();
}

DataOutputStyleHelper::~DataOutputStyleHelper()
{
    qDeleteAll(m_styles);
}

void DataOutputStyleHelper::updateDefaultStyle()
{
    OutputStyle::updateDefault(m_defaultStyle);
}

void DataOutputStyleHelper::readConfig()
{
    updateDefaultStyle();

    const KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    m_useSystemTheme = config.readEntry(KateSQLConstants::Config::UseSystemDefaults, KateSQLConstants::Config::DefaultValues::UseSystemDefaults);

    const KConfigGroup group = config.group(KateSQLConstants::Config::OutputCustomizationGroup);

    const auto styleKeys = m_styles.keys();
    for (const QString &k : styleKeys) {
        OutputStyle *s = m_styles[k];

        s->foreground = m_defaultStyle.foreground;
        s->background = m_defaultStyle.background;
        s->changedForeground = m_defaultStyle.changedForeground;
        s->changedBackground = m_defaultStyle.changedBackground;
        s->font = m_defaultStyle.font;

        if (m_useSystemTheme) {
            continue;
        }

        KConfigGroup g = group.group(k);

        const QFont dummy = g.readEntry(KateSQLConstants::Config::Style::Font, m_defaultStyle.font);

        s->font.setBold(dummy.bold());
        s->font.setItalic(dummy.italic());
        s->font.setUnderline(dummy.underline());
        s->font.setStrikeOut(dummy.strikeOut());
        s->foreground.setColor(g.readEntry(KateSQLConstants::Config::Style::ForegroundColor, s->foreground.color()));
        s->background.setColor(g.readEntry(KateSQLConstants::Config::Style::BackgroundColor, s->background.color()));
        s->changedForeground.setColor(g.readEntry(KateSQLConstants::Config::Style::ChangedForegroundColor, s->changedForeground.color()));
        s->changedBackground.setColor(g.readEntry(KateSQLConstants::Config::Style::ChangedBackgroundColor, s->changedBackground.color()));
    }
}

bool DataOutputStyleHelper::useSystemLocale() const
{
    return m_useSystemLocale;
}

void DataOutputStyleHelper::setUseSystemLocale(bool useSystemLocale)
{
    m_useSystemLocale = useSystemLocale;
}

static bool isNumeric(const int type)
{
    return (type >= QMetaType::Type::Int && type <= QMetaType::Type::Double) || type == QMetaType::Type::Long || type == QMetaType::Type::Short;
}

static QVariant displayValue(const QVariant &value, bool useSystemLocale)
{
    if (value.isNull()) {
        return QLatin1String("NULL");
    }

    const auto type = value.typeId();
    if (type == QMetaType::Type::Bool) {
        return value.toBool() ? QLatin1String("True") : QLatin1String("False");
    }
    if (type == QMetaType::Type::QByteArray) {
        return value.toByteArray().left(255);
    }
    if (useSystemLocale) {
        switch (type) {
        case QMetaType::Type::QDate:
            return QLocale().toString(value.toDate(), QLocale::ShortFormat);
        case QMetaType::Type::QTime:
            return QLocale().toString(value.toTime());
        case QMetaType::Type::QDateTime:
            return QLocale().toString(value.toDateTime(), QLocale::ShortFormat);
        case QMetaType::Type::Short:
        case QMetaType::Type::Int:
            return QLocale().toString(value.toInt());
        case QMetaType::Type::UInt:
            return QLocale().toString(value.toUInt());
        case QMetaType::Type::Long:
        case QMetaType::Type::LongLong:
            return QLocale().toString(value.toLongLong());
        case QMetaType::Type::ULongLong:
            return QLocale().toString(value.toULongLong());
        case QMetaType::Type::Float16:
        case QMetaType::Type::Float:
            return QLocale().toString(value.toFloat());
        case QMetaType::Type::Double:
            return QLocale().toString(value.toDouble());
        }
    }
    return value.toString();
}

static QLatin1String getStyleKey(const QVariant &value, int type)
{
    namespace Key = KateSQLConstants::Config::TypesToStyle;

    if (value.isNull()) {
        return Key::Null;
    }

    switch (type) {
    case QMetaType::Type::Bool:
        return Key::Bool;
    case QMetaType::Type::Short:
    case QMetaType::Type::Int:
    case QMetaType::Type::UInt:
    case QMetaType::Type::Long:
    case QMetaType::Type::LongLong:
    case QMetaType::Type::ULongLong:
    case QMetaType::Type::Float16:
    case QMetaType::Type::Float:
    case QMetaType::Type::Double:
        return Key::Number;
    case QMetaType::Type::QByteArray:
        return Key::Blob;
    case QMetaType::Type::QDate:
    case QMetaType::Type::QTime:
    case QMetaType::Type::QDateTime:
        return Key::DateTime;
    default:
        return Key::Text;
    }
}

QVariant DataOutputStyleHelper::styleData(const QVariant &value, int role, bool isDirty) const
{
    const auto type = value.typeId();

    switch (role) {
    case Qt::FontRole:
        if (m_useSystemTheme) {
            return m_defaultStyle.font;
        }
        return m_styles.value(getStyleKey(value, type))->font;
    case Qt::ForegroundRole: {
        const auto *s = m_useSystemTheme ? &m_defaultStyle : m_styles.value(getStyleKey(value, type));
        return isDirty ? s->changedForeground : s->foreground;
    }
    case Qt::BackgroundRole: {
        const auto *s = m_useSystemTheme ? &m_defaultStyle : m_styles.value(getStyleKey(value, type));
        return isDirty ? s->changedBackground : s->background;
    }
    case Qt::TextAlignmentRole:
        return QVariant(isNumeric(type) ? (Qt::AlignRight | Qt::AlignVCenter) : Qt::AlignVCenter);
    case Qt::DisplayRole:
        return displayValue(value, m_useSystemLocale);
    case Qt::UserRole:
        if (isNumeric(type) || type == QMetaType::QDate || type == QMetaType::QTime || type == QMetaType::QDateTime)
            return displayValue(value, m_useSystemLocale);
        if (getStyleKey(value, type) == KateSQLConstants::Config::TypesToStyle::Text)
            return value;
        return {};
    default:
        return {};
    }
}
