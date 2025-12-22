// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// This file has heavily copied from apptemplatesmodel.cpp from KAppTemplates
// With authors:
// SPDX-FileCopyrightText: 2007 Alexander Dymo <adymo@kdevelop.org>
// SPDX-FileCopyrightText: 2008 Anne-Marie Mahfouf <annma@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AppWizardReader.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KTar>
#include <KZip>

#include <QStandardPaths>
#include <QTemporaryDir>

using namespace Qt::Literals::StringLiterals;

AppWizardReader::AppWizardReader(QObject *parent)
    : QObject(parent)
{
}

QMap<QString, AppWizardReader::TemplateData> AppWizardReader::appWizardTemplates() const
{
    QStringList templateArchives;
    const QStringList templatePaths =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, u"kdevappwizard/templates/"_s, QStandardPaths::LocateDirectory);
    for (const QString &templatePath : std::as_const(templatePaths)) {
        const auto templateFiles = QDir(templatePath).entryList(QDir::Files);
        for (const QString &templateArchive : templateFiles) {
            templateArchives.append(templatePath + templateArchive);
        }
    }

    const QString templateDataBasePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/kdevappwizard/"_s;
    const QString localDescriptionsDir = templateDataBasePath + u"template_descriptions/"_s;
    QDir dir(localDescriptionsDir);
    if (!dir.exists()) {
        dir.mkpath(localDescriptionsDir);
    }

    QMap<QString, TemplateData> map;

    for (const auto &archName : std::as_const(templateArchives)) {
        std::unique_ptr<KArchive> templateArchive = nullptr;
        if (archName.endsWith(u".zip"_s)) {
            templateArchive = std::make_unique<KZip>(archName);
        } else {
            templateArchive = std::make_unique<KTar>(archName, u"application/x-bzip"_s);
        }
        TemplateData data;
        if (templateArchive->open(QIODevice::ReadOnly)) {
            QFileInfo templateInfo(archName);
            const QString descriptionFileName = templateInfo.baseName() + u".kdevtemplate"_s;
            const KArchiveEntry *templateEntry = templateArchive->directory()->entry(descriptionFileName);
            // but could be different name, if e.g. downloaded, so make a guess
            if (!templateEntry || !templateEntry->isFile()) {
                for (const auto &entryName : templateArchive->directory()->entries()) {
                    if (entryName.endsWith(u".kdevtemplate"_s)) {
                        data.kAppTemplateFile = entryName;
                        templateEntry = templateArchive->directory()->entry(entryName);
                        break;
                    }
                }
            }

            if (!templateEntry || !templateEntry->isFile()) {
                qDebug("template %ls does not contain .kdevtemplate file", qUtf16Printable(archName));
                continue;
            }
            const KArchiveFile *templateFile = (KArchiveFile *)templateEntry;

            const QString destinationPath = localDescriptionsDir + descriptionFileName;
            if (templateFile->name() == descriptionFileName) {
                templateFile->copyTo(localDescriptionsDir);
            } else {
                // Rename the extracted description
                // so that its basename matches the basename of the template archive
                // Use temporary dir to not overwrite other files with same name
                QTemporaryDir tempDir;
                templateFile->copyTo(tempDir.path());
                QFile::remove(destinationPath);
                QFile::rename(tempDir.path() + QLatin1Char('/') + templateFile->name(), destinationPath);
            }

            KConfig templateConfig(destinationPath);
            KConfigGroup general(&templateConfig, QStringLiteral("General"));
            data.name = general.readEntry("Name");
            data.description = general.readEntry("Comment");
            data.icon = general.readEntry("Icon");
            data.fileToOpen = general.readEntry("ShowFilesAfterGeneration");
            data.packagePath = archName;
            QString category = general.readEntry("Category");
            QString key = category + u'/' + data.name;

            map.insert(key, data);

        } else {
            qDebug("could not open template %ls", qUtf16Printable(archName));
        }
    }
    return map;
}

QList<AppWizardReader::Replacement> AppWizardReader::replacements() const
{
    QList<Replacement> reps;
    reps.append({i18n("Application Name"), "%{APPNAME}"_ba, "MyApp"_ba});
    reps.append({i18n("Application Version"), "%{VERSION}"_ba, "0.1"_ba});
    reps.append({i18n("Author"), "%{AUTHOR}"_ba, QByteArray()});
    reps.append({i18n("E-Mail"), "%{EMAIL}"_ba, QByteArray()});
    return reps;
}

bool AppWizardReader::extractTemplateTo(const QString &packageFile, const QString &dest) const
{
    std::unique_ptr<KArchive> arch = nullptr;
    if (packageFile.endsWith(u".zip"_s)) {
        arch = std::make_unique<KZip>(packageFile);
    } else {
        arch = std::make_unique<KTar>(packageFile, u"application/x-bzip"_s);
    }

    if (!arch->open(QIODevice::ReadOnly)) {
        qWarning("Failed to open template archive");
        return false;
    }

    const KArchiveDirectory *root = arch->directory();
    bool ok = root->copyTo(dest, true);
    if (!ok) {
        qWarning("Failed to extract the template directory");
        return false;
    }
    arch->close();

    return true;
}

#include "moc_AppWizardReader.cpp"
