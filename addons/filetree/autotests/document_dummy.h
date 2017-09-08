/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef TEST_DOCUMENT_H
#define TEST_DOCUMENT_H

#include <KTextEditor/Document>
#include <KTextEditor/ModificationInterface>

class DummyDocument : public KTextEditor::Document
{
  Q_OBJECT

  public:
    DummyDocument (const QString &url = QString())
      : KTextEditor::Document(nullptr, nullptr)
      , m_name(QLatin1String("foo"))
      , m_encoding()
      , m_views()
    {
      setUrl(url);
      m_name = url.section(QLatin1Char('/'), -1); // some nice default to mimic the KateDocument
    }
    DummyDocument (const char *url) : DummyDocument(QString::fromLatin1(url)) {}

    virtual ~DummyDocument () {}
    KTextEditor::View *createView ( QWidget *, KTextEditor::MainWindow * = nullptr ) Q_DECL_OVERRIDE { return nullptr; }
    virtual KTextEditor::View *activeView() const { return nullptr; }
    QList<KTextEditor::View*> views() const Q_DECL_OVERRIDE { return m_views; }

    QString documentName() const Q_DECL_OVERRIDE { return m_name; }

    QString mimeType() Q_DECL_OVERRIDE { return QString(); }
    QByteArray checksum() const Q_DECL_OVERRIDE { return QByteArray(); }
    bool setEncoding (const QString &) Q_DECL_OVERRIDE { return false; }
    QString encoding () const Q_DECL_OVERRIDE { return m_encoding; }

    bool documentReload () Q_DECL_OVERRIDE { return true; }
    bool documentSave () Q_DECL_OVERRIDE { return true; }
    bool documentSaveAs () Q_DECL_OVERRIDE { return true; }

    bool isEditingTransactionRunning() const Q_DECL_OVERRIDE { return false; }
    QString text () const Q_DECL_OVERRIDE { return QString(); }
    QString text ( const KTextEditor::Range&, bool = false ) const Q_DECL_OVERRIDE { return QString(); }
    QChar characterAt( const KTextEditor::Cursor&) const Q_DECL_OVERRIDE { return QChar(); }
    QString wordAt(const KTextEditor::Cursor&) const Q_DECL_OVERRIDE { return QString(); }
    KTextEditor::Range wordRangeAt(const KTextEditor::Cursor&) const Q_DECL_OVERRIDE { return KTextEditor::Range(); }
    bool isValidTextPosition(const KTextEditor::Cursor&) const Q_DECL_OVERRIDE { return true; }
    QStringList textLines ( const KTextEditor::Range&, bool = false ) const Q_DECL_OVERRIDE { return QStringList(); }
    QString line ( int ) const Q_DECL_OVERRIDE { return QString(); }
    int lines () const Q_DECL_OVERRIDE { return 0; }
    KTextEditor::Cursor documentEnd() const Q_DECL_OVERRIDE { return KTextEditor::Cursor(); }
    int totalCharacters() const Q_DECL_OVERRIDE { return 0; }
    int lineLength ( int ) const Q_DECL_OVERRIDE { return 0; }

    bool setText ( const QString & ) Q_DECL_OVERRIDE { return false; }
    bool setText ( const QStringList & ) Q_DECL_OVERRIDE { return false; }
    bool clear () Q_DECL_OVERRIDE { return true; }
    bool insertText ( const KTextEditor::Cursor &, const QString &, bool = false ) Q_DECL_OVERRIDE { return false; }
    bool insertText ( const KTextEditor::Cursor &, const QStringList &, bool = false ) Q_DECL_OVERRIDE { return false; }
    bool removeText ( const KTextEditor::Range &, bool = false ) Q_DECL_OVERRIDE { return false; }
    bool insertLine ( int, const QString & ) Q_DECL_OVERRIDE { return false; }
    bool insertLines ( int, const QStringList & ) Q_DECL_OVERRIDE { return false; }
    bool removeLine ( int ) Q_DECL_OVERRIDE { return false; }
    KTextEditor::DefaultStyle defaultStyleAt(const KTextEditor::Cursor &) const Q_DECL_OVERRIDE { return KTextEditor::dsNormal; }
    QString mode() const Q_DECL_OVERRIDE { return QString(); }
    QString highlightingMode() const Q_DECL_OVERRIDE { return QString(); }
    QStringList modes() const Q_DECL_OVERRIDE { return QStringList(); }
    QStringList highlightingModes() const Q_DECL_OVERRIDE {return QStringList(); }
    bool setMode(const QString &) Q_DECL_OVERRIDE { return false; }
    bool setHighlightingMode(const QString &) Q_DECL_OVERRIDE { return false; }
    QString highlightingModeSection( int ) const Q_DECL_OVERRIDE { return QString(); }
    QString modeSection( int ) const Q_DECL_OVERRIDE { return QString(); }
    bool print() Q_DECL_OVERRIDE { return false; }
    void printPreview() Q_DECL_OVERRIDE {}
    QStringList embeddedHighlightingModes() const Q_DECL_OVERRIDE { return QStringList(); }
    QString highlightingModeAt(const KTextEditor::Cursor &) Q_DECL_OVERRIDE { return QString(); }
    bool isLineModified(int) const Q_DECL_OVERRIDE { return false; }
    bool isLineSaved(int) const Q_DECL_OVERRIDE { return false; }
    bool isLineTouched(int) const Q_DECL_OVERRIDE { return false; }

    // KParts::ReadWritePart
    bool saveFile() Q_DECL_OVERRIDE { return false; }

  // make QObject happy
  Q_SIGNALS:
    void modifiedOnDisk(KTextEditor::Document*, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason);

  public:
    void setUrl(const QString &url) { KParts::ReadOnlyPart::setUrl(QUrl(url)); }
    void setName(const QString &name) { m_name = name; }
    
    void readSessionConfig(const KConfigGroup &, const QSet<QString> & = QSet<QString>()) Q_DECL_OVERRIDE {}
    void writeSessionConfig(KConfigGroup &, const QSet<QString> & = QSet<QString>()) Q_DECL_OVERRIDE {}

    bool postMessage(KTextEditor::Message *) Q_DECL_OVERRIDE { return false; }

    bool isDataRecoveryAvailable() const Q_DECL_OVERRIDE { return false; }
    void recoverData() Q_DECL_OVERRIDE {}
    void discardDataRecovery() Q_DECL_OVERRIDE {}

  private:
    QString m_name, m_encoding;
    QList<KTextEditor::View*> m_views;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

