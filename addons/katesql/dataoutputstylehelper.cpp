/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

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
    m_defaultStyle.font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    m_defaultStyle.foreground = qApp->palette().text();
    m_defaultStyle.background = qApp->palette().base();
}

void DataOutputStyleHelper::readConfig()
{
    updateDefaultStyle();

    const KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    const bool useSystemDefaults = config.readEntry(KateSQLConstants::Config::UseSystemDefaults, false);

    const KConfigGroup group = config.group(KateSQLConstants::Config::OutputCustomizationGroup);

    const auto styleKeys = m_styles.keys();
    for (const QString &k : styleKeys) {
        OutputStyle *s = m_styles[k];

        s->foreground = m_defaultStyle.foreground;
        s->background = m_defaultStyle.background;
        s->font = m_defaultStyle.font;

        if (useSystemDefaults) {
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

    switch (value.typeId()) {
    case QMetaType::Type::UnknownType:
    case QMetaType::Type::Void:
    case QMetaType::Type::Nullptr:
        return QLatin1String("NULL");
    case QMetaType::Type::QByteArray:
        return value.toByteArray().left(255);
    case QMetaType::Type::Bool:
        return value.toBool() ? QLatin1String("True") : QLatin1String("False");
    case QMetaType::Type::QDate:
        if (useSystemLocale)
            return QLocale().toString(value.toDate(), QLocale::ShortFormat);
        break;
    case QMetaType::Type::QTime:
        if (useSystemLocale)
            return QLocale().toString(value.toTime());
        break;
    case QMetaType::Type::QDateTime:
        if (useSystemLocale)
            return QLocale().toString(value.toDateTime(), QLocale::ShortFormat);
        break;
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
    case QMetaType::Type::Int:
    case QMetaType::Type::UInt:
    case QMetaType::Type::LongLong:
    case QMetaType::Type::ULongLong:
    case QMetaType::Type::Double:
    case QMetaType::Type::Long:
    case QMetaType::Type::Short:
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

QVariant DataOutputStyleHelper::styleData(const QVariant &value, int role) const
{
    const auto type = value.typeId();

    switch (role) {
    case Qt::FontRole:
        return m_styles.value(getStyleKey(value, type))->font;
    case Qt::ForegroundRole:
        return m_styles.value(getStyleKey(value, type))->foreground;
    case Qt::BackgroundRole:
        return m_styles.value(getStyleKey(value, type))->background;
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
