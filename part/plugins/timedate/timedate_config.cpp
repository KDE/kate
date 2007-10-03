/**
  * This file is part of the KDE libraries
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "timedate_config.h"
#include "timedate.h"

#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>

#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klineedit.h>

TimeDateConfig::TimeDateConfig(QWidget *parent, const QVariantList &args)
    : KCModule(TimeDatePluginFactory::componentData(), parent, args)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *info = new QLabel(i18n(
     "%y\t2-digit year excluding century (00 - 99)\n"
     "%Y\tfull year number\n"
     "%:m\tmonth number, without leading zero (1 - 12)\n"
     "%m\tmonth number, 2 digits (01 - 12)\n"
     "%b\tabbreviated month name\n"
     "%B\tfull month name\n"
     "%e\tday of the month (1 - 31)\n"
     "%d\tday of the month, 2 digits (01 - 31)\n"
     "%a\tabbreviated weekday name\n"
     "%A\tfull weekday name\n"
     "\n"
     "%H\thour in the 24 hour clock, 2 digits (00 - 23)\n"
     "%k\thour in the 24 hour clock, without leading zero (0 - 23)\n"
     "%I\thour in the 12 hour clock, 2 digits (01 - 12)\n"
     "%l\thour in the 12 hour clock, without leading zero (1 - 12)\n"
     "%M\tminute, 2 digits (00 - 59)\n"
     "%S\tseconds (00 - 59)\n"
     "%P\t\"am\" or \"pm\"\n"
     "%p\t\"AM\" or \"PM\"\n"));

    QHBoxLayout *hlayout = new QHBoxLayout(this);
    QLabel *lformat = new QLabel(i18n("Format"));
    format = new KLineEdit(this);
    hlayout->addWidget(lformat);
    hlayout->addWidget(format);

    layout->addWidget(info);
    layout->addLayout(hlayout);

    setLayout(layout);

    load();

    QObject::connect(format, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
}

TimeDateConfig::~TimeDateConfig()
{
}

void TimeDateConfig::save()
{
    if (TimeDatePlugin::self())
    {
        TimeDatePlugin::self()->setFormat(format->text());
        TimeDatePlugin::self()->writeConfig();
    }
    else
    {
        KConfigGroup cg(KGlobal::config(), "TimeDate Plugin");
        cg.writeEntry("string", format->text());
    }

    emit changed(false);
}

void TimeDateConfig::load()
{
    if (TimeDatePlugin::self())
    {
        TimeDatePlugin::self()->readConfig();
        format->setText(TimeDatePlugin::self()->format());
    }
    else
    {
        KConfigGroup cg(KGlobal::config(), "TimeDate Plugin" );
        format->setText(cg.readEntry("string", localizedTimeDate));
    }

    emit changed(false);
}

void TimeDateConfig::defaults()
{
    format->setText(localizedTimeDate);

    emit changed(true);
}

void TimeDateConfig::slotChanged()
{
    emit changed(true);
}

#include "timedate_config.moc"
