/****************************************************************************
 **    - This file is part of the Artistic Comment KTextEditor-Plugin -    **
 **  - Copyright 2009-2010 Jonathan Schmidt-Dominé <devel@the-user.org> -  **
 **                                  ----                                  **
 **   - This program is free software; you can redistribute it and/or -    **
 **    - modify it under the terms of the GNU Library General Public -     **
 **    - License version 3, or (at your option) any later version, as -    **
 ** - published by the Free Software Foundation.                         - **
 **                                  ----                                  **
 **  - This library is distributed in the hope that it will be useful, -   **
 **   - but WITHOUT ANY WARRANTY; without even the implied warranty of -   **
 ** - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU -  **
 **          - Library General Public License for more details. -          **
 **                                  ----                                  **
 ** - You should have received a copy of the GNU Library General Public -  **
 ** - License along with the kdelibs library; see the file COPYING.LIB. -  **
 **  - If not, write to the Free Software Foundation, Inc., 51 Franklin -  **
 **      - Street, Fifth Floor, Boston, MA 02110-1301, USA. or see -       **
 **                  - <http://www.gnu.org/licenses/>. -                   **
 ***************************************************************************/

#ifndef ACOMMENTVIEW_H
#define ACOMMENTVIEW_H

#include <QObject>
#include <KXMLGUIClient>

class QAction;
class KMenu;
class KDialog;

#include "ui_styledialog.h"

class ACommentView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit ACommentView(KTextEditor::View *view = 0);
    ~ACommentView();
private slots:
    void insertAComment(QAction *action);
    void disableOptions(int type);
    void loadStyle(QString name);
    void changeEntry();
private:
    KTextEditor::View *m_view;
    KMenu *m_menu;
    KDialog *m_dialog;
    Ui::StyleDialog m_ui;
};

#endif
