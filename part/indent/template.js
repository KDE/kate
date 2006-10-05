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
 */
function indentLine(line, indent, ch)
{
    return -2;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
