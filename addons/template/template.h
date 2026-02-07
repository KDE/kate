// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "abstractdatamodel.h"
#include "importcategory.h"

#ifdef BUILD_APPWIZARD
#include "AppWizardReader.h"
#endif

#include <QDialog>
#include <QMap>
#include <QModelIndex>
#include <QPushButton>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui
{
class Template;
}
QT_END_NAMESPACE

class QFileInfo;

class Template : public QDialog
{
    Q_OBJECT

public:
    Template(QWidget *parent = nullptr);
    ~Template();

    void setOutputFolder(const QString &path);

Q_SIGNALS:
    void templateCopied(const QString &fileToOpen);

private:
    struct TreeData : AbstractData {
        ~TreeData()
        {
        }
        enum DataRoles {
            PathRole = Qt::UserRole,
            ConfigJsonRole,
        };

        QVariant data(int role = Qt::DisplayRole, int column = 0) override;
        int columns() override
        {
            return 1;
        }

        QString path;
        QString configFile;
    };

    struct ConfigData : AbstractData {
        ~ConfigData()
        {
        }
        enum DataRoles {
            ReplaceDescRole = Qt::UserRole,
            ReplacePlaceHolderRole,
            ReplaceValueRole,
            MustBeLowerCaseRole,
            GeneratedFilesRole,
        };

        QVariant data(int role = Qt::DisplayRole, int column = 0) override;
        int columns() override
        {
            return 2;
        }
        Qt::ItemFlags flags(int column) const override;
        bool setData(const QVariant &value, int role = Qt::DisplayRole, int column = 0) override;

        QString m_desc;
        QByteArray m_placeholder;
        QByteArray m_value;
        bool m_mustBeLowercase = false;
        QStringList m_generatedFiles;
    };

    QModelIndex findChildIndex(const QString &folderName, const QModelIndex &parent);
    QModelIndex addUserTemplateEntry(const QFileInfo &info);
    void addEntries(const QFileInfo &info, const QModelIndex &parent);

    typedef QMap<QByteArray, QByteArray> ReplaceMap;
    bool copyFile(const QString &src, const QString &trgt, const ReplaceMap &fileReplaceMap, const ReplaceMap &replaceMap);
    bool copyFolder(const QString &src, const QString &trgt, const ReplaceMap &fileReplaceMap, const ReplaceMap &replaceMap, const QStringList &fileSkipList);
    QStringList fileNames(const QString &src);
    void createFromTemplate();

    void cancel();

    void templateIndexChanged(const QModelIndex &newIndex);

    void checkIfConfigIsReady();

    void selectFolder();

    void exportTemplate();
    void importTemplate();
    void removeTemplate();

    Ui::Template *ui;
    QPushButton *m_createButton;
    QPushButton *m_exportButton;
    QPushButton *m_removeButton;
    AbstractDataModel m_selectionModel;
    AbstractDataModel m_configModel;
    ImportCategory m_categorySelector;
    QString m_fileToOpen;

#ifdef BUILD_APPWIZARD
    void addAppWizardTemplates();
    void appWizardTemplateSelected(const QString &category);
    void createFromAppWizardTemplate(const QString &category);
    QMap<QString, AppWizardReader::TemplateData> m_appWizMap;
    bool m_hasCMakeLists = false;
    bool m_hasKateproject = false;
#endif
};
