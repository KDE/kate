/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _HL_PARAM_EDIT_H_
#define _HL_PARAM_EDIT_H_

#include <qhbox.h>

class HLParamEdit:public QHBox
{
  Q_OBJECT

  public:
    HLParamEdit(QWidget *parent);
    ~HLParamEdit();

    void ListParameter(QString listname);
    void TextParameter(int length, QString text,bool regExp=false);
    QString text();

  private:
    class QLineEdit *textEdit;
    class QLabel *listLabel;
    class QPushButton *listChoose;
    class QPushButton *listNew;
    class QPushButton *listEdit;


  signals:
    void textChanged(const QString&);
};

#define HlEUnknown 0
#define HlEContext 1
#define HlEItem 2

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
