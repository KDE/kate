/* kate-script
 * author: Christoph Cullmann
 * license: BSD
 * revision: 1
 * kate-version: 3.4
 * functions: quickCodingExpand
 *
 * Like the "Zen Coding" for HTML, this aims to be a variant for C++ and other languages.
 */

// required katepart js libraries
require ("range.js");
require ("underscore.js");

function action (cmd)
{
  var a = new Object();
  a.icon = "";
  a.category = i18n("Quick Coding");
  a.interactive = false;
  if ( cmd == 'quickCodingExpand' ) {
    a.text = i18n("Expand Abbreviation");
    a.shortcut = 'Ctrl+Alt+#';
  }

  return a;
}

function help (cmd)
{
  if (cmd == 'quickCodingExpand') {
    return i18n("Expand Quick Coding Abbreviation");
  }
}

/**
 * We want a capitalize function for strings
 */
String.prototype.capitalize = function() {
    return this.charAt(0).toUpperCase() + this.slice(1);
}

/**
 * cpp expansion command
 */
function quickCodingExpand ()
{
    /**
     * some settings
     * FIXME: should be read from document/view...
     */
    var indentChars = "    ";
    
    /**
     * get current cursor position as start
     * get current line of text until cursor position
     */
    var start = view.cursorPosition();
    var textInFronOfCursor = document.text (new Cursor(start.line, 0), start);
    debug ("we look at: " + textInFronOfCursor);
    
    /**
     * try to read out the abbreviation
     */
    var matches = textInFronOfCursor.match (/\w+#.*/);
    if (!matches) {
        debug ("found no abbreviation");
        return;
    }
    
    /**
     * construct full abbreviation string and get its position
     */
    var abbreviation = matches[0];
    var abbreviationIndex = textInFronOfCursor.indexOf (abbreviation);
    var abbreviationStart = new Cursor (start.line, abbreviationIndex);
    debug ("found at " + abbreviationIndex + " abbreviation: " + textInFronOfCursor);
    
    /**
     * now we have an abbreviation, something like "class#n:Test#p:Lala"
     * split this now up into the parts, # separated
     */
    var abbreviationParts = abbreviation.split ("#");
    if (abbreviationParts.length < 1) {
        debug ("abbreviation not splitable in at least one ");
        return;
    }
    
    /**
     * construct abbreviation object
     * e.g. for c#n:Test#p:Parent => { "n" => "Test", "p" => "Parent" }
     */
    var abbreviationObject = {};
    for (var i = 1; i < abbreviationParts.length; ++i) {
        var matches = abbreviationParts[i].match (/^(\w+):(.*)$/);
        if (matches && matches[1] && matches[2]) {
            abbreviationObject[matches[1]] = matches[2];
        } else {
            var matches = abbreviationParts[i].match (/^(\w+)$/);
            if (matches && matches[1])
                abbreviationObject[matches[1]] = true;
        }
    }
    
    /**
     * choose right language for abbreviations
     * FIXME: remember that more generic, not this if then else trash here
     */
    var language = "none";
    if (document.highlightingMode().match(/C\+\+/)) {
        language = "cpp";
    }
    
    /**
     * get abbreviation template
     * else nothing to do
     */
    var template = read("quickcoding/" + language + "/" + abbreviationParts[0] + ".template");
    if (!template)
        return;
    
    /**
     * instanciate the template
     */
    var instanciatedTemplate = _.template (template) (abbreviationObject);
    if (!instanciatedTemplate)
        return;
    
    /**
     * start editing transaction
     */
    document.editBegin ();
    
    /**
     * remove abbreviation
     */
    document.removeText (abbreviationStart, start);
    
    /**
     * insert instanciated template with right indentation
     */
    var text = instanciatedTemplate.split ("\n");
    for (i = 0; i < text.length; ++i) {
        /**
         * now: for first line, we do nothing, for otheres we add the prefix of the first line
         */
        if (i > 0)
            document.insertText (new Cursor (abbreviationStart.line + i, 0), document.text (new Cursor (abbreviationStart.line, 0), abbreviationStart));
                
        /**
         * create current textline, remove the 4 spaces place holder for one indent level
         */
        var textLine = text[i];
        if (i + 1 < text.length)
            textLine += "\n";
        textLine = textLine.replace (/    /g, indentChars);
        
        /**
         * insert the line itself
         */
        document.insertText (new Cursor (abbreviationStart.line + i, abbreviationStart.column), textLine);
    }
    
    /**
     * end editing transaction
     */
    document.editEnd ();
    
    /**
     * set cursor position back
     */
    view.setCursorPosition (abbreviationStart);
}
