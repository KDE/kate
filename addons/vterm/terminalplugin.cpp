#include "terminalplugin.h"

#include "qtermwidget/qtermwidget.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KTextEditor/Editor>

#include <QAction>
#include <QDir>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QProcess>
#include <QVBoxLayout>
#include <QtGlobal>

#include "KateTerminalWidget.h"

K_PLUGIN_FACTORY_WITH_JSON(TerminalPluginFactory, "terminalplugin.json", registerPlugin<TerminalPlugin>();)

TerminalPlugin::TerminalPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

QObject *TerminalPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new TerminalPluginView(this, mainWindow);
}

TerminalPluginView::TerminalPluginView(TerminalPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(plugin)
    , m_mainWindow(mainwindow)
{
    const auto icon = QIcon::fromTheme(QStringLiteral("dialog-scripts"));
    m_toolview.reset(m_mainWindow->createToolView(plugin, QStringLiteral("vtermwidget"), KTextEditor::MainWindow::Bottom, icon, i18n("Terminal")));

    auto w = new KateTerminalWidget(m_toolview.get());
    auto a = new QAction(QStringLiteral("New Tab"), this);
    connect(a, &QAction::triggered, this, [w] {
        w->createSession({}, {});
    });
    a->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    m_termWidget = w;
    w->addAction(a);

    m_termWidget->installEventFilter(this);

    connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &TerminalPluginView::handleEsc);
}

TerminalPluginView::~TerminalPluginView() = default;

void TerminalPluginView::handleEsc(QEvent *event)
{
    auto keyEvent = dynamic_cast<QKeyEvent *>(event);
    if (keyEvent && keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
        m_mainWindow->hideToolView(m_toolview.get());
    }
}

bool TerminalPluginView::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_termWidget && e->type() == QEvent::Show) {
        m_termWidget->showShellInDir({});
    }

    if (e->type() == QEvent::KeyRelease || e->type() == QEvent::KeyPress) {
        handleEsc(e);
    }

    return QObject::eventFilter(o, e);
}

#include "terminalplugin.moc"
