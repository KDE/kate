/** kate-script
 * name: Ruby
 * license: LGPL
 * author: Robin Pedersen <robin.pedersen@runbox.com>
 * version: 1
 * kate-version: 3.0
 * type: indentation
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

//BEGIN USER CONFIGURATION
//END USER CONFIGURATION


// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = "cdefhilnrsuw}]";

// Indent after lines that match this regexp
var rxIndent = /^\s*(def|if|unless|for|while|until|class|module|else|elsif|case|when|begin|rescue|ensure|catch)\b/;

// Unindent lines that match this regexp
var rxUnindent = /^\s*((end|when|else|elsif|rescue|ensure)\b|[\]\}])(.*)$/;


// debug to the term in blue so that it's easier to make out amongst all
// of Kate's other debug output.
function dbg(s)
{
  debug("\u001B[44m" + s + "\u001B[0m");
}

function isComment(attr)
{
  return (attr == 30 || attr == 31);
}

function isString(attr)
{
  return (attr == 13 || attr == 14);
}

function isCommentAttr(line, column)
{
  var attr = document.attribute(line, column);
  return isComment(attr);
}

// Return the closest non-empty line, ignoring comments
// (result <= line). Return -1 if the document
function findPrevNonCommentLine(line)
{
  line = document.prevNonEmptyLine(line);
  while (line >= 0 && isCommentAttr(line, document.firstColumn(line))) {
    line = document.prevNonEmptyLine(line - 1);
  }
  return line;
}

function isLineContinuing(line)
{
  return /\\$/.test(document.line(line));
}

function isStmtContinuing(line)
{
  // TODO: Continuing if line ends with / operator, but not /regexp/
  //       Ignoring / for now, because it's probably far less common
  //       to have lines ending with division than regexp

  var cnt = document.line(line);
  var len = cnt.length;

  // TODO:  Duplicate code here and in 'isBlockStart'

  // Check for operators at end of line
  var pos = 0;
  var rx = /(.*?)((\+|\-|\*|\=|&&|\|\||\band\b|\bor\b|,)\s*)/;
  while (rx.test(cnt)) {
    var start = pos + RegExp.$1.length;
    var end = start + RegExp.$2.length;
    var attr = document.attribute(line, start);
    if (!isString(attr) && !isComment(attr)) {
      if (end == len)
        return true;
      if (isComment(document.attribute(line, end)))
        return true;
    }
    cnt = cnt.substring(end - pos, len - pos);
    pos = end;
  }
  return false;
}

// Return the first line that is not preceeded by a "continuing" line.
// Return currLine if currLine <= 0
function findStmtStart(currLine)
{
  var p, l = currLine;
  do {
    if (l <= 0) return l;
    p = l;
    l = findPrevNonCommentLine(l - 1);
  } while ((l == p-1 && isLineContinuing(l)) || isStmtContinuing(l));
  return p;
}

// Statement class
function Statement(start, end)
{
  this.start = start;
  this.end = end;

  // Convert to string for debugging
  this.toString = function() {
    return "{" + this.start + "," + this.end + "}";
  }

  // Return document.attribute at the given offset in a statement
  this.attribute = function(offset) {
    var line = this.start;
    while (line < this.end && document.lineLength(line) < offset) {
      offset -= document.lineLength(line++) + 1;
    }
    return document.attribute(line, offset);
  }

  this.indent = function() {
    return document.firstVirtualColumn(this.start)
  }

  // Return the content of the statement from the document
  this.content = function() {
    var cnt = "";
    for (var l = this.start; l <= this.end; l++) {
      cnt += document.line(l).replace(/\\$/, " ");
      if (l < this.end)
        cnt += " ";
    }
    return cnt;
  }
}

// Returns a tuple that contains the first and last line of the
// previous statement before line.
function findPrevStmt(line)
{
  var stmtEnd = findPrevNonCommentLine(line);
  var stmtStart = findStmtStart(stmtEnd);

  return new Statement(stmtStart, stmtEnd);
}

function isBlockStart(stmt)
{
  var cnt = stmt.content();
  var len = cnt.length;

  if (rxIndent.test(cnt))
    return true;

  var pos = 0;
  var rx = /(.*?)((\bdo\b|\{)(\s*\|.*\|)?\s*)/;
  while (rx.test(cnt)) {
    var start = pos + RegExp.$1.length;
    var end = start + RegExp.$2.length;
    var attr = stmt.attribute(start);
    if (!isString(attr) && !isComment(attr)) {
      if (end == len)
        return true;
      if (isComment(stmt.attribute(end)))
        return true;
    }
    cnt = cnt.substring(end - pos, len - pos);
    pos = end;
  }
  return false;
}

function isBlockEnd(stmt)
{
  var cnt = stmt.content();

  return rxUnindent.test(cnt);
}

function findBlockStart(line)
{
  var nested = 0;
  var stmt = new Statement(line, line);
  while (true) {
    if (stmt.start < 0) return stmt;

    stmt = findPrevStmt(stmt.start - 1);
    if (isBlockEnd(stmt)) {
      nested++;
    }
    if (isBlockStart(stmt)) {
      if (nested == 0)
        return stmt;
      else
        nested--;
    }
  }
}

// check if the trigger characters are in the right context,
// otherwise running the indenter might be annoying to the user
function isValidTrigger(line, ch)
{
  if (ch == "" || ch == "\n")
    return true; // Explicit align or new line
  if (rxUnindent.test(document.line(line))) {
    if (RegExp.$3 == "")
      return true; // Exact match
  }
  return false;
}

// indent gets three arguments: line, indentwidth in spaces,
// typed character indent
function indent(line, indentWidth, ch)
{
  if (!isValidTrigger(line, ch))
    return -2;

  var prevStmt = findPrevStmt(line - 1);
  if (prevStmt.end < 0)
    return -2; // Can't indent the first line

  var prevStmtCnt = prevStmt.content();
  var prevStmtInd = prevStmt.indent();

  // Handle indenting of multiline statements.
  // Manually indenting to be able to force spaces.
  if ((prevStmt.end == line-1 && isLineContinuing(prevStmt.end)) || isStmtContinuing(prevStmt.end)) {
    var len = document.firstColumn(prevStmt.end);
    var str = document.line(prevStmt.end).substr(0, len);
    if (!document.startsWith(line, str)) {
      document.removeText(line, 0, line, document.firstColumn(line));
      if (prevStmt.start == prevStmt.end)
        str += "    ";
      document.insertText(line, 0, str);
    }
    return -2;
  }

  if (rxUnindent.test(document.line(line))) {
    var startStmt = findBlockStart(line);
    if (startStmt.start >= 0)
      return startStmt.indent();
    else
      return -2;
  }

  if (isBlockStart(prevStmt)) {
    return prevStmtInd + indentWidth;
  } else if (prevStmtCnt.search(/[\[\{]\s*$/) != -1) {
    return prevStmtInd + indentWidth;
  }

  // Keep current
  return prevStmtInd;
}

// kate: indent-width 2; indent-spaces on;
