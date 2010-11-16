/* This file is part of the KDE project
   Copyright (C) 2010 Miquel Sabaté <mikisabate@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#ifndef KATE_SCRIPT_CONSOLE_H
#define KATE_SCRIPT_CONSOLE_H


#include "kateviewhelpers.h"

class QTextEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;


/**
 * Manage JavaScript, allowing the user directly type commands as in
 * KateCommnadLineBar environment. It also allows the user to define
 * his own JavaScript functions and to redefine each one of them.
 */
class KateScriptConsoleEngine
{
  public:
    /** Constructor + Destructor */
    KateScriptConsoleEngine(KateView * view);
    virtual ~KateScriptConsoleEngine();

    /**
     * Execute a command or a set of functions
     * @param text text containing a command or a set of functions.
     * @return a printable message about the execution.
     */
    const QString & execute(const QString & text);

  private:
    /** Url of utils.js file */
    QString m_utilsUrl;

    /** Just a Kate view :) */
    KateView * m_view;

  private:
    /**
     * Get the name of the first defined function
     * @param text text containing a command or a set of functions.
     * @param msg an error string
     * @return the name of the first function defined or a void QString if
     * there's no functions.
     */
    const QString getFirstFunctionName(const QString & text, QString & msg);
};


class KateScriptConsole : public KateViewBarWidget
{
  Q_OBJECT

  public:
    KateScriptConsole(KateView * view, QWidget * parent = NULL);
    virtual ~KateScriptConsole();

    void setupLayout();

  protected:
    // overriden
    virtual void closed();
    virtual void switched();

  private:
    QVBoxLayout * layout;
    QHBoxLayout * hLayout;
    QTextEdit * m_edit;
    QPushButton * m_execute;
    QLabel * m_result;
    QSize initialSize, endSize;
    KateView * m_view;
    KateScriptConsoleEngine * m_engine;

  public slots:
    void executePressed();
};


#endif /* KATE_SCRIPT_CONSOLE_H */


// kate: space-indent on; indent-width 2; replace-tabs on;

