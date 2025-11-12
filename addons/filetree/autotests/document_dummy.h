/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2005 Dominik Haumann (dhaumann@kde.org) (documentation)

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KTextEditor/Document>
#include <QIcon>
#include <QPixmap>

class DummyDocument : public KTextEditor::Document
{
    Q_OBJECT

public:
    explicit DummyDocument(const QString &url = QString())
        : KTextEditor::Document(nullptr, KPluginMetaData(), nullptr)
        , m_name(QStringLiteral("foo"))
        , m_encoding()
        , m_views()
    {
        setUrl(url);
        m_name = url.section(QLatin1Char('/'), -1); // some nice default to mimic the KateDocument
    }
    explicit DummyDocument(const char *url)
        : DummyDocument(QString::fromLatin1(url))
    {
    }

    ~DummyDocument() override
    {
    }
    KTextEditor::View *createView(QWidget *, KTextEditor::MainWindow * = nullptr) override
    {
        return nullptr;
    }
    virtual KTextEditor::View *activeView() const
    {
        return nullptr;
    }
    QList<KTextEditor::View *> views() const override
    {
        return m_views;
    }

    QString documentName() const override
    {
        return m_name;
    }

    QString mimeType() override
    {
        return {};
    }
    QByteArray checksum() const override
    {
        return {};
    }
    bool setEncoding(const QString &) override
    {
        return false;
    }
    QString encoding() const override
    {
        return m_encoding;
    }

    bool documentReload() override
    {
        return true;
    }
    bool documentSave() override
    {
        return true;
    }
    bool documentSaveAs() override
    {
        return true;
    }

    bool isEditingTransactionRunning() const override
    {
        return false;
    }
    QString text() const override
    {
        return {};
    }
    QString text(KTextEditor::Range, bool = false) const override
    {
        return {};
    }
    QChar characterAt(KTextEditor::Cursor) const override
    {
        return {};
    }
    QString wordAt(KTextEditor::Cursor) const override
    {
        return {};
    }
    KTextEditor::Range wordRangeAt(KTextEditor::Cursor) const override
    {
        return KTextEditor::Range();
    }
    bool isValidTextPosition(KTextEditor::Cursor) const override
    {
        return true;
    }
    QStringList textLines(KTextEditor::Range, bool = false) const override
    {
        return {};
    }
    QString line(int) const override
    {
        return {};
    }
    int lines() const override
    {
        return 0;
    }
    KTextEditor::Cursor documentEnd() const override
    {
        return KTextEditor::Cursor();
    }
    qsizetype totalCharacters() const override
    {
        return 0;
    }
    int lineLength(int) const override
    {
        return 0;
    }

    bool setText(const QString &) override
    {
        return false;
    }
    bool setText(const QStringList &) override
    {
        return false;
    }
    bool clear() override
    {
        return true;
    }
    bool insertText(KTextEditor::Cursor, const QString &, bool = false) override
    {
        return false;
    }
    bool insertText(KTextEditor::Cursor, const QStringList &, bool = false) override
    {
        return false;
    }
    bool removeText(KTextEditor::Range, bool = false) override
    {
        return false;
    }
    bool insertLine(int, const QString &) override
    {
        return false;
    }
    bool insertLines(int, const QStringList &) override
    {
        return false;
    }
    bool removeLine(int) override
    {
        return false;
    }
    KSyntaxHighlighting::Theme::TextStyle defaultStyleAt(KTextEditor::Cursor) const override
    {
        return KSyntaxHighlighting::Theme::TextStyle::Normal;
    }
    QString mode() const override
    {
        return {};
    }
    QString highlightingMode() const override
    {
        return {};
    }
    QStringList modes() const override
    {
        return {};
    }
    QStringList highlightingModes() const override
    {
        return {};
    }
    bool setMode(const QString &) override
    {
        return false;
    }
    bool setHighlightingMode(const QString &) override
    {
        return false;
    }
    QString highlightingModeSection(int) const override
    {
        return {};
    }
    QString modeSection(int) const override
    {
        return {};
    }
    bool print() override
    {
        return false;
    }
    void printPreview() override
    {
    }
    QStringList embeddedHighlightingModes() const override
    {
        return {};
    }
    QString highlightingModeAt(KTextEditor::Cursor) override
    {
        return {};
    }
    bool isLineModified(int) const override
    {
        return false;
    }
    bool isLineSaved(int) const override
    {
        return false;
    }
    bool isLineTouched(int) const override
    {
        return false;
    }

    // KParts::ReadWritePart
    bool saveFile() override
    {
        return false;
    }

    KTextEditor::MovingCursor *newMovingCursor(KTextEditor::Cursor, KTextEditor::MovingCursor::InsertBehavior) override
    {
        return nullptr;
    }
    KTextEditor::MovingRange *newMovingRange(KTextEditor::Range, KTextEditor::MovingRange::InsertBehaviors, KTextEditor::MovingRange::EmptyBehavior) override
    {
        return nullptr;
    }
    qint64 revision() const override
    {
        return 0;
    };
    qint64 lastSavedRevision() const override
    {
        return 0;
    };
    void lockRevision(qint64) override
    {
    }
    void unlockRevision(qint64) override
    {
    }
    void transformCursor(KTextEditor::Cursor &, KTextEditor::MovingCursor::InsertBehavior, qint64, qint64 = -1) override
    {
    }
    void transformCursor(int &, int &, KTextEditor::MovingCursor::InsertBehavior, qint64, qint64 = -1) override
    {
    }
    void transformRange(KTextEditor::Range &, KTextEditor::MovingRange::InsertBehaviors, KTextEditor::MovingRange::EmptyBehavior, qint64, qint64 = -1) override
    {
    }
    QStringList configKeys() const override
    {
        return {};
    }
    QVariant configValue(const QString &) override
    {
        return {};
    }
    void setConfigValue(const QString &, const QVariant &) override
    {
    }
    void setModifiedOnDisk(ModifiedOnDiskReason) override
    {
    }
    void setModifiedOnDiskWarning(bool) override
    {
    }
    uint mark(int) override
    {
        return 0;
    }
    void setMark(int, uint) override
    {
    }
    void clearMark(int) override
    {
    }
    void addMark(int, uint) override
    {
    }
    void removeMark(int, uint) override
    {
    }
    const QHash<int, KTextEditor::Mark *> &marks() override
    {
        static const QHash<int, KTextEditor::Mark *> s;
        return s;
    }
    void clearMarks() override
    {
    }
    void setMarkDescription(MarkTypes, const QString &) override
    {
    }
    QString markDescription(MarkTypes) const override
    {
        return {};
    }
    void setEditableMarks(uint) override
    {
    }
    uint editableMarks() const override
    {
        return 0;
    }
    void setMarkIcon(MarkTypes, const QIcon &) override
    {
    }
    QIcon markIcon(MarkTypes) const override
    {
        return {};
    }
    void setAnnotationModel(KTextEditor::AnnotationModel *) override
    {
    }
    KTextEditor::AnnotationModel *annotationModel() const override
    {
        return nullptr;
    }

public:
    void setUrl(const QString &url)
    {
        KParts::ReadOnlyPart::setUrl(QUrl(url));
    }
    void setName(const QString &name)
    {
        m_name = name;
    }

    void readSessionConfig(const KConfigGroup &, const QSet<QString> & = QSet<QString>()) override
    {
    }
    void writeSessionConfig(KConfigGroup &, const QSet<QString> & = QSet<QString>()) override
    {
    }

    bool postMessage(KTextEditor::Message *) override
    {
        return false;
    }

    bool isDataRecoveryAvailable() const override
    {
        return false;
    }
    void recoverData() override
    {
    }
    void discardDataRecovery() override
    {
    }

    qsizetype cursorToOffset(KTextEditor::Cursor) const override
    {
        return 0;
    }
    KTextEditor::Cursor offsetToCursor(qsizetype) const override
    {
        return {};
    }

private:
    QString m_name, m_encoding;
    QList<KTextEditor::View *> m_views;
};
