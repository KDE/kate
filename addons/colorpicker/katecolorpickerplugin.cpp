/*
    SPDX-FileCopyrightText: 2018 Sven Brauch <mail@svenbrauch.de>
    SPDX-FileCopyrightText: 2018 Michal Srb <michalsrb@gmail.com>
    SPDX-FileCopyrightText: 2020 Jan Paul Batrina <jpmbatrina01@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katecolorpickerplugin.h"
#include "colorpickerconfigpage.h"

#include <algorithm>

#include <KTextEditor/Document>
#include <KTextEditor/InlineNoteProvider>
#include <KTextEditor/InlineNoteInterface>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QColor>
#include <QColorDialog>
#include <QFontMetricsF>
#include <QHash>
#include <QPainter>
#include <QRegularExpression>
#include <QVariant>

QRegularExpression ColorPickerInlineNoteProvider::s_colorRegEx = QRegularExpression();
bool ColorPickerInlineNoteProvider::s_putPreviewAfterColor = true;

ColorPickerInlineNoteProvider::ColorPickerInlineNoteProvider(KTextEditor::Document *doc)
    : m_doc(doc)
    , m_startChangedLines(-1)
    , m_previousNumLines(-1)
{
    // initialize the color regex
    updateColorMatchingCriteria();
    s_colorRegEx.setPatternOptions(QRegularExpression::DontCaptureOption);

    for (auto view : m_doc->views()) {
        qobject_cast<KTextEditor::InlineNoteInterface*>(view)->registerInlineNoteProvider(this);
    }

    connect(m_doc, &KTextEditor::Document::viewCreated, this, [this](KTextEditor::Document *,  KTextEditor::View *view) {
        qobject_cast<KTextEditor::InlineNoteInterface*>(view)->registerInlineNoteProvider(this);
    });

    // textInserted and textRemoved are emitted per line, then the last line is followed by a textChanged signal
    connect(m_doc, &KTextEditor::Document::textInserted, this, [this](KTextEditor::Document *, const KTextEditor::Cursor &cur, const QString &) {
        int line = cur.line();
        if (m_startChangedLines == -1 || m_startChangedLines > line)  {
            m_startChangedLines = line;
        }
    });
    connect(m_doc, &KTextEditor::Document::textRemoved, this, [this](KTextEditor::Document *, const KTextEditor::Range &range, const QString &) {
        int startLine = range.start().line();
        if (m_startChangedLines == -1 || m_startChangedLines > startLine) {
            m_startChangedLines = startLine;
        }
    });

    connect(m_doc, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *) {
        int newNumLines = m_doc->lines();
        if (m_startChangedLines == -1) {
            // textChanged not preceded by textInserted or textRemoved. This probably means that either:
            //   *empty line(s) were inserted/removed (TODO: Update only the lines directly below the removed/inserted empty line(s))
            //   *the document is newly opened so we update all lines
            updateNotes();
        } else {
            // if the change involves the insertion/deletion of lines, all lines after it get shifted, so we need to update all of them
            int endLine =  m_previousNumLines != newNumLines ? newNumLines-1 : -1;
            updateNotes(m_startChangedLines, endLine);
        }

        m_startChangedLines = -1;
        m_previousNumLines = newNumLines;
    });

    updateNotes();
}

ColorPickerInlineNoteProvider::~ColorPickerInlineNoteProvider()
{
    for (auto view : m_doc->views()) {
        qobject_cast<KTextEditor::InlineNoteInterface*>(view)->unregisterInlineNoteProvider(this);
    }
}

void ColorPickerInlineNoteProvider::updateColorMatchingCriteria()
{
    QString colorRegex;
    KConfigGroup config(KSharedConfig::openConfig(), "ColorPicker");

    QList <int> matchHexLengths = config.readEntry("HexLengths", QList<int>{12, 9, 8, 6, 3});
    // sort by decreasing number of digits to maximize matched hex
    std::sort(matchHexLengths.rbegin(), matchHexLengths.rend());
    if (matchHexLengths.size() > 0) {
        colorRegex = QLatin1String("#(%1)(?![[:xdigit:]])");
        QStringList hexRegex;
        for (const int hexLength : matchHexLengths) {
            hexRegex.append(QStringLiteral("[[:xdigit:]]{%1}").arg(hexLength));
        }
        colorRegex = colorRegex.arg(hexRegex.join(QLatin1String("|")));
    }

    if (config.readEntry("NamedColors", true)) {
        if (!colorRegex.isEmpty()) {
            colorRegex = QStringLiteral("(%1)|").arg(colorRegex);
        }

        QHash <int, QStringList> colorsByLength;
        int numColors = 0;
        for (const QString &color : QColor::colorNames()) {
            const int colorLength = color.length();
            if (!colorsByLength.contains(colorLength)) {
                colorsByLength.insert(colorLength, {});
            }
            colorsByLength[colorLength].append(color);
            ++numColors;
        }

        QList<int> colorLengths = colorsByLength.keys();
        // sort by descending length so that longer color names are prioritized
        std::sort(colorLengths.rbegin(), colorLengths.rend());
        QStringList colorNames;
        colorNames.reserve(numColors);
        for (const int length : colorLengths) {
            colorNames.append(colorsByLength[length]);
        }

        colorRegex.append(QStringLiteral("(?<![-\\w])(%1)(?![-\\w])").arg(colorNames.join(QLatin1String("|"))));
    }

    if (colorRegex.isEmpty()) {
        // No matching criteria enabled. Set to regex negative lookahead to match nothing.
        colorRegex = QLatin1String("(?!)");
    }

    s_colorRegEx.setPattern(colorRegex);
    s_putPreviewAfterColor = config.readEntry("PreviewAfterColor", true);
}

void ColorPickerInlineNoteProvider::updateNotes(int startLine, int endLine) {
    int maxLine = m_doc->lines() - 1;
    startLine = startLine < -1 ? -1 : startLine;
    endLine = endLine > maxLine ? maxLine  : endLine;

    if (startLine == -1) {
        startLine = 0;
        endLine = maxLine;
    }

    if (endLine == -1) {
        endLine = startLine;
    }

    for (int line = startLine; line <= endLine; ++line) {
        m_colorNoteIndices.remove(line);
        emit inlineNotesChanged(line);
    }

}

QVector<int> ColorPickerInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_colorNoteIndices.contains(line)) {
        m_colorNoteIndices.insert(line, {});

        auto matchIterator = s_colorRegEx.globalMatch(m_doc->line(line).toLower());
        while (matchIterator.hasNext()) {
            const auto match = matchIterator.next();
            int colorOtherIndex = match.capturedStart();
            int colorNoteIndex = colorOtherIndex + match.capturedLength();
            if (!s_putPreviewAfterColor) {
                colorOtherIndex = colorNoteIndex;
                colorNoteIndex = match.capturedStart();
            }

            m_colorNoteIndices[line][colorNoteIndex] = colorOtherIndex;
        }
    }

    return m_colorNoteIndices.value(line).keys().toVector();
}

QSize ColorPickerInlineNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    return QSize(note.lineHeight(), note.lineHeight());
}

void ColorPickerInlineNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter& painter) const
{
    const auto line = note.position().line();
    auto colorEnd = note.position().column();
    auto colorStart = m_colorNoteIndices[line][colorEnd];
    if (colorStart > colorEnd) {
        colorEnd = colorStart;
        colorStart = note.position().column();
    }

    const auto color = QColor(m_doc->text({line, colorStart, line, colorEnd}));
    auto penColor = color;
    penColor.setAlpha(255);
    // ensure that the border color is always visible
    painter.setPen( (penColor.value() < 128 ? penColor.lighter(150) : penColor.darker(150)) );
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
    auto colorStart = m_colorNoteIndices[line][colorEnd];
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
    const auto newColor = QColorDialog::getColor(oldColor, const_cast<KTextEditor::View*>(note.view()), title, dialogOptions);
    if (!newColor.isValid()) {
        return;
    }

    // include alpha channel if the new color has transparency or the old color included transparency (#AARRGGBB, 9 hex digits)
    auto colorNameFormat = (newColor.alpha() != 255 || colorEnd-colorStart == 9) ? QColor::HexArgb : QColor::HexRgb;
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

void KateColorPickerPlugin::addDocument(KTextEditor::Document *doc) {
    if (!m_inlineColorNoteProviders.contains(doc)) {
        m_inlineColorNoteProviders.insert(doc, new ColorPickerInlineNoteProvider(doc));
    }

    connect(doc, &KTextEditor::Document::destroyed, this, [this, doc]() {
        m_inlineColorNoteProviders.remove(doc);
    });
}

void KateColorPickerPlugin::readConfig() {
    ColorPickerInlineNoteProvider::updateColorMatchingCriteria();
    for (auto colorNoteProvider : m_inlineColorNoteProviders.values()) {
        colorNoteProvider->updateNotes();
    }
}

#include "katecolorpickerplugin.moc"
