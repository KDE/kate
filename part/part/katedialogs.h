/* This file is part of the KDE libraries
   Copyright (C) 2002 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KATE_DIALOGS_H
#define KATE_DIALOGS_H

#include "katehighlight.h"
#include "../interfaces/document.h"

#include <kdialog.h>
#include <kdialogbase.h>
#include <klistview.h>
#include <qtabwidget.h>
#include <kcolorbutton.h>
#include <qcolor.h>
#include <qlistview.h>

class QWidgetStack;
class QVBox;
class  KListView;
class QListViewItem;
struct syntaxContextData;
class QCheckBox;

#define HlEUnknown 0
#define HlEContext 1
#define HlEItem 2

/*
    QListViewItem subclass to display/edit a style, bold/italic is check boxes,
    normal and selected colors are boxes, which will display a color chooser when
    activated.
    The context name for the style will be drawn using the editor default font and
    the chosen colors.
    This widget id designed to handle the default as well as the individual hl style
    lists.
    This widget is designed to work with the StyleListView class exclusively.
    Added by anders, jan 23 2002.
*/
class StyleListItem : public QListViewItem {
  public:
    StyleListItem( QListView *parent=0, const QString & stylename=0,
                   class ItemStyle *defaultstyle=0, class ItemData *data=0 );
    ~StyleListItem() {};

    /* mainly for readability */
    enum Property { ContextName, Bold, Italic, Color, SelColor, UseDefStyle };

    /* reimp */
    virtual int width ( const QFontMetrics & fm, const QListView * lv, int c ) const;
    /* calls changeProperty() if it makes sense considering pos. */
    void activate( int column, const QPoint &localPos );
    /* For bool fields, toggles them, for color fields, display a color chooser */
    void changeProperty( Property p );
    /* style context name */
    QString contextName() { return text(0); };
    /* only true for a hl mode item using it's default style */
    bool defStyle();
    /* true for default styles */
    bool isDefault();
    /* whichever style is active (st for hl mode styles not using
       the default style, ds otherwise) */
    class ItemStyle* style() { return is; };
  protected:
    /* reimp */
    void paintCell(QPainter *p, const QColorGroup& cg, int col, int width, int align);
  private:
    /* private methods to change properties */
    void toggleBold();
    void toggleItalic();
    void toggleDefStyle();
    void setCol();
    void setSelCol();
    /* helper function to copy the default style into the ItemData,
       when a property is changed and we are using default style. */
    void setCustStyle();

    class ItemStyle *is, // the style currently in use
              *ds; // default style for hl mode contexts and default styles
    class ItemData *st;  // itemdata for hl mode contexts
};

/*
    QListView that automatically adds columns for StyleListItems and provides a
    popup menu and a slot to edit a style using the keyboard.
    Added by anders, jan 23 2002.
*/
class StyleListView : public QListView {
    Q_OBJECT
  friend class StyleListItem;
  public:
    StyleListView( QWidget *parent=0, bool showUseDefaults=false, QColor textcol=QColor() );
    ~StyleListView() {};
    /* Display a popupmenu for item i at the specified global position, eventually with a title,
       promoting the context name of that item */
    void showPopupMenu( StyleListItem *i, const QPoint &globalPos, bool showtitle=false );
  private slots:
    /* Display a popupmenu for item i at item position */
    void showPopupMenu( QListViewItem *i );
    /* call item to change a property, or display a menu */
    void slotMousePressed( int, QListViewItem*, const QPoint&, int );
    /* asks item to change the property in q */
    void mSlotPopupHandler( int z );
  private:
    QColor bgcol, selcol, normalcol;
    QFont docfont;
};

/**
   This widget provides a checkable list of all available mimetypes,
   and a list of selected ones, as well as a corresponding list of file
   extensions, an optional text and an optional edit button (not working yet).
   Mime types is presented in a list view, with name, comment and patterns columns.
   Added by anders, jan 23, 2002
*/
#include <kmimetype.h>
#include <qvbox.h>
#include <qstringlist.h>
class KMimeTypeChooser : public QVBox {
    Q_OBJECT
  public:
    KMimeTypeChooser( QWidget *parent=0, const QString& text=QString::null, const QStringList &selectedMimeTypes=0,
                      bool editbutton=true, bool showcomment=true, bool showpattern=true );
    ~KMimeTypeChooser() {};
    QStringList selectedMimeTypesStringList();
    QStringList patterns();
  public slots:
    void editMimeType();
    void slotCurrentChanged(QListViewItem* i);
  private:
    QListView *lvMimeTypes;
    QPushButton *btnEditMimeType;
};

/**
   @short A Dialog to choose some mimetypes.
   Provides a checkable tree list of mimetypes, with icons and optinally comments and patterns,
   and an (optional) button to display the mimetype kcontrol module.

   @param title The title of the dialog
   @param text A Text to display above the list
   @param selectedMimeTypes A list of mimetype names, theese will be checked in the list if they exist.
   @param editbutton Whether to display the a "Edit Mimetypes" button.
   @param showcomment If this is true, a column displaying the mimetype's comment will be added to the list view.
   @param showpatterns If this is true, a column displaying the mimetype's patterns will be added to the list view.
   Added by anders, dec 19, 2001
*/
class KMimeTypeChooserDlg : public KDialogBase {
  public:
    KMimeTypeChooserDlg( QWidget *parent=0,
                         const QString &caption=QString::null, const QString& text=QString::null,
                         const QStringList &selectedMimeTypes=QStringList(),
                         bool editbutton=true, bool showcomment=true, bool showpatterns=true );
    ~KMimeTypeChooserDlg();
    QStringList mimeTypes();
    QStringList patterns();
  private:
    KMimeTypeChooser *chooser;
};

class HlConfigPage : public Kate::ConfigPage
{
  Q_OBJECT

  public:
    HlConfigPage (QWidget *parent, class KateDocument *doc);
    ~HlConfigPage ();

  private:
    KateDocument *m_doc;
    class HighlightDialogPage *page;
    class HlManager *hlManager;
    HlDataList hlDataList;
    ItemStyleList defaultStyleList;

  public slots:
    void apply ();
    void reload ();   
    void reset () {};
    void defaults () {};
};

class HighlightDialogPage : public QTabWidget
{
    Q_OBJECT
  public:
    HighlightDialogPage(HlManager *, ItemStyleList *, HlDataList *, int hlNumber,
                    QWidget *parent=0, const char *name=0);
    void saveData();

  protected slots:

    void hlChanged(int);
    void hlEdit();
    void hlNew();
    void hlDownload();
    void showMTDlg();
  protected:
    ItemStyleList *defaultItemStyleList;

    void writeback();
    QComboBox *itemCombo, *hlCombo;
    QLineEdit *wildcards;
    QLineEdit *mimetypes;
    StyleListView *lvStyles;

    HlDataList *hlDataList;
    HlData *hlData;
//    ItemData *itemData;
};

class ItemInfo
{
  public:
    ItemInfo():trans_i18n(),length(0){};
    ItemInfo(QString _trans,int _length):trans_i18n(_trans),length(_length){};
    QString trans_i18n;
    int length;
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
