/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001-2002 Michael Goffioul <goffioul@imec.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#include "kateprintsettings.h"

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <kcolorbutton.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qwhatsthis.h>
#include <klocale.h>

KatePrintSettings::KatePrintSettings(KPrinter *printer, QWidget *parent, const char *name)
: KPrintDialogPage(parent, name), m_printer(printer)
{
  setTitle(i18n("Text Settings"));

  m_usebox = new QCheckBox(i18n("Use &Box"), this);
  m_useheader = new QCheckBox(i18n("Use He&ader"), this);
  m_boxcolor = new KColorButton(Qt::black, this);
  m_headercolor = new KColorButton(Qt::lightGray, this);
  m_fontcolor = new KColorButton(Qt::black, this);
  m_headerright = new QLineEdit(this);
  m_headercenter = new QLineEdit(this);
  m_headerleft = new QLineEdit(this);
  m_boxwidth = new QSpinBox(0, 99, 1, this);

  QLabel  *m_boxcolorlabel = new QLabel(i18n("C&olor:"), this);
  QLabel  *m_boxwidthlabel = new QLabel(i18n("W&idth:"), this);
  QLabel  *m_headercolorlabel = new QLabel(i18n("Bac&kground Color:"), this);
  QLabel  *m_fontcolorlabel = new QLabel(i18n("&Font Color:"), this);
  QLabel  *m_headerlabel = new QLabel(i18n("For&mat:"), this);

  m_headerlabel->setBuddy(m_headerright);
  m_boxcolorlabel->setBuddy(m_boxcolor);
  m_headercolorlabel->setBuddy(m_headercolor);
  m_fontcolorlabel->setBuddy(m_fontcolor);
  m_boxwidthlabel->setBuddy(m_boxwidth);

  // layout
  QHBoxLayout  *l00 = new QHBoxLayout(this, 0, 10);
  QVBoxLayout  *l0 = new QVBoxLayout(0, 0, 5);
  l00->addLayout(l0);
  l0->addWidget(m_usebox);
  QHBoxLayout  *l1 = new QHBoxLayout(0, 0, 5);
  l0->addLayout(l1);
  l1->addSpacing(20);
  l1->addWidget(m_boxwidthlabel);
  l1->addWidget(m_boxwidth);
  l1->addSpacing(10);
  l1->addWidget(m_boxcolorlabel);
  l1->addWidget(m_boxcolor);
  l1->addStretch(1);
  l0->addSpacing(5);
  l0->addWidget(m_useheader);
  QHBoxLayout  *l2 = new QHBoxLayout(0, 0, 5), *l3 = new QHBoxLayout(0, 0, 5);
  l0->addLayout(l2);
  l0->addLayout(l3);
  l2->addSpacing(20);
  l2->addWidget(m_headercolorlabel);
  l2->addWidget(m_headercolor);
  l2->addSpacing(10);
  l2->addWidget(m_fontcolorlabel);
  l2->addWidget(m_fontcolor);
  l2->addStretch(1);
  l3->addSpacing(20);
  l3->addWidget(m_headerlabel);
  l3->addWidget(m_headerright);
  l3->addWidget(m_headercenter);
  l3->addWidget(m_headerleft);
  l3->addStretch(1);
  // FIXME: add kind of preview
  l00->addStretch(1);

  // connections
  connect(m_usebox, SIGNAL(toggled(bool)), m_boxcolor, SLOT(setEnabled(bool)));
  connect(m_usebox, SIGNAL(toggled(bool)), m_boxwidth, SLOT(setEnabled(bool)));
  connect(m_useheader, SIGNAL(toggled(bool)), m_headercolor, SLOT(setEnabled(bool)));
  connect(m_useheader, SIGNAL(toggled(bool)), m_fontcolor, SLOT(setEnabled(bool)));
  connect(m_useheader, SIGNAL(toggled(bool)), m_headerright, SLOT(setEnabled(bool)));
  connect(m_useheader, SIGNAL(toggled(bool)), m_headercenter, SLOT(setEnabled(bool)));
  connect(m_useheader, SIGNAL(toggled(bool)), m_headerleft, SLOT(setEnabled(bool)));

  // default values
  m_headerright->setText("%y");
  m_headercenter->setText("%f");
  m_headerleft->setText("%p");
  m_boxwidth->setValue(0);
  m_usebox->setChecked(true);
  m_useheader->setChecked(true);

  QString  s = i18n("<p>Format of the page header. The following tags are supported:</p>"
      "<ul><li><tt>%u</tt> : current user name</li>"
      "<li><tt>%d</tt> : complete date/time in short format</li>"
      "<li><tt>%D</tt> : complete date/time in long format</li>"
      "<li><tt>%h</tt> : current time</li>"
      "<li><tt>%y</tt> : current date in short format</li>"
      "<li><tt>%Y</tt> : current date in long format</li>"
      "<li><tt>%f</tt> : file name</li>"
      "<li><tt>%P</tt> : path name (with file name)</li>"
      "<li><tt>%p</tt> : page number</li>"
      "</ul><br>"
      "<u>Note:</u> Do <b>not</b> use the '|' (vertical bar) character.");
  QWhatsThis::add(m_headerright, s);
  QWhatsThis::add(m_headercenter, s);
  QWhatsThis::add(m_headerleft, s);
  QWhatsThis::add(m_headerlabel, s);
}

KatePrintSettings::~KatePrintSettings()
{
}

void KatePrintSettings::getOptions(QMap<QString,QString>& opts, bool)
{
  opts["app-kate-usebox"] = (m_usebox->isChecked() ? "true" : "false");
  opts["app-kate-useheader"] = (m_useheader->isChecked() ? "true" : "false");
  opts["app-kate-boxcolor"] = m_boxcolor->color().name();
  opts["app-kate-headercolor"] = m_headercolor->color().name();
  opts["app-kate-fontcolor"] = m_fontcolor->color().name();
  opts["app-kate-boxwidth"] = QString::number(m_boxwidth->value());
  opts["app-kate-headerformat"] = m_headerright->text() + "|" + m_headercenter->text() + "|" + m_headerleft->text();
}

void KatePrintSettings::setOptions(const QMap<QString,QString>& opts)
{
  m_usebox->setChecked(opts["app-kate-usebox"] == "true");
  m_useheader->setChecked(opts["app-kate-useheader"] == "true");
  QString  val;
  if (!(val=opts["app-kate-boxcolor"]).isEmpty()) m_boxcolor->setColor(QColor(val));
  if (!(val=opts["app-kate-headercolor"]).isEmpty()) m_headercolor->setColor(QColor(val));
  if (!(val=opts["app-kate-fontcolor"]).isEmpty()) m_fontcolor->setColor(QColor(val));
  m_boxwidth->setValue(opts["app-kate-boxwidth"].toInt());
  QStringList  tags = QStringList::split('|', opts["app-kate-headerformat"], true);
  if (tags.count() == 3)
  {
    m_headerright->setText(tags[0]);
    m_headercenter->setText(tags[1]);
    m_headerleft->setText(tags[2]);
  }
}

#include "kateprintsettings.moc"
