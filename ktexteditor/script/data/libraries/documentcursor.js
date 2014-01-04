// This file is part of the Kate project within KDE.
// (C) 2013 Dominik Haumann <dhaumann kde org>
// License: LGPL v2 or v3

require("cursor.js");

function DocumentCursor(doc, lineOrCursor, column) {

  if (arguments.length === 1) {
    if (doc instanceof DocumentCursor) {
      return doc.clone();
    } else {
      this.document = doc;
      this.line = 0;
      this.column = 0;
    }
  } else if (arguments.length === 2 && typeof lineOrCursor == "object") {
    this.document = doc;
    this.line = lineOrCursor.line;
    this.column = lineOrCursor.column;
  } else if (arguments.length === 3 && typeof lineOrCursor == "number"
                                    && typeof column == "number") {
    this.document = doc;
    this.line = parseInt(lineOrCursor, 10);
    this.column = parseInt(column, 10);
  } else {
    throw "Wrong usage of DocumentCursor constructor";
  }
}

DocumentCursor.prototype.clone = function() {
  return new DocumentCursor(this.document, this.line, this.column);
}

DocumentCursor.prototype.setPosition = function(line, column) {
  this.line = line;
  this.column = column;
}

DocumentCursor.prototype.isValid = function() {
  return (this.line >= 0) && (this.column >= 0);
}

DocumentCursor.prototype.compareTo = function(other) {
  if (this.line > other.line || (this.line === other.line && this.column > other.column)) {
    return 1;
  }
  if (this.line < other.line || (this.line === other.line && this.column < other.column)) {
    return -1;
  }
  return 0;
}

DocumentCursor.prototype.equals = function(other) {
  return (this.document === other.document && this.line === other.line && this.column === other.column);
}

DocumentCursor.prototype.toString = function() {
  if (this.isValid()) {
    return "DocumentCursor(" + this.line+ "," + this.column+ ")";
  } else {
    return "DocumentCursor()";
  }
}

DocumentCursor.invalid = function(doc) {
  return new DocumentCursor(doc, -1, -1);
}




DocumentCursor.prototype.isValidTextPosition = function() {
  return this.isValid() && this.line < this.document.lines() && this.column <= this.document.lineLength(this.line);
}

DocumentCursor.prototype.atStartOfLine = function() {
  return this.isValidTextPosition() && this.column === 0;
}

DocumentCursor.prototype.atEndOfLine = function() {
  return this.isValidTextPosition() && this.column === this.document.lineLength(this.line);
}

DocumentCursor.prototype.atStartOfDocument = function() {
  return this.line === 0 && this.column === 0;
}

DocumentCursor.prototype.atEndOfDocument = function() {
  return this.isValid() && (this.line === this.document.lines() - 1) && this.column === this.document.lineLength(this.line);
}

DocumentCursor.prototype.gotoNextLine = function() {
  var ok = this.isValid() && (this.line + 1 < this.document.lines());
  if (ok) {
    this.line = this.line + 1;
    this.column = 0;
  }
  return ok;
}

DocumentCursor.prototype.gotoPreviousLine = function() {
  var ok = this.line > 0 && this.column >= 0;
  if (ok) {
    this.line = this.line - 1;
    this.column = 0;
  }
  return ok;
}

DocumentCursor.prototype.move = function(nChars, wrapAtEol) {
  // validity checks
  if (typeof wrapAtEol != "boolean") {
    wrapAtEol = true;
  }

  if (!this.isValid()) {
    return false;
  }

  var c = new Cursor(this.line, this.column);

  // cache lineLength to minimize calls of KateDocument::lineLength(), as
  // results in locating the correct block in the text buffer every time,
  // which is relatively slow
  var lineLength = this.document.lineLength(c.line);

  // special case: cursor position is not in valid text, then the algo does
  // not work for Wrap mode. Hence, catch this special case by setting
  // c.column() to the lineLength()
  if (nChars > 0 && wrapAtEol && c.column > lineLength) {
    c.column = lineLength;
  }

  while (nChars !== 0) {
    if (nChars > 0) {
      if (wrapAtEol) {
        var advance = Math.min(lineLength - c.column, nChars);

        if (nChars > advance) {
          if (c.line + 1 >= this.document.lines()) {
            return false;
          }

          c.line += 1;
          c.column = 0;
          nChars -= advance + 1; // +1 because of end-of-line wrap

          // advanced one line, so cache correct line length again
          lineLength = this.document.lineLength(c.line);
        } else {
          c.column += nChars;
          nChars = 0;
        }
      } else { // NoWrap
        c.column = c.column + nChars;
        nChars = 0;
      }
    } else {
      var back = Math.min(c.column, -nChars);
      if (-nChars > back) {
        if (c.line === 0) {
          return false;
        }

        c.line -= 1;
        lineLength = this.document.lineLength(c.line);
        c.column = lineLength;
        nChars += back + 1; // +1 because of wrap-around at start-of-line
      } else {
        c.column += nChars;
        nChars = 0;
      }
    }
  }

  this.line = c.line;
  this.column = c.column;
  return true;
}

DocumentCursor.prototype.toCursor = function() {
  return new Cursor(this.line, this.column);
}

DocumentCursor.prototype.toVirtualCursor = function() {
  return document.toVirtualCursor(this);
}

// kate: indent-width 2; replace-tabs on;
