/* 
Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INSANE_HTML_PLUGIN_H
#define INSANE_HTML_PLUGIN_H

#include <ktexteditor/plugin.h>
#include <kxmlguiclient.h>
#include <ktexteditor/view.h>
#include <QMap>

class InsaneHTMLPluginLEView: public QObject, public KXMLGUIClient {
  Q_OBJECT
public:
  InsaneHTMLPluginLEView(QObject *parent, KTextEditor::View *view);
  virtual ~InsaneHTMLPluginLEView();
public Q_SLOTS:
  void expand();
private:
  KTextEditor::View *m_view;
  QStringList m_emptyTags;
  QMultiMap<QString,QString> m_defaultAttributes;
  int find_region_start(int cursor_x, const QString& line, int * filtercount);
  int find_region_end(int cursor_x, const QString& line, int * filtercount);
  QStringList parse(const QString& input, int offset, int *newOffset=0);
  QString parseIdentifier(const QString& input, int *offset,bool firstDigit=false);
  int parseNumber(const QString& input, int *offset);
  void apply_filter_e(QStringList *lines);
  void apply_filter_c(QStringList *lines);
};

class InsaneHTMLPluginLE: public KTextEditor::Plugin {
  Q_OBJECT
public:
  InsaneHTMLPluginLE(QObject *parent, const QList<QVariant> data=QList<QVariant>());
  virtual ~InsaneHTMLPluginLE(){}
  virtual void addView (KTextEditor::View *view);
  virtual void removeView (KTextEditor::View *view);
private:
  QMap<KTextEditor::View*,InsaneHTMLPluginLEView*> m_map;
};
#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
