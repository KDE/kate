/* This file is part of the KDE project
   Copyright (C) 2006 Joseph Wenninger (jowenn@kde.org)

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

#ifndef KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACEADAPTOR_H
#define KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACEADAPTOR_H

#include <QtDBus/QtDBus>

#include "highlightinginterface.h"
#include "document.h"

namespace KTextEditor
{

/// For documentation see HighlightingInterface
class HighlightingInterfaceAdaptor: public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface","org.kde.KTextEditor.Highlighting")
  Q_PROPERTY(QString highlighting READ highlighting WRITE setHighlighting)
  Q_PROPERTY(QStringList highlightings READ highlightings)

  public:
    HighlightingInterfaceAdaptor (QObject *obj, HighlightingInterface *iface)
      : QDBusAbstractAdaptor(obj), m_iface(iface)
    {
      connect(obj,SIGNAL(highlightingChanged(KTextEditor::Document *)),this,SIGNAL(highlightingChanged(KTextEditor::Document *)));
    }

    ~HighlightingInterfaceAdaptor () {}
    
  /*
   * Access to the highlighting subsystem
   */
  public Q_SLOTS:
    /**
     * Return the name of the currently used highlighting
     * \return name of the used highlighting
     * 
     */
    QString highlighting() const { return m_iface->highlighting (); }
    
    /**
     * Set the current highlighting of the document by giving it's name
     * \param name name of the highlighting to use for this document
     * \return \e true on success, otherwise \e false
     */
    bool setHighlighting(const QString &name) { return m_iface->setHighlighting (name); }
    
    /**
     * Return a list of the names of all possible highlighting
     * \return list of highlighting names
     */
    QStringList highlightings() const { return m_iface->highlightings (); }

  /*
   * Important signals which are emitted from the highlighting system
   */
  Q_SIGNALS:
    /**
     * Warn anyone listening that the current document's highlighting has
     * changed.
     * 
     * \param document the document which's highlighting has changed
     */
    void highlightingChanged(KTextEditor::Document *document);
    
  private:
    HighlightingInterface *m_iface;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
