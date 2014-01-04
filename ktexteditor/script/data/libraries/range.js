// This file is part of the Kate project within KDE.
// (C) 2009 Dominik Haumann <dhaumann kde org>
// License: LGPL v2 or v3

// required katepart js libraries
require ("cursor.js");

/**
 * Prototype Range.
 */
function Range() {

  if (arguments.length === 0) {
    return new Range(0, 0, 0, 0);
  }

  if (arguments.length === 1 && typeof arguments[0] == "object") {
    // assume: range = new Range(otherRange);
    return arguments[0].clone();
  }

  if (arguments.length === 2 && typeof arguments[0] == "object"
                             && typeof arguments[1] == "object") {
    // assume: range = new Range(startCursor, endCursor);
    this.start = arguments[0].clone();
    this.end = arguments[1].clone();
  } else if (arguments.length === 4 && typeof arguments[0] == "number"
                                    && typeof arguments[1] == "number"
                                    && typeof arguments[2] == "number"
                                    && typeof arguments[3] == "number") {
    this.start = new Cursor(arguments[0], arguments[1]);
    this.end = new Cursor(arguments[2], arguments[3]);
  } else {
    throw "Wrong usage of Range constructor";
  }
}

Range.prototype.clone = function() {
  return new Range(this.start, this.end);
}

Range.prototype.isValid = function() {
  return this.start.isValid() && this.end.isValid();
}

Range.prototype.isEmpty = function() {
  return this.start.equals(this.end);
}

Range.prototype.contains = function(cursorOrRange) {
  if (cursorOrRange.start && cursorOrRange.end) {
    // assume a range
    return (cursorOrRange.start.compareTo(this.start) >= 0 &&
            cursorOrRange.end.compareTo(this.end) <= 0);
  }

  // else: assume a cursor
  return (cursorOrRange.compareTo(this.start) >= 0 &&
          cursorOrRange.compareTo(this.end) < 0);
}

Range.prototype.containsColumn = function(column) {
  return (column >= this.start.column) && (column < this.end.column);
}

Range.prototype.containsLine = function(line) {
  return ((line > this.start.line) || ((line === this.start.line) && (this.start.column === 0))) && line < this.end.line;
}

Range.prototype.overlaps = function(range) {
  if (range.start.compareTo(this.start) <= 0) {
    return range.end.compareTo(this.start) > 0;
  }
  if (range.end.compareTo(this.end) >= 0) {
    return range.start.compareTo(this.end) < 0;
  }
  return this.contains(range);
}

Range.prototype.overlapsLine = function(line) {
  return (line >= this.start.line && line <= this.end.line);
}

Range.prototype.overlapsColumn = function(column) {
  return column >= this.start.column && column <= this.end.column;
}

Range.prototype.onSingleLine = function() {
  return (this.start.line == this.end.line);
}

Range.prototype.equals = function(other) {
  return (this.start.equals(other.start) && this.end.equals(other.end));
}

Range.prototype.toString = function() {
  return "Range(" + this.start + ", " + this.end + ")";
}

Range.invalid = function() {
  return new Range(-1, -1, -1, -1);
}

// kate: indent-width 2; replace-tabs on;