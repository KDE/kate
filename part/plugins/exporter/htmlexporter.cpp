/**
 * This file is part of the KDE libraries
 * Copyright (C) 2009 Milian Wolff <mail@milianw.de>
 * Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
 * Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
 * Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 * Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "htmlexporter.h"

#include <ktexteditor/document.h>

#include <QtGui/QTextDocument>

HTMLExporter::HTMLExporter(KTextEditor::View* view, QTextStream& output, const bool encapsulate)
  : AbstractExporter(view, output, encapsulate)
{
  if ( m_encapsulate ) {
    // let's write the HTML header :
    m_output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    m_output << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
    m_output << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
    m_output << "<head>" << endl;
    m_output << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
    m_output << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
    // for the title, we write the name of the file (/usr/local/emmanuel/myfile.cpp -> myfile.cpp)
    m_output << "<title>" << view->document()->documentName() << "</title>" << endl;
    m_output << "</head>" << endl;
    m_output << "<body>" << endl;
  }

  if ( !m_defaultAttribute ) {
    m_output << "<pre>" << endl;
  } else {
    m_output << QString("<pre style='%1%2%3%4'>")
                  .arg(m_defaultAttribute->fontBold() ? "font-weight:bold;" : "")
                  .arg(m_defaultAttribute->fontItalic() ? "text-style:italic;" : "")
                  .arg("color:" + m_defaultAttribute->foreground().color().name() + ';')
                  .arg("background-color:" + m_defaultAttribute->background().color().name() + ';')
             << endl;
  }
}

HTMLExporter::~HTMLExporter()
{
  m_output << "</pre>" << endl;

  if ( m_encapsulate ) {
    m_output << "</body>" << endl;
    m_output << "</html>" << endl;
  }
}

void HTMLExporter::openLine()
{
}

void HTMLExporter::closeLine(const bool lastLine)
{
  if ( !lastLine ) {
    //we are inside a <pre>, so a \n is a new line
    m_output << "\n";
  }
}

void HTMLExporter::exportText(const QString& text, const KTextEditor::Attribute::Ptr& attrib)
{
  if ( !attrib || !attrib->hasAnyProperty() || attrib == m_defaultAttribute ) {
    m_output << Qt::escape(text);
    return;
  }

  if ( attrib->fontBold() ) {
    m_output << "<b>";
  }
  if ( attrib->fontItalic() ) {
    m_output << "<i>";
  }

  bool writeForeground = attrib->hasProperty(QTextCharFormat::ForegroundBrush)
    && (!m_defaultAttribute || attrib->foreground().color() != m_defaultAttribute->foreground().color());
  bool writeBackground = attrib->hasProperty(QTextCharFormat::BackgroundBrush)
    && (!m_defaultAttribute || attrib->background().color() != m_defaultAttribute->background().color());

  if ( writeForeground || writeBackground ) {
    m_output << QString("<span style='%1%2'>")
                  .arg(writeForeground ? "color:" + attrib->foreground().color().name() + ';' : "")
                  .arg(writeBackground ? "background:" + attrib->background().color().name() + ';' : "");
  }

  m_output << Qt::escape(text);

  if ( writeBackground || writeForeground ) {
    m_output << "</span>";
  }
  if ( attrib->fontItalic() ) {
    m_output << "</i>";
  }
  if ( attrib->fontBold() ) {
    m_output << "</b>";
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
