/* This file is part of the KDE libraries
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2003, 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
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

//BEGIN INCLUDES
#include "katehighlight.h"

#include "katehighlighthelpers.h"
#include "katetextline.h"
#include "katedocument.h"
#include "katesyntaxdocument.h"
#include "katerenderer.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "kateconfig.h"
#include "kateextendedattribute.h"
#include "katedefaultcolors.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <kcomponentdata.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kmenu.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include <ktexteditor/highlightinterface.h>

#include <QtCore/QSet>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QVarLengthArray>
//END

//BEGIN defines
// x is a QString. if x is "true" or "1" this expression returns "true"
#define IS_TRUE(x) x.toLower() == QLatin1String("true") || x.toInt() == 1
//END defines

//BEGIN STATICS
namespace {
const QString stdDeliminator = QString (" \t.():!+,-<=>%&*/;?[]^{|}~\\");

QColor toColor(const QString& configEntry)
{
  // note: color is stored in hex format in the config files, i.e.: ffafafae
  // it's not using the leading hash though so the QString ctor taking a QStirng
  // fails.
  return QColor(QRgb(configEntry.toUInt(0, 16)));
}

}
//END

//BEGIN KateHighlighting
KateHighlighting::KateHighlighting(const KateSyntaxModeListItem *def) : refCount(0)
{
  errorsAndWarnings = "";
  building=false;
  noHl = false;
  m_foldingIndentationSensitive = false;
  folding=false;

  if (def == 0)
  {
    noHl = true;
    iName = "None"; // not translated internal name (for config and more)
    iNameTranslated = i18nc("Syntax highlighting", "None"); // user visible name
    iSection = "";
    makeNoneContext();
  }
  else
  {
    iName = def->name;
    iNameTranslated = def->nameTranslated;
    iSection = def->section;
    iHidden = def->hidden;
    identifier = def->identifier;
    iVersion=def->version;
    iStyle = def->style;
    iAuthor=def->author;
    iLicense=def->license;
  }

   deliminator = stdDeliminator;
}

KateHighlighting::~KateHighlighting()
{
  // cleanup ;)
  cleanup ();

  qDeleteAll(m_additionalData);
}

void KateHighlighting::makeNoneContext()
{
  iHidden = false;
  m_additionalData.insert( "none", new HighlightPropertyBag );
  m_additionalData["none"]->deliminator = stdDeliminator;
  m_additionalData["none"]->wordWrapDeliminator = stdDeliminator;
  m_hlIndex[0] = "none";
  m_ctxIndex[0]= "none";
  m_contexts.push_back(new KateHlContext("None",
                                         0,
                                         KateHlContextModification(),
                                         false,
                                         KateHlContextModification(),
                                         false,
                                         false,
                                         false,
                                         KateHlContextModification()));
}

void KateHighlighting::cleanup ()
{
  qDeleteAll (m_contexts);
  m_contexts.clear ();

  qDeleteAll (m_hlItemCleanupList);
  m_hlItemCleanupList.clear ();

  m_attributeArrays.clear ();

  internalIDList.clear();
}

KateHlContext *KateHighlighting::generateContextStack (Kate::TextLineData::ContextStack &contextStack,
                                                       KateHlContextModification modification,
                                                       int &indexLastContextPreviousLine)
{
  while (true)
  {
    switch (modification.type)
    {
      /**
       * stay, do nothing, just return the last context
       * in the stack or 0
       */
      case KateHlContextModification::doNothing:
        return contextNum (contextStack.isEmpty() ? 0 : contextStack.last());

      /**
       * just add a new context to the stack
       * and return this one
       */
      case KateHlContextModification::doPush:
        contextStack.append (modification.newContext);
        return contextNum (modification.newContext);

      /**
       * pop some contexts + add a new one afterwards, immediate....
       */
      case KateHlContextModification::doPopsAndPush:
        // resize stack
        contextStack.resize ((modification.pops >= contextStack.size()) ? 0 : (contextStack.size() - modification.pops));

        // push imediate the new context....
        // don't handle the previous line stuff at all....
        // ### TODO ### think about this
        contextStack.append (modification.newContext);
        return contextNum (modification.newContext);

      /**
       * do only pops...
       */
      default:
      {
        // resize stack
        contextStack.resize ((modification.pops >= contextStack.size()) ? 0 : (contextStack.size() - modification.pops));

        // handling of context of previous line....
        if (indexLastContextPreviousLine >= (contextStack.size()-1))
        {
          // set new index, if stack is empty, this is -1, done for eternity...
          indexLastContextPreviousLine = contextStack.size() - 1;

          // stack already empty, nothing to do...
          if ( contextStack.isEmpty() )
            return contextNum (0);

          KateHlContext *c = contextNum(contextStack.last());

          // this must be a valid context, or our context stack is borked....
          Q_ASSERT (c);

          // handle line end context as new modificationContext
          modification = c->lineEndContext;
          continue;
        }

        return contextNum (contextStack.isEmpty() ? 0 : contextStack.last());
      }
    }
  }

  // should never be reached
  Q_ASSERT (false);

  return contextNum (0);
}

/**
 * Creates a new dynamic context or reuse an old one if it has already been created.
 */
int KateHighlighting::makeDynamicContext(KateHlContext *model, const QStringList *args)
{
  QPair<KateHlContext *, QString> key(model, args->front());
  short value;

  if (dynamicCtxs.contains(key))
    value = dynamicCtxs[key];
  else
  {
#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010) << "new stuff: " << startctx;
#endif

    KateHlContext *newctx = model->clone(args);

    m_contexts.push_back (newctx);

    value = startctx++;
    dynamicCtxs[key] = value;
    KateHlManager::self()->incDynamicCtxs();
  }

  // kDebug(13010) << "Dynamic context: using context #" << value << " (for model " << model << " with args " << *args << ")";

  return value;
}

/**
 * Drop all dynamic contexts. Shall be called with extreme care, and shall be immediately
 * followed by a full HL invalidation.
 */
void KateHighlighting::dropDynamicContexts()
{
  if (refCount == 0)  // unused highlighting - nothing to drop
    return;

  if (noHl)  // "normal texts" highlighting - no context list
    return;

  qDeleteAll(m_contexts.begin()+base_startctx, m_contexts.end());  // delete dynamic contexts (after base_startctx)
  m_contexts.resize (base_startctx);

  dynamicCtxs.clear();
  startctx = base_startctx;
}

void KateHighlighting::doHighlight ( const Kate::TextLineData *_prevLine,
                                     Kate::TextLineData *textLine,
                                     const Kate::TextLineData *nextLine,
                                     bool &ctxChanged,
                                     int tabWidth,
                                     QVector<ContextChange>* contextChanges)
{
  if (!textLine)
    return;

  // in all cases, remove old hl, or we will grow to infinite ;)
  textLine->clearAttributes ();
  
  // reset folding start
  textLine->clearMarkedAsFoldingStart ();

  // no hl set, nothing to do more than the above cleaning ;)
  if (noHl)
    return;

  const bool firstLine = (_prevLine == 0);
  const Kate::TextLine dummy = Kate::TextLine (new Kate::TextLineData ());
  const Kate::TextLineData * prevLine = firstLine ? dummy.data() : _prevLine;

  int previousLine = -1;
  KateHlContext *context;

  // duplicate the ctx stack, only once !
  Kate::TextLineData::ContextStack ctx (prevLine->contextStack());

  if (ctx.isEmpty())
  {
    // If the stack is empty, we assume to be in Context 0 (Normal)
    if (firstLine) {
      context = contextNum(0);
    } else {
      context = generateContextStack(ctx, contextNum(0)->lineEndContext, previousLine); //get stack ID to use
    }
  }
  else
  {
    //kDebug(13010) << "\t\tctxNum = " << ctxNum << " contextList[ctxNum] = " << contextList[ctxNum]; // ellis

    //if (lineContinue)   kDebug(13010)<<QString("The old context should be %1").arg((int)ctxNum);
    context = contextNum(ctx.last());

    //kDebug(13010)<<"test1-2-1-text2";

    previousLine = ctx.size()-1; //position of the last context ID of th previous line within the stack

    // hl continue set or not ???
    if (prevLine->hlLineContinue())
      previousLine--;
    else
      context = generateContextStack(ctx, context->lineEndContext, previousLine); //get stack ID to use

    //kDebug(13010)<<"test1-2-1-text4";

    //if (lineContinue)   kDebug(13010)<<QString("The new context is %1").arg((int)ctxNum);
  }

  // text, for programming convenience :)
  QChar lastChar = ' ';
  const QString& text = textLine->string();
  const int len = textLine->length();

  // calc at which char the first char occurs, set it to length of line if never
  const int firstChar = textLine->firstChar();
  const int startNonSpace = (firstChar == -1) ? len : firstChar;

  // last found item
  KateHlItem *item = 0;

  // loop over the line, offset gives current offset
  int offset = 0;

  KateHighlighting::HighlightPropertyBag* additionalData = m_additionalData[context->hlId];
  KateHlContext* oldContext = context;

  // optimization: list of highlighting items that need their cache reset
  static QVarLengthArray<KateHlItem*> cachingItems;

  // catch empty lines
  if (len == 0) {
    // regenerate context stack if needed
    if (context->emptyLineContext)
        context = generateContextStack (ctx, context->emptyLineContextModification, previousLine);
  } else {
    /**
     * check if the folding begin/ends are balanced!
     * constructed on demand!
     */
    QHash<short, int> *foldingStartToCount = 0;
    
    /**
     * loop over line content!
     */
    QChar lastDelimChar = 0;
    KateHlContext* previous = context;
    while (offset < len)
    {
      // If requested (happens from completion), return where context changes occur.
      if ( contextChanges && ( offset == 0 || context != previous ) )
      {
        previous = context;
        const ContextChange change = {context, offset};
        contextChanges->append(change);
      }
      bool anItemMatched = false;
      bool customStartEnableDetermined = false;

      foreach (item, context->items)
      {
        // does we only match if we are firstNonSpace?
        if (item->firstNonSpace && (offset > startNonSpace))
          continue;

        // have we a column specified? if yes, only match at this column
        if ((item->column != -1) && (item->column != offset))
          continue;

        if (!item->alwaysStartEnable)
        {
          if (item->customStartEnable)
          {
              if ( oldContext != context ) {
                oldContext = context;
                additionalData = m_additionalData[oldContext->hlId];
              }
              if (customStartEnableDetermined || additionalData->deliminator.contains(lastChar))
                customStartEnableDetermined = true;
              else
                continue;
          }
          else
          {
            if (lastDelimChar == lastChar) {
            } else if ( stdDeliminator.contains(lastChar) ) {
              lastDelimChar = lastChar;
            } else {
              continue;
            }
          }
        }

        int offset2 = item->checkHgl(text, offset, len-offset);
        if ( item->haveCache && !item->cachingHandled ) {
          cachingItems.append(item);
          item->cachingHandled = true;
        }

        if (offset2 <= offset)
          continue;

        // dominik: on lookAhead, do not preocess any data by fixing offset2
        if (item->lookAhead) {
          offset2 = offset;
        } else {
          // make sure the rule does not violate the text line length
          if (offset2 > len)
            offset2 = len;
        }

        // BUG 144599: Ignore a context change that would push the same context
        // without eating anything... this would be an infinite loop!
        if ( item->lookAhead && ( item->ctx.pops < 2 && item->ctx.newContext == ( ctx.isEmpty() ? 0 : ctx.last() ) ) )
          continue;

        // regenerate context stack if needed
        context = generateContextStack (ctx, item->ctx, previousLine);

        // dynamic context: substitute the model with an 'instance'
        if (context->dynamic)
        {
          // try to retrieve captures from regexp
          QStringList captures;
          item->capturedTexts (captures);
          if (!captures.empty())
          {
            // Replace the top of the stack and the current context
            int newctx = makeDynamicContext(context, &captures);
            if (ctx.size() > 0)
              ctx[ctx.size() - 1] = newctx;

            context = contextNum(newctx);
          }
        }
          
        // handle folding end or begin
        if (item->region || item->region2) {
          /**
           * for each end region, decrement counter for that type, erase if count reaches 0!
           */
          if (item->region2 && foldingStartToCount) {
            QHash<short, int>::iterator end = foldingStartToCount->find (-item->region2);
            if (end != foldingStartToCount->end()) {
              if (end.value() > 1)
                --(end.value());
              else
                foldingStartToCount->erase (end);
            }
          }

          /**
           * increment counter for each begin region!
           */
          if (item->region) {
            // construct on demand!
            if (!foldingStartToCount)
              foldingStartToCount = new QHash<short, int> ();

            ++(*foldingStartToCount)[item->region];
          }
        }

        // even set attributes or end of region! ;)
        int attribute = item->onlyConsume ? context->attr : item->attr;
        if ((attribute > 0 && !item->lookAhead) || item->region2)
          textLine->addAttribute (Kate::TextLineData::Attribute (offset, offset2-offset, attribute, item->region2));

        // create 0 length attribute for begin of region, if any!
        if (item->region)
          textLine->addAttribute (Kate::TextLineData::Attribute (offset2, 0, attribute, item->region));

        // only process, if lookAhead is false
        if (!item->lookAhead) {
          offset = offset2;
          lastChar = text[offset-1];
        }

        anItemMatched = true;
        break;
      }

      // something matched, continue loop
      if (anItemMatched)
        continue;

      item = 0;

      // nothing found: set attribute of one char
      // anders: unless this context does not want that!
      if ( context->fallthrough )
      {
      // set context to context->ftctx.
        context=generateContextStack(ctx, context->ftctx, previousLine);  //regenerate context stack

      //kDebug(13010)<<"context num after fallthrough at col "<<z<<": "<<ctxNum;
      // the next is necessary, as otherwise keyword (or anything using the std delimitor check)
      // immediately after fallthrough fails. Is it bad?
      // jowenn, can you come up with a nicer way to do this?
      /*  if (offset)
          lastChar = text[offset - 1];
        else
          lastChar = '\\';*/
        continue;
      }
      else
      {
        // set attribute if any
        if (context->attr > 0)
          textLine->addAttribute (Kate::TextLineData::Attribute (offset, 1, context->attr, 0));

        lastChar = text[offset];
        offset++;
      }
    }
    
    /**
     * check if folding is not balanced and we have more starts then ends
     * then this line is a possible folding start!
     */
    if (foldingStartToCount) {
      /**
       * possible folding start, if imbalanced, aka hash not empty!
       */
      if (!foldingStartToCount->isEmpty())
        textLine->markAsFoldingStartAttribute ();
      
      /**
       * kill hash
       */
      delete foldingStartToCount;
      foldingStartToCount = 0;
    }
  }
  
  /**
   * has the context stack changed?
   */
  if ((ctxChanged = (ctx != textLine->contextStack()))) {
    /**
     * try to share the simple stack that contains only 0
     */
    static const Kate::TextLineData::ContextStack onlyDefaulContext (1, 0);
    if (ctx == onlyDefaulContext)
      textLine->setContextStack(onlyDefaulContext);
    
    /**
     * next try: try to share data with last line
     */
    else if (ctx == prevLine->contextStack())
      textLine->setContextStack(prevLine->contextStack());
    
    /**
     * ok, really use newly constructed stack!
     */
    else
      textLine->setContextStack(ctx);
  }

  // write hl continue flag
  textLine->setHlLineContinue (item && item->lineContinue());

  // check for indentation based folding
  if (m_foldingIndentationSensitive && (tabWidth > 0) && !textLine->markedAsFoldingStartAttribute ()) {
    bool skipIndentationBasedFolding = false;
    for(int i = ctx.size() - 1; i >= 0; --i) {
      if (contextNum(ctx[i])->noIndentationBasedFolding) {
        skipIndentationBasedFolding = true;
        break;
      }
    }
    
    /**
     * compute if we increase indentation in next line
     */
    if (!skipIndentationBasedFolding && !isEmptyLine (textLine) && !isEmptyLine (nextLine)
        && (textLine->indentDepth (tabWidth) < nextLine->indentDepth (tabWidth))) 
       textLine->markAsFoldingStartIndentation ();
  }
  
  // invalidate caches
  for ( int i = 0; i < cachingItems.size(); ++i) {
    cachingItems[i]->cachingHandled = false;
    cachingItems[i]->haveCache = false;
  }
  cachingItems.clear();
}

void KateHighlighting::getKateExtendedAttributeList (const QString &schema, QList<KateExtendedAttribute::Ptr> &list, KConfig* cfg)
{
  KConfigGroup config(cfg?cfg:KateHlManager::self()->getKConfig(),
                      "Highlighting " + iName + " - Schema " + schema);

  list.clear();
  createKateExtendedAttribute(list);

  foreach (KateExtendedAttribute::Ptr p, list)
  {
    Q_ASSERT(p);

    QStringList s = config.readEntry(p->name(), QStringList());

//    kDebug(13010)<<p->name<<s.count();
    if (s.count()>0)
    {

      while(s.count()<10) s<<"";
      QString name = p->name();
      bool spellCheck = p->performSpellchecking();
      p->clear();
      p->setName(name);
      p->setPerformSpellchecking(spellCheck);

      QString tmp=s[0]; if (!tmp.isEmpty()) p->setDefaultStyleIndex(tmp.toInt());

      tmp=s[1]; if (!tmp.isEmpty()) p->setForeground(toColor(tmp));

      tmp=s[2]; if (!tmp.isEmpty()) p->setSelectedForeground(toColor(tmp));

      tmp=s[3]; if (!tmp.isEmpty()) p->setFontBold(tmp!="0");

      tmp=s[4]; if (!tmp.isEmpty()) p->setFontItalic(tmp!="0");

      tmp=s[5]; if (!tmp.isEmpty()) p->setFontStrikeOut(tmp!="0");

      tmp=s[6]; if (!tmp.isEmpty()) p->setFontUnderline(tmp!="0");

      tmp=s[7]; if (!tmp.isEmpty()) p->setBackground(toColor(tmp));

      tmp=s[8]; if (!tmp.isEmpty()) p->setSelectedBackground(toColor(tmp));

      tmp=s[9]; if (!tmp.isEmpty() && tmp!=QLatin1String("---")) p->setFontFamily(tmp);

    }
  }
}

void KateHighlighting::getKateExtendedAttributeListCopy( const QString &schema, QList< KateExtendedAttribute::Ptr >& list, KConfig* cfg )
{
  QList<KateExtendedAttribute::Ptr> attributes;
  getKateExtendedAttributeList(schema, attributes,cfg);

  list.clear();

  foreach (const KateExtendedAttribute::Ptr &attribute, attributes)
    list.append(KateExtendedAttribute::Ptr(new KateExtendedAttribute(*attribute.data())));
}


/**
 * Saves the attribute definitions to the config file.
 *
 * @param schema The id of the schema group to save
 * @param list QList<KateExtendedAttribute::Ptr> containing the data to be used
 */
void KateHighlighting::setKateExtendedAttributeList(const QString &schema, QList<KateExtendedAttribute::Ptr> &list, KConfig *cfg, bool writeDefaultsToo)
{ 
  KConfigGroup config(cfg?cfg:KateHlManager::self()->getKConfig(),
                      "Highlighting " + iName + " - Schema "+ schema);

  QStringList settings;

  KateAttributeList defList;
  KateHlManager::self()->getDefaults(schema, defList);
  
  foreach (const KateExtendedAttribute::Ptr& p, list)
  {
    Q_ASSERT(p);

    settings.clear();
    uint defStyle=p->defaultStyleIndex();
    KTextEditor::Attribute::Ptr a(defList[defStyle]);
    settings<<QString::number(p->defaultStyleIndex(),10);
    settings<<(p->hasProperty(QTextFormat::ForegroundBrush)?QString::number(p->foreground().color().rgb(),16):(writeDefaultsToo?QString::number(a->foreground().color().rgb(),16):""));
    settings<<(p->hasProperty(KTextEditor::Attribute::SelectedForeground)?QString::number(p->selectedForeground().color().rgb(),16):(writeDefaultsToo?QString::number(a->selectedForeground().color().rgb(),16):""));
    settings<<(p->hasProperty(QTextFormat::FontWeight)?(p->fontBold()?"1":"0"):(writeDefaultsToo?(a->fontBold()?"1":"0"):""));
    settings<<(p->hasProperty(QTextFormat::FontItalic)?(p->fontItalic()?"1":"0"):(writeDefaultsToo?(a->fontItalic()?"1":"0"):""));
    settings<<(p->hasProperty(QTextFormat::FontStrikeOut)?(p->fontStrikeOut()?"1":"0"):(writeDefaultsToo?(a->fontStrikeOut()?"1":"0"):""));
    settings<<(p->hasProperty(QTextFormat::FontUnderline)?(p->fontUnderline()?"1":"0"):(writeDefaultsToo?(a->fontUnderline()?"1":"0"):""));
    settings<<(p->hasProperty(QTextFormat::BackgroundBrush)?QString::number(p->background().color().rgb(),16):((writeDefaultsToo && a->hasProperty(QTextFormat::BackgroundBrush))?QString::number(a->background().color().rgb(),16):""));
    settings<<(p->hasProperty(KTextEditor::Attribute::SelectedBackground)?QString::number(p->selectedBackground().color().rgb(),16):((writeDefaultsToo&& a->hasProperty(KTextEditor::Attribute::SelectedBackground))?QString::number(a->selectedBackground().color().rgb(),16):""));
    settings<<(p->hasProperty(QTextFormat::FontFamily)?(p->fontFamily()):(writeDefaultsToo?a->fontFamily():QString()));
    settings<<"---";
    config.writeEntry(p->name(),settings);
  }
}

const QHash<QString, QChar>& KateHighlighting::getCharacterEncodings( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib ) ]->characterEncodings;
}

const KatePrefixStore& KateHighlighting::getCharacterEncodingsPrefixStore( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib ) ]->characterEncodingsPrefixStore;
}

const QHash<QChar, QString>& KateHighlighting::getReverseCharacterEncodings( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib ) ]->reverseCharacterEncodings;
}

int KateHighlighting::getEncodedCharactersInsertionPolicy( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib ) ]->encodedCharactersInsertionPolicy;
}

void KateHighlighting::addCharacterEncoding( const QString& key, const QString& encoding, const QChar& c )
{
  m_additionalData[ key ]->characterEncodingsPrefixStore.addPrefix(encoding);
  m_additionalData[ key ]->characterEncodings[ encoding ] = c;
  m_additionalData[ key ]->reverseCharacterEncodings[ c ] = encoding;
}

/**
 * Increase the usage count, and trigger initialization if needed.
 */
void KateHighlighting::use()
{
  if (refCount == 0)
    init();

  refCount++;
}

/**
 * Decrease the usage count, and trigger cleanup if needed.
 */
void KateHighlighting::release()
{
  refCount--;

  if (refCount == 0)
    done();
}

/**
 * Initialize a context for the first time.
 */

void KateHighlighting::init()
{
  if (noHl)
    return;

  qDeleteAll(m_contexts);
  m_contexts.clear ();

  makeContextList();

  if (noHl) // something went wrong, fill something in
    makeNoneContext();
}


/**
 * If there is no document using the highlighting style free the complete
 * context structure.
 */
void KateHighlighting::done()
{
  if (noHl)
    return;

  cleanup ();
}

/**
 * KateHighlighting - createKateExtendedAttribute
 * This function reads the itemData entries from the config file, which specifies the
 * default attribute styles for matched items/contexts.
 *
 * @param list A reference to the internal list containing the parsed default config
 */
void KateHighlighting::createKateExtendedAttribute(QList<KateExtendedAttribute::Ptr> &list)
{
  // If the internal list isn't already available read the config file
  if (!noHl) {
    if (internalIDList.isEmpty())
      makeContextList();

    list=internalIDList;
  }

  // If no highlighting is selected or we have no attributes we need only one default.
  if (noHl || list.isEmpty())
    list.append(KateExtendedAttribute::Ptr(new KateExtendedAttribute(i18n("Normal Text"), KTextEditor::HighlightInterface::dsNormal)));
}

/**
 * Adds the styles of the currently parsed highlight to the itemdata list
 */
void KateHighlighting::addToKateExtendedAttributeList()
{
  //Tell the syntax document class which file we want to parse and which data group
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getGroupInfo("highlighting","itemData");

  KateDefaultColors colors;
  //begin with the real parsing
  while (KateHlManager::self()->syntax->nextGroup(data))
  {
    // read all attributes
    QString color = KateHlManager::self()->syntax->groupData(data,QString("color"));
    QString selColor = KateHlManager::self()->syntax->groupData(data,QString("selColor"));
    QString bold = KateHlManager::self()->syntax->groupData(data,QString("bold"));
    QString italic = KateHlManager::self()->syntax->groupData(data,QString("italic"));
    QString underline = KateHlManager::self()->syntax->groupData(data,QString("underline"));
    QString strikeOut = KateHlManager::self()->syntax->groupData(data,QString("strikeOut"));
    QString bgColor = KateHlManager::self()->syntax->groupData(data,QString("backgroundColor"));
    QString selBgColor = KateHlManager::self()->syntax->groupData(data,QString("selBackgroundColor"));
    QString spellChecking = KateHlManager::self()->syntax->groupData(data,QString("spellChecking"));
    QString fontFamily = KateHlManager::self()->syntax->groupData(data,QString("fontFamily"));

    KateExtendedAttribute::Ptr newData(new KateExtendedAttribute(
            buildPrefix+KateHlManager::self()->syntax->groupData(data,QString("name")).simplified(),
            KateExtendedAttribute::indexForStyleName(KateHlManager::self()->syntax->groupData(data,QString("defStyleNum")))));

    /* here the custom style overrides are specified, if needed */
    if (!color.isEmpty())
      newData->setForeground(colors.adaptToScheme(QColor(color), KateDefaultColors::ForegroundColor));
    if (!selColor.isEmpty())
      newData->setSelectedForeground(colors.adaptToScheme(QColor(selColor), KateDefaultColors::ForegroundColor));
    if (!bold.isEmpty()) newData->setFontBold( IS_TRUE(bold) );
    if (!italic.isEmpty()) newData->setFontItalic( IS_TRUE(italic) );
    // new attributes for the new rendering view
    if (!underline.isEmpty()) newData->setFontUnderline( IS_TRUE(underline) );
    if (!strikeOut.isEmpty()) newData->setFontStrikeOut( IS_TRUE(strikeOut) );
    if (!bgColor.isEmpty())
      newData->setBackground(colors.adaptToScheme(QColor(bgColor), KateDefaultColors::BackgroundColor));
    if (!selBgColor.isEmpty())
      newData->setSelectedBackground(colors.adaptToScheme(QColor(selBgColor), KateDefaultColors::BackgroundColor));
    // is spellchecking desired?
    if (!spellChecking.isEmpty()) newData->setPerformSpellchecking( IS_TRUE(spellChecking) );
    if (!fontFamily.isEmpty()) newData->setFontFamily(fontFamily);

    internalIDList.append(newData);
  }

  //clean up
  if (data)
    KateHlManager::self()->syntax->freeGroupInfo(data);
}

/**
 * KateHighlighting - lookupAttrName
 * This function is  a helper for makeContextList and createKateHlItem. It looks the given
 * attribute name in the itemData list up and returns its index
 *
 * @param name the attribute name to lookup
 * @param iDl the list containing all available attributes
 *
 * @return The index of the attribute, or 0 if the attribute isn't found
 */
int  KateHighlighting::lookupAttrName(const QString& name, QList<KateExtendedAttribute::Ptr> &iDl)
{
  const QString needle = buildPrefix + name;
  for (int i = 0; i < iDl.count(); i++)
    if (iDl.at(i)->name() == needle)
      return i;

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"Couldn't resolve itemDataName:"<<name;
#endif

  return 0;
}

/**
 * KateHighlighting - createKateHlItem
 * This function is  a helper for makeContextList. It parses the xml file for
 * information.
 *
 * @param data Data about the item read from the xml file
 * @param iDl List of all available itemData entries.
 *            Needed for attribute name->index translation
 * @param RegionList list of code folding region names
 * @param ContextNameList list of context names
 *
 * @return A pointer to the newly created item object
 */
KateHlItem *KateHighlighting::createKateHlItem(KateSyntaxContextData *data,
                                               QList<KateExtendedAttribute::Ptr> &iDl,
                                               QStringList *RegionList,
                                               QStringList *ContextNameList)
{
  // No highlighting -> exit
  if (noHl)
    return 0;

  // get the (tagname) itemd type
  QString dataname=KateHlManager::self()->syntax->groupItemData(data,QString(""));

  // code folding region handling:
  QString beginRegionStr=KateHlManager::self()->syntax->groupItemData(data,QString("beginRegion"));
  QString endRegionStr=KateHlManager::self()->syntax->groupItemData(data,QString("endRegion"));

  signed char regionId=0;
  signed char regionId2=0;

  if (!beginRegionStr.isEmpty())
  {
    regionId = RegionList->indexOf(beginRegionStr);

    if (regionId==-1) // if the region name doesn't already exist, add it to the list
    {
      (*RegionList)<<beginRegionStr;
      regionId = RegionList->indexOf(beginRegionStr);
    }

    regionId++;

#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010) << "########### BEG REG: "  << beginRegionStr << " NUM: " << regionId;
#endif
  }

  if (!endRegionStr.isEmpty())
  {
    regionId2 = RegionList->indexOf(endRegionStr);

    if (regionId2==-1) // if the region name doesn't already exist, add it to the list
    {
      (*RegionList)<<endRegionStr;
      regionId2 = RegionList->indexOf(endRegionStr);
    }

    regionId2 = -regionId2 - 1;

#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010) << "########### END REG: "  << endRegionStr << " NUM: " << regionId2;
#endif
  }

  int attr = 0;
  QString tmpAttr=KateHlManager::self()->syntax->groupItemData(data,QString("attribute")).simplified();
  bool onlyConsume = tmpAttr.isEmpty();

  // only relevant for non consumer
  if (!onlyConsume)
  {
    if (QString("%1").arg(tmpAttr.toInt())==tmpAttr)
    {
      errorsAndWarnings+=i18n(
          "<b>%1</b>: Deprecated syntax. Attribute (%2) not addressed by symbolic name<br />",
      buildIdentifier, tmpAttr);
      attr=tmpAttr.toInt();
    }
    else
      attr=lookupAttrName(tmpAttr,iDl);
  }

  // Info about context switch
  KateHlContextModification context = -1;
  QString unresolvedContext;
  QString tmpcontext=KateHlManager::self()->syntax->groupItemData(data,QString("context"));
  if (!tmpcontext.isEmpty())
    context=getContextModificationFromString(ContextNameList, tmpcontext,unresolvedContext);

  // Get the char parameter (eg DetectChar)
  QChar chr;
  if (! KateHlManager::self()->syntax->groupItemData(data,QString("char")).isEmpty()) {
    chr= (KateHlManager::self()->syntax->groupItemData(data,QString("char")))[0];
  }

  // Get the String parameter (eg. StringDetect)
  QString stringdata=KateHlManager::self()->syntax->groupItemData(data,QString("String"));

  // Get a second char parameter (char1) (eg Detect2Chars)
  QChar chr1;
  if (! KateHlManager::self()->syntax->groupItemData(data,QString("char1")).isEmpty()) {
    chr1= (KateHlManager::self()->syntax->groupItemData(data,QString("char1")))[0];
  }

  // Will be removed eventually. Atm used for StringDetect, WordDetect, keyword and RegExp
  const QString & insensitive_str = KateHlManager::self()->syntax->groupItemData(data,QString("insensitive"));
  bool insensitive = IS_TRUE( insensitive_str );

  // for regexp only
  bool minimal = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data,QString("minimal")) );

  // dominik: look ahead and do not change offset. so we can change contexts w/o changing offset1.
  bool lookAhead = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data,QString("lookAhead")) );

  bool dynamic= IS_TRUE(KateHlManager::self()->syntax->groupItemData(data,QString("dynamic")) );

  bool firstNonSpace = IS_TRUE(KateHlManager::self()->syntax->groupItemData(data,QString("firstNonSpace")) );

  int column = -1;
  QString colStr = KateHlManager::self()->syntax->groupItemData(data,QString("column"));
  if (!colStr.isEmpty())
    column = colStr.toInt();

  // Create the item corresponding to its type and set its parameters
  KateHlItem *tmpItem;

  if (dataname=="keyword")
  {
    bool keywordInsensitive = insensitive_str.isEmpty() ? !casesensitive : insensitive;
    KateHlKeyword *keyword=new KateHlKeyword(attr,context,regionId,regionId2,keywordInsensitive,
                                             m_additionalData[ buildIdentifier ]->deliminator);

    //Get the entries for the keyword lookup list
    keyword->addList(KateHlManager::self()->syntax->finddata("highlighting",stringdata));
    tmpItem=keyword;
  }
  else if (dataname=="Float") tmpItem= (new KateHlFloat(attr,context,regionId,regionId2));
  else if (dataname=="Int") tmpItem=(new KateHlInt(attr,context,regionId,regionId2));
  else if (dataname=="DetectChar") tmpItem=(new KateHlCharDetect(attr,context,regionId,regionId2,chr));
  else if (dataname=="Detect2Chars") tmpItem=(new KateHl2CharDetect(attr,context,regionId,regionId2,chr,chr1));
  else if (dataname=="RangeDetect") tmpItem=(new KateHlRangeDetect(attr,context,regionId,regionId2, chr, chr1));
  else if (dataname=="LineContinue") tmpItem=(new KateHlLineContinue(attr, context, regionId, regionId2, chr));
  else if (dataname=="StringDetect") tmpItem=(new KateHlStringDetect(attr,context,regionId,regionId2,stringdata,insensitive));
  else if (dataname=="WordDetect") tmpItem=(new KateHlWordDetect(attr,context,regionId,regionId2,stringdata,insensitive));
  else if (dataname=="AnyChar") tmpItem=(new KateHlAnyChar(attr,context,regionId,regionId2,stringdata));
  else if (dataname=="RegExpr") tmpItem=(new KateHlRegExpr(attr,context,regionId,regionId2,stringdata, insensitive, minimal));
  else if (dataname=="HlCChar") tmpItem= ( new KateHlCChar(attr,context,regionId,regionId2));
  else if (dataname=="HlCHex") tmpItem= (new KateHlCHex(attr,context,regionId,regionId2));
  else if (dataname=="HlCOct") tmpItem= (new KateHlCOct(attr,context,regionId,regionId2));
  else if (dataname=="HlCFloat") tmpItem= (new KateHlCFloat(attr,context,regionId,regionId2));
  else if (dataname=="HlCStringChar") tmpItem= (new KateHlCStringChar(attr,context,regionId,regionId2));
  else if (dataname=="DetectSpaces") tmpItem= (new KateHlDetectSpaces(attr,context,regionId,regionId2));
  else if (dataname=="DetectIdentifier") tmpItem= (new KateHlDetectIdentifier(attr,context,regionId,regionId2));
  else
  {
    // oops, unknown type. Perhaps a spelling error in the xml file
    return 0;
  }

  // set lookAhead & dynamic properties
  tmpItem->lookAhead = lookAhead;
  tmpItem->dynamic = dynamic;
  tmpItem->firstNonSpace = firstNonSpace;
  tmpItem->column = column;
  tmpItem->onlyConsume = onlyConsume;

  if (!unresolvedContext.isEmpty())
  {
    unresolvedContextReferences.insert(&(tmpItem->ctx),unresolvedContext);
  }

  // remember all to delete them
  m_hlItemCleanupList.append (tmpItem);

  return tmpItem;
}

int KateHighlighting::attribute(int ctx) const
{
  return m_contexts[ctx]->attr;
}

bool KateHighlighting::attributeRequiresSpellchecking( int attr )
{
  QList<KTextEditor::Attribute::Ptr> attributeList = attributes("");
  if(attr < attributeList.length() && attributeList[attr]->hasProperty(KateExtendedAttribute::Spellchecking)) {
    return attributeList[attr]->boolProperty(KateExtendedAttribute::Spellchecking);
  }
  return true;
}

QString KateHighlighting::hlKeyForContext(int i) const
{
  int k = 0;
  QMap<int,QString>::const_iterator it = m_ctxIndex.constEnd();
  while ( it != m_ctxIndex.constBegin() )
  {
    --it;
    k = it.key();
    if ( i >= k )
      break;
  }
  return it.value();
}

QString KateHighlighting::hlKeyForAttrib( int i ) const
{
  // find entry. This is faster than QMap::find. m_hlIndex always has an entry
  // for key '0' (it is "none"), so the result is always valid.
  int k = 0;
  QMap<int,QString>::const_iterator it = m_hlIndex.constEnd();
  while ( it != m_hlIndex.constBegin() )
  {
    --it;
    k = it.key();
    if ( i >= k )
      break;
  }
  return it.value();
}

bool KateHighlighting::isInWord( QChar c, int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib ) ]->deliminator.indexOf(c) < 0
      && !c.isSpace()
      && c != QChar::fromAscii('"') && c != QChar::fromAscii('\'') && c != QChar::fromAscii('`');
}

bool KateHighlighting::canBreakAt( QChar c, int attrib ) const
{
  static const QString& sq = KGlobal::staticQString("\"'");
  return (m_additionalData[ hlKeyForAttrib( attrib ) ]->wordWrapDeliminator.indexOf(c) != -1) && (sq.indexOf(c) == -1);
}

QLinkedList<QRegExp> KateHighlighting::emptyLines(int attrib) const
{
#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"hlKeyForAttrib: "<<hlKeyForAttrib(attrib);
#endif

  return m_additionalData[hlKeyForAttrib(attrib)]->emptyLines;
}

signed char KateHighlighting::commentRegion(int attr) const {
  QString commentRegion=m_additionalData[ hlKeyForAttrib( attr ) ]->multiLineRegion;
  return (commentRegion.isEmpty()?0:(commentRegion.toShort()));
}

bool KateHighlighting::canComment( int startAttrib, int endAttrib ) const
{
  QString k = hlKeyForAttrib( startAttrib );
  return ( k == hlKeyForAttrib( endAttrib ) &&
      ( ( !m_additionalData[k]->multiLineCommentStart.isEmpty() && !m_additionalData[k]->multiLineCommentEnd.isEmpty() ) ||
       ! m_additionalData[k]->singleLineCommentMarker.isEmpty() ) );
}

QString KateHighlighting::getCommentStart( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib) ]->multiLineCommentStart;
}

QString KateHighlighting::getCommentEnd( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib ) ]->multiLineCommentEnd;
}

QString KateHighlighting::getCommentSingleLineStart( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib) ]->singleLineCommentMarker;
}

KateHighlighting::CSLPos KateHighlighting::getCommentSingleLinePosition( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib) ]->singleLineCommentPosition;
}

const QHash<QString, QChar>& KateHighlighting::characterEncodings( int attrib ) const
{
  return m_additionalData[ hlKeyForAttrib( attrib) ]->characterEncodings;
}

/**
 * Helper for makeContextList. It parses the xml file for
 * information, how single or multi line comments are marked
 */
void KateHighlighting::readCommentConfig()
{
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data=KateHlManager::self()->syntax->getGroupInfo("general","comment");

  QString cmlStart="", cmlEnd="", cmlRegion="", cslStart="";
  CSLPos cslPosition=CSLPosColumn0;

  if (data)
  {
    while  (KateHlManager::self()->syntax->nextGroup(data))
    {
      if (KateHlManager::self()->syntax->groupData(data,"name")=="singleLine")
      {
        cslStart=KateHlManager::self()->syntax->groupData(data,"start");
        QString cslpos=KateHlManager::self()->syntax->groupData(data,"position");
        if (cslpos=="afterwhitespace")
          cslPosition=CSLPosAfterWhitespace;
        else
          cslPosition=CSLPosColumn0;
      }
      else if (KateHlManager::self()->syntax->groupData(data,"name")=="multiLine")
      {
        cmlStart=KateHlManager::self()->syntax->groupData(data,"start");
        cmlEnd=KateHlManager::self()->syntax->groupData(data,"end");
        cmlRegion=KateHlManager::self()->syntax->groupData(data,"region");
      }
    }

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }

  m_additionalData[buildIdentifier]->singleLineCommentMarker = cslStart;
  m_additionalData[buildIdentifier]->singleLineCommentPosition = cslPosition;
  m_additionalData[buildIdentifier]->multiLineCommentStart = cmlStart;
  m_additionalData[buildIdentifier]->multiLineCommentEnd = cmlEnd;
  m_additionalData[buildIdentifier]->multiLineRegion = cmlRegion;
}




void KateHighlighting::readEmptyLineConfig()
{
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data=KateHlManager::self()->syntax->getGroupInfo("general","emptyLine");

  QLinkedList<QRegExp> exprList;

  if (data)
  {
    while  (KateHlManager::self()->syntax->nextGroup(data))
    {
#ifdef HIGHLIGHTING_DEBUG
      kDebug(13010)<<"creating an empty line regular expression";
#endif

      QString regexprline=KateHlManager::self()->syntax->groupData(data,"regexpr");
      bool regexprcase=(KateHlManager::self()->syntax->groupData(data,"casesensitive").toUpper().compare("TRUE")==0);
      exprList.append(QRegExp(regexprline,regexprcase?Qt::CaseSensitive:Qt::CaseInsensitive));
    }
      KateHlManager::self()->syntax->freeGroupInfo(data);
  }

  m_additionalData[buildIdentifier]->emptyLines = exprList;
}






/**
 * Helper for makeContextList. It parses the xml file for information,
 * if keywords should be treated case(in)sensitive and creates the keyword
 * delimiter list. Which is the default list, without any given weak deliminiators
 */
void KateHighlighting::readGlobalKeywordConfig()
{
  deliminator = stdDeliminator;

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"readGlobalKeywordConfig:BEGIN";
#endif

  // Tell the syntax document class which file we want to parse
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","keywords");

  if (data)
  {
#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010)<<"Found global keyword config";
#endif

    casesensitive = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data,QString("casesensitive")) );

    //get the weak deliminators
    weakDeliminator=(KateHlManager::self()->syntax->groupItemData(data,QString("weakDeliminator")));

#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010)<<"weak delimiters are: "<<weakDeliminator;
#endif

    // remove any weakDelimitars (if any) from the default list and store this list.
    for (int s=0; s < weakDeliminator.length(); s++)
    {
      int f = deliminator.indexOf (weakDeliminator[s]);

      if (f > -1)
        deliminator.remove (f, 1);
    }

    QString addDelim = (KateHlManager::self()->syntax->groupItemData(data,QString("additionalDeliminator")));

    if (!addDelim.isEmpty())
      deliminator=deliminator+addDelim;

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
  else
  {
    //Default values
    casesensitive=true;
    weakDeliminator=QString("");
  }

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"readGlobalKeywordConfig:END";
  kDebug(13010)<<"delimiterCharacters are: "<<deliminator;
#endif

  m_additionalData[buildIdentifier]->deliminator = deliminator;
}

/**
 * Helper for makeContextList. It parses the xml file for any wordwrap
 * deliminators, characters * at which line can be broken. In case no keyword
 * tag is found in the xml file, the wordwrap deliminators list defaults to the
 * standard denominators. In case a keyword tag is defined, but no
 * wordWrapDeliminator attribute is specified, the deliminator list as computed
 * in readGlobalKeywordConfig is used.
 *
 * @return the computed delimiter string.
 */
void KateHighlighting::readWordWrapConfig()
{
#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"readWordWrapConfig:BEGIN";
#endif

  // Tell the syntax document class which file we want to parse
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","keywords");

  QString wordWrapDeliminator = stdDeliminator;
  if (data)
  {
#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010)<<"Found global keyword config";
#endif

    wordWrapDeliminator = (KateHlManager::self()->syntax->groupItemData(data,QString("wordWrapDeliminator")));
    //when no wordWrapDeliminator is defined use the deliminator list
    if ( wordWrapDeliminator.length() == 0 ) wordWrapDeliminator = deliminator;

#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010) << "word wrap deliminators are " << wordWrapDeliminator;
#endif

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"readWordWrapConfig:END";
#endif

  m_additionalData[buildIdentifier]->wordWrapDeliminator = wordWrapDeliminator;
}

void KateHighlighting::readIndentationConfig()
{
  m_indentation = "";

  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","indentation");

  if (data)
  {
    m_indentation = (KateHlManager::self()->syntax->groupItemData(data,QString("mode")));

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
}

void KateHighlighting::readFoldingConfig()
{
#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"readfoldignConfig:BEGIN";
#endif

  // Tell the syntax document class which file we want to parse
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","folding");

  if (data)
  {
#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010)<<"Found global keyword config";
#endif

    m_foldingIndentationSensitive = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data, QString("indentationsensitive")) );

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
  else
  {
    //Default values
    m_foldingIndentationSensitive = false;
  }

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"readfoldingConfig:END";
  kDebug(13010)<<"############################ use indent for fold are: "<<m_foldingIndentationSensitive;
#endif
}

void KateHighlighting::readSpellCheckingConfig()
{
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data=KateHlManager::self()->syntax->getGroupInfo("spellchecking","encoding");

  if (data)
  {
    while  (KateHlManager::self()->syntax->nextGroup(data))
    {
        QString encoding = KateHlManager::self()->syntax->groupData(data,"string");
        QString character = KateHlManager::self()->syntax->groupData(data,"char");
        QString ignored = KateHlManager::self()->syntax->groupData(data,"ignored");

        const bool ignoredIsTrue = IS_TRUE(ignored);
        if(encoding.isEmpty() || (character.isEmpty() && !ignoredIsTrue))
        {
          continue;
        }
        QRegExp newLineRegExp("\\r|\\n");
        if(encoding.indexOf(newLineRegExp) >= 0)
        {
          encoding.replace(newLineRegExp, "<\\n|\\r>");

#ifdef HIGHLIGHTING_DEBUG
          kDebug() << "Encoding" << encoding
                                 << "contains new-line characters. Ignored.";
#endif
        }
        QChar c = (character.isEmpty() || ignoredIsTrue) ? QChar() : character[0];
        addCharacterEncoding(buildIdentifier, encoding, c);
    }

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }

  data=KateHlManager::self()->syntax->getConfig("spellchecking","configuration");
  if (data)
  {
    QString policy = KateHlManager::self()->syntax->groupItemData(data,"encodingReplacementPolicy");
    QString policyLowerCase = policy.toLower();
    int p;

    if(policyLowerCase == "encodewhenpresent") {
      p = KateDocument::EncodeWhenPresent;
    }
    else if(policyLowerCase == "encodealways") {
      p = KateDocument::EncodeAlways;
    }
    else {
      p = KateDocument::EncodeNever;
    }

    m_additionalData[buildIdentifier]->encodedCharactersInsertionPolicy = p;

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
}

void  KateHighlighting::createContextNameList(QStringList *ContextNameList,int ctx0)
{
#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"creatingContextNameList:BEGIN";
#endif

  if (ctx0 == 0)
      ContextNameList->clear();

  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);

  KateSyntaxContextData *data=KateHlManager::self()->syntax->getGroupInfo("highlighting","context");

  int id=ctx0;

  if (data)
  {
    while (KateHlManager::self()->syntax->nextGroup(data))
    {
      QString tmpAttr=KateHlManager::self()->syntax->groupData(data,QString("name")).simplified();
      if (tmpAttr.isEmpty())
      {
        tmpAttr = QString("!KATE_INTERNAL_DUMMY! %1").arg(id);
        errorsAndWarnings += i18n("<b>%1</b>: Deprecated syntax. Context %2 has no symbolic name<br />", buildIdentifier, id-ctx0);
      }
      else tmpAttr = buildPrefix + tmpAttr;
      (*ContextNameList) << tmpAttr;
      id++;
    }
    KateHlManager::self()->syntax->freeGroupInfo(data);
  }

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"creatingContextNameList:END";
#endif
}

KateHlContextModification KateHighlighting::getContextModificationFromString(QStringList *ContextNameList, QString tmpLineEndContext, /*NO CONST*/ QString &unres)
{
  // nothing unresolved
  unres = "";

  // context to push on stack
  int context = -1;

  // number of contexts to pop
  int pops = 0;

  // we allow arbitrary #stay and #pop at the start
  bool anyFound = false;
  while (tmpLineEndContext.startsWith(QLatin1String("#stay")) ||
         tmpLineEndContext.startsWith(QLatin1String("#pop")))
  {
    // ignore stay
    if (tmpLineEndContext.startsWith(QLatin1String("#stay")))
    {
      tmpLineEndContext.remove (0, 5);
    }
    else // count the pops
    {
      ++pops;
      tmpLineEndContext.remove (0, 4);
    }

    anyFound = true;
  }

  /**
   * we want a ! if we have found any pop or push and still have stuff in the string...
   */
  if (anyFound && !tmpLineEndContext.isEmpty())
  {
    if (tmpLineEndContext.startsWith('!'))
      tmpLineEndContext.remove (0, 1);
  }

  /**
   * empty string, done
   */
  if (tmpLineEndContext.isEmpty())
    return KateHlContextModification (context, pops);

  /**
   * handle the remaining string, this might be a ##contextname
   * or a normal contextname....
   */
  if ( tmpLineEndContext.contains("##"))
  {
    int o = tmpLineEndContext.indexOf("##");
    // FIXME at least with 'foo##bar'-style contexts the rules are picked up
    // but the default attribute is not
    QString tmp=tmpLineEndContext.mid(o+2);
    if (!embeddedHls.contains(tmp))  embeddedHls.insert(tmp,KateEmbeddedHlInfo());
    unres=tmp+':'+tmpLineEndContext.left(o);

#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010) << "unres = " << unres;
#endif

    context=0;
  }

  else
  {
    context=ContextNameList->indexOf(buildPrefix+tmpLineEndContext);
    if (context==-1)
    {
      context=tmpLineEndContext.toInt();
      errorsAndWarnings+=i18n(
        "<B>%1</B>:Deprecated syntax. Context %2 not addressed by a symbolic name"
        , buildIdentifier, tmpLineEndContext);
    }
//#warning restructure this the name list storage.
//    context=context+buildContext0Offset;
  }

  return KateHlContextModification (context, pops);
}

/**
 * The most important initialization function for each highlighting. It's called
 * each time a document gets a highlighting style assigned. parses the xml file
 * and creates a corresponding internal structure
 */
void KateHighlighting::makeContextList()
{
  if (noHl)  // if this a highlighting for "normal texts" only, tere is no need for a context list creation
    return;

  embeddedHls.clear();
  embeddedHighlightingModes.clear();
  unresolvedContextReferences.clear();
  RegionList.clear();
  ContextNameList.clear();

  // prepare list creation. To reuse as much code as possible handle this
  // highlighting the same way as embedded onces
  embeddedHls.insert(iName,KateEmbeddedHlInfo());

  bool something_changed;
  // the context "0" id is 0 for this hl, all embedded context "0"s have offsets
  startctx=base_startctx=0;
  // inform everybody that we are building the highlighting contexts and itemlists
  building=true;

  do
  {
#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010)<<"**************** Outer loop in make ContextList";
    kDebug(13010)<<"**************** Hl List count:"<<embeddedHls.count();
#endif

    something_changed=false; //assume all "embedded" hls have already been loaded
    for (KateEmbeddedHlInfos::iterator it=embeddedHls.begin(); it!=embeddedHls.end();++it)
    {
      if (!it.value().loaded)  // we found one, we still have to load
      {
#ifdef HIGHLIGHTING_DEBUG
        kDebug(13010)<<"**************** Inner loop in make ContextList";
#endif

        QString identifierToUse;

#ifdef HIGHLIGHTING_DEBUG
        kDebug(13010)<<"Trying to open highlighting definition file: "<< it.key();
#endif

        if (iName==it.key()) // the own identifier is known
          identifierToUse=identifier;
        else                 // all others have to be looked up
          identifierToUse=KateHlManager::self()->identifierForName(it.key());

#ifdef HIGHLIGHTING_DEBUG
        kDebug(13010)<<"Location is:"<< identifierToUse;
#endif

        if (identifierToUse.isEmpty() )
          kWarning(13010)<<"Unknown highlighting description referenced:" << it.key() << "in" << identifier;

        buildPrefix=it.key()+':';  // attribute names get prefixed by the names
                                   // of the highlighting definitions they belong to


#ifdef HIGHLIGHTING_DEBUG
        kDebug(13010)<<"setting ("<<it.key()<<") to loaded";
#endif

        //mark hl as loaded
        it=embeddedHls.insert(it.key(),KateEmbeddedHlInfo(true,startctx));
        //set class member for context 0 offset, so we don't need to pass it around
        buildContext0Offset=startctx;
        //parse one hl definition file
        startctx=addToContextList(identifierToUse,startctx);

        if (noHl) return;  // an error occurred

        base_startctx = startctx;
        something_changed=true; // something has been loaded
      }
    }
  } while (something_changed);  // as long as there has been another file parsed
                  // repeat everything, there could be newly added embedded hls.


#ifdef HIGHLIGHTING_DEBUG
  // at this point all needed highlighing (sub)definitions are loaded. It's time
  // to resolve cross file  references (if there are any)#
  kDebug(13010)<<"Unresolved contexts, which need attention: "<<unresolvedContextReferences.count();
#endif

  //optimize this a littlebit
  for (KateHlUnresolvedCtxRefs::iterator unresIt=unresolvedContextReferences.begin();
       unresIt != unresolvedContextReferences.end();
       ++unresIt)
  {
    QString incCtx = unresIt.value();

#ifdef HIGHLIGHTING_DEBUG
    kDebug(13010) << "Context " <<incCtx << " is unresolved";
#endif

    // only resolve '##Name' contexts here; handleKateHlIncludeRules() can figure
    // out 'Name##Name'-style inclusions, but we screw it up
    if (incCtx.endsWith(':')) {
#ifdef HIGHLIGHTING_DEBUG
      kDebug(13010)<<"Looking up context0 for ruleset "<<incCtx;
#endif

      incCtx = incCtx.left(incCtx.length()-1);
      //try to find the context0 id for a given unresolvedReference
      KateEmbeddedHlInfos::const_iterator hlIt=embeddedHls.constFind(incCtx);
      if (hlIt!=embeddedHls.constEnd())
        *(unresIt.key())=hlIt.value().context0;
    }
  }

  // eventually handle KateHlIncludeRules items, if they exist.
  // This has to be done after the cross file references, because it is allowed
  // to include the context0 from a different definition, than the one the rule
  // belongs to
  handleKateHlIncludeRules();

  embeddedHighlightingModes = embeddedHls.keys();
  embeddedHighlightingModes.removeOne(iName);

  embeddedHls.clear(); //save some memory.
  unresolvedContextReferences.clear(); //save some memory
  RegionList.clear();  // I think you get the idea ;)
  ContextNameList.clear();


  // if there have been errors show them
  if (!errorsAndWarnings.isEmpty())
  KMessageBox::detailedSorry(QApplication::activeWindow(),i18n(
        "There were warning(s) and/or error(s) while parsing the syntax "
        "highlighting configuration."),
        errorsAndWarnings, i18n("Kate Syntax Highlighting Parser"));

  // we have finished
  building=false;
  
  Q_ASSERT(m_contexts.size() > 0);
}

void KateHighlighting::handleKateHlIncludeRules()
{
  // if there are noe include rules to take care of, just return
#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"KateHlIncludeRules, which need attention: " <<includeRules.size();
#endif

  if (includeRules.isEmpty()) return;

  buildPrefix="";
  QString dummy;

  // By now the context0 references are resolved, now more or less only inner
  // file references are resolved. If we decide that arbitrary inclusion is
  // needed, this doesn't need to be changed, only the addToContextList
  // method.

  //resolve context names
  for (KateHlIncludeRules::iterator it=includeRules.begin(); it!=includeRules.end(); )
  {
    if ((*it)->incCtx.newContext==-1) // context unresolved ?
    {

      if ((*it)->incCtxN.isEmpty())
      {
        // no context name given, and no valid context id set, so this item is
        // going to be removed
        KateHlIncludeRules::iterator it1=it;
        ++it1;
        delete (*it);
        includeRules.erase(it);
        it=it1;
      }
      else
      {
        // resolve name to id
        (*it)->incCtx=getContextModificationFromString(&ContextNameList,(*it)->incCtxN,dummy).newContext;

#ifdef HIGHLIGHTING_DEBUG
        kDebug(13010)<<"Resolved "<<(*it)->incCtxN<< " to "<<(*it)->incCtx.newContext<<" for include rule";
#endif

        // It would be good to look here somehow, if the result is valid
      }
    }
    else ++it; //nothing to do, already resolved (by the cross definition reference resolver)
  }

  // now that all KateHlIncludeRule items should be valid and completely resolved,
  // do the real inclusion of the rules.
  // recursiveness is needed, because context 0 could include context 1, which
  // itself includes context 2 and so on.
  //  In that case we have to handle context 2 first, then 1, 0
  //TODO: catch circular references: eg 0->1->2->3->1
  for (int i=0; i<includeRules.count(); i++)
       handleKateHlIncludeRulesRecursive(i, &includeRules);
  qDeleteAll(includeRules);
  includeRules.clear();
}

void KateHighlighting::handleKateHlIncludeRulesRecursive(int index, KateHlIncludeRules *list)
{
  if (index < 0 || index >= list->count()) return;  //invalid iterator, shouldn't happen, but better have a rule prepared ;)

  int index1 = index;
  int ctx = list->at(index1)->ctx;

  if (ctx == -1) return;  // skip already processed entries

  // find the last entry for the given context in the KateHlIncludeRules list
  // this is need if one context includes more than one. This saves us from
  // updating all insert positions:
  // eg: context 0:
  // pos 3 - include context 2
  // pos 5 - include context 3
  // During the building of the includeRules list the items are inserted in
  // ascending order, now we need it descending to make our life easier.
  while (index < list->count() && list->at(index)->ctx == ctx)
  {
    index1 = index;
    ++index;
  }

  // iterate over each include rule for the context the function has been called for.
  while (index1 >= 0 && index1 < list->count() && list->at(index1)->ctx == ctx)
  {
    KateHlContextModification ctx1 = list->at(index1)->incCtx;

    //let's see, if the included context includes other contexts
    for (int index2 = 0; index2 < list->count(); ++index2)
    {
      if (list->at(index2)->ctx == ctx1.newContext)
      {
        if (index2 == index1) {
          // prevent accidental infinite recursion
          kWarning() << "infinite recursion in IncludeRules in language file for " << iName << "in context" << list->at(index1)->incCtxN;
          continue;
        }
        //yes it does, so first handle that include rules, since we want to
        // include those subincludes too
        handleKateHlIncludeRulesRecursive(index2, list);
        break;
      }
    }

    // if the context we want to include had sub includes, they are already inserted there.
    KateHlContext *dest=m_contexts[ctx];
    KateHlContext *src=m_contexts[ctx1.newContext];
//     kDebug(3010)<<"linking included rules from "<<ctx<<" to "<<ctx1;

    // If so desired, change the dest attribute to the one of the src.
    // Required to make commenting work, if text matched by the included context
    // is a different highlight than the host context.
    if ( list->at(index1)->includeAttrib )
      dest->attr = src->attr;

    // insert the included context's rules starting at position p
    int p = list->at(index1)->pos;

    // remember some stuff
    int oldLen = dest->items.size();
    uint itemsToInsert = src->items.size();

    // resize target
    dest->items.resize (oldLen + itemsToInsert);

    // move old elements
    for (int i=oldLen-1; i >= p; --i)
      dest->items[i+itemsToInsert] = dest->items[i];

    // insert new stuff
    for (uint i=0; i < itemsToInsert; ++i  )
      dest->items[p+i] = src->items[i];

    index = index1; //backup the iterator
    --index1;  //move to the next entry, which has to be take care of
    list->at(index)->ctx = -1;  // set ctx to -1 to mark already processed entries
  }
}

/**
 * Add one highlight to the contextlist.
 *
 * @return the number of contexts after this is added.
 */
int KateHighlighting::addToContextList(const QString &ident, int ctx0)
{
  //kDebug(13010)<<"=== Adding hl with ident '"<<ident<<"' ctx0="<<ctx0;

  buildIdentifier=ident;
  QString dummy;

  // Let the syntax document class know, which file we'd like to parse
  if (!KateHlManager::self()->syntax->setIdentifier(ident))
  {
    noHl=true;
    KMessageBox::information(QApplication::activeWindow(),i18n(
        "Since there has been an error parsing the highlighting description, "
        "this highlighting will be disabled"));
    return 0;
  }

  // only read for the own stuff
  if (identifier == ident)
  {
    readIndentationConfig ();
  }

  RegionList<<"!KateInternal_TopLevel!";

  m_hlIndex[internalIDList.count()] = ident;
  m_ctxIndex[ctx0]=ident;

  // clear and reuse or create new
  if (m_additionalData[ident])
    *m_additionalData[ident] = HighlightPropertyBag ();
  else
    m_additionalData.insert( ident, new HighlightPropertyBag );

  // fill out the propertybag
  readCommentConfig();
  readEmptyLineConfig();
  readGlobalKeywordConfig();
  readWordWrapConfig();

  // only read for ourself
  if (identifier == ident)
    readFoldingConfig ();

  readSpellCheckingConfig();

  // This list is needed for the translation of the attribute parameter,
  // if the itemData name is given instead of the index
  addToKateExtendedAttributeList();
  QList<KateExtendedAttribute::Ptr> iDl = internalIDList;

  createContextNameList(&ContextNameList,ctx0);

#ifdef HIGHLIGHTING_DEBUG
  kDebug(13010)<<"Parsing Context structure";
#endif

  //start the real work
  uint i=buildContext0Offset;
  KateSyntaxContextData * data = KateHlManager::self()->syntax->getGroupInfo("highlighting", "context");
  if (data)
  {
    while (KateHlManager::self()->syntax->nextGroup(data))
    {
#ifdef HIGHLIGHTING_DEBUG
      kDebug(13010)<<"Found a context in file, building structure now";
#endif

      //BEGIN - Translation of the attribute parameter
      QString tmpAttr=KateHlManager::self()->syntax->groupData(data,QString("attribute")).simplified();
      int attr;
      if (QString("%1").arg(tmpAttr.toInt())==tmpAttr)
        attr=tmpAttr.toInt();
      else
        attr=lookupAttrName(tmpAttr,iDl);
      //END - Translation of the attribute parameter

      QString tmpLineEndContext=KateHlManager::self()->syntax->groupData(data,QString("lineEndContext")).simplified();
      KateHlContextModification context;

      context=getContextModificationFromString(&ContextNameList, tmpLineEndContext,dummy);

      QString tmpNIBF = KateHlManager::self()->syntax->groupData(data, QString("noIndentationBasedFolding") );
      bool noIndentationBasedFolding=IS_TRUE(tmpNIBF);

      //BEGIN get fallthrough props
      KateHlContextModification ftc = 0; // fallthrough context
      QString tmpFt = KateHlManager::self()->syntax->groupData(data, QString("fallthrough") );
      const bool ft = IS_TRUE(tmpFt);
      if ( ft )
      {
        QString tmpFtc = KateHlManager::self()->syntax->groupData( data, QString("fallthroughContext") );

        ftc=getContextModificationFromString(&ContextNameList, tmpFtc,dummy);

        // stay is not allowed, we need to #pop or push some context...
        if (ftc.type == KateHlContextModification::doNothing) ftc = 0;

#ifdef HIGHLIGHTING_DEBUG
        kDebug(13010)<<"Setting fall through context (context "<<i<<"): "<<ftc.newContext;
#endif
      }
      //END fallthrough props
      
      // empty line context
      QString emptyLineContext = KateHlManager::self()->syntax->groupData( data, QString("lineEmptyContext") );
      KateHlContextModification emptyLineContextModification;
      if (!emptyLineContext.isEmpty())
        emptyLineContextModification = getContextModificationFromString(&ContextNameList, emptyLineContext, dummy);

      bool dynamic = false;
      QString tmpDynamic = KateHlManager::self()->syntax->groupData(data, QString("dynamic") );
      if ( tmpDynamic.toLower() == "true" ||  tmpDynamic.toInt() == 1 )
        dynamic = true;

      KateHlContext *ctxNew = new KateHlContext (
        ident,
        attr,
        context,
        ft, ftc, dynamic, noIndentationBasedFolding,
        !emptyLineContext.isEmpty(), emptyLineContextModification);

      m_contexts.push_back (ctxNew);

#ifdef HIGHLIGHTING_DEBUG
      kDebug(13010) << "INDEX: " << i << " LENGTH " << m_contexts.size()-1;
#endif

      //Let's create all items for the context
      while (KateHlManager::self()->syntax->nextItem(data))
      {
//         kDebug(13010)<< "In make Contextlist: Item:";

        // KateHlIncludeRules : add a pointer to each item in that context
        // TODO add a attrib includeAttrib
        QString tag = KateHlManager::self()->syntax->groupItemData(data,QString(""));
        if ( tag == "IncludeRules" ) //if the new item is an Include rule, we have to take special care
        {
          QString incCtx = KateHlManager::self()->syntax->groupItemData( data, QString("context"));
          QString incAttrib = KateHlManager::self()->syntax->groupItemData( data, QString("includeAttrib"));
          bool includeAttrib = IS_TRUE( incAttrib );
          
          // only context refernces of type Name, ##Name, and Subname##Name are allowed
          if (incCtx.startsWith("##") || (!incCtx.startsWith('#')))
          {
            int incCtxi = incCtx.indexOf ("##");
            //#stay, #pop is not interesting here
            if (incCtxi >= 0)
            {
              QString incSet = incCtx.mid(incCtxi + 2);
              QString incCtxN = incSet + ':' + incCtx.left(incCtxi);
              
              //a cross highlighting reference
#ifdef HIGHLIGHTING_DEBUG
              kDebug(13010)<<"Cross highlight reference <IncludeRules>, context "<<incCtxN;
#endif

              KateHlIncludeRule *ir=new KateHlIncludeRule(i,m_contexts[i]->items.count(),incCtxN,includeAttrib);

              //use the same way to determine cross hl file references as other items do
              if (!embeddedHls.contains(incSet))
                embeddedHls.insert(incSet,KateEmbeddedHlInfo());
#ifdef HIGHLIGHTING_DEBUG
              else
                kDebug(13010)<<"Skipping embeddedHls.insert for "<<incCtxN;
#endif

              unresolvedContextReferences.insert(&(ir->incCtx), incCtxN);

              includeRules.append(ir);
            }
            else
            {
              // a local reference -> just initialize the include rule structure
              incCtx=buildPrefix+incCtx.simplified ();
              includeRules.append(new KateHlIncludeRule(i,m_contexts[i]->items.count(),incCtx, includeAttrib));
            }
          }

          continue;
        }
// TODO -- can we remove the block below??
#if 0
                QString tag = KateHlManager::self()->syntax->groupKateExtendedAttribute(data,QString(""));
                if ( tag == "IncludeRules" ) {
                  // attrib context: the index (jowenn, i think using names here
                  // would be a cool feat, goes for mentioning the context in
                  // any item. a map or dict?)
                  int ctxId = getIdFromString(&ContextNameList,
                                               KateHlManager::self()->syntax->groupKateExtendedAttribute( data, QString("context")),dummy); // the index is *required*
                  if ( ctxId > -1) { // we can even reuse rules of 0 if we want to:)
                    kDebug(13010)<<"makeContextList["<<i<<"]: including all items of context "<<ctxId;
                    if ( ctxId < (int) i ) { // must be defined
                      for ( c = m_contexts[ctxId]->items.first(); c; c = m_contexts[ctxId]->items.next() )
                        m_contexts[i]->items.append(c);
                    }
                    else
                      kDebug(13010)<<"Context "<<ctxId<<"not defined. You can not include the rules of an undefined context";
                  }
                  continue; // while nextItem
                }
#endif
        KateHlItem * c = createKateHlItem(data, iDl, &RegionList, &ContextNameList);
        if (c)
        {
          m_contexts[i]->items.append(c);

          // Not supported completely atm and only one level. Subitems.(all have
          // to be matched to at once)
          KateSyntaxContextData * datasub = KateHlManager::self()->syntax->getSubItems(data);
          for (bool tmpbool=KateHlManager::self()->syntax->nextItem(datasub);
               tmpbool;
               tmpbool=KateHlManager::self()->syntax->nextItem(datasub))
          {
            c->subItems.resize (c->subItems.size()+1);
            c->subItems[c->subItems.size()-1] = createKateHlItem(datasub,iDl,&RegionList,&ContextNameList);
          }
          KateHlManager::self()->syntax->freeGroupInfo(datasub);
        }
      }
      i++;
    }

    KateHlManager::self()->syntax->freeGroupInfo(data);
  } else {
    // error handling: no "context" element at all in the xml file
    noHl = true;
    kWarning() << "There is no \"context\" in the highlighting file:" << buildIdentifier;
  }


  if (RegionList.count()!=1)
    folding=true;

  folding = folding || m_foldingIndentationSensitive;

  //BEGIN Resolve multiline region if possible
  if (!m_additionalData[ ident ]->multiLineRegion.isEmpty()) {
    long commentregionid=RegionList.indexOf( m_additionalData[ ident ]->multiLineRegion );
    if (-1==commentregionid) {
      errorsAndWarnings+=i18n(
          "<b>%1</b>: Specified multiline comment region (%2) could not be resolved<br />"
                             , buildIdentifier,  m_additionalData[ ident ]->multiLineRegion );
      m_additionalData[ ident ]->multiLineRegion.clear();

#ifdef HIGHLIGHTING_DEBUG
      kDebug(13010)<<"ERROR comment region attribute could not be resolved";
#endif
    } else {
      m_additionalData[ ident ]->multiLineRegion=QString::number(commentregionid+1);

#ifdef HIGHLIGHTING_DEBUG
      kDebug(13010)<<"comment region resolved to:"<<m_additionalData[ ident ]->multiLineRegion;
#endif
    }
  }
  //END Resolve multiline region if possible
  return i;
}

void KateHighlighting::clearAttributeArrays ()
{
  QMutableHashIterator< QString, QList<KTextEditor::Attribute::Ptr> > it = m_attributeArrays;
  while (it.hasNext())
  {
    it.next();

    // k, schema correct, let create the data
    KateAttributeList defaultStyleList;

    KateHlManager::self()->getDefaults(it.key(), defaultStyleList);

    QList<KateExtendedAttribute::Ptr> itemDataList;
    getKateExtendedAttributeList(it.key(), itemDataList);

    uint nAttribs = itemDataList.count();
    QList<KTextEditor::Attribute::Ptr>& array = it.value();
    array.clear();

    for (uint z = 0; z < nAttribs; z++)
    {
      KateExtendedAttribute::Ptr itemData = itemDataList.at(z);
      KTextEditor::Attribute::Ptr newAttribute( new KTextEditor::Attribute(*defaultStyleList.at(itemData->defaultStyleIndex())) );

      if (itemData && itemData->hasAnyProperty())
        *newAttribute += *itemData;

      array.append(newAttribute);
    }
  }
}

QList<KTextEditor::Attribute::Ptr> KateHighlighting::attributes (const QString &schema)
{
  // found it, already floating around
  if (m_attributeArrays.contains(schema))
    return m_attributeArrays[schema];

  // k, schema correct, let create the data
  QList<KTextEditor::Attribute::Ptr> array;
  KateAttributeList defaultStyleList;

  KateHlManager::self()->getDefaults(schema, defaultStyleList);

  QList<KateExtendedAttribute::Ptr> itemDataList;
  getKateExtendedAttributeList(schema, itemDataList);

  uint nAttribs = itemDataList.count();
  for (uint z = 0; z < nAttribs; z++)
  {
    KateExtendedAttribute::Ptr itemData = itemDataList.at(z);
    KTextEditor::Attribute::Ptr newAttribute( new KTextEditor::Attribute(*defaultStyleList.at(itemData->defaultStyleIndex())) );

    if (itemData && itemData->hasAnyProperty())
      *newAttribute += *itemData;

    array.append(newAttribute);
  }

  m_attributeArrays.insert(schema, array);

  return array;
}

QStringList KateHighlighting::getEmbeddedHighlightingModes() const
{
  return embeddedHighlightingModes;
}

bool KateHighlighting::isEmptyLine(const Kate::TextLineData *textline) const
{
  const QString &txt=textline->string();
  if (txt.isEmpty())
    return true;
  
  QLinkedList<QRegExp> l;
  l=emptyLines(textline->attribute(0));
  if (l.isEmpty()) return false;
  foreach(const QRegExp &re,l) {
    if (re.exactMatch(txt)) return true;
  }
  return false;
}

//END

// kate: space-indent on; indent-width 2; replace-tabs on;
