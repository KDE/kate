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

#ifndef ACOMMENTPLUGIN_H
#define ACOMMENTPLUGIN_H

#include <KTextEditor/Plugin>

namespace KTextEditor
{
class View;
}

#include "artisticcomment.h"

#include <QMap>

#include <KSharedConfig>

class ACommentView;

class ACommentPlugin
        : public KTextEditor::Plugin
{
public:
    /// Constructor
    explicit ACommentPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    /// Destructor
    virtual ~ACommentPlugin();

    void addView(KTextEditor::View *view);
    void removeView(KTextEditor::View *view);

private:
    QList<class ACommentView*> m_views;
};

#endif
