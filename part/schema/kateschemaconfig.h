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

#include "katedialogs.h"
#include "katecolortreewidget.h"
#include "kateextendedattribute.h"

#include <QtCore/QMap>
#include <QtGui/QFont>

#include <kconfig.h>

class KateStyleTreeWidget;

class KComboBox;
class KTabWidget;


class KateSchemaConfigColorTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigColorTab();
    ~KateSchemaConfigColorTab();

    QColor backgroundColor() const;
    QColor selectionColor() const;

  public Q_SLOTS:
    void apply();
    void reload();
    void schemaChanged( const QString &newSchema );

    void importSchema(KConfigGroup& config);
    void exportSchema(KConfigGroup& config);

  Q_SIGNALS:
    void changed();

  private:
    QVector<KateColorItem> colorItemList() const;
    QVector<KateColorItem> readConfig(KConfigGroup& config);

  private:
    // multiple shemas may be edited. Hence, we need one ColorList for each schema
    QMap<QString, QVector<KateColorItem> > m_schemas;
    QString m_currentSchema;

    KateColorTreeWidget* ui;
};

class KateSchemaConfigFontTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigFontTab();
    ~KateSchemaConfigFontTab();

  public:
    void readConfig (KConfig *config);

    void importSchema(KConfigGroup& config);
    void exportSchema(KConfigGroup& config);

  public Q_SLOTS:
    void apply();
    void reload();
    void schemaChanged( const QString &newSchema );

  Q_SIGNALS:
    void changed();

  private:
    class KFontChooser *m_fontchooser;
    QMap<QString, QFont> m_fonts;
    QString m_currentSchema;

  private Q_SLOTS:
    void slotFontSelected( const QFont &font );
};

class KateSchemaConfigDefaultStylesTab : public QWidget
{
  Q_OBJECT

  public:
    KateSchemaConfigDefaultStylesTab(KateSchemaConfigColorTab* colorTab);
    ~KateSchemaConfigDefaultStylesTab();

  Q_SIGNALS:
    void changed();

  public:
    void schemaChanged (const QString &schema);
    void reload ();
    void apply ();

    KateAttributeList *attributeList (const QString &schema);
    void exportSchema(const QString &schema, KConfig *cfg);
    void importSchema(const QString& schemaName, const QString &schema, KConfig *cfg);

  protected:
    virtual void showEvent(QShowEvent* event);
    void updateColorPalette(const QColor& textColor);

  private:
    KateStyleTreeWidget* m_defaultStyles;
    QHash<QString,KateAttributeList*> m_defaultStyleLists;
    KateSchemaConfigColorTab* m_colorTab;
    QString m_currentSchema;
};

class KateSchemaConfigHighlightTab : public QWidget
{
  Q_OBJECT

  public:
    explicit KateSchemaConfigHighlightTab(KateSchemaConfigDefaultStylesTab *page, KateSchemaConfigColorTab* colorTab);
    ~KateSchemaConfigHighlightTab();

    void schemaChanged (const QString &schema);
    void reload ();
    void apply ();

  Q_SIGNALS:
    void changed();

  protected Q_SLOTS:
    void hlChanged(int z);

  public Q_SLOTS:
    void exportHl(QString schema=QString(),int hl=-1,KConfig* cfg=0);
    void importHl(const QString& fromSchemaName=QString(), QString schema=QString(), int hl=-1, KConfig *cfg=0);

  protected:
    virtual void showEvent(QShowEvent* event);
    void updateColorPalette(const QColor& textColor);

  private:
    KateSchemaConfigDefaultStylesTab *m_defaults;
    KateSchemaConfigColorTab* m_colorTab;

    KComboBox *hlCombo;
    KateStyleTreeWidget *m_styles;

    QString m_schema;
    int m_hl;

    QHash<QString, QHash<int, QList<KateExtendedAttribute::Ptr> > > m_hlDict;
  
public:
    QList<int> hlsForSchema(const QString &schema);
    bool loadAllHlsForSchema(const QString &schema);
};

class KateSchemaConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    explicit KateSchemaConfigPage ( QWidget *parent);
    virtual ~KateSchemaConfigPage ();

  public Q_SLOTS:
    void apply();
    void reload();
    void reset();
    void defaults();
    void exportFullSchema();
    void importFullSchema();

  private Q_SLOTS:
    void deleteSchema ();
    bool newSchema (const QString& newName = QString());
    void schemaChanged (const QString &schema);
    void comboBoxIndexChanged (int currentIndex);

  private:
    void refillCombos(const QString& schemaName, const QString& defaultSchemaName);
    QString requestSchemaName(const QString& suggestedName);

  private:
    QString m_currentSchema;

    class KTabWidget *m_tabWidget;
    class QPushButton *btndel;
    class KComboBox *defaultSchemaCombo;
    class KComboBox *schemaCombo;
    KateSchemaConfigColorTab *m_colorTab;
    KateSchemaConfigFontTab *m_fontTab;
    KateSchemaConfigDefaultStylesTab *m_defaultStylesTab;
    KateSchemaConfigHighlightTab *m_highlightTab;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
