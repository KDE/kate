/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATEDYNAMICANIMATION_H
#define KATEDYNAMICANIMATION_H

#include <QObject>

#include <ktexteditor/attribute.h>

#include "katesmartrange.h"

class QTimer;

class KateSmartRange;
class KateView;
class KateDocument;

/**
 * Handles drawing and any required animation of ranges needing dynamic highlighting.
 *
 * @author Hamish Rodda \<rodda@kde.org\>
 */
class KateDynamicAnimation : public QObject
{
  Q_OBJECT

  public:
    KateDynamicAnimation(KateDocument* doc, KateSmartRange* range, KTextEditor::Attribute::ActivationType type);
    KateDynamicAnimation(KateView* view, KateSmartRange* range, KTextEditor::Attribute::ActivationType type);
    virtual ~KateDynamicAnimation();

    void init();

    KateDocument* document() const;
    KateView* view() const;
    KateSmartRange* range() const;
    KTextEditor::Attribute* dynamicAttribute() const;

    // The magic... add the dynamic highlight to the static highlight at this position
    void mergeToAttribute(KTextEditor::Attribute& attrib) const;

    void finish();

  public slots:
    void timeout();

  signals:
    void redraw(KateSmartRange* range);

  private:
    QVariant mergeWith(const QVariant& base, const QVariant& dynamic, int percent) const;

    KateSmartRangePtr m_range;
    KTextEditor::Attribute::ActivationType m_type;
    QTimer* m_timer;

    // Meaning of sequence (for now):
    // 0 - start of animation
    // 1 - 99 - intro
    // 100 - plateau
    // 101 - 200 - cycle
    // 201 - 300 - outro
    int m_sequence;
};

#endif
