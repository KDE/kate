/*
    SPDX-FileCopyrightText: 2026 Gary Wang <opensource@blumia.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateshownonprintableplugin.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QAction>
#include <QFontMetricsF>
#include <QPainter>

ShowNonPrintableInlineNoteProvider::ShowNonPrintableInlineNoteProvider(KTextEditor::Document *doc)
    : m_doc(doc)
{
    const auto views = m_doc->views();
    for (auto view : views) {
        view->registerInlineNoteProvider(this);
    }

    connect(m_doc, &KTextEditor::Document::viewCreated, this, [this](KTextEditor::Document *, KTextEditor::View *view) {
        view->registerInlineNoteProvider(this);
    });

    auto lineChanged = [this](const int line) {
        if (m_startChangedLines == -1 || m_endChangedLines == -1) {
            m_startChangedLines = line;
        } else if (line == m_startChangedLines - 1) {
            m_startChangedLines = line;
        } else if (line < m_startChangedLines || line > m_endChangedLines) {
            updateNotes(m_startChangedLines, m_endChangedLines);
            m_startChangedLines = line;
            m_endChangedLines = -1;
        }
        m_endChangedLines = line >= m_endChangedLines ? line + 1 : m_endChangedLines;
    };

    connect(m_doc, &KTextEditor::Document::textInserted, this, [lineChanged](KTextEditor::Document *, const KTextEditor::Cursor &cur, const QString &) {
        lineChanged(cur.line());
    });
    connect(m_doc, &KTextEditor::Document::lineWrapped, this, [lineChanged](KTextEditor::Document *, KTextEditor::Cursor cur) {
        lineChanged(cur.line());
    });
    connect(m_doc, &KTextEditor::Document::lineUnwrapped, this, [lineChanged](KTextEditor::Document *, int line) {
        lineChanged(line);
    });
    connect(m_doc, &KTextEditor::Document::textRemoved, this, [lineChanged](KTextEditor::Document *, const KTextEditor::Range &range, const QString &) {
        lineChanged(range.start().line());
    });
    connect(m_doc, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *) {
        int newNumLines = m_doc->lines();
        if (m_startChangedLines == -1) {
            updateNotes();
        } else {
            if (m_previousNumLines != newNumLines) {
                m_endChangedLines = newNumLines > m_previousNumLines ? newNumLines : m_previousNumLines;
            }
            updateNotes(m_startChangedLines, m_endChangedLines);
        }

        m_startChangedLines = -1;
        m_endChangedLines = -1;
        m_previousNumLines = newNumLines;
    });

    updateNotes();
}

ShowNonPrintableInlineNoteProvider::~ShowNonPrintableInlineNoteProvider()
{
    if (m_doc) {
        const auto views = m_doc->views();
        for (auto view : views) {
            view->unregisterInlineNoteProvider(this);
        }
    }
}

void ShowNonPrintableInlineNoteProvider::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        m_notes.clear();
        Q_EMIT inlineNotesReset();
    }
}

void ShowNonPrintableInlineNoteProvider::updateNotes(int startLine, int endLine)
{
    if (m_notes.isEmpty()) {
        return;
    }

    startLine = startLine < -1 ? -1 : startLine;
    if (startLine == -1) {
        startLine = 0;
        const int lastLine = m_doc->lines();
        endLine = lastLine > m_previousNumLines ? lastLine : m_previousNumLines;
    }

    if (endLine == -1) {
        endLine = startLine;
    }

    for (int line = startLine; line <= endLine; ++line) {
        int removed = m_notes.remove(line);
        if (removed != 0) {
            Q_EMIT inlineNotesChanged(line);
        }
    }
}

QList<int> ShowNonPrintableInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_enabled) {
        return {};
    }

    auto it = m_notes.constFind(line);
    if (it != m_notes.cend()) {
        QList<int> columns;
        const auto &list = it.value();
        for (const auto &note : list) {
            columns.append(note.column);
        }
        return columns;
    }

    const QString lineText = m_doc->line(line);
    QList<NoteData> notesForLine;
    for (int i = 0; i < lineText.length(); ++i) {
        ushort unicode = lineText.at(i).unicode();
        // This condition is to filter characters that are not printable, it should
        // line up to nonPrintableSpacesRegExp from KateRenderer::paintTextLine() but
        // for now we just filter out the important ones. Following big switch is the
        // short name of these non-printable characters.
        if (unicode < 0x20 || unicode == 0x7F || unicode == 0x200B) {
            QString name;
            switch (unicode) {
            case 0x00:
                name = QStringLiteral("NUL");
                break;
            case 0x01:
                name = QStringLiteral("SOH");
                break;
            case 0x02:
                name = QStringLiteral("STX");
                break;
            case 0x03:
                name = QStringLiteral("ETX");
                break;
            case 0x04:
                name = QStringLiteral("EOT");
                break;
            case 0x05:
                name = QStringLiteral("ENQ");
                break;
            case 0x06:
                name = QStringLiteral("ACK");
                break;
            case 0x07:
                name = QStringLiteral("BEL");
                break;
            case 0x08:
                name = QStringLiteral("BS");
                break;
            case 0x09:
                continue; // Tab is visually handled by Kate itself usually, ignoring
            case 0x0A:
                name = QStringLiteral("LF");
                break;
            case 0x0B:
                name = QStringLiteral("VT");
                break;
            case 0x0C:
                name = QStringLiteral("FF");
                break;
            case 0x0D:
                name = QStringLiteral("CR");
                break;
            case 0x0E:
                name = QStringLiteral("SO");
                break;
            case 0x0F:
                name = QStringLiteral("SI");
                break;
            case 0x10:
                name = QStringLiteral("DLE");
                break;
            case 0x11:
                name = QStringLiteral("DC1");
                break;
            case 0x12:
                name = QStringLiteral("DC2");
                break;
            case 0x13:
                name = QStringLiteral("DC3");
                break;
            case 0x14:
                name = QStringLiteral("DC4");
                break;
            case 0x15:
                name = QStringLiteral("NAK");
                break;
            case 0x16:
                name = QStringLiteral("SYN");
                break;
            case 0x17:
                name = QStringLiteral("ETB");
                break;
            case 0x18:
                name = QStringLiteral("CAN");
                break;
            case 0x19:
                name = QStringLiteral("EM");
                break;
            case 0x1A:
                name = QStringLiteral("SUB");
                break;
            case 0x1B:
                name = QStringLiteral("ESC");
                break;
            case 0x1C:
                name = QStringLiteral("FS");
                break;
            case 0x1D:
                name = QStringLiteral("GS");
                break;
            case 0x1E:
                name = QStringLiteral("RS");
                break;
            case 0x1F:
                name = QStringLiteral("US");
                break;
            case 0x7F:
                name = QStringLiteral("DEL");
                break;
            case 0x200B:
                name = QStringLiteral("ZWSP");
                break;
            default:
                name = QStringLiteral("?");
                break;
            }
            notesForLine.append({i + 1, name}); // Display right after the character
        }
    }

    if (!notesForLine.isEmpty()) {
        m_notes.insert(line, notesForLine);
    }

    QList<int> columns;
    columns.reserve(notesForLine.size());
    for (const auto &note : std::as_const(notesForLine)) {
        columns.append(note.column);
    }
    return columns;
}

QSize ShowNonPrintableInlineNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    if (!m_enabled) {
        return QSize(0, 0);
    }

    const auto line = note.position().line();
    const auto column = note.position().column();

    auto it = m_notes.constFind(line);
    if (it == m_notes.cend()) {
        return QSize(0, 0);
    }

    QString text;
    const auto &list = it.value();
    for (const auto &n : list) {
        if (n.column == column) {
            text = n.text;
            break;
        }
    }

    if (text.isEmpty()) {
        return QSize(0, 0);
    }

    const QFontMetricsF fm(note.font());
    return QSize(fm.boundingRect(text).width() + 4, note.lineHeight());
}

void ShowNonPrintableInlineNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const
{
    if (!m_enabled) {
        return;
    }

    const auto line = note.position().line();
    const auto column = note.position().column();

    auto it = m_notes.constFind(line);
    if (it == m_notes.cend()) {
        return;
    }

    QString text;
    const auto &list = it.value();
    for (const auto &n : list) {
        if (n.column == column) {
            text = n.text;
            break;
        }
    }

    if (text.isEmpty()) {
        return;
    }

    painter.save();

    const QFontMetricsF fm(note.font());
    QRectF rect(0, 0, fm.boundingRect(text).width() + 4, note.lineHeight() - 2);
    rect.moveTop(1);

    QColor bgColor = painter.pen().color();
    bgColor.setAlphaF(0.1f);
    QColor borderColor = painter.pen().color();
    borderColor.setAlphaF(0.5f);
    QColor textColor = painter.pen().color();

    painter.setPen(borderColor);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect, 2, 2);

    painter.setPen(textColor);
    painter.drawText(rect, Qt::AlignCenter, text);

    painter.restore();
}

KateShowNonPrintablePluginView::KateShowNonPrintablePluginView(KateShowNonPrintablePlugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
{
    KXMLGUIClient::setComponentName(QStringLiteral("kateshownonprintable"), i18n("Show Non-Printable Characters"));
    setXMLFile(QStringLiteral("ui.rc"));

    m_toggleAction = new QAction(i18n("Toggle Show Non-Printable Characters"), this);
    m_toggleAction->setCheckable(true);
    m_toggleAction->setChecked(m_plugin->isEnabled());

    actionCollection()->addAction(QStringLiteral("toggle_show_non_printable"), m_toggleAction);

    connect(m_toggleAction, &QAction::toggled, m_plugin, &KateShowNonPrintablePlugin::setEnabled);
    connect(m_plugin, &KateShowNonPrintablePlugin::enabledChanged, m_toggleAction, &QAction::setChecked);

    m_mainWindow->guiFactory()->addClient(this);
}

KateShowNonPrintablePluginView::~KateShowNonPrintablePluginView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

K_PLUGIN_FACTORY_WITH_JSON(KateShowNonPrintablePluginFactory, "kateshownonprintableplugin.json", registerPlugin<KateShowNonPrintablePlugin>();)

KateShowNonPrintablePlugin::KateShowNonPrintablePlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

KateShowNonPrintablePlugin::~KateShowNonPrintablePlugin() = default;

QObject *KateShowNonPrintablePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    const auto views = mainWindow->views();
    for (auto view : views) {
        addDocument(view->document());
    }

    connect(mainWindow, &KTextEditor::MainWindow::viewCreated, this, [this](KTextEditor::View *view) {
        addDocument(view->document());
    });

    return new KateShowNonPrintablePluginView(this, mainWindow);
}

void KateShowNonPrintablePlugin::addDocument(KTextEditor::Document *doc)
{
    if (!m_inlineNoteProviders.contains(doc)) {
        auto provider = std::make_unique<ShowNonPrintableInlineNoteProvider>(doc);
        provider->setEnabled(m_enabled);
        m_inlineNoteProviders.emplace(doc, std::move(provider));
    }

    connect(doc, &KTextEditor::Document::aboutToClose, this, [this, doc]() {
        m_inlineNoteProviders.erase(doc);
    });
}

void KateShowNonPrintablePlugin::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        Q_EMIT enabledChanged(m_enabled);

        for (auto &pair : m_inlineNoteProviders) {
            pair.second->setEnabled(m_enabled);
        }
    }
}

#include "kateshownonprintableplugin.moc"
