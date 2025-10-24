/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004 Joseph Wenninger <jowenn@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katesavemodifieddialog.h"

#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageDialog>
#include <KStandardGuiItem>

#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTreeWidget>
#include <QVBoxLayout>

class AbstractKateSaveModifiedDialogCheckListItem : public QTreeWidgetItem
{
public:
    AbstractKateSaveModifiedDialogCheckListItem(const QString &title, const QString &url)
    {
        setFlags(flags() | Qt::ItemIsUserCheckable);
        setText(0, title);
        setText(1, url);
        setCheckState(0, Qt::Checked);
        setState(InitialState);
    }
    ~AbstractKateSaveModifiedDialogCheckListItem() override
    {
    }
    virtual bool synchronousSave(QWidget *dialogParent) = 0;
    enum STATE {
        InitialState,
        SaveOKState,
        SaveFailedState
    };
    STATE state() const
    {
        return m_state;
    }
    void setState(enum STATE state)
    {
        m_state = state;
        switch (state) {
        case InitialState:
            setIcon(0, QIcon());
            break;
        case SaveOKState:
            setIcon(0, QIcon::fromTheme(QStringLiteral("dialog-ok")));
            // QStringLiteral("ok") icon should probably be QStringLiteral("dialog-success"), but we don't have that icon in KDE 4.0
            break;
        case SaveFailedState:
            setIcon(0, QIcon::fromTheme(QStringLiteral("dialog-error")));
            break;
        }
    }

private:
    STATE m_state = InitialState;
};

class KateSaveModifiedDocumentCheckListItem : public AbstractKateSaveModifiedDialogCheckListItem
{
public:
    explicit KateSaveModifiedDocumentCheckListItem(KTextEditor::Document *document)
        : AbstractKateSaveModifiedDialogCheckListItem(document->documentName(), document->url().toString())
    {
        m_document = document;
    }
    ~KateSaveModifiedDocumentCheckListItem() override
    {
    }
    bool synchronousSave(QWidget *dialogParent) override
    {
        if (m_document->url().isEmpty()) {
            const QUrl url = QFileDialog::getSaveFileUrl(dialogParent, i18n("Save As (%1)", m_document->documentName()));
            if (!url.isEmpty()) {
                if (!m_document->saveAs(url)) {
                    setState(SaveFailedState);
                    setText(1, m_document->url().toString());
                    return false;
                } else {
                    bool sc = m_document->waitSaveComplete();
                    setText(1, m_document->url().toString());
                    if (!sc) {
                        setState(SaveFailedState);
                        return false;
                    } else {
                        setState(SaveOKState);
                        return true;
                    }
                }
            } else {
                // setState(SaveFailedState);
                return false;
            }
        } else {
            // document has an existing location
            if (!m_document->save()) {
                setState(SaveFailedState);
                setText(1, m_document->url().toString());
                return false;
            } else {
                bool sc = m_document->waitSaveComplete();
                setText(1, m_document->url().toString());
                if (!sc) {
                    setState(SaveFailedState);
                    return false;
                } else {
                    setState(SaveOKState);
                    return true;
                }
            }
        }

        return false;
    }

private:
    KTextEditor::Document *m_document;
};

KateSaveModifiedDialog::KateSaveModifiedDialog(QWidget *parent, const std::vector<KTextEditor::Document *> &documents)
    : QDialog(parent)
{
    const bool multipleDocuments = documents.size() != 1;

    setWindowTitle(multipleDocuments ? i18n("Save Documents") : i18n("Close Document"));
    setObjectName(QStringLiteral("KateSaveModifiedDialog"));
    setModal(true);

    auto *wrapperLayout = new QGridLayout;
    setLayout(wrapperLayout);

    auto *mainLayout = new QVBoxLayout;
    wrapperLayout->addLayout(mainLayout, 0, 1);

    // label

    m_label = new QLabel;

    if (!multipleDocuments) {
        // Display a simpler dialog, similar to a QMessageBox::warning one

        // Display a "warning" label as QMessageBox does
        const auto icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
        const int iconSize = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this);
        auto *iconLabel = new QLabel;
        iconLabel->setPixmap(icon.pixmap(QSize(iconSize, iconSize)));
        wrapperLayout->addWidget(iconLabel, 0, 0);

        m_label->setText(i18n("The document \"%1\" has been modified. Do you want to save your changes or discard them?", documents.front()->documentName()));
        m_label->setWordWrap(true);

    } else {
        m_label->setText(i18n("<qt>The following documents have been modified. Do you want to save them before closing?</qt>"));
    }

    mainLayout->addWidget(m_label);

    // main view
    m_list = new QTreeWidget(this);
    mainLayout->addWidget(m_list);
    m_list->setColumnCount(2);
    m_list->setHeaderLabels(QStringList{i18n("Documents"), i18n("Location")});
    m_list->setRootIsDecorated(true);

    for (KTextEditor::Document *doc : documents) {
        m_list->addTopLevelItem(new KateSaveModifiedDocumentCheckListItem(doc));
    }
    m_list->resizeColumnToContents(0);

    connect(m_list, &QTreeWidget::itemChanged, this, &KateSaveModifiedDialog::slotItemActivated);

    auto *selectAllButton = new QPushButton(i18n("Se&lect All"), this);
    mainLayout->addWidget(selectAllButton);
    connect(selectAllButton, &QPushButton::clicked, this, &KateSaveModifiedDialog::slotSelectAll);

    if (!multipleDocuments) {
        m_list->hide();
        selectAllButton->hide();
    }

    // dialog buttons
    auto *buttons = new QDialogButtonBox(this);
    wrapperLayout->addWidget(buttons, 1, 1);

    m_saveButton = new QPushButton;
    KGuiItem::assign(m_saveButton, KStandardGuiItem::save());
    buttons->addButton(m_saveButton, QDialogButtonBox::YesRole);
    connect(m_saveButton, &QPushButton::clicked, this, &KateSaveModifiedDialog::slotSaveSelected);

    auto *discardButton = new QPushButton;
    KGuiItem::assign(discardButton, KStandardGuiItem::discard());
    buttons->addButton(discardButton, QDialogButtonBox::NoRole);
    connect(discardButton, &QPushButton::clicked, this, &KateSaveModifiedDialog::slotDoNotSave);

    auto *cancelButton = new QPushButton;
    KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
    cancelButton->setDefault(true);
    buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);
    connect(cancelButton, &QPushButton::clicked, this, &KateSaveModifiedDialog::reject);
    cancelButton->setFocus();
}

KateSaveModifiedDialog::~KateSaveModifiedDialog()
{
}

void KateSaveModifiedDialog::slotItemActivated(QTreeWidgetItem *, int)
{
    bool enableSaveButton = false;

    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        if (m_list->topLevelItem(i)->checkState(0) == Qt::Checked) {
            enableSaveButton = true;
            break;
        }
    }

    m_saveButton->setEnabled(enableSaveButton);
}

void KateSaveModifiedDialog::slotSelectAll()
{
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        m_list->topLevelItem(i)->setCheckState(0, Qt::Checked);
    }

    m_saveButton->setEnabled(true);
}

void KateSaveModifiedDialog::slotSaveSelected()
{
    if (doSave()) {
        done(QDialog::Accepted);
    }
}

void KateSaveModifiedDialog::slotDoNotSave()
{
    done(QDialog::Accepted);
}

bool KateSaveModifiedDialog::doSave()
{
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        auto *cit = static_cast<AbstractKateSaveModifiedDialogCheckListItem *>(m_list->topLevelItem(i));

        if (cit->checkState(0) == Qt::Checked && (cit->state() != AbstractKateSaveModifiedDialogCheckListItem::SaveOKState)) {
            if (!cit->synchronousSave(this /*perhaps that should be the kate mainwindow*/)) {
                if (cit->state() == AbstractKateSaveModifiedDialogCheckListItem::SaveFailedState) {
                    KMessageBox::error(this, i18n("Data you requested to be saved could not be written. Please choose how you want to proceed."));
                }
                return false;
            }
        } else if ((cit->checkState(0) != Qt::Checked) && (cit->state() == AbstractKateSaveModifiedDialogCheckListItem::SaveFailedState)) {
            cit->setState(AbstractKateSaveModifiedDialogCheckListItem::InitialState);
        }
    }

    return true;
}

void KateSaveModifiedDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        KMessageDialog::beep(KMessageDialog::WarningTwoActionsCancel, m_label->text(), this);
    }
}

bool KateSaveModifiedDialog::queryClose(QWidget *parent, const std::vector<KTextEditor::Document *> &documents)
{
    KateSaveModifiedDialog d(parent, documents);
    return (d.exec() != QDialog::Rejected);
}
