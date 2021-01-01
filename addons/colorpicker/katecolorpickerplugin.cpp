/*
    SPDX-FileCopyrightText: 2018 Sven Brauch <mail@svenbrauch.de>
    SPDX-FileCopyrightText: 2018 Michal Srb <michalsrb@gmail.com>
    SPDX-FileCopyrightText: 2020 Jan Paul Batrina <jpmbatrina01@gmail.com>
    SPDX-FileCopyrightText: 2021 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katecolorpickerplugin.h"
#include "colorpickerconfigpage.h"

#include <algorithm>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Document>
#include <KTextEditor/InlineNoteInterface>
#include <KTextEditor/InlineNoteProvider>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QColor>
#include <QColorDialog>
#include <QFontMetricsF>
#include <QHash>
#include <QPainter>
#include <QVariant>

QVector<QString> ColorPickerInlineNoteProvider::s_namedColors = QColor::colorNames().toVector();

ColorPickerInlineNoteProvider::ColorPickerInlineNoteProvider(KTextEditor::Document *doc)
    : m_doc(doc)
{
    // initialize the color regex
    updateColorMatchingCriteria();

    for (auto view : m_doc->views()) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->registerInlineNoteProvider(this);
    }

    connect(m_doc, &KTextEditor::Document::viewCreated, this, [this](KTextEditor::Document *, KTextEditor::View *view) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->registerInlineNoteProvider(this);
    });

    auto lineChanged = [this](const int line) {
        if (m_startChangedLines == -1 || m_endChangedLines == -1) {
            m_startChangedLines = line;
        // changed line is directly above/below the previous changed line, so we just update them
        } else if (line == m_endChangedLines) { // handled below. Condition added here to avoid fallthrough
        } else if (line == m_startChangedLines-1) {
            m_startChangedLines = line;
        } else if (line < m_startChangedLines || line > m_endChangedLines) {
            // changed line is outside the range of previous changes. Change proably skipped lines
            updateNotes(m_startChangedLines, m_endChangedLines);
            m_startChangedLines = line;
            m_endChangedLines = -1;
        }

        m_endChangedLines = line >= m_endChangedLines ? line + 1 : m_endChangedLines;
    };

    // textInserted and textRemoved are emitted per line, then the last line is followed by a textChanged signal
    connect(m_doc, &KTextEditor::Document::textInserted, this, [lineChanged](KTextEditor::Document *, const KTextEditor::Cursor &cur, const QString &) {
        lineChanged(cur.line());
    });
    connect(m_doc, &KTextEditor::Document::textRemoved, this, [lineChanged](KTextEditor::Document *, const KTextEditor::Range &range, const QString &) {
        lineChanged(range.start().line());
    });
    connect(m_doc, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *) {
        int newNumLines = m_doc->lines();
        if (m_startChangedLines == -1) {
            // textChanged not preceded by textInserted or textRemoved. This probably means that either:
            //   *empty line(s) were inserted/removed (TODO: Update only the lines directly below the removed/inserted empty line(s))
            //   *the document is newly opened so we update all lines
            updateNotes();
        } else {
            if (m_previousNumLines != newNumLines) {
                // either whole line(s) were removed or inserted. We update all lines (even those that are now non-existent) below m_startChangedLines
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

ColorPickerInlineNoteProvider::~ColorPickerInlineNoteProvider()
{
    for (auto view : m_doc->views()) {
        qobject_cast<KTextEditor::InlineNoteInterface *>(view)->unregisterInlineNoteProvider(this);
    }
}

void ColorPickerInlineNoteProvider::updateColorMatchingCriteria()
{
    KConfigGroup config(KSharedConfig::openConfig(), "ColorPicker");
    m_matchHexLengths = config.readEntry("HexLengths", QList<int>{12, 9, 8, 6, 3}).toVector();
    m_putPreviewAfterColor = config.readEntry("PreviewAfterColor", true);
    m_matchNamedColors = config.readEntry("NamedColors", false);
}

void ColorPickerInlineNoteProvider::updateNotes(int startLine, int endLine) {
    startLine = startLine < -1 ? -1 : startLine;
    if (startLine == -1) {
        startLine = 0;
        //  we use whichever of newNumLines and m_previousNumLines are longer so that note indices for non-existent lines are also removed
        const int lastLine = m_doc->lines();
        endLine = lastLine > m_previousNumLines ? lastLine : m_previousNumLines;
    }

    if (endLine == -1) {
        endLine = startLine;
    }

    for (int line = startLine; line < endLine; ++line) {
        m_colorNoteIndices.remove(line);
        emit inlineNotesChanged(line);
    }
}

ColorRange ColorPickerInlineNoteProvider::findNamedColor(const QStringView lineText, int start) const
{
    // only return when a valid named color is found, a # is found, or we have reached the end of the line
    while (start < lineText.length()) {
        for ( ; start < lineText.length(); ++start) {
            const QChar chr = lineText.at(start);
            if (chr.isLetter()) {
                break;
            } else if (chr == QLatin1Char('#')) {
                // return current position so that hex colors can be matched
                return {-1, start};
            }
        }

        int end;
        for (end = start+1; end < lineText.length(); ++end) {
            if (!lineText.at(end).isLetter()) {
                break;
            }
        }

        if (end <= start+1) {
            ++start;
            continue;
        }

        const auto color = lineText.mid(start, end-start);
        const auto matchIter = std::lower_bound(s_namedColors.cbegin(), s_namedColors.cend(), color, [](const QStringView other, const QStringView color) {
            return other.compare(color, Qt::CaseInsensitive) < 0;
        });

        if (matchIter == s_namedColors.cend() || color.compare(*matchIter, Qt::CaseInsensitive) != 0) {
            start = end;
            continue;
        }

        return {start, end};
    }

    return {-1, start};
}

ColorRange ColorPickerInlineNoteProvider::findHexColor(const QStringView lineText, int start) const
{
    if (m_matchHexLengths.size() == 0) {
        return {-1, -1};
    }

    // find the first occurrence of #. For simplicity, we assume that named colors from <start> to the first # or space have already been handled
    for ( ; start < lineText.length(); ++start) {
        const QChar chr = lineText.at(start);
        if (chr == QLatin1Char('#')) {
            break;
        } else if (chr.isSpace()) {
            // return the next position since a named color might start from there
            return {-1, start+1};
        }
    }

    int end;
    for (end = start+1; end < lineText.length(); ++end) {
        QChar chr = lineText.at(end).toLower();
        if (! (chr.isDigit() || (chr >= QLatin1Char('a') && chr <= QLatin1Char('f'))) ) {
            break;
        }
    }

    if (!m_matchHexLengths.contains(end-start-1)) {
        return {-1, end};
    }

    return {start, end};
}

QVector<int> ColorPickerInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_colorNoteIndices.contains(line)) {
        m_colorNoteIndices.insert(line, {});

        const QString lineText = m_doc->line(line);
        for (int lineIndex = 0; lineIndex < lineText.length(); ++lineIndex) {
            ColorRange color;
            bool isNamedColor = true;
            if (m_matchNamedColors) {
                color = findNamedColor(lineText, lineIndex);
            }

            if (color.start == -1) {
                color = findHexColor(lineText, color.end == -1 ? lineIndex : color.end);
                isNamedColor = false;
            }

            if (color.start == -1) {
                if (color.end != -1) {
                    lineIndex = color.end - 1;
                }
                continue;
            }

            // check if the color found is surrounded by letters, numbers, or -
            if (color.start != 0) {
                const QChar chr = lineText.at(color.start-1);
                if (chr == QLatin1Char('-') || (isNamedColor && chr.isLetterOrNumber())) {
                    lineIndex = color.end - 1;
                    continue;
                }
            }
            if (color.end < lineText.length()) {
                const QChar chr = lineText.at(color.end);
                if (chr == QLatin1Char('-') || chr.isLetterOrNumber()) {
                    lineIndex = color.end - 1;
                    continue;
                }
            }

            const int end = color.end;
            if (m_putPreviewAfterColor) {
                color.end = color.start;
                color.start = end;
            }

            m_colorNoteIndices[line].colorNoteIndices.append(color.start);
            m_colorNoteIndices[line].otherColorIndices.append(color.end);
            lineIndex = end - 1;
        }
    }

    return m_colorNoteIndices[line].colorNoteIndices;
}

QSize ColorPickerInlineNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    return QSize(note.lineHeight()-1, note.lineHeight()-1);
}

void ColorPickerInlineNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter) const
{
    const auto line = note.position().line();
    auto colorEnd = note.position().column();

    const QVector<int> &colorNoteIndices = m_colorNoteIndices[line].colorNoteIndices;
    // Since the colorNoteIndices are inserted in left-to-right (increasing) order in inlineNotes(), we can use binary search to find the index (or color note number) for the line
    const int colorNoteNumber = std::lower_bound(colorNoteIndices.cbegin(), colorNoteIndices.cend(), colorEnd) - colorNoteIndices.cbegin();
    auto colorStart = m_colorNoteIndices[line].otherColorIndices[colorNoteNumber];

    if (colorStart > colorEnd) {
        colorEnd = colorStart;
        colorStart = note.position().column();
    }

    const auto color = QColor(m_doc->text({line, colorStart, line, colorEnd}));
    // ensure that the border color is always visible
    QColor penColor = color;
    penColor.setAlpha(255);
    painter.setPen(penColor.value() < 128 ? penColor.lighter(150) : penColor.darker(150));

    painter.setBrush(color);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const QFontMetricsF fm(note.font());
    const int inc = note.underMouse() ? 1 : 0;
    const int ascent = fm.ascent();
    const int margin = (note.lineHeight() - ascent) / 2;
    painter.drawRect(margin - inc, margin - inc, ascent - 1 + 2 * inc, ascent - 1 + 2 * inc);
}

void ColorPickerInlineNoteProvider::inlineNoteActivated(const KTextEditor::InlineNote &note, Qt::MouseButtons, const QPoint &)
{
    const auto line = note.position().line();
    auto colorEnd = note.position().column();

    const QVector<int> &colorNoteIndices = m_colorNoteIndices[line].colorNoteIndices;
    // Since the colorNoteIndices are inserted in left-to-right (increasing) order in inlineNotes, we can use binary search to find the index (or color note number) for the line
    const int colorNoteNumber = std::lower_bound(colorNoteIndices.cbegin(), colorNoteIndices.cend(), colorEnd) - colorNoteIndices.cbegin();
    auto colorStart = m_colorNoteIndices[line].otherColorIndices[colorNoteNumber];
    if (colorStart > colorEnd) {
        colorEnd = colorStart;
        colorStart = note.position().column();
    }

    const auto oldColor = QColor(m_doc->text({line, colorStart, line, colorEnd}));
    QColorDialog::ColorDialogOptions dialogOptions = QColorDialog::ShowAlphaChannel;
    QString title = i18n("Select Color (Hex output)");
    if (!m_doc->isReadWrite()) {
        dialogOptions |= QColorDialog::NoButtons;
        title = i18n("View Color [Read only]");
    }
    const QColor newColor = QColorDialog::getColor(oldColor, const_cast<KTextEditor::View*>(note.view()), title, dialogOptions);
    if (!newColor.isValid()) {
        return;
    }

    // include alpha channel if the new color has transparency or the old color included transparency (#AARRGGBB, 9 hex digits)
    auto colorNameFormat = (newColor.alpha() != 255 || colorEnd - colorStart == 9) ? QColor::HexArgb : QColor::HexRgb;
    m_doc->replaceText({line, colorStart, line, colorEnd}, newColor.name(colorNameFormat));
}

K_PLUGIN_FACTORY_WITH_JSON(KateColorPickerPluginFactory, "katecolorpickerplugin.json", registerPlugin<KateColorPickerPlugin>();)

KateColorPickerPlugin::KateColorPickerPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

KateColorPickerPlugin::~KateColorPickerPlugin()
{
    qDeleteAll(m_inlineColorNoteProviders);
}

QObject *KateColorPickerPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
    for (auto view : m_mainWindow->views()) {
        addDocument(view->document());
    }

    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, [this](KTextEditor::View *view) {
        addDocument(view->document());
    });

    return nullptr;
}

void KateColorPickerPlugin::addDocument(KTextEditor::Document *doc)
{
    if (!m_inlineColorNoteProviders.contains(doc)) {
        m_inlineColorNoteProviders.insert(doc, new ColorPickerInlineNoteProvider(doc));
    }

    connect(doc, &KTextEditor::Document::destroyed, this, [this, doc]() {
        m_inlineColorNoteProviders.remove(doc);
    });
}

void KateColorPickerPlugin::readConfig() {
    for (auto colorNoteProvider : m_inlineColorNoteProviders.values()) {
        colorNoteProvider->updateColorMatchingCriteria();
        colorNoteProvider->updateNotes();
    }
}

#include "katecolorpickerplugin.moc"
