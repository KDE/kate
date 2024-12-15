// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QDir>
#include <QMap>
#include <QObject>
#include <QString>

class AppWizardReader : public QObject
{
    Q_OBJECT
public:
    struct Replacement {
        QString descr;
        QString placeholder;
        QString defaultValue;
    };

    struct TemplateData {
        QString name;
        QString description;
        QString icon; // to not copy to destination
        QString fileToOpen;
        QString packagePath;
    };

    explicit AppWizardReader(QObject *parent = nullptr);

    /**
     * @returns a map of app-wizard template data with the Category + Name as key
     *
     */
    QMap<QString, TemplateData> appWizardTemplates() const;

    /**
     * @return the replacement map with the placeholder as key and default values
     */

    QList<Replacement> replacements() const;

    /**
     * @return true if the archive was extracted successfully
     */
    bool extractTemplateTo(const QString &packageFile, const QString &dest) const;
};
