/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katetextbuffer.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

int main (int argc, char *argv[])
{
  // construct core app
  QCoreApplication app (argc, argv);

  // get arguments
  QString encoding = app.arguments().at(1);
  QString inFile = app.arguments().at(2);
  QString outFile = app.arguments().at(3);

  Kate::TextBuffer buffer (0);

  // set codec
  buffer.setFallbackTextCodec (QTextCodec::codecForName ("ISO 8859-15"));
  buffer.setTextCodec (QTextCodec::codecForName (encoding.toLatin1()));

  // switch to Mac EOL, this will test eol detection, as files are normal unix or dos
  buffer.setEndOfLineMode (Kate::TextBuffer::eolMac);

  // load file
  bool encodingErrors = false;
  if (!buffer.load (inFile, encodingErrors) || encodingErrors)
    return 1;

  // save file
  if (!buffer.save (outFile))
    return 1;

  return 0;
}
