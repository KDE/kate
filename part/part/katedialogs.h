/***************************************************************************
                          katedialogs.cpp  -  description
                             -------------------
    begin                : Sat 31 March 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KATE_DIALOGS_H
#define KATE_DIALOGS_H

#include "kateglobal.h"
#include "katesyntaxdocument.h"
#include "katehighlight.h"
#include "katedocument.h"

#include <kdialog.h>
#include <kdialogbase.h>
#include <klistview.h>
#include <qtabwidget.h>
#include <kcolorbutton.h>

class QWidgetStack;
class QVBox;
class  KListView;
class QListViewItem;
struct syntaxContextData;
class QCheckBox;

#define HlEUnknown 0
#define HlEContext 1
#define HlEItem 2

class StyleChanger : public QWidget {
    Q_OBJECT
  public:
    StyleChanger(QWidget *parent );
    void setRef(ItemStyle *);
    void setEnabled(bool);
  protected slots:
    void changed();
  protected:
    ItemStyle *style;
    KColorButton *col;
    KColorButton *selCol;
    QCheckBox *bold;
    QCheckBox *italic;
};

class HlConfigPage : public Kate::ConfigPage
{
  Q_OBJECT

  public:
    HlConfigPage (QWidget *parent, KateDocument *doc);
    ~HlConfigPage ();

  private:
    KateDocument *myDoc;
    class HighlightDialogPage *page;
    class HlManager *hlManager;
    HlDataList hlDataList;
    ItemStyleList defaultStyleList;

  public slots:
    void apply ();
    void reload ();
};

class HighlightDialogPage : public QTabWidget
{
    Q_OBJECT
  public:
    HighlightDialogPage(HlManager *, ItemStyleList *, HlDataList *, int hlNumber,
                    QWidget *parent=0, const char *name=0);
    void saveData();

  protected slots:
    void defaultChanged(int);

    void hlChanged(int);
    void itemChanged(int);
    void changed();
    void hlEdit();
    void hlNew();
    void hlDownload();
  protected:
    StyleChanger *defaultStyleChanger;
    ItemStyleList *defaultItemStyleList;

    void writeback();
    QComboBox *itemCombo, *hlCombo;
    QLineEdit *wildcards;
    QLineEdit *mimetypes;
    QCheckBox *styleDefault;
    StyleChanger *styleChanger;

    HlDataList *hlDataList;
    HlData *hlData;
    ItemData *itemData;
};

class ItemInfo
{
  public:
    ItemInfo():trans_i18n(),length(0){};
    ItemInfo(QString _trans,int _length):trans_i18n(_trans),length(_length){};
    QString trans_i18n;
    int length;
};

class HighlightDialog : public KDialogBase
{
  Q_OBJECT
  public:
    HighlightDialog( HlManager *hlManager, ItemStyleList *styleList,
                                  HlDataList *highlightDataList,
                                  int hlNumber, QWidget *parent,
                                  const char *name=0, bool modal=true );
  private:
    HighlightDialogPage *content;
  protected:
    virtual void done(int r);
};

class HlEditDialog : public KDialogBase
{
    Q_OBJECT
  public:
    HlEditDialog(HlManager *,QWidget *parent=0, const char *name=0, bool modal=true, HlData *data=0);
  private:
    class QWidgetStack *stack;
    class QVBox *contextOptions, *itemOptions;
    class KListView *contextList;
    class QListViewItem *currentItem;
    void initContextOptions(class QVBox *co);
    void initItemOptions(class QVBox *co);
    void loadFromDocument(HlData *hl);
    void showContext();
    void showItem();

    QListViewItem *addContextItem(QListViewItem *_parent,QListViewItem *prev,struct syntaxContextData *data);
    void insertTranslationList(QString tag, QString trans,int length);
    void newDocument();

    class QLineEdit *ContextDescr;
    class QComboBox *ContextAttribute;
    class QComboBox *ContextLineEnd;
    class KIntNumInput *ContextPopCount;

    class QComboBox *ItemType;
    class QComboBox *ItemContext;
    class HLParamEdit *ItemParameter;
    class QComboBox *ItemAttribute;
    class KIntNumInput *ItemPopCount;

    class QMap<int,QString> id2tag;
    class QMap<int,ItemInfo> id2info;
    class QMap<QString,int> tag2id;

    class AttribEditor *attrEd;
    int transTableCnt;
  protected slots:
    void pageChanged(QWidget *);


    void currentSelectionChanged ( QListViewItem * );
    void contextDescrChanged(const QString&);
    void contextLineEndChanged(int);
    void contextAttributeChanged(int);
    void ContextPopCountChanged(int count);
    void contextAddNew();

    void ItemTypeChanged(int id);
    void ItemParameterChanged(const QString& name);
    void ItemAttributeChanged(int attr);
    void ItemContextChanged(int cont);
    void ItemPopCountChanged(int count);
    void ItemAddNew();
};

#endif
