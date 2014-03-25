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

    virtual ~DummyDocument () {};
    virtual KTextEditor::View *createView ( QWidget *, KTextEditor::MainWindow * = nullptr ) { return 0; }
    virtual KTextEditor::View *activeView() const { return 0; }
    virtual QList<KTextEditor::View*> views() const { return m_views; }

    virtual QString documentName() const { return m_name; }

    virtual QString mimeType() { return QString(); }
    virtual QByteArray checksum() const { return QByteArray(); }
    virtual bool setEncoding (const QString &) { return false; }
    virtual QString encoding () const { return m_encoding; }

    virtual bool documentReload () { return true; }
    virtual bool documentSave () { return true; }
    virtual bool documentSaveAs () { return true; }

    virtual QString text () const { return QString(); }
    virtual QString text ( const KTextEditor::Range&, bool = false ) const { return QString(); }
    virtual QChar characterAt( const KTextEditor::Cursor&) const { return QChar(); }
    virtual QString wordAt(const KTextEditor::Cursor&) const { return QString(); }
    virtual KTextEditor::Range wordRangeAt(const KTextEditor::Cursor&) const { return KTextEditor::Range(); }
    virtual bool isValidTextPosition(const KTextEditor::Cursor&) const { return true; }
    virtual QStringList textLines ( const KTextEditor::Range&, bool = false ) const { return QStringList(); }
    virtual QString line ( int ) const { return QString(); }
    virtual int lines () const { return 0; }
    virtual KTextEditor::Cursor documentEnd() const { return KTextEditor::Cursor(); }
    virtual int totalCharacters() const { return 0; }
    virtual int lineLength ( int ) const { return 0; }

    virtual bool setText ( const QString & ) { return false; }
    virtual bool setText ( const QStringList & ) { return false; }
    virtual bool clear () { return true; }
    virtual bool insertText ( const KTextEditor::Cursor &, const QString &, bool = false ) { return false; }
    virtual bool insertText ( const KTextEditor::Cursor &, const QStringList &, bool = false ) { return false; }
    virtual bool removeText ( const KTextEditor::Range &, bool = false ) { return false; }
    virtual bool insertLine ( int, const QString & ) { return false; }
    virtual bool insertLines ( int, const QStringList & ) { return false; }
    virtual bool removeLine ( int ) { return false; }
    virtual KTextEditor::DefaultStyle defaultStyleAt(const KTextEditor::Cursor &) const { return KTextEditor::dsNormal; }
    virtual QString mode() const { return QString(); }
    virtual QString highlightingMode() const { return QString(); }
    virtual QStringList modes() const { return QStringList(); }
    virtual QStringList highlightingModes() const {return QStringList(); }
    virtual bool setMode(const QString &) { return false; }
    virtual bool setHighlightingMode(const QString &) { return false; }
    virtual QString highlightingModeSection( int ) const { return QString(); }
    virtual QString modeSection( int ) const { return QString(); }
    virtual bool print() { return false; }
    virtual void printPreview() {}
    virtual QStringList embeddedHighlightingModes() const { return QStringList(); }
    virtual QString highlightingModeAt(const KTextEditor::Cursor &) { return QString(); }
    virtual bool isLineModified(int) const { return false; }
    virtual bool isLineSaved(int) const { return false; }
    virtual bool isLineTouched(int) const { return false; }

    // KParts::ReadWritePart
    virtual bool saveFile() { return false; }

  // make QObject happy
  Q_SIGNALS:
    void modifiedOnDisk(KTextEditor::Document*, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason);

  public:
    void setUrl(const QString &url) { KParts::ReadOnlyPart::setUrl(QUrl(url)); }
    void setName(const QString &name) { m_name = name; }
    
    void readSessionConfig(const KConfigGroup &, const QSet<QString> & = QSet<QString>()) {}
    void writeSessionConfig(KConfigGroup &, const QSet<QString> & = QSet<QString>()) {}

    bool postMessage(KTextEditor::Message *) { return false; }

    bool isDataRecoveryAvailable() const { return false; }
    void recoverData() {}
    void discardDataRecovery() {}

  private:
    QString m_name, m_encoding;
    QList<KTextEditor::View*> m_views;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

