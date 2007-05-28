/* This file is part of the KDE libraries
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef __KATE_SYNTAXMANAGER_H__
#define __KATE_SYNTAXMANAGER_H__

#include "katetextline.h"
#include "kateextendedattribute.h"

#include <kconfig.h>
#include <kactionmenu.h>

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QMap>

#include <QtCore/QRegExp>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <QtCore/QDate>
#include <QtCore/QLinkedList>

class KateHlContext;
class KateHlItem;
class KateHlData;
class KateHlIncludeRule;
class KateSyntaxDocument;
class KateTextLine;
class KateSyntaxModeListItem;
class KateSyntaxContextData;
class KateHighlighting;

class QMenu;

class KateHlManager : public QObject
{
  Q_OBJECT

  public:
    KateHlManager();
    ~KateHlManager();

    static KateHlManager *self();
    
    KateSyntaxDocument *syntaxDocument () { return syntax; }

    inline KConfig *getKConfig() { return &m_config; }

    KateHighlighting *getHl(int n);
    int nameFind(const QString &name);

    QString identifierForName(const QString&);

    // methodes to get the default style count + names
    static uint defaultStyles();
    static QString defaultStyleName(int n, bool translateNames = false);

    void getDefaults(uint schema, KateAttributeList &);
    void setDefaults(uint schema, KateAttributeList &);

    int highlights();
    QString hlName(int n);
    QString hlNameTranslated (int n);
    QString hlSection(int n);
    bool hlHidden(int n);

    void incDynamicCtxs() { ++dynamicCtxsCount; }
    int countDynamicCtxs() { return dynamicCtxsCount; }
    void setForceNoDCReset(bool b) { forceNoDCReset = b; }

    // be carefull: all documents hl should be invalidated after having successfully called this method!
    bool resetDynamicCtxs();

  Q_SIGNALS:
    void changed();

  private:
    friend class KateHighlighting;

    // This list owns objects it holds, thus they should be deleted when the object is removed
    QList<KateHighlighting*> hlList;
    // This hash does not own the objects it holds, thus they should not be deleted
    QHash<QString, KateHighlighting*> hlDict;

    KConfig m_config;
    QStringList commonSuffixes;

    KateSyntaxDocument *syntax;

    int dynamicCtxsCount;
    QTime lastCtxsReset;
    bool forceNoDCReset;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
