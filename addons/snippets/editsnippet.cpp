/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  SPDX-FileCopyrightText: 2007 Robert Gruber <rgruber@users.sourceforge.net>
 *  SPDX-FileCopyrightText: 2010 Milian Wolff <mail@milianw.de>
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "editsnippet.h"

#include "ui_editsnippet.h"

#include "snippet.h"
#include "snippetrepository.h"
#include "snippetstore.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

#include <QAction>
#include <QPushButton>
#include <QWhatsThis>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

static const char s_configFile[] = "kate-snippetsrc";

static KTextEditor::View *createView(QWidget *tabWidget)
{
    auto document = KTextEditor::Editor::instance()->createDocument(tabWidget);
    auto view = document->createView(tabWidget);

    view->action(QStringLiteral("file_save"))->setEnabled(false);
    tabWidget->layout()->addWidget(view);
    view->setStatusBarEnabled(false);
    return view;
}

EditSnippet::EditSnippet(SnippetRepository *repository, Snippet *snippet, QWidget *parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::EditSnippetBase>())
    , m_repo(repository)
    , m_snippet(snippet)
    , m_topBoxModified(false)
{
    Q_ASSERT(m_repo);
    m_ui->setupUi(this);

    connect(this, &QDialog::accepted, this, &EditSnippet::save);

    m_okButton = m_ui->buttons->button(QDialogButtonBox::Ok);
    KGuiItem::assign(m_okButton, KStandardGuiItem::ok());
    m_ui->buttons->addButton(m_okButton, QDialogButtonBox::AcceptRole);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);

    auto cancelButton = m_ui->buttons->button(QDialogButtonBox::Cancel);
    KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
    m_ui->buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_snippetView = createView(m_ui->snippetTab);
    if (!m_repo->fileTypes().isEmpty()) {
        m_snippetView->document()->setMode(m_repo->fileTypes().first());
    }

    connect(m_ui->modeComboBox, &QComboBox::currentIndexChanged, this, [this]() {
        if (m_ui->modeComboBox->currentData().toInt() == Snippet::TextTemplate) {
            m_ui->snippetLabel->setText(
                i18n("Text to insert into the document (see the "
                     "<a href=\"help:/kate/kate-application-plugin-snippets.html#snippet-input-template\">handbook</a> "
                     "for special fields)."));
            m_snippetView->document()->setMode(QStringLiteral("Normal"));
        } else {
            m_ui->snippetLabel->setText(
                i18n("JavaScript code to evaluate (see the "
                     "<a href=\"help:/kate/kate-application-plugin-snippets.html#snippet-input-script\">handbook</a> "
                     "for details)."));
            m_snippetView->document()->setMode(QStringLiteral("JavaScript"));
        }
    });
    m_ui->snippetLabel->setOpenExternalLinks(true);
    m_ui->modeComboBox->addItem(i18n("Text template"), QVariant(Snippet::TextTemplate));
    m_ui->modeComboBox->addItem(i18n("Script"), QVariant(Snippet::Script));
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 15, 0)
    m_ui->modeComboBox->hide();
    m_ui->modeComboBoxLabel->hide();
#endif

    m_scriptsView = createView(m_ui->scriptTab);
    m_scriptsView->document()->setMode(QStringLiteral("JavaScript"));
    m_scriptsView->document()->setText(m_repo->script());
    m_scriptsView->document()->setModified(false);

    m_ui->scriptLabel->setText(
        i18n("JavaScript functions shared between all snippets in this repository (see the "
             "<a href=\"help:/kate/kate-application-plugin-snippets.html#snippet-input-library\">handbook</a>)."));
    m_ui->scriptLabel->setOpenExternalLinks(true);

    m_ui->descriptionLabel->setText(i18n("Optional description to show in tooltips. You may use basic HTML formatting."));
    m_descriptionView = createView(m_ui->descriptionTab);
    if (m_snippet != nullptr) {
        m_descriptionView->document()->setText(m_snippet->description());
    }
    m_descriptionView->document()->setModified(false);

    // view for testing the snippet
    m_testView = createView(m_ui->testWidget);
    // splitter default size ratio
    m_ui->splitter->setSizes(QList<int>() << 400 << 150);
    connect(m_ui->dotest_button, &QPushButton::clicked, this, &EditSnippet::test);

    // modified notification stuff
    connect(m_ui->snippetNameEdit, &QLineEdit::textEdited, this, &EditSnippet::topBoxModified);
    connect(m_ui->snippetNameEdit, &QLineEdit::textEdited, this, &EditSnippet::validate);
    connect(m_ui->modeComboBox, &QComboBox::currentIndexChanged, this, &EditSnippet::topBoxModified);
    connect(m_ui->snippetShortcut, &KKeySequenceWidget::keySequenceChanged, this, &EditSnippet::topBoxModified);
    connect(m_snippetView->document(), &KTextEditor::Document::textChanged, this, &EditSnippet::validate);

    // if we edit a snippet, add all existing data
    if (m_snippet) {
        setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));

        m_snippetView->document()->setText(m_snippet->snippet());
        m_ui->snippetNameEdit->setText(m_snippet->text());
#if KTEXTEDITOR_VERSION >= QT_VERSION_CHECK(6, 15, 0)
        m_ui->modeComboBox->setCurrentIndex(m_ui->modeComboBox->findData(QVariant(snippet->snippetType())));
#endif
        m_ui->snippetShortcut->setKeySequence(m_snippet->action()->shortcut());

        // unset modified flags
        m_snippetView->document()->setModified(false);
        m_topBoxModified = false;
    } else {
        setWindowTitle(i18n("Create New Snippet in Repository %1", m_repo->text()));
    }

    m_ui->messageWidget->hide();
    validate();

    m_ui->snippetNameEdit->setFocus();
    setTabOrder(m_ui->snippetNameEdit, m_snippetView);

    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String(s_configFile));
    KConfigGroup group = config->group(QStringLiteral("General"));
    const QSize savedSize = group.readEntry("Size", QSize());
    if (savedSize.isValid()) {
        resize(savedSize);
    }

    // Position cursor at the beginning
    m_snippetView->setCursorPosition({});
    m_scriptsView->setCursorPosition({});
    m_descriptionView->setCursorPosition({});
}

void EditSnippet::test()
{
    auto type = (Snippet::SnippetType)m_ui->modeComboBox->currentData().toInt();
    // NOTE: script snippets will typically want to work with existing text
    if (type != Snippet::Script) {
        m_testView->document()->clear();
    }
    Snippet snippet;
    snippet.setSnippet(m_snippetView->document()->text(), m_descriptionView->document()->text(), type);
    snippet.apply(m_testView, m_scriptsView->document()->text());
    m_testView->setFocus();
}

EditSnippet::~EditSnippet() = default;

void EditSnippet::setSnippetText(const QString &text)
{
    m_snippetView->document()->setText(text);
    validate();
}

void EditSnippet::validate()
{
    const QString &name = m_ui->snippetNameEdit->text();
    bool valid = !name.simplified().isEmpty() && !m_snippetView->document()->isEmpty();
    // make sure the snippetname includes no spaces
    if (name.contains(QLatin1Char(' ')) || name.contains(QLatin1Char('\t'))) {
        // allow with a warning
        m_ui->messageWidget->setText(i18n("Snippet names with spaces may not work well in completions"));
        m_ui->messageWidget->animatedShow();
    } else {
        // hide message widget if snippet does not include spaces
        m_ui->messageWidget->animatedHide();
    }
    m_okButton->setEnabled(valid);
}

void EditSnippet::save()
{
    Q_ASSERT(!m_ui->snippetNameEdit->text().isEmpty());

    if (!m_snippet) {
        // save as new snippet
        m_snippet = new Snippet();
        m_snippet->action(); // ensure that the snippet's QAction is created before it is added to a widget by the rowsInserted() signal
        m_repo->appendRow(m_snippet);
    }
    m_snippet->setSnippet(m_snippetView->document()->text(),
                          m_descriptionView->document()->text(),
                          (Snippet::SnippetType)m_ui->modeComboBox->currentData().toInt());
    m_snippetView->document()->setModified(false);
    m_snippet->setText(m_ui->snippetNameEdit->text());
    m_snippet->action()->setShortcut(m_ui->snippetShortcut->keySequence());
    m_repo->setScript(m_scriptsView->document()->text());
    m_scriptsView->document()->setModified(false);
    m_topBoxModified = false;
    m_repo->save();

    setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));

    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String(s_configFile));
    KConfigGroup group = config->group(QStringLiteral("General"));
    group.writeEntry("Size", size());
    group.sync();
}

void EditSnippet::reject()
{
    if (m_topBoxModified || m_snippetView->document()->isModified() || m_scriptsView->document()->isModified() || m_descriptionView->document()->isModified()) {
        int ret = KMessageBox::warningTwoActions(qApp->activeWindow(),
                                                 i18n("The snippet contains unsaved changes. Do you want to discard all changes?"),
                                                 i18n("Warning - Unsaved Changes"),
                                                 KStandardGuiItem::discard(),
                                                 KGuiItem(i18n("Continue editing")));
        if (ret == KMessageBox::SecondaryAction) {
            return;
        }
    }
    QDialog::reject();
}

void EditSnippet::topBoxModified()
{
    m_topBoxModified = true;
}

#include "moc_editsnippet.cpp"
