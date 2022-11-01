//
// Description: Widget for configuring build targets
//
// SPDX-FileCopyrightText: 2011-2022 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "targets.h"
#include <KLocalizedString>
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>

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
    copyTarget->setToolTip(i18n("Copy command or target set"));
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
    targetsView->setAlternatingRowColors(true);

    targetsView->setModel(&proxyModel);
    m_delegate = new TargetHtmlDelegate(view);
    targetsView->setItemDelegate(m_delegate);
    targetsView->setSelectionBehavior(QAbstractItemView::SelectItems);
    targetsView->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    targetsView->expandAll();
    targetsView->header()->setStretchLastSection(false);
    targetsView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    targetsView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    targetsView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    QHBoxLayout *tLayout = new QHBoxLayout();

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
    tLayout->setContentsMargins(leftMargin, 0, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout(this);
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
    });

    connect(moveTargetUp, &QToolButton::clicked, this, [this] {
        const QPersistentModelIndex &currentIndex = proxyModel.mapToSource(targetsView->currentIndex());
        if (currentIndex.isValid()) {
            targetsModel.moveRowUp(currentIndex);
        }
        targetsView->scrollTo(targetsView->currentIndex());
    });
    connect(moveTargetDown, &QToolButton::clicked, this, [this] {
        const QModelIndex &currentIndex = proxyModel.mapToSource(targetsView->currentIndex());
        if (currentIndex.isValid()) {
            targetsModel.moveRowDown(currentIndex);
        }
        targetsView->scrollTo(targetsView->currentIndex());
    });

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

void TargetsUi::updateTargetsButtonStates()
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

bool TargetsUi::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == targetsView) {
            if (((keyEvent->key() == Qt::Key_Return) || (keyEvent->key() == Qt::Key_Enter)) && m_delegate && !m_delegate->isEditing()) {
                Q_EMIT enterPressed();
                return true;
            }
        }
        if (obj == targetFilterEdit) {
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
