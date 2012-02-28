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

  ci.name = "Text Area";
  ci.key = "Color Background";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
  items.append(ci);

  ci.name = "Selected Text";
  ci.key = "Color Selection";
  ci.defaultColor = KColorScheme(QPalette::Inactive, KColorScheme::Selection).background().color();
  items.append(ci);

  ci.name = "Current Line";
  ci.key = "Color Highlighted Line";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::AlternateBackground).color();
  items.append(ci);

  ci.name = "Search Highlight";
  ci.key = "Color Search Highlight";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NeutralBackground).color();
  items.append(ci);

  ci.name = "Replace Highlight";
  ci.key = "Color Replace Highlight";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  items.append(ci);


  //
  // icon border
  //
  ci.category = "Icon Border";

  ci.name = "Background Area";
  ci.key = "Color Icon Bar";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
  items.append(ci);

  ci.name = "Line Numbers";
  ci.key = "Color Line Number";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground().color();
  items.append(ci);

  ci.name = "Word Wrap Marker";
  ci.key = "Color Word Wrap Marker";
  qreal bgLuma = KColorUtils::luma(KColorScheme(QPalette::Active, KColorScheme::View).background().color());
  ci.defaultColor = KColorUtils::shade( KColorScheme(QPalette::Active, KColorScheme::View).background().color(), bgLuma > 0.3 ? -0.15 : 0.03 );
  items.append(ci);
  
  ci.name = "Saved Lines";
  ci.key = "Color Saved Lines";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  items.append(ci);

  ci.name = "Modified Lines";
  ci.key = "Color Modified Lines";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
  items.append(ci);


  //
  // text decorations
  //
  ci.category = "Text Decorations";

  ci.name = "Spelling Mistake Line";
  ci.key = "Color Spelling Mistake Line";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::NegativeText).color();
  items.append(ci);

  ci.name = "Tab and Space Markers";
  ci.key = "Color Tab Marker";
  ci.defaultColor = KColorUtils::shade(KColorScheme(QPalette::Active, KColorScheme::View).background().color(), bgLuma > 0.7 ? -0.35 : 0.3);
  items.append(ci);

  ci.name = "Bracket Highlight";
  ci.key = "Color Highlighted Bracket";
  ci.defaultColor = KColorUtils::tint(KColorScheme(QPalette::Active, KColorScheme::View).background().color(),
                                      KColorScheme(QPalette::Active, KColorScheme::View).decoration(KColorScheme::HoverColor).color());
  items.append(ci);


  //
  // marker colors
  //
  ci.category = "Marker Colors";

  ci.name = "Bookmark";
  ci.key = "Color MarkType 1";
  ci.defaultColor = Qt::blue; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = "Active Breakpoint";
  ci.key = "Color MarkType 2";
  ci.defaultColor = Qt::red; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = "Reached Breakpoint";
  ci.key = "Color MarkType 3";
  ci.defaultColor = Qt::yellow; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = "Disabled Breakpoint";
  ci.key = "Color MarkType 4";
  ci.defaultColor = Qt::magenta; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = "Execution";
  ci.key = "Color MarkType 5";
  ci.defaultColor = Qt::gray; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = "Warning";
  ci.key = "Color MarkType 6";
  ci.defaultColor = Qt::green; // TODO: if possible, derive from system color scheme
  items.append(ci);

  ci.name = "Error";
  ci.key = "Color MarkType 7";
  ci.defaultColor = Qt::red; // TODO: if possible, derive from system color scheme
  items.append(ci);


  //
  // text templates
  //
  ci.category = "Text Templates & Snippets";

  ci.name = "Background";
  ci.key = "Color Template Background";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
  items.append(ci);

  ci.name = "Editable Placeholder";
  ci.key = "Color Template Editable Placeholder";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
  items.append(ci);

  ci.name = "Focused Editable Placeholder";
  ci.key = "Color Template Focused Editable Placeholder";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::Window).background(KColorScheme::PositiveBackground).color();
  items.append(ci);
  
  ci.name = "Not Editable Placeholder";
  ci.key = "Color Template Not Editable Placeholder";
  ci.defaultColor = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
  items.append(ci);


  //
  // finally, add all elements
  //
  w->addColorItems(items);
}

// kate: replace-tabs on; indent-width 2;
