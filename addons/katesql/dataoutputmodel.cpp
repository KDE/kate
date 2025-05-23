/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputmodel.h"
#include "outputstyle.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QPalette>

inline static bool isNumeric(const int type)
{
    return (type > 1 && type < 7);
}

DataOutputModel::DataOutputModel(QObject *parent)
    : CachedSqlQueryModel(parent, 1000)
{
    m_useSystemLocale = false;

    m_styles.insert(QStringLiteral("text"), new OutputStyle());
    m_styles.insert(QStringLiteral("number"), new OutputStyle());
    m_styles.insert(QStringLiteral("null"), new OutputStyle());
    m_styles.insert(QStringLiteral("blob"), new OutputStyle());
    m_styles.insert(QStringLiteral("datetime"), new OutputStyle());
    m_styles.insert(QStringLiteral("bool"), new OutputStyle());

    readConfig();
}

DataOutputModel::~DataOutputModel()
{
    qDeleteAll(m_styles);
}

void DataOutputModel::clear()
{
    beginResetModel();

    CachedSqlQueryModel::clear();

    endResetModel();
}

void DataOutputModel::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("KateSQLPlugin"));

    KConfigGroup group = config.group(QStringLiteral("OutputCustomization"));

    const auto styleKeys = m_styles.keys();
    for (const QString &k : styleKeys) {
        OutputStyle *s = m_styles[k];

        KConfigGroup g = group.group(k);

        s->foreground = qApp->palette().text();
        s->background = qApp->palette().base();
        s->font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);

        QFont dummy = g.readEntry("font", QFontDatabase::systemFont(QFontDatabase::GeneralFont));

        s->font.setBold(dummy.bold());
        s->font.setItalic(dummy.italic());
        s->font.setUnderline(dummy.underline());
        s->font.setStrikeOut(dummy.strikeOut());
        s->foreground.setColor(g.readEntry("foregroundColor", s->foreground.color()));
        s->background.setColor(g.readEntry("backgroundColor", s->background.color()));
    }

    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

bool DataOutputModel::useSystemLocale() const
{
    return m_useSystemLocale;
}

void DataOutputModel::setUseSystemLocale(bool useSystemLocale)
{
    m_useSystemLocale = useSystemLocale;

    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

QVariant DataOutputModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::EditRole) {
        return CachedSqlQueryModel::data(index, role);
    }

    QVariant value(CachedSqlQueryModel::data(index, Qt::DisplayRole));
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
            if (useSystemLocale()) {
                return QVariant(value.toString()); // FIXME KF5 KGlobal::locale()->formatNumber(value.toString(), false));
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
            if (useSystemLocale()) {
                if (type == QMetaType::QDate) {
                    return QVariant(QLocale().toString(value.toDate(), QLocale::ShortFormat));
                }
                if (type == QMetaType::QTime) {
                    return QVariant(QLocale().toString(value.toTime()));
                }
                if (type == QMetaType::QDateTime) {
                    return QVariant(QLocale().toString(value.toDateTime(), QLocale::ShortFormat));
                }
            } else { // return sql server format
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

    return CachedSqlQueryModel::data(index, role);
}
