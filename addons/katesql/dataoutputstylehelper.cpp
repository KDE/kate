/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputstylehelper.h"
#include "outputstyle.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QPalette>
#include <QtContainerFwd>

static bool isNumeric(const int type)
{
    return (type >= QMetaType::Type::Int && type <= QMetaType::Type::Double);
}

DataOutputStyleHelper::DataOutputStyleHelper():
    m_styles(
        {
            { QStringLiteral("text"), new OutputStyle() },
            { QStringLiteral("number"), new OutputStyle() },
            { QStringLiteral("null"), new OutputStyle() },
            { QStringLiteral("blob"), new OutputStyle() },
            { QStringLiteral("datetime"), new OutputStyle() },
            { QStringLiteral("bool"), new OutputStyle() },
        }
    ),
    m_useSystemLocale(false)
{
}

DataOutputStyleHelper::~DataOutputStyleHelper()
{
    qDeleteAll(m_styles);
}

void DataOutputStyleHelper::readConfig()
{
    const KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("KateSQLPlugin"));

    const KConfigGroup group = config.group(QStringLiteral("OutputCustomization"));

    const QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    const QPalette defaultPalette = qApp->palette();
    const QBrush defaultTextBrush = defaultPalette.text();
    const QBrush defaultBaseBrush = defaultPalette.base();

    const auto styleKeys = m_styles.keys();
    for (const QString &k : styleKeys) {
        OutputStyle *s = m_styles[k];

        KConfigGroup g = group.group(k);

        s->foreground = defaultTextBrush;
        s->background = defaultBaseBrush;
        s->font = defaultFont;

        const QFont dummy = g.readEntry("font", defaultFont);

        s->font.setBold(dummy.bold());
        s->font.setItalic(dummy.italic());
        s->font.setUnderline(dummy.underline());
        s->font.setStrikeOut(dummy.strikeOut());
        s->foreground.setColor(g.readEntry("foregroundColor", s->foreground.color()));
        s->background.setColor(g.readEntry("backgroundColor", s->background.color()));
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

QVariant DataOutputStyleHelper::styleData(const QVariant &value, int role) const
{
    const auto type = value.typeId();

    if (value.isNull()) {
        if (role == Qt::FontRole) {
            return QVariant(m_styles.value(QStringLiteral("null"))->font);
        }
        if (role == Qt::ForegroundRole) {
            return QVariant(m_styles.value(QStringLiteral("null"))->foreground);
        }
        if (role == Qt::BackgroundRole) {
            return QVariant(m_styles.value(QStringLiteral("null"))->background);
        }
        if (role == Qt::DisplayRole) {
            return QVariant(QLatin1String("NULL"));
        }
    }

    if (type == QMetaType::Type::QByteArray) {
        if (role == Qt::FontRole) {
            return QVariant(m_styles.value(QStringLiteral("blob"))->font);
        }
        if (role == Qt::ForegroundRole) {
            return QVariant(m_styles.value(QStringLiteral("blob"))->foreground);
        }
        if (role == Qt::BackgroundRole) {
            return QVariant(m_styles.value(QStringLiteral("blob"))->background);
        }
        if (role == Qt::DisplayRole) {
            return QVariant(value.toByteArray().left(255));
        }
    }

    if (isNumeric(type)) {
        if (role == Qt::FontRole) {
            return QVariant(m_styles.value(QStringLiteral("number"))->font);
        }
        if (role == Qt::ForegroundRole) {
            return QVariant(m_styles.value(QStringLiteral("number"))->foreground);
        }
        if (role == Qt::BackgroundRole) {
            return QVariant(m_styles.value(QStringLiteral("number"))->background);
        }
        if (role == Qt::TextAlignmentRole) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        if (role == Qt::DisplayRole || role == Qt::UserRole) {
            if (m_useSystemLocale) {
                return QVariant(value.toString());
            } else {
                return QVariant(value.toString());
            }
        }
    }

    if (type == QMetaType::Bool) {
        if (role == Qt::FontRole) {
            return QVariant(m_styles.value(QStringLiteral("bool"))->font);
        }
        if (role == Qt::ForegroundRole) {
            return QVariant(m_styles.value(QStringLiteral("bool"))->foreground);
        }
        if (role == Qt::BackgroundRole) {
            return QVariant(m_styles.value(QStringLiteral("bool"))->background);
        }
        if (role == Qt::DisplayRole) {
            return QVariant(value.toBool() ? QLatin1String("True") : QLatin1String("False"));
        }
    }

    if (type == QMetaType::QDate || type == QMetaType::QTime || type == QMetaType::QDateTime) {
        if (role == Qt::FontRole) {
            return QVariant(m_styles.value(QStringLiteral("datetime"))->font);
        }
        if (role == Qt::ForegroundRole) {
            return QVariant(m_styles.value(QStringLiteral("datetime"))->foreground);
        }
        if (role == Qt::BackgroundRole) {
            return QVariant(m_styles.value(QStringLiteral("datetime"))->background);
        }
        if (role == Qt::DisplayRole || role == Qt::UserRole) {
            if (m_useSystemLocale) {
                if (type == QMetaType::QDate) {
                    return QVariant(QLocale().toString(value.toDate(), QLocale::ShortFormat));
                }
                if (type == QMetaType::QTime) {
                    return QVariant(QLocale().toString(value.toTime()));
                }
                if (type == QMetaType::QDateTime) {
                    return QVariant(QLocale().toString(value.toDateTime(), QLocale::ShortFormat));
                }
            } else {
                return QVariant(value.toString());
            }
        }
    }

    if (role == Qt::FontRole) {
        return QVariant(m_styles.value(QStringLiteral("text"))->font);
    }
    if (role == Qt::ForegroundRole) {
        return QVariant(m_styles.value(QStringLiteral("text"))->foreground);
    }
    if (role == Qt::BackgroundRole) {
        return QVariant(m_styles.value(QStringLiteral("text"))->background);
    }
    if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignVCenter);
    }
    if (role == Qt::DisplayRole) {
        return value.toString();
    }
    if (role == Qt::UserRole) {
        return value;
    }

    return QVariant();
}
