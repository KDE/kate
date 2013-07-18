/* This file is part of the KDE libraries
   Copyright (C) 2010 Sebastian Sauer <mail@dipe.org>
   Copyright (C) 2012 Frederik Gladhorn <gladhorn@kde.org>

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

#ifndef _KATE_VIEW_ACCESSIBLE_
#define _KATE_VIEW_ACCESSIBLE_
#ifndef QT_NO_ACCESSIBILITY
#if QT_VERSION >= 0x040800

#include "kateviewinternal.h"
#include "katetextcursor.h"
#include "katedocument.h"

#include <QtGui/QAccessible>
#include <QtGui/qaccessible2.h>
#include <klocale.h>
#include <qaccessiblewidget.h>



QString Q_GUI_EXPORT qTextBeforeOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);
QString Q_GUI_EXPORT qTextAtOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);
QString Q_GUI_EXPORT qTextAfterOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);


/**
 * This class implements a QAccessible-interface for a Kate::TextCursor. An
 * instance of \a KateViewAccessible will hold an instance of this class to
 * provide access to the cursor within a kateview.
 *
 * To test the cursor positioning using kmagnifier from kdeaccessibility you
 * can do for example;
 * @code
 * export QT_ACCESSIBILITY=1
 * kmag &
 * kwrite &
 * @endcode
 * then press F2 in kmag to switch to the "Follow Focus Mode" and see that
 * the view follows the cursor in kwrite.
 */
class KateCursorAccessible : public QAccessibleInterface
{
    public:

        enum { ChildId = 1 };

        explicit KateCursorAccessible(KateViewInternal *view)
            : QAccessibleInterface()
            , m_view(view)
        {
        }

        virtual ~KateCursorAccessible()
        {
        }

        virtual QString actionText(int action, QAccessible::Text t, int) const
        {
            if (t == QAccessible::Name) {
                switch(action) {
                    case 0: return i18n("Move To...");
                    case 1: return i18n("Move Left");
                    case 2: return i18n("Move Right");
                    case 3: return i18n("Move Up");
                    case 4: return i18n("Move Down");
                    default: break;
                }
            }
            return QString();
        }

        virtual bool doAction(int action, int, const QVariantList & params = QVariantList() )
        {
            bool ok = true;
            KTextEditor::Cursor c = m_view->getCursor();
            switch(action) {
                case 0: {
                    if (params.count() < 2) ok = false;
                    const int line = ok ? params[0].toInt(&ok) : 0;
                    const int column = ok ? params[1].toInt(&ok) : 0;
                    if (ok) c.setPosition(line, column);
                } break;
                case 1: c.setPosition(c.line(), c.column() - 1); break;
                case 2: c.setPosition(c.line(), c.column() + 1); break;
                case 3: c.setPosition(c.line() - 1, c.column()); break;
                case 4: c.setPosition(c.line() + 1, c.column()); break;
                default: ok = false; break;
            }

            return ok;
        }

        virtual int userActionCount(int) const
        {
            return 5;
        }

        virtual int childAt(int, int) const
        {
            return 0;
        }

        virtual int childCount() const
        {
            return 0;
        }

        virtual int indexOfChild(const QAccessibleInterface *) const
        {
            return 0;
        }

        virtual bool isValid() const
        {
            KTextEditor::Cursor c = m_view->getCursor();
            return c.isValid();
        }

        virtual int navigate(QAccessible::RelationFlag relation, int entry, QAccessibleInterface **target) const
        {
            Q_UNUSED(relation);
            Q_UNUSED(entry);
            *target = 0;
            return -1;
        }

        virtual QObject* object() const
        {
            return m_view;
        }

        virtual QRect rect(int) const
        {
            // return the exact position of the cursor with no width and height defined,
            QPoint p = m_view->view()->cursorPositionCoordinates();
            return QRect(m_view->mapToGlobal(p), QSize(0,0));
        }

        virtual QAccessible::Relation relationTo(int child, const QAccessibleInterface *other, int otherChild) const
        {
            Q_UNUSED(child);
            Q_UNUSED(other);
            Q_UNUSED(otherChild);
            return QAccessible::Unrelated;
        }

        virtual QAccessible::Role role(int) const
        {
            return QAccessible::Cursor;
        }

        virtual void setText(QAccessible::Text, int, const QString &)
        {
        }

        virtual QAccessible::State state(int) const
        {
            QAccessible::State s = QAccessible::Focusable | QAccessible::Focused;
            return s;
        }

        virtual QString text(QAccessible::Text, int) const
        {
            return QString();
        }

    private:
        KateViewInternal *m_view;
};

/**
 * This class implements a QAccessible-interface for a KateViewInternal.
 *
 * This is the root class for the kateview. The \a KateCursorAccessible class
 * represents the cursor in the kateview and is a child of this class.
 */
class KateViewAccessible : public QAccessibleWidgetEx, public QAccessibleTextInterface, public QAccessibleSimpleEditableTextInterface
{
        Q_ACCESSIBLE_OBJECT

    public:
        explicit KateViewAccessible(KateViewInternal *view)
            : QAccessibleWidgetEx(view), QAccessibleSimpleEditableTextInterface(this)
            , m_cursor(new KateCursorAccessible(view))
        {
        }

        virtual ~KateViewAccessible()
        {
        }

        virtual QString actionText(int action, QAccessible::Text t, int child) const
        {
            if (child == KateCursorAccessible::ChildId)
                return m_cursor->actionText(action, t, 0);
            return QString();
        }

        virtual bool doAction(int action, int child, const QVariantList & params = QVariantList() )
        {
            if (child == KateCursorAccessible::ChildId)
                return m_cursor->doAction(action, 0, params);
            return false;
        }

        virtual int userActionCount(int child) const
        {
            if (child == KateCursorAccessible::ChildId)
                return m_cursor->userActionCount(0);
            return 0;
        }

        virtual int childAt(int x, int y) const
        {
            Q_UNUSED(x);
            Q_UNUSED(y);
            return 0;
        }

        virtual int childCount() const
        {
            return 1;
        }

        virtual int indexOfChild(const QAccessibleInterface *child) const
        {
            return dynamic_cast<const KateCursorAccessible*>(child) ? KateCursorAccessible::ChildId : 0;
        }

        virtual int navigate(QAccessible::RelationFlag relation, int entry, QAccessibleInterface **target) const
        {
            if ((relation == QAccessible::Child || QAccessible::FocusChild) && entry == KateCursorAccessible::ChildId) {
                *target = new KateCursorAccessible(view());
                return KateCursorAccessible::ChildId;
            }
            *target = 0;
            return -1;
        }

        virtual QRect rect(int child) const
        {
            if (child == KateCursorAccessible::ChildId)
                return m_cursor->rect(0);
            return QRect(view()->mapToGlobal(QPoint(0,0)), view()->size());
        }

        virtual QAccessible::Relation relationTo(int child, const QAccessibleInterface *other, int otherChild) const
        {
            Q_UNUSED(child);
            Q_UNUSED(other);
            Q_UNUSED(otherChild);
            return QAccessible::Unrelated;
        }

        virtual QAccessible::Role role(int child) const
        {
            if (child == KateCursorAccessible::ChildId)
                return QAccessible::Cursor;
            return QAccessible::EditableText;
        }

        virtual void setText(QAccessible::Text t, int child, const QString & text)
        {
            if (t == QAccessible::Value && child == 0 && view()->view()->document())
                view()->view()->document()->setText(text);
        }

        virtual QAccessible::State state(int child) const
        {
            if (child == KateCursorAccessible::ChildId)
                return m_cursor->state(0);

            QAccessible::State s = QAccessibleWidgetEx::state(0);
            s |= QAccessible::Focusable;

            if (view()->hasFocus())
                s |= QAccessible::Focused;

            s |= QAccessible::HasInvokeExtension;
            return s;
        }

        virtual QString text(QAccessible::Text t, int child) const
        {
            if (child == KateCursorAccessible::ChildId)
                return m_cursor->text(t, 0);
            QString s;
            if (view()->view()->document()) {
                if (t == QAccessible::Name)
                    s = view()->view()->document()->documentName();
                if (t == QAccessible::Value)
                    s = view()->view()->document()->text();
            }
            return s;
        }

        virtual int characterCount()
        {
            return view()->view()->document()->text().size();
        }

        virtual void addSelection(int startOffset, int endOffset)
        {
            KTextEditor::Range range;
            range.setRange(cursorFromInt(startOffset), cursorFromInt(endOffset));
            view()->view()->setSelection(range);
            view()->view()->setCursorPosition(cursorFromInt(endOffset));
        }

        virtual QString attributes(int offset, int* startOffset, int* endOffset)
        {
            Q_UNUSED(offset);
            *startOffset = 0;
            *endOffset = characterCount();
            return QString();
        }
        virtual QRect characterRect(int offset, QAccessible2::CoordinateType /*coordType*/)
        {
            KTextEditor::Cursor c = cursorFromInt(offset);
            if (!c.isValid())
                return QRect();
            QPoint p = view()->view()->cursorToCoordinate(c);
            KTextEditor::Cursor endCursor = KTextEditor::Cursor(c.line(), c.column() + 1);
            QPoint size = view()->view()->cursorToCoordinate(endCursor) - p;
            return QRect(view()->mapToGlobal(p), QSize(size.x(), size.y()));
        }
        virtual int cursorPosition()
        {
            KTextEditor::Cursor c = view()->getCursor();
            return positionFromCursor(c);
        }
        virtual int offsetAtPoint(const QPoint& /*point*/, QAccessible2::CoordinateType /*coordType*/)
        {
            return 0;
        }
        virtual void removeSelection(int selectionIndex)
        {
            if (selectionIndex != 0)
                return;
            view()->view()->clearSelection();
        }
        virtual void scrollToSubstring(int /*startIndex*/, int /*endIndex*/)
        {
            // FIXME
        }
        virtual void selection(int selectionIndex, int *startOffset, int *endOffset)
        {
            if (selectionIndex != 0 || !view()->view()->selection()) {
                *startOffset = 0;
                *endOffset = 0;
                return;
            }
            KTextEditor::Range range = view()->view()->selectionRange();
            *startOffset = positionFromCursor(range.start());
            *endOffset = positionFromCursor(range.end());
        }

        virtual int selectionCount()
        {
            return view()->view()->selection() ? 1 : 0;
        }
        virtual void setCursorPosition(int position)
        {
            view()->view()->setCursorPosition(cursorFromInt(position));
        }
        virtual void setSelection(int selectionIndex, int startOffset, int endOffset)
        {
            if (selectionIndex != 0)
                return;
            KTextEditor::Range range = KTextEditor::Range(cursorFromInt(startOffset), cursorFromInt(endOffset));
            view()->view()->setSelection(range);
        }
        virtual QString text(int startOffset, int endOffset)
        {
            if (startOffset > endOffset)
                return QString();
            return view()->view()->document()->text().mid(startOffset, endOffset - startOffset);
        }

        virtual QString textAfterOffset(int offset, QAccessible2::BoundaryType boundaryType, int* startOffset, int* endOffset)
        {
            if (boundaryType == QAccessible2::LineBoundary)
                return textLine(1, offset + 1, startOffset, endOffset);

            const QString text = view()->view()->document()->text();
            return qTextAfterOffsetFromString(offset, boundaryType, startOffset, endOffset, text);
        }
        virtual QString textAtOffset(int offset, QAccessible2::BoundaryType boundaryType, int* startOffset, int* endOffset)
        {
            if (boundaryType == QAccessible2::LineBoundary)
                return textLine(0, offset, startOffset, endOffset);

            const QString text = view()->view()->document()->text();
            return qTextAtOffsetFromString(offset, boundaryType, startOffset, endOffset, text);
        }
        virtual QString textBeforeOffset(int offset, QAccessible2::BoundaryType boundaryType, int* startOffset, int* endOffset)
        {
            if (boundaryType == QAccessible2::LineBoundary)
                return textLine(-1, offset - 1, startOffset, endOffset);

            const QString text = view()->view()->document()->text();
            return qTextBeforeOffsetFromString(offset, boundaryType, startOffset, endOffset, text);
        }

        virtual QVariant invokeMethodEx(Method, int, const QVariantList&)
        { return QVariant(); }

    private:
        KateViewInternal *view() const
        {
            return static_cast<KateViewInternal*>(object());
        }

        KTextEditor::Cursor cursorFromInt(int position) const
        {
            int line = 0;
            for (;;) {
                const QString lineString = view()->view()->document()->line(line);
                if (position > lineString.length()) {
                    // one is the newline
                    position -= lineString.length() + 1;
                    ++line;
                } else {
                    break;
                }
            }
            return KTextEditor::Cursor(line, position);
        }

        int positionFromCursor(const KTextEditor::Cursor &cursor) const
        {
            int pos = 0;
            for (int line = 0; line < cursor.line(); ++line) {
                // length of the line plus newline
                pos += view()->view()->document()->line(line).size() + 1;
            }
            pos += cursor.column();

            return pos;
        }

        QString textLine(int shiftLines, int offset, int* startOffset, int* endOffset) const
        {
            KTextEditor::Cursor pos = cursorFromInt(offset);
            pos.setColumn(0);
            if (shiftLines)
                pos.setLine(pos.line() + shiftLines);
            *startOffset = positionFromCursor(pos);
            QString line = view()->view()->document()->line(pos.line()) + '\n';
            *endOffset = *startOffset + line.length();
            return line;
        }

        KateCursorAccessible *m_cursor;
};

/**
 * Factory-function used to create \a KateViewAccessible instances for KateViewInternal
 * to make the KateViewInternal accessible.
 */
QAccessibleInterface* accessibleInterfaceFactory(const QString &key, QObject *object)
{
  Q_UNUSED(key)
    //if (key == QLatin1String("KateViewInternal"))
    if (KateViewInternal *view = qobject_cast<KateViewInternal*>(object))
        return new KateViewAccessible(view);
    return 0;
}

#endif
#endif

#endif
