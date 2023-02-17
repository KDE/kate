/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
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
#include <KTextEditor/Application>
#include <KTextEditor/Editor>

#include <KColorScheme>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

// BEGIN code to fetch shells on windows
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
// END

class ChangeShellDialog : public QDialog
{
    enum { Custom = 0, Default };

public:
    ChangeShellDialog(QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(i18n("Select Shell..."));

        QVBoxLayout *l = new QVBoxLayout(this);

        auto label = new QLabel(i18n("Select shell:"));
        l->addWidget(label);

        QStringList shells{i18n("Default"), i18n("Custom - Choose a shell yourself by clicking 'Browse' button")};
        shells.append(getAvailableShells());
        m_cmb = new QComboBox(this);
        m_cmb->addItems(shells);
        m_cmb->setItemData(0, Default);
        m_cmb->setItemData(1, Custom);
        connect(m_cmb, &QComboBox::currentTextChanged, this, &ChangeShellDialog::onCurrentIndexChanged);
        l->addWidget(m_cmb);

        label = new QLabel(i18n("Shell command:"));
        l->addWidget(label);

        QHBoxLayout *hl = new QHBoxLayout();
        l->addLayout(hl);
        m_le = new QLineEdit;
        m_le->setReadOnly(true);
        m_le->setPlaceholderText(i18n("Absolute path to shell and arguments e.g., /usr/bin/bash"));
        hl->addWidget(m_le);
        m_browse = new QPushButton(i18n("Browse..."));
        m_browse->setEnabled(false);
        connect(m_browse, &QPushButton::clicked, this, &ChangeShellDialog::browseClicked);
        hl->addWidget(m_browse);

        m_errorLabel = new QLabel();
        m_errorLabel->setVisible(false);
        auto p = m_errorLabel->palette();
        p.setBrush(QPalette::WindowText, KColorScheme().foreground(KColorScheme::NegativeText));
        m_errorLabel->setPalette(p);
        l->addWidget(m_errorLabel);

        m_btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        l->addStretch();
        l->addWidget(m_btnBox);
        connect(m_btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QTimer *changeTimer = new QTimer(this);
        changeTimer->callOnTimeout(this, &ChangeShellDialog::onLineEditTextChange);
        changeTimer->setInterval(500);
        changeTimer->setSingleShot(true);

        connect(m_le, &QLineEdit::textChanged, changeTimer, [changeTimer] {
            changeTimer->start();
        });

        resize(600, 200);
    }

    QString shell() const
    {
        return m_le->text();
    }

private:
    static QStringList getAvailableShells()
    {
        QStringList shells;
#ifdef Q_OS_UNIX
        QFile f(QStringLiteral("/etc/shells"));
        if (f.open(QFile::ReadOnly)) {
            const QByteArrayList lines = f.readAll().split('\n');
            for (const auto &line : lines) {
                const auto trimmed = line.trimmed();
                if (trimmed.startsWith("#") || trimmed.isEmpty()) {
                    continue;
                }
                shells << QString::fromUtf8(trimmed);
            }
        }
#endif
#ifdef Q_OS_WIN
        shells = QStringList{GetWindowPowerShell(), GetWindowGitBash(), GetWindowsShell()};
#endif
        return shells;
    }

    void browseClicked()
    {
        const QString n = QFileDialog::getOpenFileName(this, i18n("Select Shell Program"));
        m_le->setText(n);
    }

    void onCurrentIndexChanged()
    {
        if (m_cmb->currentData().isValid() && m_cmb->currentData().toInt() == Default) {
            m_le->clear();
            m_browse->setEnabled(false);
        } else if (m_cmb->currentData().isValid() && m_cmb->currentData().toInt() == Custom) {
            m_browse->setEnabled(true);
            m_le->clear();
        } else {
            m_browse->setEnabled(false);
            m_le->setText(m_cmb->currentText());
        }
    }

    void onLineEditTextChange()
    {
        const auto splitted = m_le->text().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (splitted.isEmpty()) {
            m_btnBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            return;
        }
        const auto n = splitted.first();
        QFileInfo fi(m_le->text());
        if (fi.exists() && !fi.isFile()) {
            m_btnBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            m_errorLabel->setVisible(false);
        } else if (!fi.exists()) {
            m_errorLabel->setText(i18n("The specified path doesn't exist. Please recheck"));
            m_errorLabel->setVisible(true);
            m_btnBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else if (!fi.isExecutable()) {
            m_errorLabel->setText(i18n("The specified command '%1' doesn't seem to be an executable", n));
            m_errorLabel->setVisible(true);
            m_btnBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else {
            m_errorLabel->setText({});
            m_errorLabel->setVisible(false);
            m_btnBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    QDialogButtonBox *m_btnBox;
    QPushButton *m_browse;
    QLabel *m_errorLabel;
    QComboBox *m_cmb;
    QLineEdit *m_le;
};

enum Modifier {
#ifdef Q_OS_MACOS
    // Use plain Command key for shortcuts
    ACCEL = Qt::CTRL,
#else
    // Use Ctrl+Shift for shortcuts
    ACCEL = Qt::CTRL | Qt::SHIFT,
#endif
};

struct TerminalConfig {
    QFont font;
    QString colorScheme;
    QString shell;
};

class TerminalWidget : public QTermWidget
{
public:
    TerminalWidget(const QString &dir, const TerminalConfig &config, QWidget *parent = nullptr)
        : QTermWidget(0, parent)
    {
        setContentsMargins({});
        setBidiEnabled(false);
        setTerminalFont(config.font);
        setScrollBarPosition(QTermWidget::ScrollBarRight);
        if (!config.colorScheme.isEmpty()) {
            setColorScheme(m_colorScheme = config.colorScheme);
        }

        QString shell = config.shell;

#ifdef Q_OS_WIN
        {
            if (shell.isEmpty()) {
                shell = GetWindowPowerShell();
                if (shell.isEmpty()) {
                    shell = GetWindowGitBash();
                    if (shell.isEmpty()) {
                        shell = GetWindowsShell();
                    }
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
        if (!shell.isEmpty()) {
            setShellProgram(shell);
        }

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

        connect(this, &QTermWidget::urlActivated, this, [](const QUrl &url, bool) {
            if (url.isLocalFile()) {
                KTextEditor::Editor::instance()->application()->openUrl(url);
            } else {
                QDesktopServices::openUrl(url);
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

        m.addAction(i18nc("@menu:item clicking the item shows a dialog allowing the user to change the shell program", "Change shell..."),
                    this,
                    &TerminalWidget::changeShell);

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

    void saveShell(const QString &shell)
    {
        KConfigGroup cg(KSharedConfig::openConfig(), "KateTerminal");
        cg.writeEntry("ShellProgram", shell);
    }

    void changeShell()
    {
        ChangeShellDialog d(this);
        int r = d.exec();
        if (r == QDialog::Accepted) {
            saveShell(d.shell());
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
    c.shell = cg.readEntry("ShellProgram", QString());
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
