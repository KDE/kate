/* This file is part of the KDE libraries
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

#include <QString>
#include <QDebug>
#include <KApplication>
#include <QWidget>
#include <KCmdLineArgs>
#include <QCheckBox>
#include <QPushButton>
#include <QGridLayout>

#include "katecolortreewidget.h"

#include <klocale.h>
#include <kcolorscheme.h>
#include <kcolorutils.h>

void fillList(KateColorTreeWidget* w);

int main(int argc, char *argv[])
{
  KCmdLineArgs::init(argc, argv, QByteArray("foo"), QByteArray("bar"), KLocalizedString(), QByteArray());
  KCmdLineOptions options;
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;

  QWidget* top = new QWidget();
  top->resize(500, 400);
  top->show();
  
  QGridLayout* l = new QGridLayout(top);
  top->setLayout(l);

  KateColorTreeWidget* w = new KateColorTreeWidget();
  
  QPushButton* btnSelectAll = new QPushButton("Use System Colors", top);
  QPushButton* btnSave = new QPushButton("Save to /tmp/test.cfg", top);
  QPushButton* btnLoad = new QPushButton("Load from /tmp/test.cfg", top);

  l->addWidget(w, 0, 0, 1, 3);
  l->addWidget(btnLoad, 1, 0);
  l->addWidget(btnSelectAll, 1, 1);
  l->addWidget(btnSave, 1, 2);

  fillList(w);

  QObject::connect(btnSelectAll, SIGNAL(clicked()), w, SLOT(selectDefaults()));
  QObject::connect(btnSave, SIGNAL(clicked()), w, SLOT(testWriteConfig()));
  QObject::connect(btnLoad, SIGNAL(clicked()), w, SLOT(testReadConfig()));


  return app.exec();
}

void fillList(KateColorTreeWidget* w)
{
  QVector<KateColorItem> items;

  //
  // editor background colors
  //
  KateColorItem ci;
  ci.category = "Editor Background Colors";

  ci.name = i18n("Text Area");
  ci.key = "Color Background";
  ci.whatsThis = i18n("<p>Sets the background color of the editing area.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
  items.append(ci);

  ci.name = i18n("Selected Text");
  ci.key = "Color Selection";
  ci.whatsThis = i18n("<p>Sets the background color of the selection.</p><p>To set the text color for selected text, use the &quot;<b>Configure Highlighting</b>&quot; dialog.</p>");
  ci.defaultColor = KColorScheme(QPalette::Inactive, KColorScheme::Selection).background().color();
  items.append(ci);

  ci.name = i18n("Current Line");
  ci.key = "Color Highlighted Line";
  ci.whatsThis = i18n("<p>Sets the background color of the currently active line, which means the line where your cursor is positioned.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::AlternateBackground).color();
  items.append(ci);

  ci.name = i18n("Search Highlight");
  ci.key = "Color Search Highlight";
  ci.whatsThis = i18n("<p>Sets the background color of search results.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NeutralBackground).color();
  items.append(ci);

  ci.name = i18n("Replace Highlight");
  ci.key = "Color Replace Highlight";
  ci.whatsThis = i18n("<p>Sets the background color of replaced text.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  items.append(ci);


  //
  // icon border
  //
  ci.category = i18n("Icon Border");

  ci.name = i18n("Background Area");
  ci.key = "Color Icon Bar";
  ci.whatsThis = i18n("<p>Sets the background color of the icon border.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
  items.append(ci);

  ci.name = i18n("Line Numbers");
  ci.key = "Color Line Number";
  ci.whatsThis = i18n("<p>This color will be used to draw the line numbers (if enabled) and the lines in the code-folding pane.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground().color();
  items.append(ci);

  ci.name = i18n("Word Wrap Marker");
  ci.key = "Color Word Wrap Marker";
  ci.whatsThis = i18n("<p>Sets the color of Word Wrap-related markers:</p><dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where text is going to be wrapped</dd><dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of visually-wrapped lines</dd></dl>");
  qreal bgLuma = KColorUtils::luma(KColorScheme(QPalette::Active, KColorScheme::View).background().color());
  ci.defaultColor = KColorUtils::shade( KColorScheme(QPalette::Active, KColorScheme::View).background().color(), bgLuma > 0.3 ? -0.15 : 0.03 );
  items.append(ci);
  
  ci.name = i18n("Saved Lines");
  ci.key = "Color Saved Lines";
  ci.whatsThis = i18n("<p>Sets the color of the line modification marker for saved lines.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  items.append(ci);

  ci.name = i18n("Modified Lines");
  ci.key = "Color Modified Lines";
  ci.whatsThis = i18n("<p>Sets the color of the line modification marker for modified lines.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
  items.append(ci);


  //
  // text decorations
  //
  ci.category = i18n("Text Decorations");

  ci.name = i18n("Spelling Mistake Line");
  ci.key = "Color Spelling Mistake Line";
  ci.whatsThis = i18n("<p>Sets the color of the line that is used to indicate spelling mistakes.</p>");
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::NegativeText).color();
  items.append(ci);

  ci.name = i18n("Tab and Space Markers");
  ci.key = "Color Tab Marker";
  ci.whatsThis = i18n("<p>Sets the color of the tabulator marks.</p>");
  ci.defaultColor = KColorUtils::shade(KColorScheme(QPalette::Active, KColorScheme::View).background().color(), bgLuma > 0.7 ? -0.35 : 0.3);
  items.append(ci);

  ci.name = i18n("Bracket Highlight");
  ci.key = "Color Highlighted Bracket";
  ci.whatsThis = i18n("<p>Sets the bracket matching color. This means, if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will be highlighted with this color.</p>");
  ci.defaultColor = KColorUtils::tint(KColorScheme(QPalette::Active, KColorScheme::View).background().color(),
                                      KColorScheme(QPalette::Active, KColorScheme::View).decoration(KColorScheme::HoverColor).color());
  items.append(ci);


  //
  // marker colors
  //
  ci.category = i18n("Marker Colors");

  ci.name = i18n("Bookmark");
  ci.key = "Color MarkType 1";
  ci.whatsThis = i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>");
  ci.defaultColor = Qt::blue; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Active Breakpoint");
  ci.key = "Color MarkType 2";
  ci.defaultColor = Qt::red; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Reached Breakpoint");
  ci.key = "Color MarkType 3";
  ci.defaultColor = Qt::yellow; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Disabled Breakpoint");
  ci.key = "Color MarkType 4";
  ci.defaultColor = Qt::magenta; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Execution");
  ci.key = "Color MarkType 5";
  ci.defaultColor = Qt::gray; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Warning");
  ci.key = "Color MarkType 6";
  ci.defaultColor = Qt::green; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = i18n("Error");
  ci.key = "Color MarkType 7";
  ci.defaultColor = Qt::red; // TODO: if possible, derive from system color scheme
  items.append(ci);


  //
  // text templates
  //
  ci.category = i18n("Text Templates & Snippets");
  
  ci.whatsThis = QString(); // TODO: add whatsThis for text templates

  ci.name = i18n("Background");
  ci.key = "Color Template Background";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
  items.append(ci);

  ci.name = i18n("Editable Placeholder");
  ci.key = "Color Template Editable Placeholder";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  items.append(ci);

  ci.name = i18n("Focused Editable Placeholder");
  ci.key = "Color Template Focused Editable Placeholder";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).background(KColorScheme::PositiveBackground).color();
  items.append(ci);
  
  ci.name = i18n("Not Editable Placeholder");
  ci.key = "Color Template Not Editable Placeholder";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
  items.append(ci);


  //
  // finally, add all elements
  //
  w->addColorItems(items);
}

// kate: replace-tabs on; indent-width 2;
