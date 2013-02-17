/* This file is part of the KDE libraries
   Copyright (C) 2010 Miquel Sabaté <mikisabate@gmail.com>

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


//BEGIN Includes
// Qt
#include <QtCore/QFile>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>

// KDE
#include <KStandardDirs>
#include <KLocale>

// Kate
#include "katescriptconsole.h"
#include "katetemplatescript.h"
//END Includes


//BEGIN KateScriptConsoleEngine
KateScriptConsoleEngine::KateScriptConsoleEngine(KateView * view)
    : m_view (view)
{
  m_utilsUrl = KGlobal::dirs()->findResource("data", "katepart/script/utils.js");
}

KateScriptConsoleEngine::~KateScriptConsoleEngine()
{
  /* There's nothing to do here */
}

const QString & KateScriptConsoleEngine::execute(const QString & text)
{
  static QString msg;
  msg = "";
  QString name = getFirstFunctionName(text, msg);
  if (name.isEmpty() && !msg.isEmpty()) // Error
    return msg;

  QFile file(m_utilsUrl);
  if (!file.open(QFile::ReadOnly)) {
    msg = i18n("Error: cannot open utils.js");
    return msg;
  }
  QString utilsCode = file.readAll();
  file.close();

  QString funcCode;
  if (name.isEmpty()) { // It's a command
    name = "foo";
    funcCode = utilsCode + "function foo() { " + text + " }";
  } else // It's a set of functions
    funcCode = utilsCode + text;
  KateTemplateScript script(funcCode);
  msg = script.invoke(m_view, name, "");
  if (msg.isEmpty())
    msg = i18n("Syntax Error: Parse error");
  return msg;
}

const QString KateScriptConsoleEngine::getFirstFunctionName(const QString & text, QString & msg)
{
  QString name = "";
  QRegExp reg("(function)");
  int i = reg.indexIn(text);
  if (i < 0) // there's no defined functions
    return "";
  i += 8; // "function"
  for (; text[i] != '('; ++i) {
    if (text[i] == ' ') // avoid blank spaces
      continue;
    if (text[i] == '{' || text[i] == '}' || text[i] == ')') { // bad ...
      msg = i18n("Error: There are bad defined functions");
      return "";
    }
    name.append(text[i]);
  }
  return name;
}
//END KateScriptConsoleEngine


//BEGIN KateScriptConsole
KateScriptConsole::KateScriptConsole(KateView * view, QWidget * parent)
    : KateViewBarWidget (true, parent)
    , m_view (view)
{
  Q_ASSERT(m_view != NULL);

  layout = new QVBoxLayout();
  centralWidget()->setLayout(layout);
  layout->setMargin(0);
  hLayout = new QHBoxLayout;
  m_result = new QLabel(this);
  m_edit = new QTextEdit(this);
  m_execute = new QPushButton(i18n("Execute"), this);
  m_execute->setIcon(KIcon("quickopen"));
  connect(m_execute, SIGNAL(clicked()), this, SLOT(executePressed()));

  layout->addWidget(m_edit);
  hLayout->addWidget(m_result);
  hLayout->addWidget(m_execute, 1, Qt::AlignRight);
  layout->addLayout(hLayout);

  m_engine = new KateScriptConsoleEngine(m_view);
}

KateScriptConsole::~KateScriptConsole()
{
  delete m_engine;
}

void KateScriptConsole::closed()
{
  if (viewBar())
    viewBar()->removeBarWidget(this);
  m_view->showViModeBar();
}

void KateScriptConsole::executePressed()
{
  QString text = m_edit->toPlainText();
  QString msg;
  if (!text.isEmpty()) {
    msg = m_engine->execute(text);
    m_result->setText("<b>" + msg + "</b>");
  } else
    m_result->setText("<b>" + i18n("There's no code to execute") + "</b>");
}
//END KateScriptConsole


#include "katescriptconsole.moc"


// kate: space-indent on; indent-width 2; replace-tabs on;

