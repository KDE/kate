// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "template.h"
#include "ui_template.h"

#include <KLocalizedString>

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>
#include <QTemporaryDir>

using namespace Qt::Literals::StringLiterals;

static QString lastPath(const QString &path)
{
    QStringList paths = path.split('/'_L1, Qt::SkipEmptyParts);
    if (!paths.isEmpty()) {
        return paths.last();
    }
    return path;
}

QVariant Template::TreeData::data(int role, int column)
{
    if (column != 0) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return lastPath(path);
    case TreeData::PathRole:
        return path;
    case TreeData::ConfigJsonRole:
        return configFile;
    }

    return QVariant();
}

QVariant Template::ConfigData::data(int role, int column)
{
    if (column < 0 || column > 1) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (column == 0) {
            return m_desc;
        } else {
            return m_value;
        }
    case Qt::ToolTipRole:
        return i18n("This value must be lowercase");
    case Qt::BackgroundRole:
        if (column != 1) {
            return QVariant();
        }
        return (m_mustBeLowercase && m_value.toLower() != m_value) || m_value.isEmpty() ? QBrush(Qt::red) : QVariant();
    case ConfigData::ReplaceDescRole:
        return m_desc;
    case ConfigData::ReplacePlaceHolderRole:
        return m_placeholder;
    case ConfigData::ReplaceValueRole:
        return m_value;
    case ConfigData::MustBeLowerCaseRole:
        return m_mustBeLowercase;
    case ConfigData::GeneratedFilesRole:
        return m_generatedFiles;
    }
    return QVariant();
}

Qt::ItemFlags Template::ConfigData::flags(int column) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (column == 1) {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

bool Template::ConfigData::setData(const QVariant &value, int role, int column)
{
    if (role != Qt::EditRole) {
        return false;
    }
    if (column == 0) {
        // Only for the header data
        m_desc = value.toString();
        return true;
    }
    if (column == 1) {
        m_value = value.toByteArray();
        return true;
    }

    return false;
}

Template::Template(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Template)
    , m_selectionModel(std::make_unique<TreeData>())
    , m_configModel(std::make_unique<ConfigData>())
{
    ui->setupUi(this);
    m_createButton = new QPushButton(i18n("Create"));
    m_createButton->setEnabled(false);
    ui->u_buttonBox->addButton(m_createButton, QDialogButtonBox::AcceptRole);

    connect(m_createButton, &QPushButton::clicked, this, &Template::createFromTemplate);
    connect(ui->u_buttonBox, &QDialogButtonBox::rejected, this, &Template::cancel);

    ui->u_templateTree->setHeaderHidden(true);
    ui->u_templateTree->setModel(&m_selectionModel);

    ui->u_configTreeView->setRootIsDecorated(false);
    ui->u_configTreeView->setModel(&m_configModel);

    connect(ui->u_templateTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &Template::templateIndexChanged);

    m_configModel.setHeaderData(0, Qt::Horizontal, u"Property"_s);
    m_configModel.setHeaderData(1, Qt::Horizontal, u"Value"_s);

    ui->u_configWidget->setEnabled(false);

    connect(&m_configModel, &AbstractDataModel::dataChanged, this, &Template::checkIfConfigIsReady);
    connect(ui->u_locationLineEdit, &QLineEdit::textChanged, this, &Template::checkIfConfigIsReady);

    connect(ui->u_locationToolButton, &QToolButton::clicked, this, &Template::selectFolder);

    QString userTemplates = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1String("/templates");
    addTemplateRoot(userTemplates);
    addTemplateRoot(u":/templates"_s);

#ifdef BUILD_APPWIZARD
    addAppWizardTemplates();
#endif
}

Template::~Template()
{
    delete ui;
}

void Template::addEntries(const QFileInfo &info, const QModelIndex &parent)
{
    QDir dir(info.absoluteFilePath());
    QStringList files = dir.entryList(QDir::Files | QDir::Hidden);
    if (files.contains(u"template.json"_s)) {
        std::unique_ptr<TreeData> data = std::make_unique<TreeData>();
        data->path = parent.data(TreeData::PathRole).toString();
        data->configFile = u"template.json"_s;
        m_selectionModel.setAbstractData(std::move(data), parent);
        return;
    }
    for (const auto &entry : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        std::unique_ptr<TreeData> data = std::make_unique<TreeData>();
        data->path = entry.absoluteFilePath();
        const QModelIndex cIndex = m_selectionModel.addChild(std::move(data), parent);
        addEntries(entry, cIndex);
    }
}

void Template::addTemplateRoot(const QString &path)
{
    QFileInfo entry(path);
    addEntries(entry, QModelIndex());
}

QStringList Template::fileNames(const QString &src)
{
    QDir dir(src);
    QStringList files = dir.entryList(QDir::Files | QDir::Hidden);

    for (const auto &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        files += fileNames(src + '/'_L1 + entry);
    }

    return files;
}

bool Template::copyFile(const QString &src, const QString &trgt, const ReplaceMap &fileReplaceMap, const ReplaceMap &replaceMap)
{
    QFile in(src);
    if (!in.open(QFile::ReadOnly)) {
        qWarning("Failed to open: %ls", qUtf16Printable(src));
        return false;
    }

    QByteArray fileContent = in.readAll();

    QByteArray newName = trgt.toLocal8Bit();
    // Replace first any file names
    for (auto it = fileReplaceMap.cbegin(); it != fileReplaceMap.cend(); ++it) {
        newName.replace(it.key(), it.value());
        fileContent.replace(it.key(), it.value());
    }

    // Now replace non-file-name
    for (auto it = replaceMap.cbegin(); it != replaceMap.cend(); ++it) {
        QByteArray toReplace = it.key();
        newName.replace(toReplace, it.value());
        fileContent.replace(toReplace, it.value());
    }

    if (QFileInfo::exists(QString::fromLocal8Bit(newName))) {
        qWarning("File already exists: %s", newName.constData());
        return false;
    }

    QFile out(QString::fromLocal8Bit(newName));
    if (!out.open(QFile::WriteOnly)) {
        qWarning("Failed to create: %s", newName.constData());
        return false;
    }

    out.write(fileContent);
    return true;
}

bool Template::copyFolder(const QString &src,
                          const QString &trgt,
                          const ReplaceMap &fileReplaceMap,
                          const ReplaceMap &replaceMap,
                          const QStringList &fileSkipList)
{
    QDir dir(src);

    // Copy files
    for (const auto &entry : dir.entryList(QDir::Files | QDir::Hidden)) {
        if (fileSkipList.contains(entry)) {
            continue;
        }

#ifdef BUILD_APPWIZARD
        if (entry == u"CMakeLists.txt"_s && trgt == ui->u_locationLineEdit->text()) {
            m_hasCMakeLists = true;
        }
        if (entry == u".kateproject"_s && trgt == ui->u_locationLineEdit->text()) {
            m_hasKateproject = true;
        }
#endif
        if (!copyFile(src + '/'_L1 + entry, trgt + '/'_L1 + entry, fileReplaceMap, replaceMap)) {
            return false;
        }
    }

    // Copy folders
    for (const auto &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir().mkpath(trgt + '/'_L1 + entry);
        if (!copyFolder(src + '/'_L1 + entry, trgt + '/'_L1 + entry, fileReplaceMap, replaceMap, fileSkipList)) {
            return false;
        }
    }

    return true;
}

void Template::createFromTemplate()
{
#ifdef BUILD_APPWIZARD
    QString path = ui->u_templateTree->currentIndex().data(TreeData::PathRole).toString();
    QString config = ui->u_templateTree->currentIndex().data(TreeData::ConfigJsonRole).toString();
    if (!config.endsWith(u"json"_s)) {
        return createFromAppWizardTemplate(path);
    }
#endif

    ReplaceMap replaceMap;
    ReplaceMap fileReplaceMap;
    int rows = m_configModel.rowCount(QModelIndex());

    const auto tmplIndex = ui->u_templateTree->currentIndex();
    QString srcPath = tmplIndex.data(TreeData::PathRole).toString();
    const QStringList fNames = fileNames(srcPath);

    QString fileToOpen = m_fileToOpen;
    bool lcFiles = ui->u_lowercaseCheckBox->isChecked();

    for (int i = 0; i < rows; ++i) {
        const auto index = m_configModel.index(i, 0, QModelIndex());
        const QByteArray plh = index.data(ConfigData::ReplacePlaceHolderRole).toByteArray();
        const QByteArray val = index.data(ConfigData::ReplaceValueRole).toByteArray();
        const QStringList genFiles = index.data(ConfigData::GeneratedFilesRole).toStringList();
        replaceMap.insert(plh, val);
        QByteArray fileNameReplace = lcFiles ? val.toLower() : val;

        // Add filename replacements (lowercase of class name)
        for (const auto &fileStr : fNames) {
            const QByteArray file = fileStr.toLocal8Bit();
            if (file.contains(plh)) {
                fileToOpen.replace(QString::fromLocal8Bit(plh), QString::fromLocal8Bit(fileNameReplace));
                QByteArray newName = file;
                newName.replace(plh, fileNameReplace);
                fileReplaceMap.insert(file, newName);
            }
        }

        // Add file-name replacements for generated files (lowercase of class name)
        for (const QString &genFile : genFiles) {
            const QByteArray toReplace = genFile.arg(QString::fromLocal8Bit(plh)).toLocal8Bit();
            const QByteArray replacement = genFile.arg(QString::fromLocal8Bit(fileNameReplace)).toLocal8Bit();
            fileReplaceMap.insert(toReplace, replacement);
        }
    }

    QString trgtPath = ui->u_locationLineEdit->text();
    fileToOpen = trgtPath + '/'_L1 + fileToOpen;

    QStringList fileSkipList({u"template.json"_s});

    bool ok = copyFolder(srcPath, trgtPath, fileReplaceMap, replaceMap, fileSkipList);

    if (!ok) {
        fileToOpen.clear();
    }

    Q_EMIT done(fileToOpen);
}

void Template::cancel()
{
    Q_EMIT done(QString());
}

void Template::templateIndexChanged(const QModelIndex &newIndex)
{
    m_configModel.clear();
    m_fileToOpen.clear();
    ui->u_detailsTB->setText(QString());
    ui->u_configWidget->setEnabled(false);

    QString path = newIndex.data(TreeData::PathRole).toString();
    QString config = newIndex.data(TreeData::ConfigJsonRole).toString();

    if (config.isEmpty()) {
        return;
    }

#ifdef BUILD_APPWIZARD
    if (!config.endsWith(u"json"_s)) {
        return appWizardTemplateSelected(path);
    }
#endif

    QByteArray configJson;

    ui->u_lowercaseCheckBox->setEnabled(true);

    QFile in(path + '/'_L1 + config);
    if (in.open(QFile::ReadOnly)) {
        configJson = in.readAll();
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(configJson, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning("%ls at: %d", qUtf16Printable(error.errorString()), error.offset);
    }
    const QJsonObject rootObj = doc.object();
    QString desc = rootObj.value(u"description"_s).toString();
    m_fileToOpen = rootObj.value(u"fileToOpen"_s).toString();

    ui->u_detailsTB->setText(desc);

    const QJsonArray replacements = rootObj.value(u"replacements"_s).toArray();

    for (const auto renameVal : replacements) {
        const auto renObj = renameVal.toObject();
        std::unique_ptr<ConfigData> data = std::make_unique<ConfigData>();
        data->m_desc = renObj.value(u"description"_s).toString();
        data->m_placeholder = renObj.value(u"placeholder"_s).toString().toLocal8Bit();
        data->m_value = renObj.value(u"default"_s).toString().toLocal8Bit();
        data->m_mustBeLowercase = renObj.value(u"mustBeLowercase"_s).toBool(false);
        const auto files = renObj.value(u"genratedFiles"_s).toArray();
        for (const auto file : files) {
            data->m_generatedFiles.append(file.toString());
        }
        m_configModel.addChild(std::move(data), QModelIndex());
    }

    ui->u_configTreeView->resizeColumnToContents(0);
    ui->u_configTreeView->resizeColumnToContents(1);
    ui->u_configWidget->setEnabled(true);
}

void Template::checkIfConfigIsReady()
{
    int rows = m_configModel.rowCount(QModelIndex());

    bool hasAllValues = true;
    for (int i = 0; i < rows; ++i) {
        if (m_configModel.index(i, 1, QModelIndex()).data().toString().isEmpty()) {
            hasAllValues = false;
            break;
        }
        bool lc = m_configModel.index(i, 1, QModelIndex()).data(ConfigData::MustBeLowerCaseRole).toBool();
        const QString val = m_configModel.index(i, 1, QModelIndex()).data(ConfigData::ReplaceValueRole).toString();
        if (lc && val != val.toLower()) {
            hasAllValues = false;
            break;
        }
    }
    QString folder = ui->u_locationLineEdit->text();
    bool hasFolder = !folder.isEmpty() && QDir().exists(folder);

    m_createButton->setEnabled(hasFolder && hasAllValues);
}

void Template::selectFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, i18n("Select target directory"), ui->u_locationLineEdit->text());
    if (!folder.isEmpty()) {
        ui->u_locationLineEdit->setText(folder);
    }
}

void Template::setOutputFolder(const QString &path)
{
    ui->u_locationLineEdit->setText(path);
}

#ifdef BUILD_APPWIZARD
void Template::addAppWizardTemplates()
{
    m_appWizMap = AppWizardReader().appWizardTemplates();

    if (m_appWizMap.isEmpty()) {
        return;
    }

    // create the appWiz root
    std::unique_ptr<TreeData> data = std::make_unique<TreeData>();
    data->path = i18n("KAppTemplates");
    QMap<QString, QModelIndex> indexes;
    indexes.insert(QString(), m_selectionModel.addChild(std::move(data), QModelIndex()));

    for (auto it = m_appWizMap.cbegin(); it != m_appWizMap.cend(); it++) {
        QStringList itemPaths = it.key().split(u'/');
        itemPaths.removeLast();
        QStringList paths;
        QModelIndex index = indexes.value(QString());
        for (const QString &part : itemPaths) {
            paths.append(part);
            const QString path = paths.join(u'/');
            if (!indexes.contains(path)) {
                std::unique_ptr<TreeData> data = std::make_unique<TreeData>();
                data->path = path;
                index = m_selectionModel.addChild(std::move(data), index);
                indexes.insert(path, index);
            } else {
                index = indexes.value(path);
            }
        }
        std::unique_ptr<TreeData> data = std::make_unique<TreeData>();
        data->path = it.key();
        data->configFile = it->packagePath;
        m_selectionModel.addChild(std::move(data), index);
    }
}

void Template::appWizardTemplateSelected(const QString &category)
{
    const auto &templ = m_appWizMap.value(category);
    ui->u_lowercaseCheckBox->setEnabled(false);

    ui->u_detailsTB->setText(templ.description);

    for (const auto &ren : AppWizardReader().replacements()) {
        std::unique_ptr<ConfigData> data = std::make_unique<ConfigData>();
        data->m_desc = ren.descr;
        data->m_placeholder = ren.placeholder;
        data->m_value = ren.defaultValue;
        data->m_mustBeLowercase = false;
        // data->m_generatedFiles;
        m_configModel.addChild(std::move(data), QModelIndex());
    }

    ui->u_configTreeView->resizeColumnToContents(0);
    ui->u_configTreeView->resizeColumnToContents(1);
    ui->u_configWidget->setEnabled(true);
}

void Template::createFromAppWizardTemplate(const QString &category)
{
    const auto &templ = m_appWizMap.value(category);

    QMap<QByteArray, QByteArray> replaceMap;
    int rows = m_configModel.rowCount(QModelIndex());

    for (int i = 0; i < rows; ++i) {
        const auto index = m_configModel.index(i, 0, QModelIndex());
        const QByteArray plh = index.data(ConfigData::ReplacePlaceHolderRole).toByteArray();
        const QByteArray val = index.data(ConfigData::ReplaceValueRole).toByteArray();
        replaceMap.insert(plh, val);
    }

    QString trgtPath = ui->u_locationLineEdit->text();
    QString fileToOpen = templ.fileToOpen;
    fileToOpen.replace(u"%{dest}"_s, trgtPath);
    if (!QFileInfo(fileToOpen).isAbsolute()) {
        fileToOpen = trgtPath + '/'_L1 + fileToOpen;
    }

    const QRegularExpression notWord(u"[^\\w]"_s);
    auto generateIdentifier = [&notWord](const QByteArray &appname) {
        return QString::fromLocal8Bit(appname).replace(notWord, u"_"_s).toLocal8Bit();
    };

    const QByteArray appName = replaceMap.value("%{APPNAME}"_ba);
    replaceMap["%{CURRENT_YEAR}"_ba] = QByteArray().setNum(QDate::currentDate().year());
    replaceMap["%{APPNAME}"_ba] = appName;
    replaceMap["%{APPNAMEUC}"_ba] = generateIdentifier(appName.toUpper());
    replaceMap["%{APPNAMELC}"_ba] = appName.toLower();
    replaceMap["%{APPNAMEID}"_ba] = generateIdentifier(appName);
    replaceMap["%{PROJECTDIR}"_ba] = trgtPath.toLocal8Bit();
    replaceMap["%{PROJECTDIRNAME}"_ba] = appName.toLower();

    // Update the file to open
    for (const auto &key : replaceMap.keys()) {
        fileToOpen.replace(QString::fromLocal8Bit(key), QString::fromLocal8Bit(replaceMap.value(key)));
    }

    QStringList fileSkipList({templ.kAppTemplateFile, templ.icon});

    QTemporaryDir tempDir;

    m_hasCMakeLists = false;
    m_hasKateproject = false;

    bool ok = AppWizardReader().extractTemplateTo(templ.packagePath, tempDir.path());
    if (!ok) {
        fileToOpen.clear();
    } else {
        ok = copyFolder(tempDir.path(), trgtPath, {}, replaceMap, fileSkipList);

        if (m_hasCMakeLists && !m_hasKateproject && ok) {
            copyFile(u":/templates/kateproject.in"_s, trgtPath + u"/.kateproject"_s, {}, replaceMap);
        }
    }

    if (!ok) {
        fileToOpen.clear();
    }
    Q_EMIT done(fileToOpen);
}

#endif
