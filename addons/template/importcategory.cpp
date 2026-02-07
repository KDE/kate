#include "importcategory.h"
#include "ui_importcategory.h"

#include <QDir>
#include <QRegularExpressionValidator>
#include <QStandardPaths>
#include <QValidator>

using namespace Qt::Literals::StringLiterals;

static QFileInfo userTemplatePath()
{
    return QFileInfo(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1String("/templates"));
}

QVariant ImportCategory::CategoryData::data(int role, int column)
{
    if (column != 0) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case CategoryRole:
        return category;
    }
    return {};
}

ImportCategory::ImportCategory(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportCategory)
    , m_categoryModel(std::make_unique<CategoryData>())
{
    setModal(true);
    ui->setupUi(this);
    m_importButton = new QPushButton(i18n("Import"));
    m_importButton->setEnabled(false);
    m_importButton->setDefault(true);
    connect(m_importButton, &QPushButton::clicked, this, &ImportCategory::accept);
    ui->u_buttonBox->addButton(m_importButton, QDialogButtonBox::ActionRole);

    connect(ui->u_buttonBox, &QDialogButtonBox::rejected, this, &ImportCategory::reject);

    ui->u_catTreeView->setModel(&m_categoryModel);
    connect(ui->u_catTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ImportCategory::categoryIndexChanged);

    connect(ui->u_categoryEdit, &QLineEdit::textChanged, this, &ImportCategory::categoryEditChanged);
    connect(ui->u_nameEdit, &QLineEdit::textChanged, this, &ImportCategory::templateNameChanged);

    ui->u_addCatButton->setEnabled(false);
    ui->u_addSubCatButton->setEnabled(false);
    connect(ui->u_addCatButton, &QPushButton::clicked, this, &ImportCategory::addCategory);
    connect(ui->u_addSubCatButton, &QPushButton::clicked, this, &ImportCategory::addSubCategory);

    loadRootCategories(QFileInfo(u":/templates"_s));
    loadRootCategories(userTemplatePath());

    ui->u_catTreeView->setCurrentIndex(m_categoryModel.index(0));

    // only allow valid path parts
    static const QRegularExpression rx(u"^[\\w\\-\\. ]+$"_s);
    QValidator *catValidator = new QRegularExpressionValidator(rx, this);
    QValidator *nameValidator = new QRegularExpressionValidator(rx, this);
    ui->u_categoryEdit->setValidator(catValidator);
    ui->u_nameEdit->setValidator(nameValidator);
}

ImportCategory::~ImportCategory()
{
    delete ui;
}

void ImportCategory::loadRootCategories(const QFileInfo &info)
{
    QDir dir(info.absoluteFilePath());
    // Add children
    const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        loadCategories(entry, QModelIndex());
    }
}

void ImportCategory::loadCategories(const QFileInfo &info, const QModelIndex &parent)
{
    QDir dir(info.absoluteFilePath());
    // Do not add templates as categories
    const QStringList files = dir.entryList(QDir::Files | QDir::Hidden);
    if (files.contains(u"template.json"_s)) {
        return;
    }

    // Create or find an existing category
    QModelIndex thisIndex;
    const QString categoryStr = info.fileName();
    for (int i = 0; i < m_categoryModel.rowCount(parent); ++i) {
        const QModelIndex child = m_categoryModel.index(i, 0, parent);
        if (child.data().toString() == categoryStr) {
            thisIndex = child;
            break;
        }
    }

    if (!thisIndex.isValid()) {
        // Create a new category
        std::unique_ptr<CategoryData> category = std::make_unique<CategoryData>();
        category->category = categoryStr;
        thisIndex = m_categoryModel.addChild(std::move(category), parent);
    }

    // Add children
    const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        loadCategories(entry, thisIndex);
    }
}

void ImportCategory::categoryIndexChanged(const QModelIndex &newIndex)
{
    QString catPath = ui->u_nameEdit->text();
    QModelIndex currIndex = newIndex;
    while (currIndex.isValid()) {
        catPath = currIndex.data().toString() + '/'_L1 + catPath;
        currIndex = currIndex.parent();
    }
    if (ui->u_categoryLabel->text() != catPath) {
        ui->u_categoryLabel->setText(catPath);
        bool hasName = !ui->u_nameLabel->text().isEmpty();
        bool hasCategory = m_categoryModel.rowCount(newIndex) == 0;
        m_importButton->setEnabled(hasName && hasCategory);
    }
}

void ImportCategory::categoryEditChanged()
{
    ui->u_addCatButton->setDisabled(ui->u_categoryEdit->text().isEmpty());
    ui->u_addSubCatButton->setDisabled(ui->u_categoryEdit->text().isEmpty());
}

void ImportCategory::templateNameChanged()
{
    categoryIndexChanged(ui->u_catTreeView->currentIndex());
}

void ImportCategory::addCategory()
{
    const QString &cat = ui->u_categoryEdit->text();
    if (cat.isEmpty()) {
        return;
    }
    QModelIndex currIndex = ui->u_catTreeView->currentIndex();
    if (!currIndex.isValid()) {
        return;
    }

    std::unique_ptr<CategoryData> category = std::make_unique<CategoryData>();
    category->category = cat;
    currIndex = m_categoryModel.addChild(std::move(category), currIndex.parent());
    ui->u_catTreeView->setCurrentIndex(currIndex);
    ui->u_categoryEdit->clear();
}

void ImportCategory::addSubCategory()
{
    const QString &cat = ui->u_categoryEdit->text();
    if (cat.isEmpty()) {
        return;
    }
    QModelIndex currIndex = ui->u_catTreeView->currentIndex();
    if (!currIndex.isValid()) {
        return;
    }

    std::unique_ptr<CategoryData> category = std::make_unique<CategoryData>();
    category->category = cat;
    currIndex = m_categoryModel.addChild(std::move(category), currIndex);
    ui->u_catTreeView->setCurrentIndex(currIndex);
    ui->u_categoryEdit->clear();
}

std::optional<QString> ImportCategory::getCategoryPath(const QString &defaultTemlateName)
{
    ui->u_nameEdit->setText(defaultTemlateName);
    if (exec() == QDialog::Accepted) {
        return ui->u_categoryLabel->text();
    }
    return std::nullopt;
}
