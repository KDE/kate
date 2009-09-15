function Cursor()
{
  if (arguments.length == 0) {
    return new Cursor(0, 0);
  } else if (arguments.length == 1 && typeof arguments[0] == "object") {
    // assume: cursor = new Cursor(otherCursor);
    return arguments[0].clone();
  } else if (arguments.length == 2 && typeof arguments[0] == "number"
                                   && typeof arguments[1] == "number") {
    // assume: cursor = new Cursor(line, column);
    this.line = parseInt(arguments[0]);
    this.column = parseInt(arguments[1]);
  } else {
    throw "Wrong usage of Cursor constructor";
  }

  this.clone = function() {
    return new Cursor(this.line, this.column);
  };

  this.isValid = function() {
    return (this.line >= 0) && (this.column >= 0);
  };

  this.invalid = function() {
    return new Cursor(-1, -1);
  };

  this.toString = function() {
    return "Cursor(" + this.line+ ", " + this.column+ ")";
  };
}

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

  this.contains = function() {
    // TODO: implement me
  };

  this.containsColumn = function(column) {
    return (column >= this.start.column) && (column < this.end.column);
  };

  this.containsLine = function(line) {
    return (line >= this.start.line) && (column < this.end.line);
  };

  this.overlaps = function(range) {
    // TODO: implement me
  };

  this.toString = function() {
    return "Range(" + this.start + ", " + this.end + ")";
  };
}

