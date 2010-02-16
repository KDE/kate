/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "completionmodel.h"
#include "completionmodel.moc"
#include <kdebug.h>
#include <klocale.h>
#include <ktexteditor/templateinterface.h>
#include <ktexteditor/highlightinterface.h>
#include <ktexteditor/view.h>
#include <kmessagebox.h>
#ifdef SNIPPET_EDITOR
  #include <kstandarddirs.h>
  #include <kglobal.h>
  #include <qfileinfo.h>
#endif
#include <qapplication.h>
#include <QDomDocument>
#include <QFile>

namespace KTextEditor {
  namespace CodesnippetsCore {
#ifdef SNIPPET_EDITOR
  namespace Editor {
#endif
    class SnippetCompletionEntry {
      public:
        SnippetCompletionEntry(const QString& _match,const QString& _prefix, const QString& _postfix, const QString& _arguments, const QString& _fillin):
          match(_match),prefix(_prefix),postfix(_postfix),arguments(_arguments),fillin(_fillin){}
        ~SnippetCompletionEntry(){};
        QString match;
        QString prefix;
        QString postfix;
        QString arguments;
        QString fillin;
    };

//BEGIN: CompletionModel

    SnippetCompletionModel::SnippetCompletionModel(const QString &fileType, QStringList &snippetFiles):
      KTextEditor::CodeCompletionModel2((QObject*)0),m_fileType(fileType) {
        foreach(const QString& str, snippetFiles) {
          loadEntries(str);
        }      
    }
    
    SnippetCompletionModel::~SnippetCompletionModel() {
    }
    
    bool SnippetCompletionModel::loadHeader(const QString& filename, QString* name, QString* filetype, QString* authors, QString* license) {
      name->clear();
      filetype->clear();
      authors->clear();
      license->clear();
      
      QFile f(filename);
      QDomDocument doc;
      if (f.open(QIODevice::ReadOnly)) {
        QString errorMsg;
        int line, col;
        bool success;
        success=doc.setContent(&f,&errorMsg,&line,&col);
        f.close();
        if (!success) {
          KMessageBox::error(QApplication::activeWindow(),i18n("<qt>The error <b>%4</b><br /> has been detected in the file %1 at %2/%3</qt>", filename,
              line, col, i18nc("QXml",errorMsg.toUtf8())));
          return false;
        }
      } else {
        KMessageBox::error(QApplication::activeWindow(), i18n("Unable to open %1", filename) );     
        return false;
      }
      QDomElement el=doc.documentElement();
      if (el.tagName()!="snippets") {
        KMessageBox::error(QApplication::activeWindow(), i18n("Not a valid snippet file: %1", filename) );     
        return false;
      }
      *name=el.attribute("name");
      *filetype=el.attribute("filetypes");
      *authors=el.attribute("authors");
      *license=el.attribute("license");
      return true;
    }
    
    
    
    SnippetSelectorModel *SnippetCompletionModel::selectorModel() {
      return new SnippetSelectorModel(this);
    }
    
    QString SnippetCompletionModel::fileType() {
        return m_fileType;
    }
    
    void SnippetCompletionModel::loadEntries(const QString &filename) {

      QFile f(filename);
      QDomDocument doc;
      if (f.open(QIODevice::ReadOnly)) {
        QString errorMsg;
        int line, col;
        bool success;
        success=doc.setContent(&f,&errorMsg,&line,&col);
        f.close();
        if (!success) {
          KMessageBox::error(QApplication::activeWindow(),i18n("<qt>The error <b>%4</b><br /> has been detected in the file %1 at %2/%3</qt>", filename,
              line, col, i18nc("QXml",errorMsg.toUtf8())));
          return;
        }
      } else {
        KMessageBox::error(QApplication::activeWindow(), i18n("Unable to open %1", filename) );     
        return;
      }
      QDomElement el=doc.documentElement();
      if (el.tagName()!="snippets") {
        KMessageBox::error(QApplication::activeWindow(), i18n("Not a valid snippet file: %1", filename) );     
        return;
      }
      QDomNodeList item_nodes=el.childNodes();
      
      QLatin1String match_str("match");
      QLatin1String prefix_str("displayprefix");
      QLatin1String postfix_str("displaypostfix");
      QLatin1String arguments_str("displayarguments");
      QLatin1String fillin_str("fillin");
      
      for (int i_item=0;i_item<item_nodes.count();i_item++) {
        QDomElement item=item_nodes.item(i_item).toElement();
        if (item.tagName()!="item") continue;
          
        QString match;
        QString prefix;
        QString postfix;
        QString arguments;
        QString fillin;
        QDomNodeList data_nodes=item.childNodes();
        for (int i_data=0;i_data<data_nodes.count();i_data++) {
          QDomElement data=data_nodes.item(i_data).toElement();
          QString tagName=data.tagName();
          if (tagName==match_str)
            match=data.text();
          else if (tagName==prefix_str)
            prefix=data.text();
          else if (tagName==postfix_str)
            postfix=data.text();
          else if (tagName==arguments_str)
            arguments=data.text();
          else if (tagName==fillin_str)
            fillin=data.text();
        }
        //kDebug(13040)<<prefix<<match<<postfix<<arguments<<fillin;
        m_entries.append(SnippetCompletionEntry(match,prefix,postfix,arguments,fillin));
        
      }
    }

    
    void SnippetCompletionModel::completionInvoked(KTextEditor::View* view,
      const KTextEditor::Range& range, InvocationType invocationType) {
        //kDebug(13040);
        bool my_mode=true;
        KTextEditor::HighlightInterface *hli=qobject_cast<KTextEditor::HighlightInterface*>(view->document());
        if (hli)
        {          
          kDebug()<<"me: "<<m_fileType<<" current hl in file: "<<hli->highlightingModeAt(range.end());
          if (hli->highlightingModeAt(range.end())!=m_fileType) my_mode=false;
        }
        if (my_mode) {
          if (invocationType==AutomaticInvocation) {
            //KateView *v = qobject_cast<KateView*> (view);
            
            if (range.columnWidth() >= 3) {
              m_matches.clear();
              foreach(const SnippetCompletionEntry& entry,m_entries) {
                m_matches.append(&entry);
              }
              reset();
              //kDebug(13040)<<"matches:"<<m_matches.count();
            } else {
              m_matches.clear();
              reset();
            }
          } else {
            m_matches.clear();
              foreach(const SnippetCompletionEntry& entry,m_entries) {
                m_matches.append(&entry);
              }
              reset();
          }
        } else {
            m_matches.clear();
            reset();
        }
    }
    
    QVariant SnippetCompletionModel::data( const QModelIndex & index, int role) const {
      //kDebug(13040);
      if (role == KTextEditor::CodeCompletionModel::InheritanceDepth)
        return 1;
      if (!index.parent().isValid()) {
        if (role==Qt::DisplayRole) return i18n("Snippets");
        if (role==GroupRole) return Qt::DisplayRole;
        return QVariant();
      }
      if (role == Qt::DisplayRole) {
        if (index.column() == KTextEditor::CodeCompletionModel::Name ) {
          //kDebug(13040)<<"returning: "<<m_matches[index.row()]->match;
          return m_matches[index.row()]->match;
        } else if (index.column() == KTextEditor::CodeCompletionModel::Prefix ) {
          const QString& tmp=m_matches[index.row()]->prefix;
          if (!tmp.isEmpty()) return tmp;
        } else if (index.column() == KTextEditor::CodeCompletionModel::Postfix ) {
          const QString& tmp=m_matches[index.row()]->postfix;
          if (!tmp.isEmpty()) return tmp;
        } else if (index.column() == KTextEditor::CodeCompletionModel::Arguments ) {
          const QString& tmp=m_matches[index.row()]->arguments;
          if (!tmp.isEmpty()) return tmp;
        }
      }
      return QVariant();
    }
    
    QModelIndex SnippetCompletionModel::parent(const QModelIndex& index) const {
      if (index.internalId())
        return createIndex(0, 0, 0);
      else
        return QModelIndex();
    }

    QModelIndex SnippetCompletionModel::index(int row, int column, const QModelIndex& parent) const {
      if (!parent.isValid()) {
        if (row == 0)
          return createIndex(row, column, 0); //header  index
        else
          return QModelIndex();
      } else if (parent.parent().isValid()) //we only have header and children, no subheaders
        return QModelIndex();

      if (row < 0 || row >= m_matches.count() || column < 0 || column >= ColumnCount )
        return QModelIndex();

      return createIndex(row, column, 1); // normal item index
    }

    int SnippetCompletionModel::rowCount (const QModelIndex & parent) const {
      if (!parent.isValid() && !m_matches.isEmpty())
        return 1; //one toplevel node (group header)
      else if(parent.parent().isValid())
        return 0; //we don't have sub children
      else
        return m_matches.count(); // only the children
    }
    
    void SnippetCompletionModel::executeCompletionItem2(KTextEditor::Document* document,
      const KTextEditor::Range& word, const QModelIndex& index) const {
      //document->replaceText(word, m_matches[index.row()]->fillin);
      document->removeText(word);
      KTextEditor::View *view=document->activeView();
      if (!view) return;
      KTextEditor::TemplateInterface *ti=qobject_cast<KTextEditor::TemplateInterface*>(view);
      if (ti)
        ti->insertTemplateText (word.start(), m_matches[index.row()]->fillin, QMap<QString,QString> ());
      else {
        view->setCursorPosition(word.start());
        view->insertText (m_matches[index.row()]->fillin);
      }
    }  

  #ifdef SNIPPET_EDITOR
    static void addAndCreateElement(QDomDocument& doc, QDomElement& item, const QString& name, const QString &content)
    {
      QDomElement element=doc.createElement(name);
      element.appendChild(doc.createTextNode(content));
      item.appendChild(element);
    }

    bool SnippetCompletionModel::save(const QString& filename, const QString& name, const QString& license, const QString& filetype, const QString& authors)
    {
  /*
      <snippets name="Testsnippets" filetype="*" authors="Joseph Wenninger" license="BSD">
        <item>
          <displayprefix>prefix</displayprefix>
          <match>test1</match>
          <displaypostfix>postfix</displaypostfix>
          <displayarguments>(param1, param2)</displayarguments>
          <fillin>This is a test</fillin>
        </item>
        <item>
          <match>testtemplate</match>
          <fillin>This is a test ${WHAT} template</fillin>
        </item>
      </snippets>
  */
      QDomDocument doc;
      QDomElement root=doc.createElement("snippets");
      root.setAttribute("name",name);
      root.setAttribute("filetypes",filetype);
      root.setAttribute("authors",authors);
      root.setAttribute("license",license);
      doc.appendChild(root);
      foreach(const SnippetCompletionEntry& entry, m_entries) {
        QDomElement item=doc.createElement("item");
        addAndCreateElement(doc,item,"displayprefix",entry.prefix);
        addAndCreateElement(doc,item,"match",entry.match);
        addAndCreateElement(doc,item,"displaypostfix",entry.postfix);
        addAndCreateElement(doc,item,"displayarguments",entry.arguments);
        addAndCreateElement(doc,item,"fillin",entry.fillin);
        root.appendChild(item);
      }
      //KMessageBox::information(0,doc.toString());
      QFileInfo fi(filename);
      QString outname=KGlobal::dirs()->locateLocal( "data", "ktexteditor_snippets/data/"+fi.fileName());
      if (filename!=outname) {
        QFileInfo fiout(outname);
  //      if (fiout.exists()) {
  // there could be cases that new new name clashes with a global file, but I guess it is not that often.
              bool ok=false;
              for (int i=0;i<1000;i++) {
                outname=KGlobal::dirs()->locateLocal( "data", "ktexteditor_snippets/data/"+QString("%1_").arg(i)+fi.fileName());
                QFileInfo fiout1(outname);
                if (!fiout1.exists()) {ok=true;break;}
              }
              if (!ok) {
                  KMessageBox::error(0,i18n("You have edited a data file not located in your personal data directory, but a suitable filename could not be generated for storing a clone of the file within your personal data directory."));
                  return false;
              } else KMessageBox::information(0,i18n("You have edited a data file not located in your personal data directory; as such, a renamed clone of the original data file has been created within your personal data directory."));
  //       } else
  //         KMessageBox::information(0,i18n("You have edited a data file not located in your personal data directory, creating a clone of the data file in your personal data directory"));
      }
      QFile outfile(outname);
      if (!outfile.open(QIODevice::WriteOnly)) {
        KMessageBox::error(0,i18n("Output file '%1' could not be opened for writing",outname));
        return false;
      }
      outfile.write(doc.toByteArray());
      outfile.close();
      return true;
    }
    
  QString SnippetCompletionModel::createNew(const QString& name, const QString& license,const QString& authors) {
      QDomDocument doc;
      QDomElement root=doc.createElement("snippets");
      root.setAttribute("name",name);
      root.setAttribute("filetypes","*");
      root.setAttribute("authors",authors);
      root.setAttribute("license",license);
      doc.appendChild(root);
      QString fileName=QUrl::toPercentEncoding(name)+QString(".xml");
      QString outname=KGlobal::dirs()->locateLocal( "data", "ktexteditor_snippets/data/"+fileName);
  #ifdef __GNUC__
  #warning add handling of conflicts with global names
  #endif
      QFileInfo fiout(outname);
      if (fiout.exists()) {
        bool ok=false;
        for (int i=0;i<1000;i++) {
          outname=KGlobal::dirs()->locateLocal( "data", "ktexteditor_snippets/data/"+QString("%1_").arg(i)+fileName);
          QFileInfo fiout1(outname);
          if (!fiout1.exists()) {ok=true;break;}
        }
        if (!ok) {
          KMessageBox::error(0,i18n("It was not possible to create a unique file name for the given snippet collection name"));
          return QString();
        }
      }
      QFile outfile(outname);
      if (!outfile.open(QIODevice::WriteOnly)) {
        KMessageBox::error(0,i18n("Output file '%1' could not be opened for writing",outname));
        return QString();
      }
      outfile.write(doc.toByteArray());
      outfile.close();
      
      return outname;

  }
    
  #endif  
  
//END: CompletionModel

//BEGIN: SelectorModel    
    SnippetSelectorModel::SnippetSelectorModel(SnippetCompletionModel* cmodel):
      QAbstractItemModel(cmodel),m_cmodel(cmodel)
    {
      kDebug(13040);
    }
    
    
    SnippetSelectorModel::~SnippetSelectorModel()
    {
      kDebug(13040);
    }
    
    QString SnippetSelectorModel::fileType() {
        return m_cmodel->fileType();
    }

    int SnippetSelectorModel::rowCount(const QModelIndex& parent) const
    {
      if (parent.isValid()) return 0;
      return m_cmodel->m_entries.count();
    }
    
    QModelIndex SnippetSelectorModel::index ( int row, int column, const QModelIndex & parent ) const
    {
      if (parent.isValid()) return QModelIndex();
      if (column!=0) return QModelIndex();
      if ((row>=0) && (row<m_cmodel->m_entries.count()))
        return createIndex(row,column);
      return QModelIndex();
    }
    
    QVariant SnippetSelectorModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid()) return QVariant();
        switch (role) {
          case Qt::DisplayRole:
            return m_cmodel->m_entries[index.row()].match;
            break;
          case FillInRole:
            return m_cmodel->m_entries[index.row()].fillin;
            break;
  #ifdef SNIPPET_EDITOR
          case PrefixRole:
            return m_cmodel->m_entries[index.row()].prefix;
            break;
          case MatchRole:
            return m_cmodel->m_entries[index.row()].match;
            break;
          case PostfixRole:
            return m_cmodel->m_entries[index.row()].postfix;
            break;
          case ArgumentsRole:
            return m_cmodel->m_entries[index.row()].arguments;
            break;
  #endif          
          default:
            break;
        }
        return QVariant();
    }

    QVariant SnippetSelectorModel::headerData ( int section, Qt::Orientation orientation, int role) const
    {
      if (orientation==Qt::Vertical) return QVariant();
      if (section!=0) return QVariant();
      if (role!=Qt::DisplayRole) return QVariant();
      return QString("Snippet");
    }
    
  #ifdef SNIPPET_EDITOR 
    bool SnippetSelectorModel::setData ( const QModelIndex & index, const QVariant & value, int role) {
      if (!index.isValid()) return false;
      switch (role) {
          case FillInRole:
            m_cmodel->m_entries[index.row()].fillin=value.toString();
            break;
          case PrefixRole:
            m_cmodel->m_entries[index.row()].prefix=value.toString();
            break;
          case MatchRole:
            m_cmodel->m_entries[index.row()].match=value.toString();
            dataChanged(index,index);
            break;
          case PostfixRole:
            m_cmodel->m_entries[index.row()].postfix=value.toString();
            break;
          case ArgumentsRole:
            m_cmodel->m_entries[index.row()].arguments=value.toString();
            break;
        default:
          return QAbstractItemModel::setData(index,value,role);
      }
      return true;
    }
    
    bool SnippetSelectorModel::removeRows ( int row, int count, const QModelIndex & parent) {
      if (parent.isValid()) return false;
      if (count!=1) return false;
      if (row>=rowCount(QModelIndex())) return false;
      beginRemoveRows(parent,row,row);
      m_cmodel->m_entries.removeAt(row);
      endRemoveRows();
      return true;
    }
    
    QModelIndex SnippetSelectorModel::newItem() {
      int new_row=m_cmodel->m_entries.count();
      beginInsertRows(QModelIndex(),new_row,new_row);
      m_cmodel->m_entries.append(SnippetCompletionEntry(i18n("New Snippet"),QString(),QString(),QString(),QString()));
      endInsertRows();
      return createIndex(new_row,0);
    }
  #endif
//END: SelectorModel

//BEGIN: CategorizedSnippetModel
        CategorizedSnippetModel::CategorizedSnippetModel(const QList<SnippetSelectorModel*>& models):
          QAbstractItemModel(),m_models(models) {
            foreach(SnippetSelectorModel *model,m_models) {
              connect(model, SIGNAL(destroyed(QObject*)),this,SLOT(subDestroyed(QObject*)));
            }
        }        
        
        CategorizedSnippetModel::~CategorizedSnippetModel() {
          qDeleteAll(m_models);
        }
        
        void CategorizedSnippetModel::subDestroyed(QObject* obj) {
          int i=m_models.indexOf((SnippetSelectorModel*)(obj));
          if (i==-1) return;
          m_models.removeAt(i);
          reset();
        }
        
        int CategorizedSnippetModel::rowCount(const QModelIndex& parent) const {
          if (!parent.isValid())
          {
            return m_models.count();
          } else {
//             kDebug()<<"Requested rowCount with valid parent";
//             kDebug()<<"parent.internalPointer"<<parent.internalPointer();
//             kDebug()<<"parent.internalId"<<parent.internalId();
            if (!parent.internalPointer()) {
              SnippetSelectorModel* m=m_models[parent.row()];
//               kDebug()<<"Returning child rowCount:"<<m->rowCount(QModelIndex());
              return m->rowCount(QModelIndex());
            }
          }
          return 0;          
        }
        
        QModelIndex CategorizedSnippetModel::index ( int row, int column, const QModelIndex & parent) const
        {
          if (row==-1) {
//               kDebug()<<"row==-1 -> returning invalid index"<<endl;
              return QModelIndex();
          }
          if (parent.isValid()) {
//             kDebug()<<"Requested index for valid parent"<<endl;
            if ((column==0) && ((row>=0) && (row<m_models[parent.row()]->rowCount(QModelIndex()))))
            {
              QModelIndex idx=createIndex(row,column,m_models[parent.row()]);
              kDebug()<<idx;
              return idx;
            }
          } else  {
//               kDebug()<<"Requested index for invalid parent, row="<<row;
              if ((row>=0) && (row<m_models.count()) && (column==0))
                return createIndex(row,column);                
          }
          return QModelIndex();
        }
        
        QVariant CategorizedSnippetModel::data(const QModelIndex &index, int role) const {          
          if (!index.isValid()) 
          {
//             kDebug()<<"invoked with invalid index";
            return QVariant();
          }
          if (!index.internalPointer()) {
//             kDebug()<<"invoked for entry with no internalPointer: row:"<<index.row()<<endl;
            if (role==Qt::DisplayRole)
              return m_models[index.row()]->fileType();
          } else {
//             kDebug()<<"invoked for valid parent";
            SnippetSelectorModel *m=(SnippetSelectorModel*)(index.internalPointer());
            Q_ASSERT(m);
            return m->data(m->index(index.row(),index.column(),QModelIndex()),role);
          }
          return QVariant();
        }
        
        QModelIndex CategorizedSnippetModel::parent ( const QModelIndex & index) const {
//           kDebug()<<index<<" valid?"<<index.isValid()<<" internalPointer:"<<index.internalPointer();
          if (!index.isValid()) return QModelIndex();
          if (index.internalPointer())
          {
            QModelIndex idx=createIndex(m_models.indexOf((SnippetSelectorModel*)(index.internalPointer())),0);
//             kDebug()<<"==========>"<<idx;
            return idx;
          }          
          return QModelIndex();
        }
        
        QVariant CategorizedSnippetModel::headerData ( int section, Qt::Orientation orientation, int role) const {
          if (orientation==Qt::Vertical) return QVariant();
          if (section!=0) return QVariant();
          if (role!=Qt::DisplayRole) return QVariant();
          return QString("Snippet");
        }
        
//END: CategorizedSnippetModel

#ifdef SNIPPET_EDITOR
  }
#endif
  }
}
// kate: space-indent on; indent-width 2; replace-tabs on;
