/* 
Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "insanehtmlplugin_le.h"
#include "insanehtmlplugin_le.moc"
#include <kgenericfactory.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kpassivepopup.h>
#include <ktexteditor/document.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

#undef IHP_DEBUG

K_PLUGIN_FACTORY_DECLARATION(InsaneHTMLPluginLEFactory)
K_PLUGIN_FACTORY_DEFINITION(InsaneHTMLPluginLEFactory,
        registerPlugin<InsaneHTMLPluginLE>();
        )
K_EXPORT_PLUGIN(InsaneHTMLPluginLEFactory("ktexteditor_insanehtml_le", "ktexteditor_plugins"))

InsaneHTMLPluginLE::InsaneHTMLPluginLE(QObject *parent, const QList<QVariant> data):
  KTextEditor::Plugin(parent) {
    Q_UNUSED(data);

}


void InsaneHTMLPluginLE::addView (KTextEditor::View *view) {
  m_map.insert(view,new InsaneHTMLPluginLEView(this,view));
}

void InsaneHTMLPluginLE::removeView (KTextEditor::View *view) {
  delete m_map.take(view);
}


InsaneHTMLPluginLEView::InsaneHTMLPluginLEView(QObject* parent,KTextEditor::View* view):
  QObject(parent),KXMLGUIClient(),m_view(view) {
    
    setComponentData(InsaneHTMLPluginLEFactory::componentData());

    KAction *a=actionCollection()->addAction( "tools_insanehtml_le", this,SLOT(expand()) );
    a->setText(i18n("Insane HTML (LE) Expansion"));
    a->setShortcut( Qt::CTRL + Qt::Key_Period );
             
    setXMLFile( "insanehtml_le_ui.rc" );
    
    m_view->insertChildClient(this);

    m_emptyTags<<"br"<<"hr"<<"img"<<"input"<<"meta"<<"link";
    QStringList cfgFiles=KGlobal::dirs()->findAllResources("data", "ktexteditor_insanehtml_le/xhtml.cfg",KStandardDirs::NoDuplicates);
    if (cfgFiles.count()>0) {
      KConfig attribConfig(cfgFiles[0],KConfig::SimpleConfig);
      KConfigGroup group(&attribConfig,"Default Attributes");
      foreach (const QString& tag, group.keyList()) {
        QStringList attribs=group.readEntry(tag,QStringList());
        foreach (const QString& attrib,attribs) {
          m_defaultAttributes.insert(tag,attrib);
        }
      }
    }
}

InsaneHTMLPluginLEView::~InsaneHTMLPluginLEView() {
  m_view->removeChildClient(this);
}


/*
 We expect the cursor to be in them middle or at the end of a tag and don't allow attribute definitions on the right hand side, only quantifiers
*/

int InsaneHTMLPluginLEView::find_region_end(int cursor_x, const QString& line, int *filtercount) {
  int end_x=cursor_x;
  const int len=line.length();
  while (end_x<len) {
    QChar c=line.at(end_x);
    if (c.isLetter() || c.isDigit() || (c==QChar('*')) || (c==QChar('_')) || (c==QChar('-')) || (c==QChar(':')) || (c==QChar('.')) || (c==QChar('#')) || (c==QChar(')'))  )
      end_x++;
    else if (c==QChar('|')) {
      end_x++;
      (*filtercount)++;
    } else
      break;
  }
  int tmp=end_x-1;
  if ((tmp>=0) && (tmp<len))
    if (line.at(tmp)==QChar('>')) return -1;
  return end_x;  
}


/* everything is allowed in the front*/
int InsaneHTMLPluginLEView::find_region_start(int cursor_x, const QString& line, int *filtercount) {
  int len=line.length();
  int start_x=cursor_x;
  bool in_attrib=false;
  bool in_string=false;
  while (start_x>0) {
    int tmp_x=start_x-1;
    if (tmp_x==-1) break;
    QChar c=line.at(tmp_x);
    if (c==QChar('"')) {
      if (!in_attrib) break;
      in_string=!in_string;
      start_x=tmp_x;
      continue;
    }
    if (in_string) {
      start_x=tmp_x;
      continue;
    }
    
    if (c==QChar(']')) {
      in_attrib=true;
      start_x=tmp_x;
      continue;
    }
    
    if (c==QChar('[')) {
      if (in_attrib) {
	  in_attrib=false;
	  start_x=tmp_x;
	  continue;
      } else {
	break;
      }
    }
    
    if (in_attrib) {
      start_x=tmp_x;
      continue;
    }
    
    if ( (c.isSpace() || c==QChar('=')) && (!in_attrib))
      break;
    
    
    if (c.isLetter() || c.isDigit() || (c==QChar('*')) || (c==QChar('_')) ||
      (c==QChar('-')) || (c==QChar(':')) || (c==QChar('.')) || (c==QChar('#')) || 
      (c==QChar('>')) || (c==QChar('$')) || (c==QChar('+')) || (c==QChar('(')) ||
      (c==QChar(')'))) {
      start_x=tmp_x;
      continue;
    }
    
    if (c==QChar('|')) {
      (*filtercount)++;
      start_x=tmp_x;
      continue;
    }
    
    break;
  }
  
  if (in_attrib || in_string) return -1;
  if (start_x>=len) return -1;
  if (start_x>=0) {
    if (!( (line.at(start_x).isLetter()) || (line.at(start_x)==QChar('(')) ) ) return -1;
  }
  return start_x;
}


QString InsaneHTMLPluginLEView::parseIdentifier(const QString& input, int *offset,bool firstDigit) {
  int offset_tmp=*offset;
  int len=input.length();
  QString identifier;
  if (!firstDigit) {
    if (offset_tmp<input.length()) {     
      if (input.at(offset_tmp).isDigit()) return QString();
    }
  }
  while (offset_tmp<len) {
    QChar c=input.at(offset_tmp);
    if (! (
      c.isDigit() || c.isLetter() || (c==QChar(':')) || (c==QChar('_')) || (c==QChar('-')) 
    )) break;
    identifier+=c;
    offset_tmp++;
  }
  *offset=offset_tmp;
  return identifier;
  
}

int InsaneHTMLPluginLEView::parseNumber(const QString& input, int *offset) {
  int offset_tmp=*offset;
  int len=input.length();
  QString number;
  if (offset_tmp<input.length()) {
    if (!input.at(offset_tmp).isDigit()) return 1;
  }
  while (offset_tmp<len) {
    QChar c=input.at(offset_tmp);
    if (! (
      c.isDigit()
    )) break;
    number+=c;
    offset_tmp++;
  }
  *offset=offset_tmp;
  return number.toInt();
  
}

QStringList InsaneHTMLPluginLEView::parse(const QString& input, int offset,int *newOffset) {
  QString tag;
  QStringList classes;
  QStringList sub;
  QStringList relatives;
  QString id;
  bool compound=false;
  QStringList compoundSub;
  QMap<QString,QString> attributes;
  QString attributesString;
  int multiply=1;
  bool error=false;
  tag=parseIdentifier(input,&offset);
  QStringList defAttribs=m_defaultAttributes.values(tag);
  foreach (const QString& defAttr,defAttribs)
    attributes.insert(defAttr,"");
  while (offset<input.length()) {
    QChar c=input.at(offset);
    if (c==QChar(')')) {
      offset++;
      break;
    } else if (c==QChar('(')) {
      offset++;
#ifdef IHP_DEBUG
      KPassivePopup::message(i18n("offset1 %1",offset),m_view);
#endif
      compoundSub=parse(input,offset,&offset);
      compound=true;
#ifdef IHP_DEBUG
      KPassivePopup::message(i18n("offset2 %1",offset),m_view);
#endif
      
    } else if (c==QChar('.')) {
      offset++;
      classes << parseIdentifier(input,&offset);
    } else if (c==QChar('>')) {
      offset++;
      sub=parse(input,offset,&offset);
      break;
    } else if (c==QChar('+')) {
      offset++;
      relatives=parse(input,offset,&offset);
      break;
    } else if (c==QChar('*')) {
      offset++;
      multiply=parseNumber(input,&offset);
    } else if (c==QChar('#')) {
      offset++;
      id=parseIdentifier(input,&offset);
    } else if (c==QChar('[')) {
      offset++;
      while (offset<input.length()) {
        c=input.at(offset);
        if (! ( (c==QChar(' ')) || (c==QChar('\t')) || (c==QChar(',')) ) ) {
          break;
        }
        offset++;
      }
      if (offset>=input.length()) {
        error=true;
        break;
      }
      while (offset<input.length()) {
        if (input.at(offset)==QChar(']')) {
          offset++;
          break;
        }
        QString attr=parseIdentifier(input,&offset);
        QString value;
        if (attr.isEmpty() || offset>=input.length()) {
          error=true;
          break;
        }
        c=input.at(offset);
        if (c==QChar('=')) {
          offset++;
          //PARSE PARAMETER
          if (offset>=input.length()) {
            error=true;
            break;
          }
          if (input.at(offset)==QChar('"')) {
            // parse quoted string
            offset++;
            int stringStart=offset;
            while (offset<input.length()) {
              if (input.at(offset)==QChar('"')) {
                attributes.insert(attr,input.mid(stringStart,offset-stringStart));
                offset++;
                break;
              }
              offset++;
            }
            if (offset>=input.length()) {
              error=true;
              break;
            }
            //skip whitespace and ,
            while (offset<input.length()) {
              c=input.at(offset);
              if (! ( (c==QChar(' ')) || (c==QChar('\t')) || (c==QChar(',')) ) ) {
                break;
              }
              offset++;
            }   
          } else {
            //no "
            value=parseIdentifier(input,&offset,true);
            attributes.insert(attr,value);
          }


          if (offset>=input.length()) {
            error=true;
            break;
          }
          //skip whitespace and ,
          while (offset<input.length()) {
            c=input.at(offset);
            if (! ( (c==QChar(' ')) || (c==QChar('\t')) || (c==QChar(',')) ) ) {
              break;
            }
            offset++;
          }
        } else { //no parameter for attribute specified
          if (offset>=input.length()) {
            error=true;
            break;
          }
          //skip whitespace and ,
          while (offset<input.length()) {
            c=input.at(offset);
            if (! ( (c==QChar(' ')) || (c==QChar('\t')) || (c==QChar(',')) ) ) {
              break;
            }
            offset++;
          }
          attributes.insert(attr,QString());
        }
        //offset++;
      }
      if (error) break;
    } else {
#ifdef IHP_DEBUG
  KPassivePopup::message(i18n("error %1",c),m_view);
#endif
      
      error=true;
      break;
    }
  }
  
  if (newOffset) *newOffset=offset;
  
  if (!error) {
      
      
    for(QMap<QString,QString>::const_iterator it=attributes.constBegin();it!=attributes.constEnd();++it) {
      attributesString+=" "+it.key();
      if (!it.value().isNull()) attributesString+="=\""+it.value()+"\"";
    }
    QStringList result;
    if (!compound) {
      QString idAttrib;
      if (!id.isEmpty()) idAttrib=QString(" id=\"%1\"").arg(id);
      QString classAttrib=classes.join(" ");
      QString classComment;
      if (!classAttrib.isEmpty()) classComment="."+classes.join(" .");
      if (!classAttrib.isEmpty()) classAttrib=QString(" class=\"%1\"").arg(classAttrib);

      if (!sub.isEmpty())
        sub=sub.replaceInStrings(QRegExp("^"),"  ");
      
      for (int i=1;i<=multiply;i++) {
        bool done=false;
        if (!idAttrib.isEmpty()) result<<QString("|c-#%1-c|").arg(id);
        if (!classComment.isEmpty()) result<<QString("|c-%1-c|").arg(classComment);
        if (sub.isEmpty()) {        
          if (m_emptyTags.contains(tag)) {
            result<<QString("<%1%2%3%4/>").arg(tag).arg(idAttrib).arg(classAttrib).arg(attributesString);
            done=true;
          }
        }   
        if (!done){
          if (!sub.isEmpty()) {
            result<<QString("<%1%2%3%4>").arg(tag).arg(idAttrib).arg(classAttrib).arg(attributesString);
            result<<sub;
            result<<QString("</%1>").arg(tag);
          } else
            result<<QString("<%1%2%3%4></%1>").arg(tag).arg(idAttrib).arg(classAttrib).arg(attributesString);
        }
        
        if (!idAttrib.isEmpty()) result<<QString("|c-/#%1-c|").arg(id);
        if (!classComment.isEmpty()) result<<QString("|c-/%1-c|").arg(classComment);
      }
    } else {
      for (int i=1;i<=multiply;i++) {
        QStringList  tmp=compoundSub;
        result<<tmp;
      }
    }
    if (!relatives.isEmpty())
      result<<relatives;
    return result;
  }
  return QStringList();
}

void InsaneHTMLPluginLEView::apply_filter_e(QStringList *lines) {
  lines->replaceInStrings("&","&amp;");
  lines->replaceInStrings("<","&lt;");
  lines->replaceInStrings(">","&gt;");
}

void InsaneHTMLPluginLEView::apply_filter_c(QStringList *lines) {
  lines->replaceInStrings("|c-","<!-- ");
  lines->replaceInStrings("-c|"," -->");
}

void InsaneHTMLPluginLEView::expand() {
 KTextEditor::Cursor c=m_view->cursorPosition();
 QString line=m_view->document()->line(c.line());
 int filtercount=0;
 int start_x=find_region_start(c.column(),line,&filtercount);
 int end_x=find_region_end(c.column(),line,&filtercount);
 if ( (start_x<0) || (end_x<0) || (start_x==end_x) ) {
   KPassivePopup::message(i18n("No valid Insane HTML markup detected at current cursor position"),m_view);
   return;
 }
#ifdef IHP_DEBUG
  KPassivePopup::message(i18n("This looks like valid Insane HTML markup: %1",line.mid(start_x,end_x-start_x)),m_view);
#endif
  QString region_text=line.mid(start_x,end_x-start_x);
  QStringList filters;
  while(filtercount>0) {
    int li=region_text.lastIndexOf("|");
    filters<<region_text.mid(li+1);
    region_text=region_text.left(li);
    filtercount--;
  }
  QStringList result_list=parse(region_text,0);
  while(!filters.isEmpty()) {
    //built_in_filters
    QString filter=filters.takeLast();
    if (filter=="e")
      apply_filter_e(&result_list);
    else if (filter=="c") apply_filter_c(&result_list);
  }
  //remove unwanted comment marks
  QRegExp rcm("\\|c\\-.*\\-c\\|");
  for (int i=result_list.count()-1;i>=0;i--) {
    QString tmp=result_list[i];
    tmp.remove(rcm);
    if (tmp.trimmed().isEmpty())
      result_list.takeAt(i);
    else
        result_list[i]=tmp;
  }
  //prefix with indentation
  QString line_prefix=line.left(start_x);
  line_prefix.replace(QRegExp("\\S")," ");
  for (int i=1;i<result_list.count();i++)
    result_list[i]=line_prefix+result_list[i];
  QString result=result_list.join("\n");
  KTextEditor::Document *doc=m_view->document();
  KTextEditor::Range r(c.line(),start_x,c.line(),end_x);
  doc->replaceText(r,result);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
