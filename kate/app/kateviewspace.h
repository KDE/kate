/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef KATE_VIEWSPACE_H
#define KATE_VIEWSPACE_H

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/modificationinterface.h>

#include <QList>
#include <QFrame>

class KConfigBase;
class KateViewManager;
class KateViewSpace;
class QStackedWidget;

class KateViewSpace : public QFrame
{
    Q_OBJECT

  public:
    explicit KateViewSpace(KateViewManager *, QWidget* parent = 0, const char* name = 0);
    ~KateViewSpace();

    bool isActiveSpace();
    void setActive(bool b, bool showled = false);

    /**
     * Create new view for given document
     * @param doc document to create view for
     * @return new created view
     */
    KTextEditor::View *createView (KTextEditor::Document *doc);
    void removeView(KTextEditor::View* v);

    bool showView(KTextEditor::View *view)
    {
      return showView(view->document());
    }
    bool showView(KTextEditor::Document *document);

    KTextEditor::View* currentView();
    int viewCount() const
    {
      return mViewList.count();
    }

    void saveConfig (KConfigBase* config, int myIndex, const QString& viewConfGrp);
    void restoreConfig ( KateViewManager *viewMan, const KConfigBase* config, const QString &group );

  private Q_SLOTS:
    void statusBarToggled ();

  private:
    QStackedWidget* stack;
    bool mIsActiveSpace;
    /// This list is necessary to only save the order of the accessed views.
    /// The order is important. The least recently viewed view is always the
    /// last entry in the list, i.e. mViewList.last()
    /// mViewList.count() == stack.count() is always true!
    QList<KTextEditor::View*> mViewList;
    KateViewManager *m_viewManager;
    QString m_group;
};

#endif
