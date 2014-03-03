/* kate-script
 * name: CMake
 * license: LGPL
 * author: Alex Turbov <i.zaufi@gmail.com>
 * revision: 1
 * kate-version: 3.4
 * required-syntax-style: CMake
 * indent-languages: CMake
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

/**
 * Some settings it assumes being in effect:
 * indent-width 4;
 * space-indent true;
 *
 * \todo Better to check (assert) some of that modelines...
 */

// required katepart js libraries
require ("range.js");
require ("string.js");
require ("utils.js")
require ("underscore.js")

triggerCharacters = "()<{\"";

var debugMode = false;
/// Global var to store indentation width for current document type
var gIndentWidth = 4;
/// Map of CMake calls where a key is the \e end of a corresponding \e start call
var END_BEGIN_CALL_PAIRS = {
    "endfunction": "function"
  , "endmacro": "macro"
  , "endforeach" : "foreach"
  , "endif" : {target: ["if", "else", "elseif"], push: ["endif"], pop: ["if"]}
  , "elseif" : {target: ["if", "elseif"], push: ["endif"], pop: ["if"]}
  , "else" : {target: ["if", "elseif"], push: ["endif"], pop: ["if"]}
  , "endwhile" : "while"
};
/// List of CMake commands which introduce indentation decrease
var CONTROL_FLOW_CALLS_TO_UNINDENT_AFTER = [
    "break", "return"
];
/// List of CMake commands which introduce indentation increase
var CONTROL_FLOW_CALLS_TO_INDENT_AFTER = [
    "function", "macro", "foreach", "if", "else", "elseif", "while"
];
/// List of CMake calls w/o (and/or all optional) parameters
var PARAMETERLESS_CALLS = [
    "break", "else", "elseif", "enable_testing", "endforeach", "endif", "endmacro", "endwhile", "return"
];
/// List of CMake command options which have parameter(s)
var INDENT_AFTER_OPTIONS = [
    "OUTPUT"                                                // add_custom_command
  , "COMMAND"                                               // add_custom_command, add_custom_target, add_test, execute_process, if
  , "ARGS"                                                  // add_custom_command, try_run
  , "MAIN_DEPENDENCY"                                       // add_custom_command
  , "DEPENDS"                                               // add_custom_command, add_custom_target
  , "IMPLICIT_DEPENDS"                                      // add_custom_command
  , "WORKING_DIRECTORY"                                     // add_custom_command, add_custom_target, add_test, execute_process
  , "COMMENT"                                               // add_custom_command, add_custom_target
  , "TARGET"                                                // add_custom_command, build_command, get_property, if, set_property
  , "SOURCES"                                               // add_custom_target, try_compile
  , "ALIAS"                                                 // add_executable
  , "OBJECT"                                                // add_library, add_library
  , "NAME"                                                  // add_test
  , "CONFIGURATIONS"                                        // add_test, install
  , "CONFIGURATION"                                         // build_command
  , "PROJECT_NAME"                                          // build_command
  , "RESULT"                                                // cmake_host_system_information
  , "QUERY"                                                 // cmake_host_system_information
  , "VERSION"                                               // cmake_minimum_required, cmake_policy
  , "SET"                                                   // cmake_policy
  , "GET"                                                   // cmake_policy, list
  , "NEWLINE_STYLE"                                         // configure_file
  , "EXTRA_INCLUDE"                                         // create_test_sourcelist
  , "FUNCTION"                                              // create_test_sourcelist
  , "PROPERTY"                                              // define_property, get_property, set_property
  , "BRIEF_DOCS"                                            // define_property
  , "FULL_DOCS"                                             // define_property
  , "TIMEOUT"                                               // execute_process, file
  , "RESULT_VARIABLE"                                       // execute_process, include
  , "OUTPUT_VARIABLE"                                       // execute_process, try_compile, try_run
  , "ERROR_VARIABLE"                                        // execute_process
  , "INPUT_FILE"                                            // execute_process
  , "OUTPUT_FILE"                                           // execute_process
  , "ERROR_FILE"                                            // execute_process
  , "TARGETS"                                               // export, install
  , "NAMESPACE"                                             // export, install
  , "FILE"                                                  // export, install
  , "PACKAGE"                                               // export
  , "WRITE"                                                 // file
  , "APPEND"                                                // file, list
  , "READ"                                                  // file
  , "LIMIT"                                                 // file
  , "OFFSET"                                                // file
  , "STRINGS"                                               // file
  , "LIMIT_COUNT"                                           // file
  , "LIMIT_INPUT"                                           // file
  , "LIMIT_OUTPUT"                                          // file
  , "LENGTH_MINIMUM"                                        // file
  , "LENGTH_MAXIMUM"                                        // file
  , "REGEX"                                                 // file, string
  , "GLOB"                                                  // file
  , "RELATIVE"                                              // file
  , "GLOB_RECURSE"                                          // file
  , "RENAME"                                                // file, install
  , "REMOVE"                                                // file
  , "REMOVE_RECURSE"                                        // file
  , "MAKE_DIRECTORY"                                        // file
  , "RELATIVE_PATH"                                         // file
  , "TO_CMAKE_PATH"                                         // file
  , "TO_NATIVE_PATH"                                        // file
  , "DOWNLOAD"                                              // file
  , "INACTIVITY_TIMEOUT"                                    // file
  , "STATUS"                                                // file
  , "LOG"                                                   // file
  , "EXPECTED_HASH"                                         // file
  , "EXPECTED_MD5"                                          // file
  , "TLS_VERIFY"                                            // file
  , "TLS_CAINFO"                                            // file
  , "UPLOAD"                                                // file
  , "TIMESTAMP"                                             // file, string
  , "GENERATE"                                              // file
  , "INPUT"                                                 // file
  , "CONTENT"                                               // file
  , "CONDITION"                                             // file
  , "COPY"                                                  // file
  , "INSTALL"                                               // file
  , "DESTINATION"                                           // file, install
  , "FILE_PERMISSIONS"                                      // file, install
  , "DIRECTORY_PERMISSIONS"                                 // file, install
  , "FILES_MATCHING"                                        // file, install
  , "PATTERN"                                               // file, install
  , "PERMISSIONS"                                           // file, install
  , "EXCLUDE"                                               // file, install, load_cache
  , "NAMES"                                                 // find_file, find_library, find_package, find_path, find_program
  , "HINTS"                                                 // find_file, find_library, find_package, find_path, find_program
  , "ENV"                                                   // find_file, find_library, find_path, find_program
  , "PATHS"                                                 // find_file, find_library, find_package, find_path, find_program
  , "PATH_SUFFIXES"                                         // find_file, find_library, find_package, find_path, find_program
  , "DOC"                                                   // find_file, find_library, find_path, find_program
  , "REQUIRED"                                              // find_package
  , "COMPONENTES"                                           // find_package
  , "OPTIONAL_COMPONENTES"                                  // find_package
  , "CONFIGS"                                               // find_package
  , "DIRECTORY"                                             // get_directory_property, get_property, install, set_property
  , "DEFINITION"                                            // get_directory_property
  , "PROGRAM"                                               // get_filename_component
  , "PROGRAM_ARGS"                                          // get_filename_component
  , "SOURCE"                                                // get_property, set_property
  , "TEST"                                                  // get_property, set_property
  , "CACHE"                                                 // get_property, set, set_property
  , "NOT"                                                   // if
  , "AND"                                                   // if
  , "OR"                                                    // if
  , "POLICY"                                                // if
  , "EXISTS"                                                // if
  , "IS_NEWER_THAN"                                         // if
  , "IS_DIRECTORY"                                          // if
  , "IS_SYMLINK"                                            // if
  , "IS_ABSOLUTE"                                           // if
  , "MATCHES"                                               // if
  , "LESS"                                                  // if, string
  , "GREATER"                                               // if, string
  , "EQUAL"                                                 // if, string
  , "STRLESS"                                               // if
  , "STRGREATER"                                            // if
  , "STREQUAL"                                              // if
  , "VERSION_LESS"                                          // if
  , "VERSION_GREATER"                                       // if
  , "VERSION_EQUAL"                                         // if
  , "DEFINED"                                               // if
  , "TYPE"                                                  // include_external_msproject
  , "GUID"                                                  // include_external_msproject
  , "PLATFORM"                                              // include_external_msproject
  , "EXPORT"                                                // install
  , "ARCHIVE"                                               // install
  , "LIBRARY"                                               // install
  , "RUNTIME"                                               // install
  , "FRAMEWORK"                                             // install
  , "BUNDLE"                                                // install
  , "PRIVATE_HEADER"                                        // install
  , "PUBLIC_HEADER"                                         // install
  , "RESOURCE"                                              // install
  , "COMPONENT"                                             // install
  , "FILES"                                                 // install
  , "PROGRAMS"                                              // install
  , "SCRIPT"                                                // install
  , "CODE"                                                  // install
  , "LENGTH"                                                // list, string
  , "FIND"                                                  // list, string
  , "INSERT"                                                // list
  , "REMOVE_ITEM"                                           // list
  , "REMOVE_AT"                                             // list
  , "REMOVE_DUPLICATES"                                     // list
  , "REVERSE"                                               // list
  , "SORT"                                                  // list
  , "READ_WITH_PREFIX"                                      // load_cache
  , "INCLUDE_INTERNALS"                                     // load_cache
  , "EXPR"                                                  // math
  , "UNIX_COMMAND"                                          // separate_arguments
  , "WINDOWS_COMMAND"                                       // separate_arguments
    // set_directory_properties, set_source_files_properties, set_target_properties, set_tests_properties
  , "PROPERTIES"
  , "REGULAR_EXPRESSION"                                    // source_group
  , "FILES"                                                 // source_group
  , "MATCH"                                                 // string
  , "MATCHALL"                                              // string
  , "REPLACE"                                               // string
  , "COMPARE"                                               // string
  , "NOTEQUAL"                                              // string
  , "ASCII"                                                 // string
  , "CONFIGURE"                                             // string
  , "TOUPPER"                                               // string
  , "TOLOWER"                                               // string
  , "SUBSTRING"                                             // string
  , "STRIP"                                                 // string
  , "RANDOM"                                                // string
  , "ALPHABET"                                              // string
  , "RANDOM_SEED"                                           // string
  , "MAKE_C_IDENTIFIER"                                     // string
  , "INTERFACE"                                             // target_compile_definitions, target_compile_options, target_include_directories
  , "PUBLIC"                                                // target_compile_definitions, target_compile_options, target_include_directories
  , "PRIVATE"                                               // target_compile_definitions, target_compile_options, target_include_directories
  , "LINK_PRIVATE"                                          // target_link_libraries
  , "LINK_PUBLIC"                                           // target_link_libraries
  , "CMAKE_FLAGS"                                           // try_compile, try_run
  , "COMPILE_DEFINITIONS"                                   // try_compile, try_run
  , "LINK_LIBRARIES"                                        // try_compile
  , "COPY_FILE"                                             // try_compile
  , "COPY_FILE_ERROR"                                       // try_compile
  , "COMPILE_OUTPUT_VARIABLE"                               // try_run
  , "RUN_OUTPUT_VARIABLE"                                   // try_run
];


/**
 * \brief Handle \c ENTER between parenthesis
 */
function tryParenthesisSplit_ch(cursor)
{
    var result = -1;

    if (isStringOrComment(cursor.line - 1, cursor.column))
        return result;                                      // Do nothing for comments and strings

    // Is ENTER was pressed right after '('?
    if (document.lastChar(cursor.line - 1) == '(')
    {
        // Yep, lets get align of the previous line
        var prev_line_indent = document.firstColumn(cursor.line - 1);
        result = prev_line_indent + gIndentWidth;
        // Indent a closing ')' if ENTER was pressed between '(|)'
        if (document.firstChar(cursor.line) == ')')
        {
            document.insertText(
                cursor.line
              , cursor.column
              , "\n" + String().fill(' ', prev_line_indent + gIndentWidth / 2)
              );
            view.setCursorPosition(cursor.line, cursor.column);
        }
    }

    if (result != -1)
    {
        dbg("tryParenthesisSplit_ch: result =", result);
    }
    return result;
}

/**
 * \brief Indent after some control flow CMake calls
 */
function tryIndentAfterControlFlowCalls_ch(cursor)
{
    var result = -1;

    if (isStringOrComment(cursor.line - 1, cursor.column))
        return result;                                      // Do nothing for comments and strings

    var prev_line_indent = document.firstColumn(cursor.line - 1);
    var first_prev_line_word = document.wordAt(cursor.line - 1, prev_line_indent).toLowerCase();
    dbg("tryControlFlowCalls_ch: first_prev_line_word =", first_prev_line_word);
    if (CONTROL_FLOW_CALLS_TO_INDENT_AFTER.indexOf(first_prev_line_word) != -1)
    {
        result = prev_line_indent + gIndentWidth;
    }
    else if (CONTROL_FLOW_CALLS_TO_UNINDENT_AFTER.indexOf(first_prev_line_word) != -1)
    {
        result = prev_line_indent - gIndentWidth;
    }

    if (result != -1)
    {
        dbg("tryControlFlowCalls_ch: result =", result);
    }
    return result;
}

/**
 * \brief Unindent after dandling ')'
 */
function tryAfterClosingParensis_ch(cursor)
{
    var result = -1;

    if (isStringOrComment(cursor.line - 1, cursor.column))
        return result;                                      // Do nothing for comments and strings

    // Check if dandling ')' present on a previous line
    var prev_line = document.prevNonEmptyLine(cursor.line - 1);
    dbg("tryAfterClosingParensis_ch: prev_line =", prev_line);
    var first_column = document.firstColumn(prev_line);
    if (document.charAt(prev_line, first_column) == ')')
        result = first_column - (gIndentWidth / 2);

    if (result != -1)
    {
        dbg("tryAfterClosingParensis_ch: result =", result);
    }
    return result;
}

/**
 * \brief Indent after parameterless command options
 *
 * It is common way to format options w/ parameters like this:
 * \code
 *  install(
 *      DIRECTORY ${name}
 *      DESTINATION ${DATA_INSTALL_DIR}/kate/pate
 *      FILES_MATCHING
 *          PATTERN "*.py"
 *          PATTERN "*.ui"
 *          PATTERN "*_ui.rc"
 *          PATTERN "__pycache__*" EXCLUDE
 *     )
 * \endcode
 *
 * I.e. do indent for long parameter lists.
 * This function recognize parameterless options (and do not indent after them),
 * for everything else it adds an extra indentation...
 */
function tryIndentCommandOptions_ch(cursor)
{
    var result = -1;

    if (isStringOrComment(cursor.line - 1, cursor.column))
        return result;                                      // Do nothing for comments and strings

    // Get last word from a previous line
    var last_word = document.wordAt(cursor.line - 1, document.lastColumn(cursor.line - 1));

    // ATTENTION Kate will return the last word "TARGET" for text like this: "TARGET)",
    // which is not what we've wanted... so before continue lets make sure that we've
    // got that we wanted...
    if (last_word[last_word.length - 1] != document.lastChar(cursor.line - 1))
        return result;

    dbg("tryIndentCommandOptions_ch: last_word =", last_word);
    result = document.firstColumn(cursor.line - 1)
      + (INDENT_AFTER_OPTIONS.indexOf(last_word) != -1 ? gIndentWidth : 0)
      ;

    if (result != -1)
    {
        dbg("tryIndentCommandOptions_ch: result =", result);
    }
    return result;
}

/**
 * \brief Handle \c ENTER key
 */
function caretPressed(cursor)
{
    var result = -1;
    var line = cursor.line;

    // Dunno what to do if previous line isn't available
    if (line - 1 < 0)
        return result;                                      // Nothing (dunno) to do if no previous line...

    // Register all indent functions
    var handlers = [
        tryParenthesisSplit_ch                              // Handle ENTER between parenthesis
      , tryIndentAfterControlFlowCalls_ch                   // Indent after some control flow CMake calls
      , tryAfterClosingParensis_ch                          // Unindent after dandling ')'
      , tryIndentCommandOptions_ch                          // Indent after parameterless command options
      ];

    // Apply all all functions until result gets changed
    for (
        var i = 0
      ; i < handlers.length && result == -1
      ; result = handlers[i++](cursor)
      );

    return result;
}

/**
 * \brief Try to unindent current call when <tt>(</tt> has pressed
 */
function tryUnindentCall(cursor)
{
    var result = -2;

    if (isStringOrComment(cursor.line, cursor.column))
        return result;                                      // Do nothing for comments and strings

    // Check the word before current cursor position
    var call_name = document.wordAt(cursor.line, cursor.column - 1).toLowerCase();
    dbg("tryUnindentCall: call_name =", call_name);
    var end_calls = _.keys(END_BEGIN_CALL_PAIRS);
    var stack_level = 0;
    if (_.indexOf(end_calls, call_name) != -1)
    {
        var lookup_call_name = END_BEGIN_CALL_PAIRS[call_name];
        dbg("tryUnindentCall: typeof lookup_call_name =", typeof lookup_call_name);
        var matcher;
        if (typeof lookup_call_name === "string")
        {
            matcher = function(word)
            {
                return word == lookup_call_name;
            };
        }
        else if (typeof lookup_call_name === "object")
        {
            matcher = function(word)
            {
                // dbg("tryUnindentCall: stack_level =", stack_level, ", word =", word);
                if (_.indexOf(lookup_call_name.push, word) != -1)
                {
                    stack_level += 1;
                    return false;
                }
                if (stack_level != 0)
                {
                    if (_.indexOf(lookup_call_name.pop, word) != -1)
                        stack_level -= 1;
                    return false;
                }
                return _.indexOf(lookup_call_name.target, word) != -1;
            };
        }
        else
        {
            /// \todo Is there any kind of \c assert() in JavaScript at all?
            dbg("tryUnindentCall: Unknown type of END_BEGIN_CALL_PAIRS value! Code review required!");
            return result;
        }

        // Ok, lets find the corresponding start call towards document's start...
        var found_line = -1;
        for (
            var line = document.prevNonEmptyLine(cursor.line - 1)
          ; 0 <= line && found_line == -1
          ; line = document.prevNonEmptyLine(line - 1)
          )
        {
            var first_word = document.wordAt(line, document.firstColumn(line)).toLowerCase();
            // dbg("tryUnindentCall: line =", line, ", word =", first_word);
            if (matcher(first_word))
            {
                dbg("tryUnindentCall: Found ", first_word);
                found_line = line;
            }
        }

        if (found_line != -1)
        {
            result = document.firstColumn(found_line);
            // Add closing parenthesis if none yet
            // addCharOrJumpOverIt(cursor.line, cursor.column, ')');
        }
        else
        {
            dbg("tryUnindentCall: Not Found!");
        }
    }

    if (result != -2)
    {
        dbg("tryUnindentCall: result =", result);
    }
    return result;
}

/**
 * Align closing parenthesis if it is a first character on a line
 */
function tryAlignCloseParenthesis(cursor)
{
    var result = -2;

    if (isStringOrComment(cursor.line, cursor.column))
        return result;                                      // Do nothing for comments and strings

    if (justEnteredCharIsFirstOnLine(cursor.line, cursor.column, ')'))
    {
        // Try to find corresponding open parenthesis
        var open = document.anchor(cursor.line, cursor.column - 1, '(');
        dbg("tryAlignCloseParenthesis: Found open brace at", open);
        if (open.isValid())
            result = document.firstColumn(open.line) + (gIndentWidth / 2);
    }

    if (result != -2)
    {
        dbg("tryAlignCloseParenthesis: result =", result);
    }
    return result;
}

/**
 * \brief Move cursor out of <tt>'()'</tt> if no parameters required for a given call
 */
function tryJumpOutOfParenthesis(cursor)
{
    if (isStringOrComment(cursor.line, cursor.column))
        return;                                             // Do nothing for comments and strings

    var first_word = document.wordAt(cursor.line, document.firstColumn(cursor.line)).toLowerCase();
    dbg("tryJumpOutOfParenthesis: @"+cursor.line+","+cursor.column+", char="+document.charAt(cursor));
    if (PARAMETERLESS_CALLS.indexOf(first_word) != -1)
        addCharOrJumpOverIt(cursor.line, cursor.column, ')')
}

/**
 * \brief Process one character
 *
 * NOTE Cursor positioned right after just entered character and has \c +1 in column.
 */
function processChar(line, ch)
{
    var result = -2;                                        // By default, do nothing...
    var cursor = view.cursorPosition();
    if (!cursor)
        return result;

    document.editBegin();
    // Check if char under cursor is the same as just entered,
    // and if so, remove it... to make it behave like "overwrite" mode
    if (ch != ' ' && document.charAt(cursor) == ch)
        document.removeText(line, cursor.column, line, cursor.column + 1);

    switch (ch)
    {
        case '\n':
            result = caretPressed(cursor);
            break;
        case '(':
            result = tryUnindentCall(cursor);
            tryJumpOutOfParenthesis(cursor);
            break;
        case ')':
            result = tryAlignCloseParenthesis(cursor);
            break;
        default:
            break;                                          // Nothing to do...
    }

    document.editEnd();
    return result;
}

/**
 * Try to align a given line
 * \todo More actions
 */
function indentLine(line)
{
    dbg(">> Going to indent line ", line);
    var cursor = new Cursor(line, document.firstColumn(line) + 1);
    var result = tryUnindentCall(cursor);
    if (result == -2)
        result = tryAlignCloseParenthesis(cursor);
    cursor = new Cursor(line, document.firstColumn(line));
    if (result == -2)
        result = caretPressed(cursor);
    dbg("indentLine: result =", result);

    if (result == -2)                                       // Still dunno what to do?
        result = -1;                                        // ... just align according a previous non empty line
    return result;
}

/**
 * \brief Process a newline or one of \c triggerCharacters character.
 *
 * This function is called whenever the user hits \c ENTER key.
 *
 * It gets three arguments: \c line, \c indentwidth in spaces and typed character
 *
 * Called for each newline (<tt>ch == \n</tt>) and all characters specified in
 * the global variable \c triggerCharacters. When calling \e Tools->Align
 * the variable \c ch is empty, i.e. <tt>ch == ''</tt>.
 */
function indent(line, indentWidth, ch)
{
    // NOTE Update some global variables
    gIndentWidth = indentWidth;

    dbg("indentWidth: " + indentWidth);
    dbg("      gMode: " + document.highlightingModeAt(view.cursorPosition()));
    dbg("      gAttr: " + document.attributeName(view.cursorPosition()));
    dbg("       line: " + line);
    dbg("         ch: '" + ch + "'");

    if (ch != "")
        return processChar(line, ch);

    return indentLine(line);
}

// kate: space-indent on; indent-width 4; replace-tabs on;

/**
 * \todo Move string/comment checks to \c caretPressed() instead of particular function.
 */
