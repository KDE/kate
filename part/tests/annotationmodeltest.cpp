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

#define KDE_DEFAULT_DEBUG_AREA 13050

#include "annotationmodeltest.h"
#include <QtCore/QVariant>
#include <QtGui/QBrush>
#include <QtGui/QMenu>
#include <QtGui/QAction>

#include <ktexteditor/view.h>

AnnotationModelTest::AnnotationModelTest()
{
}

AnnotationModelTest::~AnnotationModelTest()
{
}

QVariant AnnotationModelTest::data( int line, Qt::ItemDataRole role ) const
{
    switch( role )
    {
        case Qt::DisplayRole:
            if( line < 3 )
                return QString("Foobar");
            else if( line < 7 )
                return QString("Barfoo");
            else if( line < 10 )
                return QString("Foobar");
            else
                return QString("Barfoo");
        case Qt::ForegroundRole:
            if( line < 3 )
                return qVariantFromValue( QBrush( QColor( Qt::black ) ) );
            else if( line < 7 )
                return qVariantFromValue( QBrush( QColor( Qt::black ) ) );
            else if( line < 10 )
                return qVariantFromValue( QBrush( QColor( Qt::black ) ) );
            else
                return qVariantFromValue( QBrush( QColor( Qt::black ) ) );
        case Qt::BackgroundRole:
            if( line < 3 )
                return qVariantFromValue( QBrush( QColor(238, 255, 238) ) );
            else if( line < 7 )
                return qVariantFromValue( QBrush( QColor(255, 255, 205) ) );
            else if( line < 10 )
                return qVariantFromValue( QBrush( QColor(255, 238, 238) ) );
            else
                return qVariantFromValue( QBrush( QColor(238, 255, 238) ) );
        case Qt::ToolTipRole:
            if( line < 3 )
                return QString("My fine <b>Tooltip</b>");
            else if( line < 7 )
                return QString("My other fine <i>ToolTip</i>");
            else if( line < 10 )
                return QString("My fine <b>Tooltip</b>");
            else
                return QString("My other fine <i>ToolTip</i>");
        default:
            return QVariant();
    }
}

void AnnotationModelTest::annotationActivated( KTextEditor::View* view, int line )
{
    kDebug() << "Hey annotation was activated:" << line << view;
}

void AnnotationModelTest::annotationContextMenuAboutToShow( KTextEditor::View * view, QMenu * menu, int line )
{
    QAction* a = new QAction("Show diff from revision", menu);
    menu->addAction(a);
}
