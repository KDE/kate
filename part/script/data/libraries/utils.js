/*
 * name: Helper Functions (copied from cppstyle.js to be reused by other indenters)
 * license: LGPL v3
 * author: Alex Turbov <i.zaufi@gmail.com>
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

//BEGIN Utils
/**
 * \brief Print or suppress debug output depending on \c debugMode variable.
 *
 * The mentioned \c debugMode supposed to be defined by a particular indenter
 * (external code).
 */
function dbg()
{
    if (debugMode)
    {
        debug.apply(this, arguments);
    }
}

/**
 * \return \c true if attribute at given position is a \e Comment
 *
 * \note C++ highlighter use \em RegionMarker for special comments,
 * soit must be counted as well...
 */
function isComment(line, column)
{
    // Check if we are not withing a comment
    var c = new Cursor(line, column);
    var mode = document.attributeName(c);
    dbg("isComment: Check mode @ " + c + ": " + mode);
    return mode.startsWith("Doxygen")
      || mode.startsWith("Alerts")
      || document.isComment(c)
      || document.isRegionMarker(c)
      ;
}

/**
 * \return \c true if attribute at given position is a \e String
 */
function isString(line, column)
{
    // Check if we are not withing a string
    var c = new Cursor(line, column);
    var mode = document.attributeName(c);
    dbg("isString: Check mode @ " + c + ": " + mode);
    return document.isString(c) || document.isChar(c);
}

/**
 * \return \c true if attribute at given position is a \e String or \e Comment
 *
 * \note C++ highlighter use \e RegionMarker for special comments,
 * soit must be counted as well...
 */
function isStringOrComment(line, column)
{
    // Check if we are not withing a string or a comment
    var c = new Cursor(line, column);
    var mode = document.attributeName(c);
    dbg("isStringOrComment: Check mode @ " + c + ": " + mode);
    return mode.startsWith("Doxygen")
      || mode.startsWith("Alerts")
      || document.isString(c)
      || document.isChar(c)
      || document.isComment(c)
      || document.isRegionMarker(c)
      ;
}

/**
 * Split a given text line by comment into parts \e before and \e after the comment
 * \return an object w/ the following fields:
 *   \li \c hasComment -- boolean: \c true if comment present on the line, \c false otherwise
 *   \li \c before -- text before the comment
 *   \li \c after -- text of the comment
 *
 * \todo Possible it would be quite reasonable to analyze a type of the comment:
 * Is it C++ or Doxygen? Is it single or w/ some text before?
 */
function splitByComment(line)
{
    var before = "";
    var after = "";
    var text = document.line(line);
    dbg("splitByComment: text='"+text+"'");

    var comment_marker = document.commentMarker(document.attribute(line, 0));
    var text = document.line(line);
    var found = false;
    for (
        var pos = text.indexOf(comment_marker)
      ; pos != -1
      ; pos = text.indexOf(comment_marker, pos + 1)
      )
    {
        // Check attribute to be sure...
        if (isComment(line, pos))
        {
            // Got it!
            before = text.substring(0, pos);
            after = text.substring(pos + comment_marker.length, text.length);
            found = true;
            break;
        }
    }
    // If no comment actually found, then set text before to the original
    if (!found)
        before = text;
    dbg("splitByComment result: hasComment="+found+", before='"+before+"', after='"+after+"'");
    return {hasComment: found, before: before, after: after};
}

/**
 * \brief Remove possible comment from text
 */
function stripComment(line)
{
    var result = splitByComment(line);
    if (result.hasComment)
        return result.before.rtrim();
    return result.before.rtrim();
}

/**
 * Add a character \c c to the given position if absent.
 * Set new cursor position to the next one after the current.
 *
 * \return \c true if a desired char was added
 */
function addCharOrJumpOverIt(line, column, char)
{
    // Make sure there is a space at given position
    dbg("addCharOrJumpOverIt: checking @Cursor("+line+","+column+"), c='"+document.charAt(line, column)+"'");
    var result = false;
    if (document.lineLength(line) <= column || document.charAt(line, column) != char)
    {
        document.insertText(line, column, char);
        result = true;
    }
    /// \todo Does it really needed?
    view.setCursorPosition(line, column + 1);
    return result;
}

/**
 * Check if a character right before cursor is the very first on the line
 * and the same as a given one.
 */
function justEnteredCharIsFirstOnLine(line, column, char)
{
    return document.firstChar(line) == char && document.firstColumn(line) == (column - 1);
}
//END Utils

/**
 * \todo Unit tests! How?
 */

// kate: space-indent on; indent-width 4; replace-tabs on;
