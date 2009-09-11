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

#include "editorapp.h"
#include <kaboutdata.h>
#include <kcmdlineargs.h>

int main(int argc, char **argv) {
  KAboutData aboutData ("katesnippetstng_editor", 0, ki18n("Kate Snippets TNG Datafile Editor"), "0.1",
                        ki18n( "Kate Snippets TNG Datafile Editor" ), KAboutData::License_LGPL,
                        ki18n( "(c) 2009 Joseph Wenninger" ), KLocalizedString(), "http://www.kate-editor.org");
  aboutData.setOrganizationDomain("kde.org");
  aboutData.setProgramIconName("kate");
  KCmdLineArgs::init (argc, argv, &aboutData);
  KCmdLineOptions options;
  options.add("+[URL]", ki18n("Document to open"));
  KCmdLineArgs::addCmdLineOptions (options);
  EditorApp app;
  app.exec();
}

// kate: space-indent on; indent-width 2; replace-tabs on;