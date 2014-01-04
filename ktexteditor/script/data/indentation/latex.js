/** kate-script
 * name: Latex
 * author: Jeroen Vuurens <jbpvuurens@gmail.com>
 * revision: 1
 * kate-version: 3.4
 * type: indentation
 *
 * Simple indentation for Latex. This script indents sections within
 * \begin{ and \end{ parts, as well as within { and }. Parts after
 * an \ escape character are ignored for proper handling of
 * \{ \} and \%. In other cases every { is regarded as an extra
 * indent, every } as a de-indent, and everything after \% is comment.
 */

// specifies the characters which should trigger indent, beside the default '\n'
var triggerCharacters = "{}";
var lineStartsClose = /^(\s*\}|\s*\\end\{)/;

function indent(line, indentWidth, character) {
  // not necessary to indent the first line
  if (line == 0)
    return -2;

  var c = document.line(line); // current line

  // Search backwards for first non-space, non-comment line.
  var prev = line;
  while (prev--) {
    if (!document.line(prev).match(/^\s*$|^%/)) {
      var previousLine = document.line(prev); // previous non-space line
      var prevIndent = document.firstVirtualColumn(prev);
      var end = document.lineLength(prev);

      var delta = 0;    // count normal openers/closers

      // Walk over openers and closers in the remainder of the previous line.
      if (previousLine.match(lineStartsClose))
        delta++; 
      var escaped = false;
      for (var i = 0; i < end; i++) {
         var char = previousLine.charAt(i);
         if (char == "\\") { // ignore rest of line as comment
            escaped = true;
            continue;
         }
         if (escaped) {
            if (char == "b" && end > i + 5 && previousLine.substr(i, 6) == "begin{" ) {
               delta++;
            } else if (char == "e" && end > i + 3 && previousLine.substr(i, 4) == "end{" ) {
               delta--;
            }
         } else {
            if (char == "%") { // ignore rest of line as comment
               break;
            } if (char == "{") {
               delta++;
            } else if (char == "}") {
               delta--;
            }
         }
         escaped = false;
      }

      // now count the number of closers in the beginning of the current line.
      if (c.match(lineStartsClose))
	delta --;
      return Math.max(0, prevIndent + delta * indentWidth);
    }
  }
  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
