/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_SCHEMA_H__
#define __KATE_SCHEMA_H__

#include "katehighlight.h"
#include "katedialogs.h"

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtGui/QFont>

#include <kconfig.h>
#include <kaction.h>

class KateView;
class KateStyleListCaption;
class KateStyleTreeWidget;

class KColorButton;

class QMenu;
class QAction;
class QActionGroup;
class KComboBox;
class QComboBox;

namespace Ui { class SchemaConfigColorTab; }

class KateSchemaManager
{
  public:
    KateSchemaManager ();
    ~KateSchemaManager ();

    /**
     * Schema Config changed, update all renderers
     */
    void update (bool readfromfile = true);

    /**
     * return kconfig with right group set or set to Normal if not there
     */
    KConfigGroup schema (uint number);

    void addSchema (const QString &t);

    void removeSchema (uint number);

    /**
     * is this schema valid ? (does it exist ?)
     */
    bool validSchema (uint number);

    /**
     * if not found, defaults to 0
     */
    uint number (const QString &name);

    /**
     * group names in the end, no i18n involved
     */
    QString name (uint number);

    /**
     * Don't modify, list with the names of the schemas (i18n name for the default ones)
     */
    const QStringList &list () { return m_schemas; }

    static QString normalSchema ();
    static QString printingSchema ();

  private:
    KConfig m_config;
    QStringList m_schemas;
};


class KateViewSchemaAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewSchemaAction(const QString& text, QObject *parent)
       : KActionMenu(text, parent) { init(); }

    void updateMenu (KateView *view);

  private:
    void init();

    QPointer<KateView> m_view;
    QStringList names;
    QActionGroup *m_group;
    int last;

  public  Q_SLOTS:
    void slotAboutToShow();

  private Q_SLOTS:
    void setSchema();
};

//
// DIALOGS
//

class KateSchemaConfigColorTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigColorTab();
    ~KateSchemaConfigColorTab();

  private:
    // Class for storing the properties on 1 schema.
    class SchemaColors {
      public:
        QColor back, selected, current, bracket, wwmarker, iconborder, tmarker, linenumber;
        QMap<int, QColor> markerColors;  // stores all markerColors
        QMap<int, QColor> templateColors;
    };

    // schemaid=data, created when a schema is entered
    QMap<int,SchemaColors> m_schemas;
    // current schema
    int m_schema;

    Ui::SchemaConfigColorTab* ui;

  public Q_SLOTS:
    void apply();
    void schemaChanged( int newSchema );

  Q_SIGNALS:
    void changed();

  protected Q_SLOTS:
    void slotMarkerColorChanged(const QColor&);
    void slotComboBoxChanged(int index);
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

  private:
    KateStyleTreeWidget* m_defaultStyles;
    QHash<int,KateAttributeList*> m_defaultStyleLists;
};

class KateSchemaConfigHighlightTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigHighlightTab(KateSchemaConfigFontColorTab *page = 0, uint hl = 0 );
    ~KateSchemaConfigHighlightTab();

    void schemaChanged (int schema);
    void reload ();
    void apply ();

  Q_SIGNALS:
    void changed();

  protected Q_SLOTS:
    void hlChanged(int z);

  private:
    KateSchemaConfigFontColorTab *m_defaults;

    QComboBox *hlCombo;
    KateStyleTreeWidget *m_styles;

    int m_schema;
    int m_hl;

    QHash<int, QHash<int, QList<KateExtendedAttribute::Ptr> > > m_hlDict;
};

class KateSchemaConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    KateSchemaConfigPage ( QWidget *parent, class KateDocument *doc=0 );
    ~KateSchemaConfigPage ();

  public Q_SLOTS:
    void apply();
    void reload();
    void reset();
    void defaults();

  private Q_SLOTS:
    void update ();
    void deleteSchema ();
    void newSchema ();
    void schemaChanged (int schema);

    void newCurrentPage (QWidget *w);

  private:
    int m_lastSchema;
    int m_defaultSchema;

    class QTabWidget *m_tabWidget;
    class QPushButton *btndel;
    class QComboBox *defaultSchemaCombo;
    class QComboBox *schemaCombo;
    KateSchemaConfigColorTab *m_colorTab;
    KateSchemaConfigFontTab *m_fontTab;
    KateSchemaConfigFontColorTab *m_fontColorTab;
    KateSchemaConfigHighlightTab *m_highlightTab;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
