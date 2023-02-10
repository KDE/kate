#include "KateTerminalWidget.h"

#include "qtermwidget.h"

#include <KTextEditor/Editor>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QProcess>

#include <KLocalizedString>
#include <KStandardAction>

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
        setBidiEnabled(false);
        setTerminalFont(config.font);
        if (!config.colorScheme.isEmpty()) {
            setColorScheme(config.colorScheme);
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
                setShellProgram(GetWindowsShell());
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

        m_clear = KStandardAction::clear(
            this,
            [this] {
                clear();
                sendText(QStringLiteral("\n"));
            },
            this);
        m_clear->setShortcut(Modifier::ACCEL | Qt::Key_K);
        addAction(m_clear);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &TerminalWidget::contextMenu);
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
        m.exec(mapToGlobal(pos));
    }

private:
    QAction *m_copy = nullptr;
    QAction *m_paste = nullptr;
    QAction *m_find = nullptr;
    QAction *m_clear = nullptr;
};

static TerminalConfig getDefaultTerminalConfig(QWidget *w)
{
    TerminalConfig c;
    int lightness = w->palette().base().color().lightness();
    const bool darkMode = lightness < 127;
    if (darkMode) {
        c.colorScheme = (QStringLiteral("Breeze"));
    } else {
        c.colorScheme = (QStringLiteral("BlackOnWhite"));
    }
    c.font = KTextEditor::Editor::instance()->font();
    return c;
}

static bool init = false;

KateTerminalWidget::KateTerminalWidget(QWidget *parent)
    : QTabWidget(parent)
{
    setTabBarAutoHide(true);
    if (!init) {
        init = true;
        QTermWidget_initResources();
    }
    setTabsClosable(true);

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
}

QString KateTerminalWidget::foregroundProcessName() const
{
    // Not supported atm
    return {};
}
