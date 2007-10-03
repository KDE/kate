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

#ifndef TIMEDATE_H
#define TIMEDATE_H

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <kxmlguiclient.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QVariantList>

static QString localizedTimeDate =
    I18N_NOOP2("This is a localized string for default time & date printing on kate document."
               "%e means day in XX format."
               "%m means month in XX format."
               "%Y means year in XXXX format."
               "%H means hours in XX format."
               "%M means minutes in XX format."
               "Please, if in your language time or date is written in a different order, change it here",
               "%e-%m-%Y %H:%M");

class TimeDatePlugin
  : public KTextEditor::Plugin
{
  public:
    explicit TimeDatePlugin(QObject *parent = 0,
                            const QVariantList &args = QVariantList());
    virtual ~TimeDatePlugin();

    static TimeDatePlugin *self() { return plugin; }

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);

    void readConfig();
    void writeConfig();

    virtual void readConfig (KConfig *) {}
    virtual void writeConfig (KConfig *) {}

    void setFormat(const QString &format);
    QString format() const;

  private:
    static TimeDatePlugin *plugin;
    QList<class TimeDatePluginView*> m_views;
    QString m_string;
};

class TimeDatePluginView
   : public QObject, public KXMLGUIClient
{
  Q_OBJECT

  public:
    explicit TimeDatePluginView(const QString &string,
                                KTextEditor::View *view = 0);
    ~TimeDatePluginView();

    void setFormat(const QString &format);
    QString format() const;

  private Q_SLOTS:
    void slotInsertTimeDate();

  private:
    KTextEditor::View *m_view;
    QString m_string;
};

K_PLUGIN_FACTORY_DECLARATION(TimeDatePluginFactory)

#endif // TIMEDATE_H
