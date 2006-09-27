/** kate-js-indenter
 * name: Next Indenter
 * license: LGPL
 * author: Dominik Haumann <dhdev@gmx.de>
 * version: 1
 * kateversion: 3.0
 *
 * This file is part of the Kate Project.
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

indenter.indent = indent;
indenter.triggerCharacters = "{};\t";

//BEGIN indentation functions
/**
 * Return indentation filler string.
 * parameters: count [number], alignment [number]
 * If expandTab is true, the returned filler only contains spaces.
 * If expandTab is false, the returned filler contains tabs upto alignment
 * if possible, and from alignment to the end of text spaces. This implements
 * mixed indentation, i.e. indentation+alignment.
 */
function indent(line, typedChar)
{
  return 5;
}

//END main/entry functions

// kate: space-indent on; indent-width 4; replace-tabs on;
