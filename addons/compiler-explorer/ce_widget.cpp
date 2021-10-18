#include "ce_widget.h"
#include "AsmView.h"
#include "AsmViewModel.h"
#include "ce_service.h"
#include "compiledbreader.h"

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QToolButton>
#include <QTreeView>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/MainWindow>
#include <KXMLGUIFactory>

enum CE_Options {
    CE_Option_FilterLabel = 1,
    CE_Option_IntelAsm,
    CE_Option_FilterUnusedLibFuncs,
    CE_Option_FilterComments,
    CE_Option_Demangle,
};

static bool readConfigForCEOption(CE_Options o)
{
    KConfigGroup cg(KSharedConfig::openConfig(), "kate_compilerexplorer");
    switch (o) {
    case CE_Option_FilterLabel:
        return cg.readEntry("FilterUnusedLabels", true);
    case CE_Option_IntelAsm:
        return cg.readEntry("UseIntelAsmSyntax", true);
    case CE_Option_FilterUnusedLibFuncs:
        return cg.readEntry("OptionFilterLibFuncs", true);
    case CE_Option_FilterComments:
        return cg.readEntry("OptionFilterComments", true);
    case CE_Option_Demangle:
        return cg.readEntry("OptionDemangle", true);
    default:
        Q_UNREACHABLE();
    }
}

static void writeConfigForCEOption(CE_Options o, bool value)
{
    KConfigGroup cg(KSharedConfig::openConfig(), "kate_compilerexplorer");
    switch (o) {
    case CE_Option_FilterLabel:
        return cg.writeEntry("FilterUnusedLabels", value);
    case CE_Option_IntelAsm:
        return cg.writeEntry("UseIntelAsmSyntax", value);
    case CE_Option_FilterUnusedLibFuncs:
        return cg.writeEntry("OptionFilterLibFuncs", value);
    case CE_Option_FilterComments:
        return cg.writeEntry("OptionFilterComments", value);
    case CE_Option_Demangle:
        return cg.writeEntry("OptionDemangle", value);
    default:
        Q_UNREACHABLE();
    }
}

CEWidget::CEWidget(CEPluginView *pluginView, KTextEditor::MainWindow *mainWindow)
    : QWidget() // The widget will be passed on to kate, no need for a parent
    , m_pluginView(pluginView)
    , m_mainWindow(mainWindow)
    , m_asmView(new AsmView(this))
    , m_model(new AsmViewModel(this))
    , m_lineEdit(new QLineEdit(this))
    , m_languagesCombo(new QComboBox(this))
    , m_compilerCombo(new QComboBox(this))
    , m_optsCombo(new QToolButton(this))
    , m_compileButton(new QPushButton(this))
{
    doc = m_mainWindow->activeView()->document();

    setWindowTitle(QStringLiteral("Compiler Explorer - ") + doc->documentName());

    auto mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    createTopBar(mainLayout);
    createMainViews(mainLayout);

    connect(m_compileButton, &QPushButton::clicked, this, &CEWidget::doCompile);
    connect(CompilerExplorerSvc::instance(), &CompilerExplorerSvc::asmResult, this, &CEWidget::processAndShowAsm);

    connect(this, &CEWidget::lineHovered, m_model, &AsmViewModel::highlightLinkedAsm);
    connect(m_asmView, &AsmView::scrollToLineRequested, this, [this](int line) {
        m_textEditor->setCursorPosition({line, 0});
    });

    QString file = doc->url().toLocalFile();
    QString compilecmds = CompileDBReader::locateCompileCommands(m_mainWindow, file);
    QString args = CompileDBReader::filteredArgsForFile(compilecmds, file);
    m_lineEdit->setText(args);

    setFocusPolicy(Qt::StrongFocus);
}

CEWidget::~CEWidget()
{
    removeViewAsActiveXMLGuiClient();
}

bool CEWidget::shouldClose()
{
    int ret = KMessageBox::warningYesNo(this, i18n("Do you really want to close %1?", windowTitle()));
    return ret == KMessageBox::Yes;
}

void CEWidget::removeViewAsActiveXMLGuiClient()
{
    if (m_textEditor) {
        m_mainWindow->guiFactory()->removeClient(m_textEditor);
    }

    if (oldClient) {
        m_mainWindow->guiFactory()->addClient(oldClient);
    }
}

void CEWidget::setViewAsActiveXMLGuiClient()
{
    if (!m_textEditor) {
        return;
    }

    // remove current active "KTE::View" client from mainWindow
    const auto clients = m_mainWindow->guiFactory()->clients();
    for (KXMLGUIClient *c : clients) {
        if (c->componentName() == QStringLiteral("katepart") && c != m_textEditor) {
            oldClient = static_cast<KTextEditor::View *>(c);
            m_mainWindow->guiFactory()->removeClient(c);
            break;
        }
    }
    // Make us the client
    m_mainWindow->guiFactory()->addClient(m_textEditor);
}

bool CEWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_textEditor->focusProxy() && e->type() == QEvent::FocusIn) {
        setViewAsActiveXMLGuiClient();
        return QWidget::eventFilter(o, e);
    } else if (o == m_textEditor->focusProxy() && e->type() == QEvent::FocusOut) {
        removeViewAsActiveXMLGuiClient();
        return QWidget::eventFilter(o, e);
    }

    if (o != m_textEditor) {
        return QWidget::eventFilter(o, e);
    }

    if (e->type() != QEvent::HoverMove) {
        return QWidget::eventFilter(o, e);
    }

    auto event = static_cast<QHoverEvent *>(e);
    auto cursor = m_textEditor->coordinatesToCursor(event->pos());
    Q_EMIT lineHovered(cursor.line()); // Can be invalid, that is okay
    m_asmView->viewport()->update();

    return QWidget::eventFilter(o, e);
}

void CEWidget::createTopBar(class QVBoxLayout *mainLayout)
{
    QHBoxLayout *topBarLayout = new QHBoxLayout;
    mainLayout->addLayout(topBarLayout);

    topBarLayout->addWidget(m_languagesCombo);
    topBarLayout->addWidget(m_compilerCombo);
    topBarLayout->addWidget(m_optsCombo);
    topBarLayout->addWidget(m_lineEdit);
    topBarLayout->addWidget(m_compileButton);

    auto svc = CompilerExplorerSvc::instance();

    connect(svc, &CompilerExplorerSvc::languages, this, &CEWidget::setAvailableLanguages);
    svc->sendRequest(CompilerExplorer::Languages);

    connect(svc, &CompilerExplorerSvc::compilers, this, &CEWidget::setAvailableCompilers);
    svc->sendRequest(CompilerExplorer::Compilers);

    m_compileButton->setText(i18n("Compile"));

    initOptionsComboBox();
}

void CEWidget::setAvailableLanguages(const QByteArray &data)
{
    if (!doc) {
        return;
    }

    const QJsonArray langs = QJsonDocument::fromJson(data).array();

    // We use the highlightingMode to set the current active language,
    // but this needs some sort of mapping to CE's lang names
    auto currentFileLang = doc->highlightingMode();
    QString activeLang;

    m_languagesCombo->clear();

    for (const auto &langJV : langs) {
        const auto lang = langJV.toObject();
        const auto id = lang.value(QStringLiteral("id")).toString();
        const auto name = lang.value(QStringLiteral("name")).toString();

        if (name == currentFileLang) {
            activeLang = name;
        }

        m_languagesCombo->addItem(name, id);
    }
    m_languagesCombo->setCurrentText(activeLang);
    m_languagesCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    connect(m_languagesCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString id = m_languagesCombo->itemData(index).toString();
        repopulateCompilersCombo(id);
    });
}

void CEWidget::setAvailableCompilers(const QByteArray &data)
{
    if (!doc) {
        return;
    }

    const QJsonArray json = QJsonDocument::fromJson(data).array();

    m_langToCompiler.clear();

    for (const auto &value : json) {
        const auto compilerName = value[QStringLiteral("name")].toString();
        const auto lang = value[QStringLiteral("lang")].toString();
        const auto id = value[QStringLiteral("id")];

        Compiler compiler{compilerName, id};
        m_langToCompiler.push_back({lang, compiler});
    }

    repopulateCompilersCombo(doc->highlightingMode().toLower());
    m_compilerCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
}

void CEWidget::repopulateCompilersCombo(const QString &lang)
{
    auto currentFileLang = lang;

    auto compilersForLang = compilersForLanguage(currentFileLang);
    if (compilersForLang.empty()) {
        compilersForLang = m_langToCompiler;
    }

    m_compilerCombo->clear();

    for (const auto &[lang, compiler] : compilersForLang) {
        m_compilerCombo->addItem(compiler.name, compiler.id);
    }

    m_compilerCombo->setCurrentIndex(0);
}

std::vector<CEWidget::CompilerLangPair> CEWidget::compilersForLanguage(const QString &lang) const
{
    std::vector<CompilerLangPair> compilersForLang;
    for (const auto &pair : m_langToCompiler) {
        if (pair.first == lang) {
            compilersForLang.push_back(pair);
        }
    }
    return compilersForLang;
}

void CEWidget::initOptionsComboBox()
{
    QMenu *menu = new QMenu(this);
    m_optsCombo->setMenu(menu);
    m_optsCombo->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_optsCombo->setText(i18n("Options"));
    m_optsCombo->setPopupMode(QToolButton::InstantPopup);
    m_optsCombo->setArrowType(Qt::DownArrow);

    auto checkableAction = [this](const QString &name, CE_Options o) {
        QAction *action = new QAction(name, this);
        action->setCheckable(true);
        action->setChecked(readConfigForCEOption(o));
        action->setData((int)o);

        connect(action, &QAction::toggled, this, [o](bool v) {
            writeConfigForCEOption(o, v);
        });

        return action;
    };

    menu->addAction(checkableAction(i18n("Demangle Identifiers"), CE_Option_Demangle));
    menu->addAction(checkableAction(i18n("Filter Library Functions"), CE_Option_FilterUnusedLibFuncs));
    menu->addAction(checkableAction(i18n("Filter Unused Labels"), CE_Option_FilterLabel));
    menu->addAction(checkableAction(i18n("Filter Comments"), CE_Option_FilterComments));
    menu->addAction(checkableAction(i18n("Intel Syntax"), CE_Option_IntelAsm));

    menu->addAction(i18n("Change Url..."), this, [this] {
        KConfigGroup cg(KSharedConfig::openConfig(), "kate_compilerexplorer");
        QString url = cg.readEntry("kate_compilerexplorer_url", QStringLiteral("http://localhost:10240"));

        bool ok = false;
        QString newUrl = QInputDialog::getText(this,
                                               i18n("Enter Url"),
                                               i18n("Enter Url to CompilerExplorer instance. For e.g., http://localhost:10240"),
                                               QLineEdit::Normal,
                                               url,
                                               &ok);

        if (ok && !newUrl.isEmpty()) {
            CompilerExplorerSvc::instance()->changeUrl(newUrl);
            cg.writeEntry("kate_compilerexplorer_url", newUrl);
        }
    });
}

void CEWidget::createMainViews(class QVBoxLayout *mainLayout)
{
    if (!doc) {
        return;
    }

    QSplitter *splitter = new QSplitter(this);

    m_textEditor = doc->createView(this, m_mainWindow);

    setViewAsActiveXMLGuiClient();

    m_asmView->setModel(m_model);

    addExtraActionstoTextEditor();
    m_textEditor->installEventFilter(this);
    m_textEditor->focusProxy()->installEventFilter(this);

    splitter->addWidget(m_textEditor);
    splitter->addWidget(m_asmView);
    splitter->setSizes({INT_MAX, INT_MAX});

    mainLayout->addWidget(splitter);
}

QString CEWidget::currentCompiler() const
{
    return m_compilerCombo->currentData().toString();
}

bool CEWidget::compilationFailed(const QJsonObject &obj)
{
    int colWidth = m_asmView->columnWidth(AsmViewModel::Column_Text);

    QFontMetrics fm(m_asmView->font());
    int avgCharWidth = fm.averageCharWidth();

    int maxChars = colWidth / avgCharWidth;

    auto code = obj.value(QStringLiteral("code"));
    if (!code.isUndefined()) {
        int compilerReturnCode = code.toInt();
        if (compilerReturnCode != 0) {
            const auto stderror = obj.value(QStringLiteral("stderr")).toArray();
            std::vector<AsmRow> rows;

            // strip any escape sequences
            static const QRegularExpression re(QStringLiteral("[\u001b\u009b][[()#;?]*(?:[0-9]{1,4}(?:;[0-9]{0,4})*)?[0-9A-ORZcf-nqry=><]"));

            AsmRow r;
            r.text = i18n("Compiler returned: %1", compilerReturnCode);
            rows.push_back(r);

            for (const auto &err : stderror) {
                QString text = err.toObject().value(QStringLiteral("text")).toString();
                text.replace(re, QLatin1String(""));

                QStringList lines;
                lines.reserve(text.size() / maxChars);
                for (int i = 0; i < text.size(); i += maxChars) {
                    lines << text.mid(i, maxChars);
                }

                for (const auto &line : std::as_const(lines)) {
                    AsmRow r;
                    r.text = line;
                    rows.push_back(r);
                }
            }

            m_model->setDataFromCE(std::move(rows), {}, {});
            return true;
        }
    }
    return false;
}

void CEWidget::processAndShowAsm(const QByteArray &data)
{
    m_model->clear();

    std::vector<AsmRow> rows;
    QHash<SourcePos, AsmViewModel::CodeGenLines> sourceToAsm;

    const auto json = QJsonDocument::fromJson(data);

    const auto mainObj = json.object();

    if (compilationFailed(mainObj)) {
        m_model->setHasError(true);
        return;
    }
    m_model->setHasError(false);

    //     printf("%s\n", json.toJson().constData());
    const QJsonArray assembly = mainObj.value(QStringLiteral("asm")).toArray();
    rows.reserve(assembly.size());

    int currentAsmLine = 0;

    // Process the assembly
    for (const auto &line : assembly) {
        AsmRow row;

        auto labels = line[QStringLiteral("labels")].toArray();
        if (!labels.empty()) {
            for (const auto &label : labels) {
                LabelInRow l;
                auto rangeJV = label.toObject().value(QStringLiteral("range"));

                const auto range = rangeJV.toObject();
                int startCol = range.value(QStringLiteral("startCol")).toInt() - 1;
                int endCol = range.value(QStringLiteral("endCol")).toInt();
                l.col = startCol;
                l.len = endCol - startCol;
                row.labels.push_back(l);
            }
        }

        const auto source = line[QStringLiteral("source")].toObject();
        QString file = source.value(QStringLiteral("file")).toString();
        int srcLine = source.value(QStringLiteral("line")).toInt();
        int srcCol = source.value(QStringLiteral("column")).toInt();

        row.source.file = file.isEmpty() ? QString() : file; // can be empty
        row.source.line = srcLine;
        row.source.col = srcCol;

        row.text = line[QStringLiteral("text")].toString();

        rows.push_back(row);
        sourceToAsm[row.source].push_back(currentAsmLine);

        currentAsmLine++;
    }

    const QJsonObject labelDefinitions = mainObj.value(QStringLiteral("labelDefinitions")).toObject();
    // Label => Line Number
    QHash<QString, int> labelDefs;
    auto it = labelDefinitions.constBegin();
    auto end = labelDefinitions.constEnd();
    for (; it != end; ++it) {
        labelDefs.insert(it.key(), it.value().toInt());
    }

    m_model->setDataFromCE(std::move(rows), std::move(sourceToAsm), std::move(labelDefs));
    m_asmView->resizeColumnToContents(0);
}

void CEWidget::doCompile()
{
    m_model->clear();

    if (!doc) {
        return;
    }

    if (auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(m_textEditor)) {
        auto font = ciface->configValue(QStringLiteral("font")).value<QFont>();
        m_model->setFont(font);
    }

    const QString text = doc->text();
    if (text.isEmpty()) {
        return;
    }

    const auto actions = m_optsCombo->menu()->actions();

    bool demangle = false;
    bool intel = false;
    bool labels = false;
    bool comments = false;
    bool libfuncs = false;
    for (auto action : actions) {
        bool isChecked = action->isChecked();
        if (action->data().toInt() == CE_Option_Demangle)
            demangle = isChecked;
        else if (action->data().toInt() == CE_Option_FilterComments)
            comments = isChecked;
        else if (action->data().toInt() == CE_Option_FilterLabel)
            labels = isChecked;
        else if (action->data().toInt() == CE_Option_FilterUnusedLibFuncs)
            libfuncs = isChecked;
        else if (action->data().toInt() == CE_Option_IntelAsm)
            intel = isChecked;
    }

    QString args2 = m_lineEdit->text().trimmed();

    const auto json = CompilerExplorerSvc::getCompilationOptions(text, args2, intel, demangle, labels, comments, libfuncs);
    const auto compiler = currentCompiler();
    const QString endpoint = QStringLiteral("compiler/") + compiler + QStringLiteral("/compile");

    CompilerExplorerSvc::instance()->compileRequest(endpoint, json.toJson());
}

void CEWidget::addExtraActionstoTextEditor()
{
    Q_ASSERT(m_textEditor);

    auto m = m_textEditor->defaultContextMenu();

    QMenu *menu = new QMenu(this);
    menu->addAction(i18n("Reveal linked code"), this, [this] {
        auto line = m_textEditor->cursorPosition().line();
        SourcePos p{QString(), line + 1, 0};
        AsmViewModel::CodeGenLines asmLines = m_model->asmForSourcePos(p);
        //         qDebug() << "Linked code for: " << line;
        if (!asmLines.empty()) {
            auto index = m_model->index(asmLines.front(), 0);
            m_asmView->scrollTo(index, QAbstractItemView::PositionAtCenter);

            // Highlight the linked lines
            Q_EMIT lineHovered(line);
            m_asmView->viewport()->update();
            //             qDebug() << "Scrolling to: " << index.data() << asmLines.size() << asmLines.front();
        }
    });
    menu->addActions(m->actions());
    m_textEditor->setContextMenu(menu);
}
