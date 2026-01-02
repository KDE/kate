//
// Description: Widget for configuring build targets
//
// SPDX-FileCopyrightText: 2011-2022 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "targets.h"
#include <KLocalizedString>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QEvent>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

TargetsUi::TargetsUi(QObject *view, QWidget *parent)
    : QWidget(parent)
{
    proxyModel.setSourceModel(&targetsModel);

    targetFilterEdit = new QLineEdit(this);
    targetFilterEdit->setPlaceholderText(i18n("Filter targets, use arrow keys to select, Enter to execute"));
    targetFilterEdit->setClearButtonEnabled(true);

    newTarget = new QToolButton(this);
    newTarget->setToolTip(i18n("Create new set of targets"));
    newTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));

    copyTarget = new QToolButton(this);
    copyTarget->setToolTip(i18n("Clone command or target set"));
    copyTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));

    deleteTarget = new QToolButton(this);
    deleteTarget->setToolTip(i18n("Delete current target or current set of targets"));
    deleteTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));

    addButton = new QToolButton(this);
    addButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addButton->setToolTip(i18n("Add new target"));

    buildButton = new QToolButton(this);
    buildButton->setIcon(QIcon::fromTheme(QStringLiteral("run-build")));
    buildButton->setToolTip(i18n("Build selected target"));

    runButton = new QToolButton(this);
    runButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    runButton->setToolTip(i18n("Build and run selected target"));

    moveTargetUp = new QToolButton(this);
    moveTargetUp->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    moveTargetUp->setToolTip(i18n("Move selected target up"));

    moveTargetDown = new QToolButton(this);
    moveTargetDown->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    moveTargetDown->setToolTip(i18n("Move selected target down"));

    targetsView = new QTreeView(this);
    targetsView->setFrameShape(QFrame::Shape::NoFrame);
    targetsView->setAlternatingRowColors(true);

    targetsView->setModel(&proxyModel);
    m_delegate = new TargetHtmlDelegate(view);
    targetsView->setItemDelegate(m_delegate);
    targetsView->setSelectionBehavior(QAbstractItemView::SelectItems);
    targetsView->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    targetsView->expandAll();
    targetsView->header()->setStretchLastSection(false);
    targetsView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    targetsView->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    targetsView->header()->setSectionResizeMode(2, QHeaderView::Interactive);

    auto *tLayout = new QHBoxLayout();
    tLayout->addWidget(targetFilterEdit);
    tLayout->addWidget(buildButton);
    tLayout->addWidget(runButton);
    tLayout->addSpacing(15);
    tLayout->addWidget(addButton);
    tLayout->addWidget(newTarget);
    tLayout->addWidget(copyTarget);
    tLayout->addWidget(moveTargetUp);
    tLayout->addWidget(moveTargetDown);
    tLayout->addWidget(deleteTarget);
    int leftMargin = QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    int rightMargin = QApplication::style()->pixelMetric(QStyle::PM_LayoutRightMargin);
    int topMargin = QApplication::style()->pixelMetric(QStyle::PM_LayoutTopMargin);
    tLayout->setContentsMargins(leftMargin, topMargin, rightMargin, 0);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(tLayout);
    layout->addWidget(targetsView);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(targetsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TargetsUi::targetActivated);

    connect(targetsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TargetsUi::updateTargetsButtonStates);
    connect(&targetsModel, &QAbstractItemModel::dataChanged, this, &TargetsUi::updateTargetsButtonStates);
    connect(&targetsModel, &QAbstractItemModel::rowsMoved, this, &TargetsUi::updateTargetsButtonStates);

    connect(targetFilterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        proxyModel.setFilter(text);
        targetsView->expandAll();
        targetsView->scrollTo(targetsView->currentIndex());
    });

    targetsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(targetsView, &QTreeView::customContextMenuRequested, this, &TargetsUi::customTargetsMenuRequested);

    connect(newTarget, &QToolButton::clicked, this, &TargetsUi::targetSetNew);
    connect(copyTarget, &QToolButton::clicked, this, &TargetsUi::targetOrSetClone);
    connect(deleteTarget, &QToolButton::clicked, this, &TargetsUi::targetDelete);
    connect(addButton, &QToolButton::clicked, this, &TargetsUi::slotAddTargetClicked);

    targetsView->installEventFilter(this);
    targetFilterEdit->installEventFilter(this);
}

void TargetsUi::targetActivated(const QModelIndex &index)
{
    // qDebug() << index;
    if (!index.isValid()) {
        return;
    }
    QModelIndex rootItem = index;
    if (rootItem.parent().isValid()) {
        rootItem = rootItem.parent();
    }
}

void TargetsUi::updateTargetsButtonStates() const
{
    QModelIndex currentIndex = targetsView->currentIndex();
    if (!currentIndex.isValid()) {
        buildButton->setEnabled(false);
        runButton->setEnabled(false);
        moveTargetUp->setEnabled(false);
        moveTargetDown->setEnabled(false);
        return;
    }

    moveTargetUp->setEnabled(currentIndex.row() > 0);

    // If this is a root item
    if (!currentIndex.parent().isValid()) {
        // move down button
        int rows = targetsView->model()->rowCount();
        moveTargetDown->setEnabled(currentIndex.row() < rows - 1);

        // try it's first child to see if we can build/run
        currentIndex = targetsView->model()->index(0, 0, currentIndex.siblingAtColumn(0));
        if (!currentIndex.isValid()) {
            buildButton->setEnabled(false);
            runButton->setEnabled(false);
            return;
        }
    } else {
        int rows = targetsView->model()->rowCount(currentIndex.parent());
        moveTargetDown->setEnabled(currentIndex.row() < rows - 1);
    }

    const bool hasBuildCmd = !currentIndex.siblingAtColumn(1).data().toString().isEmpty();
    const bool hasRunCmd = !currentIndex.siblingAtColumn(2).data().toString().isEmpty();
    buildButton->setEnabled(hasBuildCmd);
    // Run button can be enabled even if there is no build command
    runButton->setEnabled(hasRunCmd);
}

void TargetsUi::copyCurrentItem()
{
    const QModelIndex treeIndex = targetsView->currentIndex();
    const QModelIndex modelIndex = proxyModel.mapToSource(treeIndex);
    QApplication::clipboard()->setText(targetsModel.indexToJson(modelIndex));
}

void TargetsUi::cutCurrentItem()
{
    const QModelIndex treeIndex = targetsView->currentIndex();
    const QModelIndex modelIndex = proxyModel.mapToSource(treeIndex);
    QApplication::clipboard()->setText(targetsModel.indexToJson(modelIndex));
    targetsModel.deleteItem(modelIndex);
}

void TargetsUi::pasteAfterCurrentItem()
{
    const QModelIndex treeIndex = targetsView->currentIndex();
    const QModelIndex modelIndex = proxyModel.mapToSource(treeIndex);
    targetsModel.insertAfter(modelIndex, QApplication::clipboard()->text(), currentProjectBaseDir);
}

void TargetsUi::customTargetsMenuRequested(const QPoint &pos)
{
    auto *tree = qobject_cast<QTreeView *>(sender());
    if (tree == nullptr) {
        return;
    }
    auto *menu = new QMenu(tree);

    auto *copy = new QAction(i18n("Copy"), tree);
    menu->addAction(copy);
    copy->setShortcut(QKeySequence::Copy);

    auto *cut = new QAction(i18n("Cut"), tree);
    menu->addAction(cut);
    cut->setShortcut(QKeySequence::Cut);

    auto *paste = new QAction(i18n("Paste after"), tree);
    menu->addAction(paste);
    paste->setShortcut(QKeySequence::Paste);

    if (!targetsModel.validTargetsJson(QApplication::clipboard()->text())) {
        paste->setEnabled(false);
    }

    connect(copy, &QAction::triggered, this, &TargetsUi::copyCurrentItem);
    connect(cut, &QAction::triggered, this, &TargetsUi::cutCurrentItem);
    connect(paste, &QAction::triggered, this, &TargetsUi::pasteAfterCurrentItem);

    menu->popup(tree->viewport()->mapToGlobal(pos));
}

void TargetsUi::slotAddTargetClicked()
{
    QModelIndex current = targetsView->currentIndex();
    QString currName = i18nc("Name/Label for a compilation or build command", "Build Command");
    QString currCmd;
    QString currRun;

    current = proxyModel.mapToSource(current);
    QModelIndex index = targetsModel.addCommandAfter(current, currName, currCmd, currRun);
    index = proxyModel.mapFromSource(index);
    targetsView->setCurrentIndex(index);
}

void TargetsUi::targetSetNew()
{
    static const QString configCmd = u"cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja ../"_s;
    static const QString buildCmd = u"ninja"_s;
    static const QString runCmd;

    targetFilterEdit->setText(QString());
    QModelIndex currentIndex = proxyModel.mapToSource(targetsView->currentIndex());
    QString workingDir = QDir::homePath();
    QString projDir;
    if (currentIndex.data(TargetModel::IsProjectTargetRole).toBool()) {
        workingDir = currentProjectBaseDir + u"/build"_s;
        projDir = currentProjectBaseDir;
    }

    QModelIndex index = targetsModel.insertTargetSetAfter(currentIndex, i18n("Target Set"), workingDir, false, QString(), projDir);
    index = targetsModel.addCommandAfter(index, i18nc("Name/Label for a command to configure a build", "Configure"), configCmd, QString());
    index = targetsModel.addCommandAfter(index, i18nc("Name/Label for a compilation or build command", "Build Command"), buildCmd, runCmd);
    index = proxyModel.mapFromSource(index);
    targetsView->setCurrentIndex(index);
}

void TargetsUi::targetOrSetClone()
{
    QModelIndex currentIndex = targetsView->currentIndex();
    currentIndex = proxyModel.mapToSource(currentIndex);
    targetFilterEdit->setText(QString());

    QString json = targetsModel.indexToJson(currentIndex);

    QModelIndex newIndex = targetsModel.insertAfter(currentIndex, json, currentProjectBaseDir);
    if (targetsModel.hasChildren(newIndex)) {
        newIndex = proxyModel.mapFromSource(newIndex);
        targetsView->setCurrentIndex(newIndex.model()->index(0, 0, newIndex));
        return;
    }
    newIndex = proxyModel.mapFromSource(newIndex);
    targetsView->setCurrentIndex(newIndex);
}

void TargetsUi::targetDelete()
{
    QModelIndex currentIndex = targetsView->currentIndex();
    currentIndex = proxyModel.mapToSource(currentIndex);
    targetsModel.deleteItem(currentIndex);

    if (targetsModel.rowCount() == 0) {
        targetSetNew();
    }
}

bool TargetsUi::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize && obj == targetsView) {
        QResizeEvent *e = static_cast<QResizeEvent *>(event);
        double newSizeLeft = e->size().width() - targetsView->header()->sectionSize(0);
        targetsView->header()->resizeSection(1, std::floor(columnCommandStretch * newSizeLeft));
        targetsView->header()->resizeSection(2, std::floor((1.0 - columnCommandStretch) * newSizeLeft));
    }
    if (event->type() == QEvent::ShortcutOverride) {
        // Ignore copy in ShortcutOverride and handle it in the KeyPress event
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::Cut) || ke->matches(QKeySequence::Paste)) {
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == targetsView) {
            if (keyEvent->matches(QKeySequence::Copy)) {
                copyCurrentItem();
                event->accept();
                return true;
            } else if (keyEvent->matches(QKeySequence::Cut)) {
                cutCurrentItem();
                event->accept();
                return true;
            }
            if (keyEvent->matches(QKeySequence::Paste)) {
                pasteAfterCurrentItem();
                event->accept();
                return true;
            }
            if (((keyEvent->key() == Qt::Key_Return) || (keyEvent->key() == Qt::Key_Enter)) && m_delegate && !m_delegate->isEditing()) {
                Q_EMIT enterPressed();
                return true;
            }
        } else if (obj == targetFilterEdit) {
            switch (keyEvent->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            case Qt::Key_Return:
            case Qt::Key_Enter:
                QCoreApplication::sendEvent(targetsView, event);
                return true;
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_F2:
                // NOTE: I failed to find a generic "platform edit key" shortcut, but it seems
                // Key_F2 is hard-coded on non-OSX and Return/Enter on OSX in:
                // void QAbstractItemView::keyPressEvent(QKeyEvent *event)
                if (targetFilterEdit->text().isEmpty()) {
                    QCoreApplication::sendEvent(targetsView, event);
                    return true;
                }
                break;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

#include "moc_targets.cpp"
