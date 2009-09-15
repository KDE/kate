/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
/// Copyright (C) 2008 Christoph Cullmann <cullmann@kde.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License as published by the Free Software Foundation; either
/// version 2 of the License, or (at your option) version 3.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#include "katescriptview.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katehighlight.h"
#include "katescript.h"

KateScriptView::KateScriptView(QObject *parent)
  : QObject(parent), m_view(0)
{
}

void KateScriptView::setView (KateView *view)
{
  m_view = view;
}

KateView *KateScriptView::view()
{
  return m_view;
}

KTextEditor::Cursor KateScriptView::cursorPosition ()
{
  return m_view->cursorPosition();
}

void KateScriptView::setCursorPosition(int line, int column)
{
  KTextEditor::Cursor c(line, column);
  m_view->setCursorPosition(c);
}

void KateScriptView::setCursorPosition (const KTextEditor::Cursor& cursor)
{
  m_view->setCursorPosition(cursor);
}

KTextEditor::Cursor KateScriptView::virtualCursorPosition()
{
  return m_view->cursorPositionVirtual();
}

void KateScriptView::setVirtualCursorPosition(int line, int column)
{
  setVirtualCursorPosition(KTextEditor::Cursor(line, column));
}

void KateScriptView::setVirtualCursorPosition(const KTextEditor::Cursor& cursor)
{
  m_view->setCursorPositionVisual(cursor);
}

QString KateScriptView::selectedText()
{
  return m_view->selectionText();
}

bool KateScriptView::hasSelection()
{
  return m_view->selection();
}

KTextEditor::Range KateScriptView::selection()
{
  return m_view->selectionRange();
}

void KateScriptView::setSelection(const KTextEditor::Range& range)
{
  m_view->setSelection(range);
}

void KateScriptView::removeSelectedText()
{
  m_view->removeSelectedText();
}

void KateScriptView::selectAll()
{
  m_view->selectAll();
}

void KateScriptView::clearSelection()
{
  m_view->clearSelection();
}



// kate: space-indent on; indent-width 2; replace-tabs on;

