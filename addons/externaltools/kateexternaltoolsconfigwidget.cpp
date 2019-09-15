/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "kateexternaltoolsconfigwidget.h"
#include "externaltoolsplugin.h"
#include "kateexternaltool.h"
#include "katetoolrunner.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KConfig>
#include <KConfigGroup>
#include <KIconButton>
#include <KIconLoader>
#include <KMimeTypeChooser>
#include <KSharedConfig>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>
#include <QBitmap>
#include <QComboBox>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardItem>
#include <QStandardPaths>
#include <QTreeView>

#include <unistd.h>

namespace {
    constexpr int ToolRole = Qt::UserRole + 1;

    /**
     * Helper function to create a QStandardItem that internally stores a pointer to a KateExternalTool.
     */
    QStandardItem * newToolItem(const QPixmap& icon, KateExternalTool* tool)
    {
        auto item = new QStandardItem(icon, tool->name);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
        item->setData(QVariant::fromValue(reinterpret_cast<quintptr>(tool)), ToolRole);
        return item;
    }

    /**
     * Helper function to return an internally stored KateExternalTool for a QStandardItem.
     * If a nullptr is returned, it means the QStandardItem is a category.
     */
    KateExternalTool* toolForItem(QStandardItem* item)
    {
        return item ? reinterpret_cast<KateExternalTool*>(item->data(ToolRole).value<quintptr>()) : nullptr;
    }
}

// BEGIN KateExternalToolServiceEditor
KateExternalToolServiceEditor::KateExternalToolServiceEditor(KateExternalTool* tool, QWidget* parent)
    : QDialog(parent)
    , m_tool(tool)
{
    setWindowTitle(i18n("Edit External Tool"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("system-run")));

    ui = new Ui::ToolDialog();
    ui->setupUi(this);
    ui->btnIcon->setIconSize(KIconLoader::SizeSmall);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &KateExternalToolServiceEditor::slotOKClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->btnMimeType, &QToolButton::clicked, this, &KateExternalToolServiceEditor::showMTDlg);

    Q_ASSERT(m_tool != nullptr);
    ui->edtName->setText(m_tool->name);
    if (!m_tool->icon.isEmpty())
        ui->btnIcon->setIcon(m_tool->icon);

    ui->edtExecutable->setText(m_tool->executable);
    ui->edtArgs->setText(m_tool->arguments);
    ui->edtInput->setText(m_tool->input);
    ui->edtWorkingDir->setText(m_tool->workingDir);
    ui->edtMimeType->setText(m_tool->mimetypes.join(QStringLiteral("; ")));
    ui->cmbSave->setCurrentIndex(static_cast<int>(m_tool->saveMode));
    ui->chkReload->setChecked(m_tool->reload);
    ui->cmbOutput->setCurrentIndex(static_cast<int>(m_tool->outputMode));
    ui->edtCommand->setText(m_tool->cmdname);

    // add support for variable expansion
    KTextEditor::Editor::instance()->addVariableExpansion(
        {
            ui->edtExecutable,
            ui->edtArgs,
            ui->edtInput,
            ui->edtWorkingDir
        }
    );
}

void KateExternalToolServiceEditor::slotOKClicked()
{
    if (ui->edtName->text().isEmpty() || ui->edtExecutable->text().isEmpty()) {
        QMessageBox::information(this, i18n("External Tool"),
                                 i18n("You must specify at least a name and an executable"));
        return;
    }
    accept();
}

void KateExternalToolServiceEditor::showMTDlg()
{
    QString text = i18n("Select the MimeTypes for which to enable this tool.");
    QStringList list
        = ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
    KMimeTypeChooserDialog d(i18n("Select Mime Types"), text, list, QStringLiteral("text"), this);
    if (d.exec() == QDialog::Accepted) {
        ui->edtMimeType->setText(d.chooser()->mimeTypes().join(QStringLiteral(";")));
    }
}
// END KateExternalToolServiceEditor

static std::vector<QStandardItem*> childItems(const QStandardItem * item)
{
    // collect all KateExternalTool items
    std::vector<QStandardItem*> children;
    for (int i = 0; i < item->rowCount(); ++i) {
        children.push_back(item->child(i));
    }
    return children;
}

static std::vector<KateExternalTool*> collectTools(const QStandardItemModel & model)
{
    std::vector<KateExternalTool*> tools;
    for (auto categoryItem : childItems(model.invisibleRootItem())) {
        for (auto child : childItems(categoryItem)) {
            auto tool = toolForItem(child);
            Q_ASSERT(tool != nullptr);
            tools.push_back(tool);
        }
    }
    return tools;
}

// BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget(QWidget* parent, KateExternalToolsPlugin* plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    setupUi(this);
    layout()->setMargin(0);
    lbTools->setModel(&m_toolsModel);
    lbTools->setSelectionMode(QAbstractItemView::SingleSelection);
    lbTools->setDragEnabled(true);
    lbTools->setAcceptDrops(true);
    lbTools->setDefaultDropAction(Qt::MoveAction);
    lbTools->setDropIndicatorShown(true);
    lbTools->setDragDropOverwriteMode(false);
    lbTools->setDragDropMode(QAbstractItemView::InternalMove);

    // Add... button popup menu
    auto addMenu = new QMenu();
    auto addToolAction = addMenu->addAction(i18n("Add Tool..."));
    auto addCategoryAction = addMenu->addAction(i18n("Add Category"));
    btnAdd->setMenu(addMenu);

    connect(addCategoryAction, &QAction::triggered, this, &KateExternalToolsConfigWidget::slotAddCategory);
    connect(addToolAction, &QAction::triggered, this, &KateExternalToolsConfigWidget::slotAddTool);
    connect(btnRemove, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotRemove);
    connect(btnEdit, &QPushButton::clicked, this, &KateExternalToolsConfigWidget::slotEdit);
    connect(lbTools->selectionModel(), &QItemSelectionModel::currentChanged, [this](){
        slotSelectionChanged();
    });
    connect(lbTools, &QTreeView::doubleClicked, this, &KateExternalToolsConfigWidget::slotEdit);

    m_config = new KConfig(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);

    // reset triggers a reload of the existing tools
    reset();
    slotSelectionChanged();

    connect(&m_toolsModel, &QStandardItemModel::itemChanged, [this](){
        m_changed = true;
        Q_EMIT changed();
    });
}

KateExternalToolsConfigWidget::~KateExternalToolsConfigWidget()
{
    clearTools();

    delete m_config;
}

QString KateExternalToolsConfigWidget::name() const
{
    return i18n("External Tools");
}

QString KateExternalToolsConfigWidget::fullName() const
{
    return i18n("External Tools");
}

QIcon KateExternalToolsConfigWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("system-run"));
}

void KateExternalToolsConfigWidget::reset()
{
    clearTools();
    m_toolsModel.invisibleRootItem()->setFlags(Qt::NoItemFlags);

    // the "Uncategorized" category always exists
    m_noCategory = addCategory(i18n("Uncategorized"));
    m_noCategory->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);

    // create other tools and categories
    const auto tools = m_plugin->tools();
    for (auto tool : tools) {
        auto clone = new KateExternalTool(*tool);
        auto item = newToolItem(clone->icon.isEmpty() ? blankIcon() : SmallIcon(clone->icon), clone);
        auto category = clone->category.isEmpty() ? m_noCategory : addCategory(clone->category);
        category->appendRow(item);
    }
    lbTools->expandAll();
    m_changed = false;
}

QPixmap KateExternalToolsConfigWidget::blankIcon()
{
    QPixmap pm(KIconLoader::SizeSmall, KIconLoader::SizeSmall);
    pm.fill();
    pm.setMask(pm.createHeuristicMask());
    return pm;
}

void KateExternalToolsConfigWidget::apply()
{
    if (!m_changed)
        return;
    m_changed = false;

    // collect all KateExternalTool items
    std::vector<KateExternalTool*> tools;
    for (auto categoryItem : childItems(m_toolsModel.invisibleRootItem())) {
        const QString category = (categoryItem == m_noCategory) ? QString() : categoryItem->text();
        for (auto child : childItems(categoryItem)) {
            auto tool = toolForItem(child);
            Q_ASSERT(tool != nullptr);
            // at this point, we have to overwrite the category, since it may have changed (and we never tracked this)
            tool->category = category;
            tools.push_back(tool);
        }
    }

    // write tool configuration to disk
    m_config->group("Global").writeEntry("firststart", false);
    m_config->group("Global").writeEntry("tools", static_cast<int>(tools.size()));
    for (size_t i = 0; i < tools.size(); i++) {
        const QString section = QStringLiteral("Tool ") + QString::number(i);
        KConfigGroup cg(m_config, section);
        tools[i]->save(cg);
    }

    m_config->sync();
    m_plugin->reload();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
    // update button state
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    const bool isToolItem = toolForItem(item) != nullptr;
    const bool isCategory = item && !isToolItem;

    btnEdit->setEnabled(isToolItem || isCategory);
    btnRemove->setEnabled(isToolItem);
}

bool KateExternalToolsConfigWidget::editTool(KateExternalTool* tool)
{
    bool changed = false;

    KateExternalToolServiceEditor editor(tool, this);
    editor.resize(m_config->group("Editor").readEntry("Size", QSize()));
    if (editor.exec() == QDialog::Accepted) {
        tool->name = editor.ui->edtName->text();
        tool->icon = editor.ui->btnIcon->icon();
        tool->executable = editor.ui->edtExecutable->text();
        tool->arguments = editor.ui->edtArgs->text();
        tool->input = editor.ui->edtInput->toPlainText();
        tool->workingDir = editor.ui->edtWorkingDir->text();
        tool->mimetypes = editor.ui->edtMimeType->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")),
                                                            QString::SkipEmptyParts);
        tool->saveMode = static_cast<KateExternalTool::SaveMode>(editor.ui->cmbSave->currentIndex());
        tool->reload = editor.ui->chkReload->isChecked();
        tool->outputMode = static_cast<KateExternalTool::OutputMode>(editor.ui->cmbOutput->currentIndex());
        tool->cmdname = editor.ui->edtCommand->text();

        // sticky action collection name, never changes again, so that shortcuts stay
        tool->actionName = QStringLiteral("externaltool_") + QString(tool->name).remove(QRegularExpression(QStringLiteral("\\W+")));

        changed = true;
    }

    m_config->group("Editor").writeEntry("Size", editor.size());
    m_config->sync();

    return changed;
}

QStandardItem * KateExternalToolsConfigWidget::addCategory(const QString & category)
{
    // searach for existing category
    auto items = m_toolsModel.findItems(category);
    if (!items.empty()) {
        return items.front();
    }

    // ...otherwise, create it
    auto item = new QStandardItem(category);

    // for now, categories are not movable, otherwise, the use can move a
    // category into another category, which is not supported right now
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsEditable);

    m_toolsModel.appendRow(item);
    return item;
}

QStandardItem * KateExternalToolsConfigWidget::currentCategory() const
{
    auto index = lbTools->currentIndex();
    if (!index.isValid()) {
        return m_noCategory;
    }

    auto item = m_toolsModel.itemFromIndex(index);
    auto tool = toolForItem(item);
    if (tool) {
        // the parent of a ToolItem is always a category
        return item->parent();
    }

    // item is no ToolItem, so we must have a category at hand
    return item;
}

void KateExternalToolsConfigWidget::clearTools()
{
    // collect all KateExternalTool items and delete them, since they are copies
    std::vector<KateExternalTool*> tools = collectTools(m_toolsModel);
    qDeleteAll(tools);
    tools.clear();
    m_toolsModel.clear();
}

void KateExternalToolsConfigWidget::slotAddCategory()
{
    // find unique name
    QString name = i18n("New Category");
    int i = 1;
    while (!m_toolsModel.findItems(name, Qt::MatchFixedString).isEmpty()) {
        name = (i18n("New Category %1", i++));
    }

    // add category and switch to edit mode
    auto item = addCategory(name);
    lbTools->edit(item->index());
}

//! Helper that ensures that tool->actionName is unique
static void makeActionNameUnique(KateExternalTool* tool, const std::vector<KateExternalTool*> & tools)
{
    QString name = tool->actionName;
    int i = 1;
    bool notUnique = true;
    while (notUnique) {
        auto it = std::find_if(tools.cbegin(), tools.cend(), [&name](const KateExternalTool* tool) {
            return tool->actionName == name;
        });
        if (it == tools.cend()) {
            break;
        }
        name = tool->actionName + QString::number(i);
        ++i;
    }
    tool->actionName = name;
}

void KateExternalToolsConfigWidget::slotAddTool()
{
    auto t = new KateExternalTool();
    if (editTool(t)) {
        makeActionNameUnique(t, collectTools(m_toolsModel));
        auto item = newToolItem(t->icon.isEmpty() ? blankIcon() : SmallIcon(t->icon), t);
        auto category = currentCategory();
        category->appendRow(item);
        lbTools->setCurrentIndex(item->index());

        Q_EMIT changed();
        m_changed = true;
    } else {
        delete t;
    }
}

void KateExternalToolsConfigWidget::slotRemove()
{
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    auto tool = toolForItem(item);

    if (tool) {
        item->parent()->removeRow(item->index().row());
        delete tool;
        Q_EMIT changed();
        m_changed = true;
    }
}

void KateExternalToolsConfigWidget::slotEdit()
{
    auto item = m_toolsModel.itemFromIndex(lbTools->currentIndex());
    auto tool = toolForItem(item);
    if (!tool) {
        if (item) {
            lbTools->edit(item->index());
        }
        return;
    }
    // show the item in an editor
    if (editTool(tool)) {
        // renew the icon and name
        item->setText(tool->name);
        item->setIcon(tool->icon.isEmpty() ? blankIcon() : SmallIcon(tool->icon));

        Q_EMIT changed();
        m_changed = true;
    }
}
// END KateExternalToolsConfigWidget

// kate: space-indent on; indent-width 4; replace-tabs on;
