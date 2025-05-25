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
                i18n("The text your snippet will insert into the document. "
                     "<a href=\"A template snippet can contain editable fields. They can be "
                     "cycled by pressing Tab. The following expressions can be used "
                     "in the template text to create fields: <br><tt>${field_name}</tt> "
                     "creates a simple, editable field. All subsequent occurrences of the "
                     "same field_name will mirror the contents of the first "
                     "during editing.<br><tt>${field_name=default}</tt> can be used to "
                     "specify a default value for the field, where <tt>default</tt> can be any "
                     "JavaScript expression (use quotatio marks to specify "
                     "a fixed string as default value).<br><tt>${func(other_field1, "
                     "other_field2, ...)</tt> evaluates a JavaScript function defined in the "
                     "'Scripts Library' tab on each edit, and is replaced by the value returned "
                     "by that function.<br><tt>${cursor}</tt> "
                     "can be used to mark the end position of the cursor after everything "
                     "else was filled in.\">More...</a>"));
            m_snippetView->document()->setMode(QStringLiteral("None")); // TODO
        } else {
            // TODO
            m_ui->snippetLabel->setText(i18n("Explanation for script. Individual portion vs Script Library"));
            m_snippetView->document()->setMode(QStringLiteral("JavaScript"));
        }
    });
    m_ui->modeComboBox->addItem(i18n("Text template"), QVariant(Snippet::TextTemplate));
    m_ui->modeComboBox->addItem(i18n("Script"), QVariant(Snippet::Script));

    m_scriptsView = createView(m_ui->scriptTab);
    m_scriptsView->document()->setMode(QStringLiteral("JavaScript"));
    m_scriptsView->document()->setText(m_repo->script());
    m_scriptsView->document()->setModified(false);

    // TODO: link to handbook
    m_ui->scriptLabel->setText(
        i18n("Write down JavaScript helper functions to use in your snippets here. "
             "<a href=\"Editable template fields are accessible as local variables. "
             "For example in a template snippet containing <tt>${field}</tt>, a "
             "variable called <tt>field</tt> will be present which contains the "
             "up-to-date contents of the template field. Those variables can either "
             "be used in the function statically or passed as arguments, by using the "
             "<tt>${func(field)}</tt> or <tt>${field2=func(field)}</tt> syntax in the "
             "snippet string.<br>You can use the kate scripting API to get the "
             "selected text, full text, file name and more by using the appropriate "
             "methods of the <tt>document</tt> and <tt>view</tt> objects. Refer to "
             "the scripting API documentation for more information.\">More...</a>"));

    m_ui->descriptionLabel->setText(i18n("(Optional) description to show in tooltips. May contain HTML formatting."));
    m_descriptionView = createView(m_ui->descriptionTab);
    m_descriptionView->document()->setText(m_snippet->description());
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

    auto showHelp = [](const QString &text) {
        QWhatsThis::showText(QCursor::pos(), text);
    };
    connect(m_ui->snippetLabel, &QLabel::linkActivated, showHelp);
    connect(m_ui->scriptLabel, &QLabel::linkActivated, showHelp);

    // if we edit a snippet, add all existing data
    if (m_snippet) {
        setWindowTitle(i18n("Edit Snippet %1 in %2", m_snippet->text(), m_repo->text()));

        m_snippetView->document()->setText(m_snippet->snippet());
        m_ui->snippetNameEdit->setText(m_snippet->text());
        m_ui->modeComboBox->setCurrentIndex(m_ui->modeComboBox->findData(QVariant(snippet->snippetType())));
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
