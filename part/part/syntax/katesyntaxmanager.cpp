/* This file is part of the KDE libraries
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
#include "katesyntaxmanager.h"
#include "katesyntaxmanager.moc"

#include "katetextline.h"
#include "katedocument.h"
#include "katesyntaxdocument.h"
#include "katerenderer.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "kateconfig.h"
#include "kateextendedattribute.h"
#include "katehighlight.h"

#include <kconfig.h>
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

#include <QtCore/QSet>
#include <QtGui/QAction>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
//END

//BEGIN KateHlManager
KateHlManager::KateHlManager()
  : QObject()
  , m_config ("katesyntaxhighlightingrc", KConfig::NoGlobals)
  , commonSuffixes (QString(".orig;.new;~;.bak;.BAK").split(';'))
  , syntax (new KateSyntaxDocument(&m_config))
  , dynamicCtxsCount(0)
  , forceNoDCReset(false)
{
  KateSyntaxModeList modeList = syntax->modeList();
  for (int i=0; i < modeList.count(); i++)
  {
    KateHighlighting *hl = new KateHighlighting(modeList[i]);

    int insert = 0;
    for (; insert <= hlList.count(); insert++)
    {
      if (insert == hlList.count())
        break;

      if ( QString(hlList.at(insert)->section() + hlList.at(insert)->nameTranslated()).toLower()
            > QString(hl->section() + hl->nameTranslated()).toLower() )
        break;
    }

    hlList.insert (insert, hl);
    hlDict.insert (hl->name(), hl);
  }

  // Normal HL
  KateHighlighting *hl = new KateHighlighting(0);
  hlList.prepend (hl);
  hlDict.insert (hl->name(), hl);

  lastCtxsReset.start();
}

KateHlManager::~KateHlManager()
{
  delete syntax;
  qDeleteAll(hlList);
}

KateHlManager *KateHlManager::self()
{
  return KateGlobal::self ()->hlManager ();
}

KateHighlighting *KateHlManager::getHl(int n)
{
  if (n < 0 || n >= hlList.count())
    n = 0;

  return hlList.at(n);
}

int KateHlManager::nameFind(const QString &name)
{
  int z (hlList.count() - 1);
  for (; z > 0; z--)
    if (hlList.at(z)->name() == name)
      return z;

  return z;
}

int KateHlManager::detectHighlighting (KateDocument *doc)
{
  int hl = wildcardFind( doc->url().fileName() );
  if ( hl < 0 )
    hl = mimeFind ( doc );

  return hl;
}

int KateHlManager::wildcardFind(const QString &fileName)
{
  int result = -1;
  if ((result = realWildcardFind(fileName)) != -1)
    return result;

  int length = fileName.length();
  QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
  if (fileName.endsWith(backupSuffix)) {
    if ((result = realWildcardFind(fileName.left(length - backupSuffix.length()))) != -1)
      return result;
  }

  for (QStringList::Iterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
    if (*it != backupSuffix && fileName.endsWith(*it)) {
      if ((result = realWildcardFind(fileName.left(length - (*it).length()))) != -1)
        return result;
    }
  }

  return -1;
}

int KateHlManager::realWildcardFind(const QString &fileName)
{
  QList<KateHighlighting*> highlights;

  foreach (KateHighlighting *highlight, hlList) {
    highlight->loadWildcards();

    foreach (QString extension, highlight->getPlainExtensions())
      if (fileName.endsWith(extension))
        highlights.append(highlight);

    foreach (QRegExp re, highlight->getRegexpExtensions()) {
      if (re.exactMatch(fileName))
        highlights.append(highlight);
    }
  }

  if ( !highlights.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    foreach (KateHighlighting *highlight, highlights)
    {
      if (highlight->priority() > pri)
      {
        pri = highlight->priority();
        hl = hlList.indexOf(highlight);
      }
    }
    return hl;
  }

  return -1;
}

int KateHlManager::mimeFind( KateDocument *doc )
{
  static QRegExp sep("\\s*;\\s*");

  KMimeType::Ptr mt = doc->mimeTypeForContent();

  QList<KateHighlighting*> highlights;

  foreach (KateHighlighting *highlight, hlList)
  {
    foreach (QString mimeType, highlight->getMimetypes().split( sep, QString::SkipEmptyParts ))
    {
      if ( mimeType == mt->name() ) // faster than a regexp i guess?
        highlights.append (highlight);
    }
  }

  if ( !highlights.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    foreach (KateHighlighting *highlight, highlights)
    {
      if (highlight->priority() > pri)
      {
        pri = highlight->priority();
        hl = hlList.indexOf(highlight);
      }
    }

    return hl;
  }

  return -1;
}

uint KateHlManager::defaultStyles()
{
  return 14;
}

QString KateHlManager::defaultStyleName(int n, bool translateNames)
{
  static QStringList names;
  static QStringList translatedNames;

  if (names.isEmpty())
  {
    names << "Normal";
    names << "Keyword";
    names << "Data Type";
    names << "Decimal/Value";
    names << "Base-N Integer";
    names << "Floating Point";
    names << "Character";
    names << "String";
    names << "Comment";
    names << "Others";
    names << "Alert";
    names << "Function";
    // this next one is for denoting the beginning/end of a user defined folding region
    names << "Region Marker";
    // this one is for marking invalid input
    names << "Error";

    translatedNames << i18n("Normal");
    translatedNames << i18n("Keyword");
    translatedNames << i18n("Data Type");
    translatedNames << i18n("Decimal/Value");
    translatedNames << i18n("Base-N Integer");
    translatedNames << i18n("Floating Point");
    translatedNames << i18n("Character");
    translatedNames << i18n("String");
    translatedNames << i18n("Comment");
    translatedNames << i18n("Others");
    translatedNames << i18n("Alert");
    translatedNames << i18n("Function");
    // this next one is for denoting the beginning/end of a user defined folding region
    translatedNames << i18n("Region Marker");
    // this one is for marking invalid input
    translatedNames << i18n("Error");
  }

  return translateNames ? translatedNames[n] : names[n];
}

void KateHlManager::getDefaults(uint schema, KateAttributeList &list)
{
  KTextEditor::Attribute::Ptr normal(new KTextEditor::Attribute());
  normal->setForeground(Qt::black);
  normal->setSelectedForeground(Qt::white);
  list.append(normal);

  KTextEditor::Attribute::Ptr keyword(new KTextEditor::Attribute());
  keyword->setForeground(Qt::black);
  keyword->setSelectedForeground(Qt::white);
  keyword->setFontBold(true);
  list.append(keyword);

  KTextEditor::Attribute::Ptr dataType(new KTextEditor::Attribute());
  dataType->setForeground(Qt::darkRed);
  dataType->setSelectedForeground(Qt::white);
  list.append(dataType);

  KTextEditor::Attribute::Ptr decimal(new KTextEditor::Attribute());
  decimal->setForeground(Qt::blue);
  decimal->setSelectedForeground(Qt::cyan);
  list.append(decimal);

  KTextEditor::Attribute::Ptr basen(new KTextEditor::Attribute());
  basen->setForeground(Qt::darkCyan);
  basen->setSelectedForeground(Qt::cyan);
  list.append(basen);

  KTextEditor::Attribute::Ptr floatAttribute(new KTextEditor::Attribute());
  floatAttribute->setForeground(Qt::darkMagenta);
  floatAttribute->setSelectedForeground(Qt::cyan);
  list.append(floatAttribute);

  KTextEditor::Attribute::Ptr charAttribute(new KTextEditor::Attribute());
  charAttribute->setForeground(Qt::magenta);
  charAttribute->setSelectedForeground(Qt::magenta);
  list.append(charAttribute);

  KTextEditor::Attribute::Ptr string(new KTextEditor::Attribute());
  string->setForeground(QColor::QColor("#D00"));
  string->setSelectedForeground(Qt::red);
  list.append(string);

  KTextEditor::Attribute::Ptr comment(new KTextEditor::Attribute());
  comment->setForeground(Qt::darkGray);
  comment->setSelectedForeground(Qt::gray);
  comment->setFontItalic(true);
  list.append(comment);

  KTextEditor::Attribute::Ptr others(new KTextEditor::Attribute());
  others->setForeground(Qt::darkGreen);
  others->setSelectedForeground(Qt::green);
  list.append(others);

  KTextEditor::Attribute::Ptr alert(new KTextEditor::Attribute());
  alert->setForeground(Qt::black);
  alert->setSelectedForeground( QColor::QColor("#FCC") );
  alert->setFontBold(true);
  alert->setBackground( QColor::QColor("#FCC") );
  list.append(alert);

  KTextEditor::Attribute::Ptr functionAttribute(new KTextEditor::Attribute());
  functionAttribute->setForeground(Qt::darkBlue);
  functionAttribute->setSelectedForeground(Qt::white);
  list.append(functionAttribute);

  KTextEditor::Attribute::Ptr regionmarker(new KTextEditor::Attribute());
  regionmarker->setForeground(Qt::white);
  regionmarker->setBackground(Qt::gray);
  regionmarker->setSelectedForeground(Qt::gray);
  list.append(regionmarker);

  KTextEditor::Attribute::Ptr error(new KTextEditor::Attribute());
  error->setForeground(Qt::red);
  error->setFontUnderline(true);
  error->setSelectedForeground(Qt::red);
  list.append(error);

  KConfigGroup config(KateHlManager::self()->self()->getKConfig(),
                      "Default Item Styles - Schema " + KateGlobal::self()->schemaManager()->name(schema));

  for (uint z = 0; z < defaultStyles(); z++)
  {
    KTextEditor::Attribute::Ptr i = list.at(z);
    QStringList s = config.readEntry(defaultStyleName(z), QStringList());
    if (!s.isEmpty())
    {
      while( s.count()<8)
        s << "";

      QString tmp;
      QRgb col;

      tmp=s[0]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); i->setForeground(QColor(col)); }

      tmp=s[1]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); i->setSelectedForeground(QColor(col)); }

      tmp=s[2]; if (!tmp.isEmpty()) i->setFontBold(tmp!="0");

      tmp=s[3]; if (!tmp.isEmpty()) i->setFontItalic(tmp!="0");

      tmp=s[4]; if (!tmp.isEmpty()) i->setFontStrikeOut(tmp!="0");

      tmp=s[5]; if (!tmp.isEmpty()) i->setFontUnderline(tmp!="0");

      tmp=s[6]; if (!tmp.isEmpty()) {
        if ( tmp != "-" )
        {
          col=tmp.toUInt(0,16);
          i->setBackground(QColor(col));
        }
        else
          i->clearBackground();
      }
      tmp=s[7]; if (!tmp.isEmpty()) {
        if ( tmp != "-" )
        {
          col=tmp.toUInt(0,16);
          i->setSelectedBackground(QColor(col));
        }
        else
          i->clearProperty(KTextEditor::Attribute::SelectedBackground);
      }
    }
  }
}

void KateHlManager::setDefaults(uint schema, KateAttributeList &list)
{
  KConfigGroup config(KateHlManager::self()->self()->getKConfig(),
                      "Default Item Styles - Schema " + KateGlobal::self()->schemaManager()->name(schema));

  for (uint z = 0; z < defaultStyles(); z++)
  {
    QStringList settings;
    KTextEditor::Attribute::Ptr p = list.at(z);

    settings<<(p->hasProperty(QTextFormat::ForegroundBrush)?QString::number(p->foreground().color().rgb(),16):"");
    settings<<(p->hasProperty(KTextEditor::Attribute::SelectedForeground)?QString::number(p->selectedForeground().color().rgb(),16):"");
    settings<<(p->hasProperty(QTextFormat::FontWeight)?(p->fontBold()?"1":"0"):"");
    settings<<(p->hasProperty(QTextFormat::FontItalic)?(p->fontItalic()?"1":"0"):"");
    settings<<(p->hasProperty(QTextFormat::FontStrikeOut)?(p->fontStrikeOut()?"1":"0"):"");
    settings<<(p->hasProperty(QTextFormat::FontUnderline)?(p->fontUnderline()?"1":"0"):"");
    settings<<(p->hasProperty(QTextFormat::BackgroundBrush)?QString::number(p->background().color().rgb(),16):"");
    settings<<(p->hasProperty(KTextEditor::Attribute::SelectedBackground)?QString::number(p->selectedBackground().color().rgb(),16):"");
    settings<<"---";

    config.writeEntry(defaultStyleName(z),settings);
  }

  emit changed();
}

int KateHlManager::highlights()
{
  return (int) hlList.count();
}

QString KateHlManager::hlName(int n)
{
  return hlList.at(n)->name();
}

QString KateHlManager::hlNameTranslated(int n)
{
  return hlList.at(n)->nameTranslated();
}

QString KateHlManager::hlSection(int n)
{
  return hlList.at(n)->section();
}

bool KateHlManager::hlHidden(int n)
{
  return hlList.at(n)->hidden();
}

QString KateHlManager::identifierForName(const QString& name)
{
  KateHighlighting *hl = 0;

  if ((hl = hlDict[name]))
    return hl->getIdentifier ();

  return QString();
}

bool KateHlManager::resetDynamicCtxs()
{
  if (forceNoDCReset)
    return false;

  if (lastCtxsReset.elapsed() < KATE_DYNAMIC_CONTEXTS_RESET_DELAY)
    return false;

  foreach (KateHighlighting *hl, hlList)
    hl->dropDynamicContexts();

  dynamicCtxsCount = 0;
  lastCtxsReset.start();

  return true;
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on;
