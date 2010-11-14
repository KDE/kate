// This file is part of the Kate project within KDE.
// (C) 2009 Dominik Haumann <dhaumann kde org>
// License: LGPL v2 or v3

/**
 * Prototype Range.
 */
function Range()
{
  if (arguments.length == 0) {
    return new Range(0, 0, 0, 0);
  } else if (arguments.length == 1 && typeof arguments[0] == "object") {
    // assume: range = new Range(otherRange);
    return arguments[0].clone();
  } else if (arguments.length == 2 && typeof arguments[0] == "object"
                                   && typeof arguments[1] == "object") {
    // assume: range = new Range(startCursor, endCursor);
    this.start = arguments[0].clone();
    this.end = arguments[1].clone();
  } else if (arguments.length == 4 && typeof arguments[0] == "number"
                                   && typeof arguments[1] == "number"
                                   && typeof arguments[2] == "number"
                                   && typeof arguments[3] == "number") {
    this.start = new Cursor(arguments[0], arguments[1]);
    this.end = new Cursor(arguments[2], arguments[3]);
  } else {
    throw "Wrong usage of Range constructor";
  }

  this.clone = function() {
    return new Range(this.start, this.end);
  };

  this.isValid = function() {
    return this.start.isValid() && this.end.isValid();
  };

  this.invalid = function() {
    return new Range(-1, -1, -1, -1);
  };

  this.contains = function(cursorOrRange) {
    if (cursorOrRange.start && cursorOrRange.end) {
      // assume a range
      return (cursorOrRange.start.compareTo(this.start) >= 0 &&
              cursorOrRange.end.compareTo(this.end) <= 0);
    } else {
      // assume a cursor
      return (cursorOrRange.compareTo(this.start) >= 0 &&
              cursorOrRange.compareTo(this.end) < 0);
    }
  };

  this.containsColumn = function(column) {
    return (column >= this.start.column) && (column < this.end.column);
  };

  this.containsLine = function(line) {
    return (line > this.start.line || (line == this.start.line && this.start.column == 0)) && line < this.end.line;
  };

  this.overlaps = function(range) {
    if (range.start.compareTo(this.start) <= 0) {
      return range.end.compareTo(this.start) > 0;
    } else if (range.end.compareTo(this.end) >= 0) {
      return range.start.compareTo(this.end) < 0;
    } else {
      return this.contains(range);
    }
  };

  this.overlapsLine = function(line) {
    return (line >= this.start.line && line <= this.end.line);
  }

  this.overlapsColumn = function(column) {
    return column >= this.start.column && column <= this.end.column;
  }

  this.equals = function(other) {
    return (this.start.equals(other.start) && this.end.equals(other.end));
  };

  this.toString = function() {
    return "Range(" + this.start + ", " + this.end + ")";
  };
}

