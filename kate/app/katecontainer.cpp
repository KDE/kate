/* This file is part of the KDE project
   Copyright (C) 2008 Joseph Wenninger <jowenn@kde.org>

   In addition to the following license there is an exception, that the KDE e.V.
   may decide on other forms of relicensing.

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

#include "katecontainer.h"
#include "kateapp.h"
#include "katemainwindow.h"

#include <kdebug.h>

KateContainer::KateContainer(KateApp* parent):QObject(parent),KTextEditor::ViewBarContainer(),m_app(parent) {

}

KateContainer::~KateContainer() {
}

QWidget* KateContainer::getViewBarParent(KTextEditor::View *view,KTextEditor::ViewBarContainer::Position position) {
  if (position!=KTextEditor::ViewBarContainer::BottomBar) return 0;
  KateMainWindow* mainWindow=qobject_cast<KateMainWindow*>(view->window());
  if (!mainWindow) {
    kDebug()<<"returning hardcoded 0, views window is not a KateMainWindow";
    return 0;
  }
  //Toplevel is a KateMainWindow
  kDebug()<<"window is a KateMainWindow";
  return mainWindow->horizontalViewBarContainer();
}

void KateContainer::addViewBarToLayout(KTextEditor::View *view,QWidget *bar, KTextEditor::ViewBarContainer::Position position) {
  if (position!=KTextEditor::ViewBarContainer::BottomBar) return;
  KateMainWindow* mainWindow=qobject_cast<KateMainWindow*>(view->window());
  if (!mainWindow) {
    kDebug()<<"main window is not a katemainwindow";
    return;
  }
  //Toplevel is a KateMainWindow
  kDebug()<<"window is a KateMainWindow";
  mainWindow->addToHorizontalViewBarContainer(view,bar);
}

void KateContainer::showViewBarForView(KTextEditor::View *view, KTextEditor::ViewBarContainer::Position position) {
  if (position!=KTextEditor::ViewBarContainer::BottomBar) return;
  KateMainWindow* mainWindow=qobject_cast<KateMainWindow*>(view->window());
  if (!mainWindow) {
    kDebug()<<"main window is not a katemainwindow";
    return;
  }
  mainWindow->showHorizontalViewBarForView(view);
}

void KateContainer::hideViewBarForView(KTextEditor::View *view, KTextEditor::ViewBarContainer::Position position) {
  if (position!=KTextEditor::ViewBarContainer::BottomBar) return;
  KateMainWindow* mainWindow=qobject_cast<KateMainWindow*>(view->window());
  if (!mainWindow) {
    kDebug()<<"main window is not a katemainwindow";
    return;
  }
  mainWindow->hideHorizontalViewBarForView(view);
}

void KateContainer::deleteViewBarForView(KTextEditor::View *view, KTextEditor::ViewBarContainer::Position position) {
  if (position!=KTextEditor::ViewBarContainer::BottomBar) return;
  KateMainWindow* mainWindow=qobject_cast<KateMainWindow*>(view->window());
  if (!mainWindow) {
    kDebug()<<"main window is not a katemainwindow";
    return;
  }
  mainWindow->deleteHorizontalViewBarForView(view);
}

#include "katecontainer.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
