/* kate-script
 * name: Indenter name (appears in the menu)
 * license: license (GPL, LGPL, Artistic, etc)
 * author: first name last name <email-address>
 * version: 1 (simple integer number)
 * kate-version: 3.0
 */

// indent gets three arguments: line, indentwidth in spaces, typed character 
indenter.indent = indentLine;

// specifies the characters which should trigger indent, beside the default '\n'
indenter.triggerCharacters = "{}/:;";

/**
 * Indent a line.
 * This function is called for every <return/enter> and for all trigger
 * characters. ch is the character typed by the user. ch is
 * - '\n' for newlines
 * - "" empty for the action "Tools > Align"
 * - all other characters are really typed by the user.
 *
 * Return value:
 * - return -2; - do nothing
 * - return -1; - keep indentation (searches for previous non-blank line)
 * - return  0; - All numbers >= 0 are the indent-width in spaces
 * 
 * Alternatively, an array of two elements can be returned:
 * return [ indent, align ];
 * 
 * The first element is the indent-width like above, with the same meaning
 * of the special values.
 * 
 * The second element is an absolute value representing a column for
 * "alignment". If this value is higher then the indent value, the
 * difference represents a number of spaces to be added after the indent.
 * Otherwise, it's ignored.
 * 
 * Example:
 * Assume using tabs to indent, and tab width is 4. Here ">" represents a
 * tab, and "." represents a space:
 * 1: >   >   foobar("hello",
 * 2: >   >   ......."world")
 * 
 * When indenting line 2, the script returns [8, 15]. Two tabs are inserted
 * to indent to column 8, and 7 spaces are added to align the second
 * parameter under the first, so that it stays aligned if the file is viewed
 * with a different tab width.
 */
function indentLine(line, indent, ch)
{
    return -2;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
