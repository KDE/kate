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

// $Id$

#include "hlparamedit.h"
#include "hlparamedit.moc"

#include <klocale.h>

#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>

HLParamEdit::HLParamEdit(QWidget *parent):QHBox(parent)
{
  textEdit=0;
  listLabel=0;
  listChoose=0;
  listNew=0;
  listEdit=0;
}

HLParamEdit::~HLParamEdit(){;}

void HLParamEdit::ListParameter(QString listname)
{
  delete textEdit;
  textEdit=0;

  if (!listLabel)
  {
    listLabel=new QLabel(listname,this);
    listChoose=new QPushButton(i18n("Choose"),this);
    listNew=new QPushButton(i18n("New"),this);
    listEdit=new QPushButton(i18n("Edit"),this);
  }
  listLabel->setText(listname);
  listLabel->show();
  listChoose->show();
  listNew->show();
  listEdit->show();
}

void HLParamEdit::TextParameter(int length, QString text, bool /* regExp */)
{
  delete listLabel;
  delete listChoose;
  delete listNew;
  delete listEdit;
  listLabel=0;
  listChoose=0;
  listNew=0;
  listEdit=0;

  if (!textEdit)
  {
    textEdit=new QLineEdit(this);
    connect(textEdit,SIGNAL(textChanged(const QString&)),this,SIGNAL(textChanged(const QString&)));
  }
  textEdit->setMaxLength(length);
  textEdit->setText(text);
  textEdit->show();
}

QString HLParamEdit::text()
{
  if (!textEdit)
     return QString::null;

  return textEdit->text();
}

//BEGIN HlEditDialog

#define TAG_DETECTCHAR "DetectChar"
#define TAG_DETECT2CHARS "Detect2Chars"
#define TAG_RANGEDETECT "RangeDetect"
#define TAG_STRINGDETECT "StringDetect"
#define TAG_ANYCHAR "AnyChar"
#define TAG_REGEXPR "RegExpr"
#define TAG_INT "Int"
#define TAG_FLOAT "Float"
#define TAG_KEYWORD "keyword"

/******************************************************************************/
/*                     HlEditDialog implementation                            */
/******************************************************************************/
HlEditDialog::HlEditDialog(HlManager *,QWidget *parent, const char *name, bool modal,HlData *data)
  :KDialogBase(KDialogBase::Swallow, i18n("Highlight Conditions"), Ok|Cancel, Ok, parent, name, modal)
{
  QTabWidget *tabWid=new QTabWidget(this);

/* attributes */
  tabWid->addTab(attrEd=new AttribEditor(tabWid),i18n("Attributes"));

/*Contextstructure */
  currentItem=0;
    transTableCnt=0;
  QHBox *wid=new QHBox(tabWid);
  QVBox *lbox=new QVBox(wid);
    contextList=new KListView(lbox);
    contextList->setRootIsDecorated(true);
    contextList->addColumn(i18n("Syntax Structure"));
    contextList->setSorting(-1);
    QHBox *bbox=new QHBox(lbox);
    QPushButton *addContext=new QPushButton(i18n("New Context"),bbox);
    QPushButton *addItem=new QPushButton(i18n("New Item"),bbox);
    QVGroupBox *opt  = new QVGroupBox( i18n("Options"), wid);
    stack=new QWidgetStack(opt);
    initContextOptions(contextOptions=new QVBox(stack));
    stack->addWidget(contextOptions,HlEContext);
    initItemOptions(itemOptions=new QVBox(stack));
    stack->addWidget(itemOptions,HlEItem);
    stack->raiseWidget(HlEContext);
    tabWid->addTab(wid,i18n("Structure"));
    setMainWidget(tabWid);
    if (data!=0) loadFromDocument(data);
    else newDocument();

/* context structure connects */
    connect(contextList,SIGNAL(currentChanged( QListViewItem*)),this,SLOT(currentSelectionChanged ( QListViewItem * )));
    connect(addContext,SIGNAL(clicked()),this,SLOT(contextAddNew()));
    connect(addItem,SIGNAL(clicked()),this,SLOT(ItemAddNew()));

/* attribute setting - connects */

    connect(tabWid,SIGNAL(currentChanged(QWidget*)),this,SLOT(pageChanged(QWidget*)));
    }

void HlEditDialog::pageChanged(QWidget *widget)
{
  if (widget/*==*/)
  {
      ContextAttribute->clear();
      ItemAttribute->clear();
      ContextAttribute->insertStringList(attrEd->attributeNames());
      ItemAttribute->insertStringList(attrEd->attributeNames());
  }
}

void HlEditDialog::newDocument()
{
  KStandardDirs *dirs = KGlobal::dirs();
  QStringList list=dirs->findAllResources("data","katepart/syntax/syntax.template",false,true);
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
      {
        HlData data("","",*it,0);
        loadFromDocument(&data);
        return;
      }
  KMessageBox::error(this,i18n("Can't find template file"));
}


void HlEditDialog::initContextOptions(QVBox *co)
{
  if( co!=0)
    {
        QHBox *tmp = new QHBox(co);
        (void) new QLabel(i18n("Description:"),tmp);
        ContextDescr=new QLineEdit(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("Attribute:"),tmp);
        ContextAttribute=new QComboBox(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("LineEnd:"),tmp);
        ContextLineEnd = new QComboBox(tmp);
  (ContextPopCount=new KIntNumInput(tmp))->setRange(1,20,1,false);
  connect(ContextDescr,SIGNAL(textChanged(const QString&)),this,SLOT(contextDescrChanged(const QString&)));
        connect(ContextLineEnd,SIGNAL(activated(int)),this,SLOT(contextLineEndChanged(int)));
        connect(ContextAttribute,SIGNAL(activated(int)),this,SLOT(contextAttributeChanged(int)));
  connect(ContextPopCount,SIGNAL(valueChanged(int)),this,SLOT(ContextPopCountChanged(int)));
    }
   else
     kdDebug(13010)<<"initContextOptions: Widget is 0"<<endl;
}


void HlEditDialog::insertTranslationList(QString tag, QString trans,int length)
  {
    ItemInfo data(trans,length);
    id2tag.insert(transTableCnt,tag);
    id2info.insert(transTableCnt,data);
    tag2id.insert(tag,transTableCnt);
    transTableCnt++;
  }


void HlEditDialog::initItemOptions(QVBox *co)
{
  if (co!=0)
    {
        QHBox *tmp = new QHBox(co);
        (void) new QLabel(i18n("Type:"),tmp);
        ItemType = new QComboBox(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("Parameter:"),tmp);
        ItemParameter=  new HLParamEdit(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("Attribute:"),tmp);
        ItemAttribute= new QComboBox(tmp);
        (void) new QLabel(i18n("Context switch:"),tmp);
        ItemContext = new QComboBox(tmp);
  (ItemPopCount=new KIntNumInput(tmp))->setRange(1,20,1,false);
  //ItemPopCount->hide();

        co->setSpacing(15);
        new QPushButton(i18n("Delete This Item"),co);

  /* init translation lists */
  insertTranslationList("DetectChar","DetectChar",1);
        insertTranslationList("Detect2Chars","Detect2Chars",2);
  insertTranslationList("RangeDetect","RangeDetect",2);
  insertTranslationList("StringDetect","StringDetect",-1);
  insertTranslationList("AnyChar","AnyChar",-1);
  insertTranslationList("RegExpr","RegExpr",-1);
  insertTranslationList("Int","Int",0);
  insertTranslationList("Float","Float",0);
  insertTranslationList("keyword","keyword",0);
        ItemType->clear();
        for (int i=0; i<transTableCnt; i++) ItemType->insertItem(id2info[i].trans_i18n);
        connect(ItemType,SIGNAL(activated(int)),this,SLOT(ItemTypeChanged(int)));
        connect(ItemParameter,SIGNAL(textChanged(const QString&)),this,SLOT(ItemParameterChanged(const QString&)));
        connect(ItemAttribute,SIGNAL(activated(int)),this,SLOT(ItemAttributeChanged(int)));
        connect(ItemContext,SIGNAL(activated(int)),this,SLOT(ItemContextChanged(int)));
  connect(ItemPopCount,SIGNAL(valueChanged(int)),this,SLOT(ItemPopCountChanged(int)));
    }
  else
    kdDebug(13010)<<"initItemOptions: Widget is 0"<<endl;
}

void HlEditDialog::loadFromDocument(HlData *hl)
{
  struct syntaxContextData *data;
  QListViewItem *last=0,*lastsub=0;

  HlManager::self()->syntax->setIdentifier(hl->identifier);
  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");
  int i=0;
  if (data)
    {
      while (HlManager::self()->syntax->nextGroup(data)) //<context tags>
        {
        kdDebug(13010)<< "Adding context to list"<<endl;
          last= new QListViewItem(contextList,last,
                 HlManager::self()->syntax->groupData(data,QString("name")),
                 QString("%1").arg(i),
                 HlManager::self()->syntax->groupData(data,QString("attribute")),
                 HlManager::self()->syntax->groupData(data,QString("lineEndContext")));
          i++;
          lastsub=0;
          while (HlManager::self()->syntax->nextItem(data))
              {
                kdDebug(13010)<< "Adding item to list"<<endl;
                lastsub=addContextItem(last,lastsub,data);
              }


   }
       if (data) HlManager::self()->syntax->freeGroupInfo(data);
   }
  attrEd->load(HlManager::self()->syntax);
}

QListViewItem *HlEditDialog::addContextItem(QListViewItem *_parent,QListViewItem *prev,struct syntaxContextData *data)
  {

                kdDebug(13010)<<HlManager::self()->syntax->groupItemData(data,QString("name")) << endl;

                QString dataname=HlManager::self()->syntax->groupItemData(data,QString(""));
                QString attr=(HlManager::self()->syntax->groupItemData(data,QString("attribute")));
                QString context=(HlManager::self()->syntax->groupItemData(data,QString("context")));
    char chr;
                if (! HlManager::self()->syntax->groupItemData(data,QString("char")).isEmpty())
      chr= (HlManager::self()->syntax->groupItemData(data,QString("char")).latin1())[0];
    else
                  chr=0;
    QString stringdata=HlManager::self()->syntax->groupItemData(data,QString("String"));
                char chr1;
                if (! HlManager::self()->syntax->groupItemData(data,QString("char1")).isEmpty())
      chr1= (HlManager::self()->syntax->groupItemData(data,QString("char1")).latin1())[0];
    else
                  chr1=0;
    // not used at the mom, fix warning
    //bool insensitive=(HlManager::self()->syntax->groupItemData(data,QString("insensitive"))==QString("TRUE"));
                QString param("");
                if ((dataname=="keyword") /*|| (dataname=="dataType")*/) param=stringdata;
                  else if (dataname=="DetectChar") param=chr;
                    else if ((dataname=="Detect2Chars") || (dataname=="RangeDetect")) {param=QString("%1%2").arg(chr).arg(chr1);}
                      else if ((dataname=="StringDetect") || (dataname=="AnyChar") || (dataname=="RegExpr")) param=stringdata;
                        else                     kdDebug(13010)<<"***********************************"<<endl<<"Unknown entry for Context:"<<dataname<<endl;
                kdDebug(13010)<<dataname << endl;
                return new QListViewItem(_parent,prev,i18n(dataname.latin1())+" "+param,dataname,attr,context,param);
 }


void HlEditDialog::currentSelectionChanged ( QListViewItem *it)
  {
     kdDebug(13010)<<"Update data view"<<endl<<"Depth:"<<it->depth()<<endl;
     currentItem=it;
     if (it->depth()==0) showContext();
        else showItem();
  }


/****************************************************************************/
/*                              CONTEXTS                                    */
/****************************************************************************/


void HlEditDialog::showContext()
  {
    stack->raiseWidget(HlEContext);
    ContextDescr->setText(currentItem->text(0));
    ContextAttribute->setCurrentItem(currentItem->text(2).toInt());
    ContextLineEnd->clear();
    ContextLineEnd->insertItem("#pop");
    ContextLineEnd->insertItem("#stay");
    for (QListViewItem *it=contextList->firstChild();it;it=it->nextSibling())
        ContextLineEnd->insertItem(it->text(0));
    ContextLineEnd->setCurrentItem(currentItem->text(3).startsWith("#pop")?0:(currentItem->text(3).contains("#stay")?1:currentItem->text(3).toInt()+2));
    if (currentItem->text(3).startsWith("#pop"))
      {
    QString tmp=currentItem->text(3);
    int tmpPop;
    for (tmpPop=0;tmp.startsWith("#pop");tmpPop++,tmp.remove(0,4));
    ContextPopCount->setValue(tmpPop);
    ContextPopCount->show();
    //Do something
  } else ContextPopCount->hide();
  }

void HlEditDialog::contextDescrChanged(const QString& name)
  {
    if (currentItem)
      {
        currentItem->setText(0,name);
        ContextLineEnd->changeItem(name,currentItem->text(3).toInt()+2);
      }
  }

void HlEditDialog::contextAttributeChanged(int id)
{
  if (currentItem)
     {
     currentItem->setText(2,QString("%1").arg(id));
     }
}

void HlEditDialog::contextLineEndChanged(int id)
{
  kdDebug(13010)<<"contextLineEndChanged"<<endl;
  if (currentItem)
     {
     if (id==0)
     {
       currentItem->setText(3,"#pop"); // do something
     }
     else
       if (id==1) currentItem->setText(3,"#stay");
  else
         currentItem->setText(3,QString("%1").arg(id-2));
     }
}

void HlEditDialog::ContextPopCountChanged(int count)
{
  if (currentItem)
  {
    if (currentItem->text(3).startsWith("#pop"))
    {
      QString tmp="";
      for (int i=0;i<count;i++) tmp=tmp+"#pop";
      currentItem->setText(3,tmp);
    }
  }
}



void HlEditDialog::contextAddNew()
{
  QListViewItem *it=contextList->firstChild();
  for (;it->nextSibling()!=0;it=it->nextSibling());
  it=new QListViewItem(contextList,it,i18n("New Context"),QString("%1").arg(it->text(1).toInt()),"0","0");
  contextList->setSelected(it,true);
}

/****************************************************************************/
/*                              ITEMS                                       */
/****************************************************************************/

void HlEditDialog::showItem()
  {
    stack->raiseWidget(HlEItem);
    ItemContext->clear();
    ItemContext->insertItem("#pop");
    ItemContext->insertItem("#stay");
    for (QListViewItem *it=contextList->firstChild();it;it=it->nextSibling())
        ItemContext->insertItem(it->text(0));
    uint tmpCtx;
    ItemContext->setCurrentItem(tmpCtx=(currentItem->text(3).startsWith("#pop")?0:(currentItem->text(3).contains("#stay")?1:currentItem->text(3).toInt()+2)));
    kdDebug(13000)<<QString("showItem(): tmpCtx=%1").arg(tmpCtx)<<endl;
    if (tmpCtx==0)
    {
      kdDebug(13000)<<"Showing ItempPopCount"<<endl;
      ItemPopCount->show();
  QString tmp=currentItem->text(3);
  for (tmpCtx=0;tmp.startsWith("#pop");tmpCtx++,tmp.remove(0,4));
  ItemPopCount->setValue(tmpCtx);
    }
    else ItemPopCount->hide();
    ItemAttribute->setCurrentItem(currentItem->text(2).toInt());
    if (currentItem->text(1)==TAG_KEYWORD)
    {
  ItemParameter->ListParameter(currentItem->text(4));
  ItemParameter->show();
    }
    else
    {
      QMap<QString,int>::Iterator iter=tag2id.find(currentItem->text(1));
      if (iter==tag2id.end())
          kdDebug(13010)<<"Oops, unknown itemtype in showItem: "<<currentItem->text(1)<<endl;
      else
        {
          ItemType->setCurrentItem(*iter);
          if (id2info[*iter].length==0) ItemParameter->hide();
            else
             {
                 ItemParameter->TextParameter(id2info[*iter].length,currentItem->text(4));
                 ItemParameter->show();
             }
        }
     }

  }

void HlEditDialog::ItemTypeChanged(int id)
{
  if (currentItem)
     {
     currentItem->setText(1,id2tag[id]);
     ItemParameter->TextParameter(id2info[id].length,"");
     ItemParameterChanged(ItemParameter->text());
     }
}

void HlEditDialog::ItemParameterChanged(const QString& name)
{
  if (currentItem)
    {
      currentItem->setText(2,name);
      currentItem->setText(0,id2info[ItemType->currentItem()].trans_i18n+" "+currentItem->text(2));
    }
}

void HlEditDialog::ItemAttributeChanged(int attr)
{
   if (currentItem)
     {
       currentItem->setText(2,QString("%1").arg(attr));
     }
}

void HlEditDialog::ItemContextChanged(int cont)
{
   if (currentItem)
     {
       if (cont>1)
  {
    currentItem->setText(3,QString("%1").arg(cont-2));
    ItemPopCount->hide();
  }
       else
  {
    if (cont==0)
    {
      ItemPopCount->setValue(1);
      currentItem->setText(3,"#pop");
      ItemPopCount->show();
    }
    else
    {
      ItemPopCount->hide();
      currentItem->setText(3,"#push");
    }
  }
     }
}

void HlEditDialog::ItemPopCountChanged(int count)
{
  if (currentItem)
  {
    if (currentItem->text(3).startsWith("#pop"))
    {
      QString tmp="";
      for (int i=0;i<count;i++) tmp=tmp+"#pop";
      currentItem->setText(3,tmp);
    }
  }
}

void HlEditDialog::ItemAddNew()
{
  QListViewItem *it;
  if (currentItem)
    {
      if (currentItem->depth()==0) it=currentItem->firstChild();
        else
          it=currentItem;
      if (it) for (;it->nextSibling();it=it->nextSibling());
      (void) new QListViewItem(it ? it->parent() : currentItem,it,"StringDetect "+i18n("New Item"),"StringDetect",i18n("New Item"),0,it ? it->parent()->text(1) : currentItem->text(1));
    }
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on;
