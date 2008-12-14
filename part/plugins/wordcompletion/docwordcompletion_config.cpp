/**
  * This file is part of the KDE libraries
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
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

#include "docwordcompletion_config.h"
#include "docwordcompletion.h"

#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QSpinBox>

#include <khbox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kgenericfactory.h>

#ifndef KDE_USE_FINAL
K_PLUGIN_FACTORY_DECLARATION(DocWordCompletionFactory)
#endif

DocWordCompletionConfig::DocWordCompletionConfig(QWidget *parent, const QVariantList &args)
    : KCModule(DocWordCompletionFactory::componentData(), parent, args)
{
    QVBoxLayout *lo = new QVBoxLayout( this );
    lo->setSpacing( KDialog::spacingHint() );

    cbAutoPopup = new QCheckBox( i18n("Automatically &show completion list"), this );
    lo->addWidget( cbAutoPopup );

    hbTreshold = new KHBox( this );
    hbTreshold->setSpacing( KDialog::spacingHint() );
    lo->addWidget( hbTreshold );
    QLabel *l = new QLabel( i18nc(
        "Translators: This is the first part of two strings which will comprise the "
        "sentence 'Show completions when a word is at least N characters'. The first "
        "part is on the right side of the N, which is represented by a spinbox "
        "widget, followed by the second part: 'characters long'. Characters is a "
        "integer number between and including 1 and 30. Feel free to leave the "
        "second part of the sentence blank if it suits your language better. ",
        "Show completions &when a word is at least"), hbTreshold );
    sbAutoPopup = new QSpinBox( hbTreshold );
    sbAutoPopup->setRange( 1, 30 );
    sbAutoPopup->setSingleStep( 1 );
    l->setBuddy( sbAutoPopup );
    lSbRight = new QLabel( i18nc(
        "This is the second part of two strings that will comprise the sentence "
        "'Show completions when a word is at least N characters'",
        "characters long."), hbTreshold );

    cbAutoPopup->setWhatsThis(i18n(
        "Enable the automatic completion list popup as default. The popup can "
        "be disabled on a view basis from the 'Tools' menu.") );
    sbAutoPopup->setWhatsThis(i18n(
        "Define the length a word should have before the completion list "
        "is displayed.") );

    lo->addStretch();

    QObject::connect(cbAutoPopup, SIGNAL(stateChanged(int)), this, SLOT(slotChanged()));

    QObject::connect(sbAutoPopup, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

    load();

    // The reason for emitting the changed signal here is that this plugin adds a menu
    // entry "[x] Auto pop up completion list". As this setting is on the plugin
    // configuration dialog too, the behavior is the next:
    //
    // 1. This dialog is not going to show when opened the properties of the view.
    //    It is going to show the properties that will be applied to new views,
    //    and the properties that are going to be applied when opening Kate/Kwrite
    //    again.
    //
    // 2. If we go to the plugin configuration dialog and the state of this property
    //    is different than the one on the menu of the view, set the property view to this
    //    dialog property value only if we hit "OK". If this signal weren't emitted,
    //    KPluginSelector would just ignore the saving of the settings (because
    //    it considers that no changes were made to this dialog. Probably nobody
    //    clicked on any widget of this config dialog, but the properties values of this dialog
    //    and the view menu ones are different).

    QMetaObject::invokeMethod(this, "changed", Qt::QueuedConnection);
}

DocWordCompletionConfig::~DocWordCompletionConfig()
{
}

void DocWordCompletionConfig::save()
{
    if (DocWordCompletionPlugin::self())
    {
        DocWordCompletionPlugin::self()->setTreshold(sbAutoPopup->value());
        DocWordCompletionPlugin::self()->setAutoPopupEnabled(cbAutoPopup->isChecked());
        DocWordCompletionPlugin::self()->writeConfig();
    }
    else
    {
        KConfigGroup cg(KGlobal::config(), "DocWordCompletion Plugin");
        cg.writeEntry("treshold", sbAutoPopup->value());
        cg.writeEntry("autopopup", cbAutoPopup->isChecked());
    }

    emit changed(false);
}

void DocWordCompletionConfig::load()
{
    if (DocWordCompletionPlugin::self())
    {
        DocWordCompletionPlugin::self()->readConfig();
        sbAutoPopup->setValue(DocWordCompletionPlugin::self()->treshold());
        cbAutoPopup->setChecked(DocWordCompletionPlugin::self()->autoPopupEnabled());
    }
    else
    {
        KConfigGroup cg(KGlobal::config(), "DocWordCompletion Plugin");
        sbAutoPopup->setValue(cg.readEntry("treshold", 3));
        cbAutoPopup->setChecked(cg.readEntry("autopopup", true));
    }

    emit changed(false);
}

void DocWordCompletionConfig::defaults()
{
    cbAutoPopup->setChecked(true);
    sbAutoPopup->setValue(3);

    emit changed(true);
}

void DocWordCompletionConfig::slotChanged()
{
    hbTreshold->setEnabled(cbAutoPopup->isChecked());
    emit changed(true);
}

#include "docwordcompletion_config.moc"
