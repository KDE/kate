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

#ifndef _KATE_CONTAINER_
#define _KATE_CONTAINER_

#include <KTextEditor/ContainerInterface>

class KateApp;
class QWidget;
namespace KTextEditor {
  class View;
}

class KateContainer: public QObject, public KTextEditor::ViewBarContainer {
  Q_OBJECT
  Q_INTERFACES( KTextEditor::ViewBarContainer )
  public:
    KateContainer(KateApp* parent);
    virtual ~KateContainer();
  
  public:
    //KTextEditor::HorizontalViewBarContainer
    virtual QWidget* getViewBarParent(KTextEditor::View *view,KTextEditor::ViewBarContainer::Position position);
    virtual void addViewBarToLayout(KTextEditor::View *view,QWidget *bar,KTextEditor::ViewBarContainer::Position position);
    virtual void showViewBarForView(KTextEditor::View *view,KTextEditor::ViewBarContainer::Position position);
    virtual void hideViewBarForView(KTextEditor::View *view,KTextEditor::ViewBarContainer::Position position);
    virtual void deleteViewBarForView(KTextEditor::View *view,KTextEditor::ViewBarContainer::Position position);
  private:
    KateApp *m_app;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
