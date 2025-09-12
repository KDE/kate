/*
    SPDX-License-Identifier: LGPL-2.0-only
    SPDX-FileCopyrightText: 2004 Anders Lund <anders@alweb.dk>
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
*/

#include "katemwmodonhddialog.h"

#include "diffwidget.h"
#include "gitprocess.h"
#include "hostprocess.h"
#include "kateapp.h"
#include "katemainwindow.h"
#include "ktexteditor_utils.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KTextEditor/Document>

#include <QDir>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QStyle>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class KateDocItem : public QTreeWidgetItem
{
public:
    KateDocItem(KTextEditor::Document *doc, const QString &status, QTreeWidget *tw)
        : QTreeWidgetItem(tw)
        , document(doc)
    {
        setText(0, doc->url().toString());
        setText(1, status);
        if (!doc->isModified()) {
            setCheckState(0, Qt::Checked);
        } else {
            setCheckState(0, Qt::Unchecked);
        }
    }
    QPointer<KTextEditor::Document> document;
};

KateMwModOnHdDialog::KateMwModOnHdDialog(const QList<KTextEditor::Document *> &docs, QWidget *parent)
    : QDialog(parent)
    , m_blockAddDocument(false)
{
    setWindowTitle(i18n("Documents Modified on Disk"));
    setModal(true);

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    // Message
    auto *hb = new QHBoxLayout;
    mainLayout->addLayout(hb);

    // dialog text
    auto *icon = new QLabel(this);
    hb->addWidget(icon);
    icon->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(style()->pixelMetric(QStyle::PM_LargeIconSize, nullptr, this)));

    auto *t = new QLabel(i18n("<qt>The documents listed below have changed on disk.<p>Select one "
                              "or more at once, and press an action button until the list is empty.</p></qt>"),
                         this);
    hb->addWidget(t);
    hb->setStretchFactor(t, 1000);

    // Document list
    docsTreeWidget = new QTreeWidget(this);
    mainLayout->addWidget(docsTreeWidget);
    QStringList header;
    header << i18n("Filename") << i18n("Status on Disk");
    docsTreeWidget->setHeaderLabels(header);
    docsTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    docsTreeWidget->setRootIsDecorated(false);

    m_stateTexts << QString() << i18n("Modified") << i18n("Created") << i18n("Deleted");
    for (auto &doc : docs) {
        const auto docInfo = KateApp::self()->documentManager()->documentInfo(doc);
        if (!docInfo) {
            qWarning("Unexpected null doc info");
            continue;
        }
        new KateDocItem(doc, m_stateTexts[static_cast<uint>(docInfo->modifiedOnDiscReason)], docsTreeWidget);

        // ensure proper cleanups to avoid dangling pointers, we can arrive here multiple times, use unique connection
        connect(doc, &KTextEditor::Document::destroyed, this, &KateMwModOnHdDialog::removeDocument, Qt::UniqueConnection);
    }
    docsTreeWidget->header()->setStretchLastSection(false);
    docsTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    docsTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(docsTreeWidget, &QTreeWidget::currentItemChanged, this, &KateMwModOnHdDialog::slotSelectionChanged);
    connect(docsTreeWidget, &QTreeWidget::itemChanged, this, &KateMwModOnHdDialog::slotCheckedFilesChanged);

    // Diff line
    hb = new QHBoxLayout;
    mainLayout->addLayout(hb);

    btnDiff = new QPushButton(QIcon::fromTheme(QStringLiteral("document-preview")), i18n("&View Difference"), this);
    btnDiff->setWhatsThis(i18n("Calculates the difference between the editor contents and the disk file for the selected document."));
    hb->addWidget(btnDiff);
    connect(btnDiff, &QPushButton::clicked, this, &KateMwModOnHdDialog::slotDiff);

    // Dialog buttons
    dlgButtons = new QDialogButtonBox(this);
    mainLayout->addWidget(dlgButtons);

    auto *ignoreButton = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-warning")), i18n("&Ignore Changes"));
    ignoreButton->setToolTip(i18n("Remove modified flag from selected documents"));
    dlgButtons->addButton(ignoreButton, QDialogButtonBox::RejectRole);
    connect(ignoreButton, &QPushButton::clicked, this, &KateMwModOnHdDialog::slotIgnore);

    auto *overwriteButton = new QPushButton;
    KGuiItem::assign(overwriteButton, KStandardGuiItem::overwrite());
    overwriteButton->setToolTip(i18n("Overwrite selected documents, discarding disk changes"));
    dlgButtons->addButton(overwriteButton, QDialogButtonBox::DestructiveRole);
    connect(overwriteButton, &QPushButton::clicked, this, &KateMwModOnHdDialog::slotOverwrite);

    auto *reloadButton = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("&Reload"));
    reloadButton->setDefault(true);
    reloadButton->setToolTip(i18n("Reload selected documents from disk"));
    dlgButtons->addButton(reloadButton, QDialogButtonBox::DestructiveRole);
    connect(reloadButton, &QPushButton::clicked, this, &KateMwModOnHdDialog::slotReload);

    // buttons will only be enabled when items are checked. see slotCheckedFilesChanged()
    dlgButtons->setEnabled(false);
    slotCheckedFilesChanged(nullptr, 0);

    slotSelectionChanged(nullptr, nullptr);
}

KateMwModOnHdDialog::~KateMwModOnHdDialog()
{
    // ensure no cleanup of documents during shutdown
    disconnect();

    KateMainWindow::unsetModifiedOnDiscDialogIfIf(this);

    // if there are any living processes, disconnect them now before we get destroyed
    const QList<QProcess *> children = findChildren<QProcess *>();
    for (QProcess *child : children) {
        disconnect(child, nullptr, nullptr, nullptr);
    }
}

void KateMwModOnHdDialog::slotIgnore()
{
    handleSelected(Ignore);
}

void KateMwModOnHdDialog::slotOverwrite()
{
    handleSelected(Overwrite);
}

void KateMwModOnHdDialog::slotReload()
{
    handleSelected(Reload);
}

void KateMwModOnHdDialog::handleSelected(int action)
{
    // don't alter the treewidget via addDocument, we modify it here!
    m_blockAddDocument = true;

    // collect all items we can remove
    std::vector<QTreeWidgetItem *> itemsToDelete;
    for (QTreeWidgetItemIterator it(docsTreeWidget); *it; ++it) {
        auto *item = static_cast<KateDocItem *>(*it);
        KateDocumentInfo *docInfo = KateApp::self()->documentManager()->documentInfo(item->document);
        if (!item->document || !docInfo) {
            itemsToDelete.push_back(item);
            continue;
        }

        if (item->checkState(0) == Qt::Checked) {
            KTextEditor::Document::ModifiedOnDiskReason reason = docInfo->modifiedOnDiscReason;
            bool success = true;
            item->document->setModifiedOnDisk(KTextEditor::Document::OnDiskUnmodified);

            switch (action) {
            case Overwrite:
                success = item->document->save();
                if (!success) {
                    KMessageBox::error(this, i18n("Could not save the document \n'%1'", item->document->url().toString()));
                }
                break;

            case Reload:
                item->document->documentReload();
                break;

            default:
                break;
            }

            if (success) {
                itemsToDelete.push_back(item);
            } else {
                item->document->setModifiedOnDisk(reason);
            }
        }
    }

    // remove the marked items, addDocument is blocked, this is save!
    qDeleteAll(itemsToDelete);

    // any documents left unhandled?
    if (!docsTreeWidget->topLevelItemCount()) {
        accept();
    }

    // update the dialog buttons
    slotCheckedFilesChanged(nullptr, 0);

    // allow addDocument again
    m_blockAddDocument = false;
}

void KateMwModOnHdDialog::slotSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    auto *currentDocItem = static_cast<KateDocItem *>(current);
    if (currentDocItem && currentDocItem->document) {
        KateDocumentInfo *docInfo = KateApp::self()->documentManager()->documentInfo(currentDocItem->document);
        // set the diff button enabled
        btnDiff->setEnabled(docInfo && docInfo->modifiedOnDiscReason != KTextEditor::Document::OnDiskDeleted);
    }
}

void KateMwModOnHdDialog::slotCheckedFilesChanged(QTreeWidgetItem *, int column)
{
    if (column != 0) {
        // we only need to react when the checkbox (in column 0) changes
        return;
    }

    for (QTreeWidgetItemIterator it(docsTreeWidget); *it; ++it) {
        auto *item = static_cast<KateDocItem *>(*it);
        if (item->checkState(0) == Qt::Checked) {
            // at least 1 item is checked so we enable the buttons
            dlgButtons->setEnabled(true);
            return;
        }
    }

    // no items checked, disable all buttons
    dlgButtons->setEnabled(false);
}

// ### the code below is slightly modified from kdelibs/kate/part/katedialogs,
// class KateModOnHdPrompt.
void KateMwModOnHdDialog::slotDiff()
{
    if (!btnDiff->isEnabled()) { // diff button already pressed, proc not finished yet
        return;
    }

    if (!docsTreeWidget->currentItem()) {
        return;
    }

    QPointer<KTextEditor::Document> doc = (static_cast<KateDocItem *>(docsTreeWidget->currentItem()))->document;
    KateDocumentInfo *docInfo = KateApp::self()->documentManager()->documentInfo(doc);
    if (!doc || !docInfo) {
        return;
    }

    // don't try to diff a deleted file
    if (docInfo->modifiedOnDiscReason == KTextEditor::Document::OnDiskDeleted) {
        return;
    }

    auto f = new QTemporaryFile(this);
    f->open();
    f->write(doc->text().toUtf8().constData());
    f->flush();

    auto *p = new QProcess(this);
    QStringList args = {QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-index")};
    args << f->fileName();
    args << doc->url().toLocalFile();
    if (!setupGitProcess(*p, QDir::currentPath(), args)) {
        Utils::showMessage(i18n("Please install git to view diffs"), QIcon(), QStringLiteral("KateMwModOnHdDialog"), MessageType::Error);
        btnDiff->setEnabled(false);
        return;
    }

    connect(p, &QProcess::finished, this, [p, f, this, doc](int, QProcess::ExitStatus) {
        f->deleteLater();
        p->deleteLater();
        slotGitDiffDone(p, doc);
    });
    setCursor(Qt::WaitCursor);
    btnDiff->setEnabled(false);
    startHostProcess(*p);
}

void KateMwModOnHdDialog::slotGitDiffDone(QProcess *p, KTextEditor::Document *doc)
{
    setCursor(Qt::ArrowCursor);
    slotSelectionChanged(docsTreeWidget->currentItem(), nullptr);

    const QProcess::ExitStatus es = p->exitStatus();

    if (es != QProcess::NormalExit) {
        KMessageBox::error(this,
                           i18n("The diff command failed. Please make sure that "
                                "git is installed and in your PATH."),
                           i18n("Error Creating Diff"));
        return;
    }

    const QByteArray out = p->readAllStandardOutput();
    if (!out.isEmpty()) {
        DiffParams params;
        QString s = QFileInfo(doc->url().toLocalFile()).fileName() + i18n("[OLD]");
        QString t = QFileInfo(doc->url().toLocalFile()).fileName() + i18n("[NEW]");
        params.tabTitle = s + QStringLiteral("..") + t;
        params.workingDir = p->workingDirectory();
        auto mw = static_cast<KateMainWindow *>(parent());
        DiffWidgetManager::openDiff(out, params, mw->wrapper());
    }
}

void KateMwModOnHdDialog::addDocument(KTextEditor::Document *doc)
{
    // guard this e.g. during handleSelected
    if (m_blockAddDocument || !KateApp::self()->documentManager()->documentInfo(doc)) {
        return;
    }

    // avoid double occurrences, we want a fresh item
    removeDocument(doc);

    uint reason = static_cast<uint>(KateApp::self()->documentManager()->documentInfo(doc)->modifiedOnDiscReason);
    if (reason) {
        new KateDocItem(doc, m_stateTexts[reason], docsTreeWidget);
        connect(doc, &KTextEditor::Document::destroyed, this, &KateMwModOnHdDialog::removeDocument, Qt::UniqueConnection);
    }

    if (!docsTreeWidget->topLevelItemCount()) {
        accept();
    }
}

void KateMwModOnHdDialog::removeDocument(QObject *doc)
{
    for (QTreeWidgetItemIterator it(docsTreeWidget); *it; ++it) {
        auto *item = static_cast<KateDocItem *>(*it);
        if (item->document == static_cast<KTextEditor::Document *>(doc)) {
            disconnect(item->document, nullptr, this, nullptr);
            delete item;
            break;
        }
    }
}

void KateMwModOnHdDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == 0) {
        if (event->key() == Qt::Key_Escape) {
            event->accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}

void KateMwModOnHdDialog::closeEvent(QCloseEvent *e)
{
    if (!docsTreeWidget->topLevelItemCount()) {
        QDialog::closeEvent(e);
    } else {
        e->ignore();
    }
}

#include "moc_katemwmodonhddialog.cpp"
