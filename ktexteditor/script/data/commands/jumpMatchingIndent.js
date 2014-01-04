/* kate-script
 * author: Alan Prescott
 * license: GPLv2
 * revision: 1
 * kate-version: 3.4
 * functions: jumpIndentUp, jumpIndentDown
 *
 * Move cursor to next/previous line with an equal indent - especially useful
 * for moving around blocks of Python code.
 *
 * A first attempt at a kate script created by modifying the jump.js script
 * written by Dzikri Aziz.
 * I've been looking for this feature in an editor ever since I came across it
 * in an old MS-DOS editor called Edwin.
 */

// required katepart js libraries
require ("range.js");

function jumpIndentDown() {
  return _jumpIndent();
}


function jumpIndentUp() {
  return _jumpIndent( true );
}


function action( cmd ) {
  var a = new Object();
  a.icon = "";
  a.category = i18n("Navigation");
  a.interactive = false;
  if ( cmd == 'jumpIndentUp' ) {
    a.text = i18n("Move cursor to previous matching indent");
    a.shortcut = 'Alt+Shift+Up';
  }
  else if ( cmd == 'jumpIndentDown' ) {
    a.text = i18n("Move cursor to next matching indent");
    a.shortcut = 'Alt+Shift+Down';
  }

  return a;
}


function help( cmd ) {
  if (cmd == 'jumpIndentUp') {
    return i18n("Move cursor to previous matching indent");
  }
  else if (cmd == 'jumpIndentDown') {
    return i18n("Move cursor to next matching indent");
  }
}

function _jumpIndent( up ) {
  var iniLine = view.cursorPosition().line,
      iniCol = view.cursorPosition().column,
      lines  = document.lines(),
      indent, target;

  indent = document.firstColumn(iniLine);
  target = iniLine;

  if ( up === true ) {
    if ( target > 0 ) {
      target--;
      while ( target > 0 && document.firstColumn(target) != indent ) {
        target--;
      }
    }
  }

  else {
    if ( target < lines ) {
      target++;
      while ( target < lines && document.firstColumn(target) != indent ) {
        target++;
      }
    }
  }

  view.setCursorPosition(target, iniCol);
}
