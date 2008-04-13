/* This file is part of the KDE libraries
   Copyright 2008 Andreas Pakulat <apaku@gmx.de>

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

#ifndef ANNOTATIONMODELTEST_H
#define ANNOTATIONMODELTEST_H

#include <ktexteditor/annotationinterface.h>
namespace KTextEditor
{
class View;
}
class QMenu;

class AnnotationModelTest : public KTextEditor::AnnotationModel
{
  Q_OBJECT
public:
    AnnotationModelTest();
    ~AnnotationModelTest();

    QVariant data( int, Qt::ItemDataRole ) const;
public slots:
    void annotationActivated( KTextEditor::View* view, int line );
    void annotationContextMenuAboutToShow( KTextEditor::View* view, QMenu* menu, int line );
};

#endif
