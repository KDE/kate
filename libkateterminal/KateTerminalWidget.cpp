#include "KateTerminalWidget.h"

#include "qtermwidget.h"

#include <QActionGroup>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QProcess>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KStandardAction>
#include <KTextEditor/Editor>

enum Modifier {
#ifdef Q_OS_MACOS
    // Use plain Command key for shortcuts
    ACCEL = Qt::CTRL,
#else
    // Use Ctrl+Shift for shortcuts
    ACCEL = Qt::CTRL | Qt::SHIFT,
#endif
};

QString checkFile(const QStringList &dirList, const QString &filePath)
{
    for (const QString &root : dirList) {
        QFileInfo info(root, filePath);
        if (info.exists()) {
            return QDir::toNativeSeparators(info.filePath());
        }
    }
    return QString();
}

QString GetWindowPowerShell()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList dirList;
    dirList << env.value(QStringLiteral("windir"), QStringLiteral("C:\\Windows"));
    return checkFile(dirList, QStringLiteral("System32\\WindowsPowerShell\\v1.0\\powershell.exe"));
}

QString GetWindowsShell()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString windir = env.value(QStringLiteral("windir"), QStringLiteral("C:\\Windows"));
    QFileInfo info(windir, QStringLiteral("System32\\cmd.exe"));
    return QDir::toNativeSeparators(info.filePath());
}

QString GetWindowGitBash()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList dirList;
    dirList << env.value(QStringLiteral("ProgramW6432"), QStringLiteral("C:\\Program Files"));
    dirList << env.value(QStringLiteral("ProgramFiles"), QStringLiteral("C:\\Program Files"));
    return checkFile(dirList, QStringLiteral("Git\\bin\\bash.exe"));
}

struct TerminalConfig {
    QFont font;
    QString colorScheme;
};

class TerminalWidget : public QTermWidget
{
public:
    TerminalWidget(const QString &dir, const TerminalConfig &config, QWidget *parent = nullptr)
        : QTermWidget(parent)
    {
        setContentsMargins({});
        setBidiEnabled(false);
        setTerminalFont(config.font);
        setScrollBarPosition(QTermWidget::ScrollBarRight);
        if (!config.colorScheme.isEmpty()) {
            setColorScheme(m_colorScheme = config.colorScheme);
        }

#ifdef Q_OS_WIN
        {
            auto shell = GetWindowPowerShell();
            if (shell.isEmpty()) {
                shell = GetWindowGitBash();
                if (shell.isEmpty()) {
                    shell = GetWindowsShell();
                }
            }
            if (!shell.isEmpty()) {
                setShellProgram(shell);
                setEnvironment(QProcess::systemEnvironment());
                setWorkingDirectory(dir);
                startShellProgram();
            } else {
                qWarning() << "Unable to find shell! Terminal wont work";
            }
        }
#else
        setWorkingDirectory(dir);
        startShellProgram();
#endif

        m_copy = KStandardAction::copy(this, &QTermWidget::copyClipboard, this);
        m_copy->setShortcut(Modifier::ACCEL | Qt::Key_C);
        addAction(m_copy);

        m_paste = KStandardAction::paste(this, &QTermWidget::pasteClipboard, this);
        m_paste->setShortcut(Modifier::ACCEL | Qt::Key_V);
        addAction(m_paste);

        m_find = KStandardAction::find(this, &QTermWidget::toggleShowSearchBar, this);
        m_find->setShortcut(Modifier::ACCEL | Qt::Key_F);
        addAction(m_find);

        connect(this, &QTermWidget::copyAvailable, this, [this](bool s) {
            m_copy->setEnabled(s);
        });

        m_clear = KStandardAction::clear(this, &QTermWidget::clear, this);
        m_clear->setShortcut(Modifier::ACCEL | Qt::Key_K);
        addAction(m_clear);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &TerminalWidget::contextMenu);

        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, [this] {
            if (getTerminalFont() != KTextEditor::Editor::instance()->font()) {
                setTerminalFont(KTextEditor::Editor::instance()->font());
            }
        });
    }

    void contextMenu(QPoint pos)
    {
        QMenu m(this);
        if (m_copy->isEnabled()) {
            m.addAction(m_copy);
        }
        m.addAction(m_paste);
        m.addAction(m_find);
        m.addAction(m_clear);
        const auto actions = filterActions(pos);
        m.addActions(actions);

        auto schemeMenu = new QMenu(&m);
        schemeMenu->setTitle(i18n("Colorschemes"));

        const QStringList schemes = getAvailableColorSchemes();
        QActionGroup *g = new QActionGroup(schemeMenu);
        for (const auto &s : schemes) {
            auto a = schemeMenu->addAction(s);
            a->setActionGroup(g);
            a->setCheckable(true);
            if (a->text() == m_colorScheme) {
                a->setChecked(true);
            }
            connect(a, &QAction::toggled, this, [s, this] {
                saveColorscheme(s);
            });
        }
        m.addMenu(schemeMenu);

        m.exec(mapToGlobal(pos));
    }

    void saveColorscheme(const QString &scheme)
    {
        if (m_colorScheme != scheme) {
            KConfigGroup cg(KSharedConfig::openConfig(), "KateTerminal");
            cg.writeEntry("Colorscheme", scheme);
            setColorScheme(scheme);
        }
    }

private:
    QAction *m_copy = nullptr;
    QAction *m_paste = nullptr;
    QAction *m_find = nullptr;
    QAction *m_clear = nullptr;
    QString m_colorScheme;
};

static TerminalConfig getDefaultTerminalConfig(QWidget *)
{
    TerminalConfig c;
    KConfigGroup cg(KSharedConfig::openConfig(), "KateTerminal");
    c.colorScheme = cg.readEntry("Colorscheme", (QStringLiteral("WhiteOnBlack")));
    c.font = KTextEditor::Editor::instance()->font();
    return c;
}

static bool init = false;

KateTerminalWidget::KateTerminalWidget(QWidget *parent)
    : QTabWidget(parent)
{
    setContentsMargins({});
    setTabBarAutoHide(true);
    if (!init) {
        init = true;
        QTermWidget_initResources();
    }
    setDocumentMode(true);
    setTabsClosable(true);
    setTabPosition(QTabWidget::TabPosition::South);

    connect(this, &QTabWidget::tabCloseRequested, this, [this](int idx) {
        auto w = static_cast<TerminalWidget *>(widget(idx));
        if (!w) {
            return;
        }
        w->deleteLater();
        removeTab(idx);
    });
}

KateTerminalWidget::~KateTerminalWidget() = default;

void KateTerminalWidget::showShellInDir(const QString &dir)
{
    if (count() == 0) {
        newTab(dir);
    }
}

void KateTerminalWidget::sendInput(const QString &text)
{
    if (count() == 0) {
        return;
    }
    static_cast<TerminalWidget *>(currentWidget())->sendText(text);
}

void KateTerminalWidget::createSession(const QString &, const QString &workingDir)
{
    newTab(workingDir);
}

void KateTerminalWidget::newTab(const QString &workingDir)
{
    auto w = new TerminalWidget(workingDir, getDefaultTerminalConfig(this), this);
    addTab(w, i18n("Terminal %1", count() + 1));
    connect(w, &QTermWidget::finished, this, [this, w] {
        int idx = indexOf(w);
        if (idx >= 0) {
            removeTab(idx);
        }
    });
    connect(w, &QTermWidget::overrideShortcutCheck, this, &KateTerminalWidget::overrideShortcutCheck);
    setCurrentWidget(w);
}

QString KateTerminalWidget::foregroundProcessName() const
{
    // Not supported atm
    return {};
}

bool KateTerminalWidget::isAvailable()
{
    // CONPTY_MINIMAL_WINDOWS_VERSION 18309
    qint32 buildNumber = QSysInfo::kernelVersion().split(QLatin1String(".")).last().toInt();
    if (buildNumber < 18309)
        return false;
    return true;
}

QString KateTerminalWidget::currentWorkingDirectory() const
{
    if (count() == 0) {
        return QDir::currentPath();
    }
    return static_cast<TerminalWidget *>(currentWidget())->workingDirectory();
}

void KateTerminalWidget::tabRemoved(int)
{
    if (count() == 0) {
        deleteLater();
    }
}
