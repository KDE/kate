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

#include "snippeteditornewdialog.h"
#include "snippeteditornewdialog.moc"
#include <kguiitem.h>

SnippetEditorNewDialog::SnippetEditorNewDialog(QWidget *parent):
  KDialog(parent),Ui::NewSnippetFileView()
{
  setWindowModality(Qt::WindowModal);
  setupUi(mainWidget());
  setCaption(i18n("Create new snippet file"));
  setButtons(KDialog::Ok | KDialog::Cancel);
  setButtonGuiItem(KDialog::Ok,KGuiItem(i18n("Create"),"document-save"));
  
}

SnippetEditorNewDialog::~SnippetEditorNewDialog()
{
}


