/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2012 Dominik Haumann <dhaumann kde org>

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

#ifndef __KATE_SCHEMA_CONFIG_H__
#define __KATE_SCHEMA_CONFIG_H__

#include "katehighlight.h"
#include "katedialogs.h"
#include "katecolortreewidget.h"

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtGui/QFont>

#include <kconfig.h>
#include <kaction.h>

class KateView;
class KateStyleTreeWidget;

class KComboBox;
class KTabWidget;


class KateSchemaConfigColorTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigColorTab();
    ~KateSchemaConfigColorTab();

  public Q_SLOTS:
    void apply();
    void schemaChanged( int newSchema );

  Q_SIGNALS:
    void changed();
    
  private:
    QVector<KateColorItem> colorItemList() const;

  private:
    // multiple shemas may be edited. Hence, we need one ColorList for each schema
    QMap<int, QVector<KateColorItem> > m_schemas;
    int m_currentSchema;

    KateColorTreeWidget* ui;
};

typedef QMap<int,QFont> FontMap; // ### remove it

class KateSchemaConfigFontTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigFontTab();
    ~KateSchemaConfigFontTab();

  public:
    void readConfig (KConfig *config);

  public Q_SLOTS:
    void apply();
    void schemaChanged( int newSchema );

  Q_SIGNALS:
    void changed();

  private:
    class KFontChooser *m_fontchooser;
    FontMap m_fonts;
    int m_schema;

  private Q_SLOTS:
    void slotFontSelected( const QFont &font );
};

class KateSchemaConfigFontColorTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigFontColorTab();
    ~KateSchemaConfigFontColorTab();

  Q_SIGNALS:
    void changed();

  public:
    void schemaChanged (uint schema);
    void reload ();
    void apply ();

    KateAttributeList *attributeList (uint schema);
    void exportDefaults(int schema, KConfig *cfg);
    void importDefaults(const QString& schemaName, int schema, KConfig *cfg);
  private:
    KateStyleTreeWidget* m_defaultStyles;
    QHash<int,KateAttributeList*> m_defaultStyleLists;    
};

class KateSchemaConfigHighlightTab : public QWidget
{
  Q_OBJECT

  public:
    explicit KateSchemaConfigHighlightTab(KateSchemaConfigFontColorTab *page = 0);
    ~KateSchemaConfigHighlightTab();

    void schemaChanged (int schema);
    void reload ();
    void apply ();

  Q_SIGNALS:
    void changed();
    
  protected Q_SLOTS:
    void hlChanged(int z);
  public Q_SLOTS:
    void exportHl(int schema=-1,int hl=-1,KConfig* cfg=0);
    void importHl(const QString& fromSchemaName=QString(), int schema=-1, int hl=-1, KConfig *cfg=0);
        
  private:
    KateSchemaConfigFontColorTab *m_defaults;

    KComboBox *hlCombo;
    KateStyleTreeWidget *m_styles;

    int m_schema;
    int m_hl;

    QHash<int, QHash<int, QList<KateExtendedAttribute::Ptr> > > m_hlDict;
  public:
    QList<int> hlsForSchema(int schema);
    bool loadAllHlsForSchema(int schema);
};

class KateSchemaConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    explicit KateSchemaConfigPage ( QWidget *parent);
    ~KateSchemaConfigPage ();

  public Q_SLOTS:
    void apply();
    void reload();
    void reset();
    void defaults();
    void exportFullSchema();
    void importFullSchema();
  private Q_SLOTS:
    void update ();
    void deleteSchema ();
    void newSchema (const QString& newName=QString());
    void schemaChanged (int schema);

    void newCurrentPage(int);

  private:
    int m_lastSchema;
    int m_defaultSchema;

    class KTabWidget *m_tabWidget;
    class QPushButton *btndel;
    class KComboBox *defaultSchemaCombo;
    class KComboBox *schemaCombo;
    KateSchemaConfigColorTab *m_colorTab;
    KateSchemaConfigFontTab *m_fontTab;
    KateSchemaConfigFontColorTab *m_fontColorTab;
    KateSchemaConfigHighlightTab *m_highlightTab;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
