#
# Copyright 2009, 2013, Shaheed Haque <srhaque@theiet.org>.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

from __future__ import print_function

class GdbCommandDb(object):
    """A database of all GDB commands.

    Classify them as GDB does, and then add them to a global structure.
    New and overriding commands can then be added.
    """

    #
    # The keyword dictionary contains a series of key, value pairs of the form
    #
    #   "keyword" : ( apropos, nextLevel | None, classification | None, function | None )
    #
    # The nextLevel allows sequences of keywords to be represented.
    # Given that the entries are added such that the command prefixes
    # are added before leaf commands, the apropos string is guaranteed to be
    # that of the prefix.
    #
    # The classification is only present for leaf entries, and reflects the GDB
    # help classification of the command.
    #
    # The function item is None for GDB's own commands, and the implementation for our versions.
    #
    keyword_db = None

    #
    # Command classifications according to GDB.
    #
    classes_db = None

    def __init__(self, helpText):
        """From GDB's "help all" output, find all the commands it has.

        @param helpText         Output of GDB "help all".
        """
        super(GdbCommandDb, self).__init__()
        #
        # Start from scratch.
        #
        self.keyword_db = dict()
        self.classes_db = list()
        #
        # First, read all the command line help to find out what GDB has.
        #
        clazz = None
        for line in helpText:
            line = line.strip()
            if line.startswith("Command class"):
                clazz = line[15:]
                self.classes_db.append(clazz)
            elif line.startswith("Unclassified commands"):
                clazz = "unclassified"
                self.classes_db.append(clazz)
            elif line.find(" -- ") > -1:
                (command, apropos) = line.split(" -- ")
                #
                # Add the command to the database.
                #
                keywords = command.split(" ")
                dictionary = self.keyword_db
                for i in range(len(keywords)):
                    if keywords[i] in dictionary:
                        (oldApropos, oldLevel, oldClazz, oldFunction) = dictionary[keywords[i]]
                        if oldLevel:
                            #
                            # We already have a dictionary at this level.
                            #
                            dictionary = oldLevel
                        else:
                            #
                            # Replace the value in the current level with a
                            # new dictionary representing the additional level
                            # of nesting, and move the original value into it.
                            # Note that the old classification is kept; this
                            # allows the entry to function as a leaf.
                            #
                            newLevel = dict()
                            dictionary[keywords[i]] = (oldApropos, newLevel, oldClazz, oldFunction)
                            dictionary = newLevel
                    else:
                        #
                        # Add the new keyword to the current dictionary.
                        #
                        dictionary[keywords[i]] = (apropos, None, clazz, None)
            elif line:
                raise Exception("Unmatched line '{}'".format(line))

    def addCustom(self, function):
        """Add a custom command to the global database.
        Will override any previously added entry (i.e. from GDB).
        """
        try:
            helpText = function.__doc__.split("\n")
            clazz = helpText[1].lstrip("\t")
            (command, apropos) = helpText[3].lstrip("\t").split(" -- ")
            command = command.lstrip()
        except AttributeError as e:
            #
            # If we are overriding an existing command, maybe we already have
            # the information we need?
            #
            command = function.__name__[3:].replace("__", "-").replace("_", " ")
            (matchedKeywords, unmatchedKeyword, completions, lastMatchedEntry) = self.lookup(command)
            if command != matchedKeywords or unmatchedKeyword:
                raise AttributeError("No help for: {}".format(function.__name__))
            apropos = completions[0]
            clazz = completions[2]
        #
        # Add the command to the database.
        #
        keywords = command.split(" ")
        dictionary = self.keyword_db
        for i in range(len(keywords) - 1):
            #
            # This is a prefix for the final keyword, navigate the tree.
            #
            if keywords[i] in dictionary:
                (oldApropos, oldLevel, oldClazz, oldFunction) = dictionary[keywords[i]]
                if oldLevel:
                    #
                    # We already have a dictionary at this level.
                    #
                    dictionary = oldLevel
                else:
                    #
                    # Replace the value in the current level with a
                    # new dictionary representing the additional level
                    # of nesting, and move the original value into it.
                    # Note that the old classification is kept; this
                    # allows the entry to function as a leaf.
                    #
                    newLevel = dict()
                    dictionary[keywords[i]] = (oldApropos, newLevel, oldClazz, oldFunction)
                    dictionary = newLevel
            else:
                #
                # Add the new keyword to the current dictionary.
                #
                dictionary[keywords[i]] = ("", None, clazz, None)
        #
        # Add the final keyword.
        #
        keyword = keywords[len(keywords) - 1]
        if keyword in dictionary:
            #
            # Keep any dictionary we already have at this level.
            #
            (oldApropos, oldLevel, oldClazz, oldFunction) = dictionary[keyword]
            dictionary[keyword] = (apropos, oldLevel, clazz, function)
        else:
            #
            # Add the new keyword to the current dictionary.
            #
            dictionary[keyword] = (apropos, None, clazz, function)

    def lookup(self, line):
        """
        Match the given line containing abbreviated keywords with the keyword
        database, and return a string with any matched keywords, the first
        unmatched keyword and a set of possible completions:

            (matchedKeywords, None | unmatchedKeyword, completions, lastMatchedEntry)

        None unmatchedKeyword means that the entire string was matched, and
        the completions is the matched dictionary entry.
        A completions set gives possible completions to resolve ambiguity.
        A completions dictionary means the line itself was not matched.
        """
        dictionary = self.keyword_db
        matchedKeywords = ""
        matchedEntry = None
        previous_dictionary = dictionary
        previousMatchedEntry = matchedEntry
        keywords = line.split()
        for i in range(len(keywords)):
            if not dictionary:
                #
                # Whoops, we have more input than dictionary levels...
                #
                return (matchedKeywords, keywords[i], None, matchedEntry)
            previous_dictionary = dictionary
            previousMatchedEntry = matchedEntry
            matches = [key for key in dictionary if key.startswith(keywords[i])]
            #
            # Success if we have exactly one match, or an exact hit on the first item.
            #
            if len(matches) == 1 or len(matches) > 1 and sorted(matches)[0] == keywords[i]:
                #
                # A match! Accumulate the matchedKeywords string.
                #
                matches = sorted(matches)
                matchedEntry = dictionary[matches[0]]
                (oldApropos, oldLevel, oldClazz, oldFunction) = matchedEntry
                if len(matchedKeywords) > 0:
                    matchedKeywords = " ".join((matchedKeywords, matches[0]))
                else:
                    matchedKeywords = matches[0]
                #
                # And prepare for the next level.
                #
                dictionary = oldLevel
            elif len(matches) == 0:
                #
                # No match for the current keyword.
                #
                return (matchedKeywords, keywords[i], dictionary, previousMatchedEntry)
            else:
                #
                # Ambiguous match.
                #
                return (matchedKeywords, keywords[i], matches, None)
        #
        # All keywords matched!
        #
        return (matchedKeywords, None, matchedEntry, matchedEntry)

    def walk(self, userCallback, userFilter, userArg, indentation = "", prefix = ""):
        """
        Walk the contents of the database.
        """
        self.walkLevel(self.keyword_db, userCallback, userFilter, userArg, indentation, prefix)

    def walkLevel(self, level, userCallback, userFilter, userArg, indentation = "", prefix = ""):
        """
        Walk the contents of the database level.
        """
        for keyword in sorted(level.keys()):
            (oldApropos, oldLevel, oldClazz, oldFunction) = level[keyword]
            userCallback(userFilter, userArg, indentation, prefix, keyword, oldApropos, oldClazz, oldFunction)
            if oldLevel:
                self.walkLevel(oldLevel, userCallback, userFilter, userArg, indentation + ".   ", prefix + keyword + " ")

    _result = None
    def __repr__(self):
        def callback(clazzPrefix, arg, indentation, prefix, keyword, apropos, clazz, function):
            """
            Dump the contents of the database as help text. Only leaf items which
            match the given classification prefix are emitted.
            """
            #   "keyword" : ( apropos, nextLevel | None, classification | None, function | None )
            if clazz.startswith(clazzPrefix) :
                if not function:
                    function = "None"
                else:
                    function = function.__name__
                self._result += indentation + "'" + keyword + "', " + function + "\n"

        self._result = ""
        self.walk(callback, "", self._result)
        result = self._result
        self._result = None
        return result

if __name__ == "__main__":
    helpText = [
        "Command class: aliases",
        "",
        "ni -- Step one instruction",
        "rc -- Continue program being debugged but run it in reverse",
        "",
        "Command class: breakpoints",
        "",
        "awatch -- Set a watchpoint for an expression",
        "break -- Set breakpoint at specified line or function",
        "break-range -- Set a breakpoint for an address range",
        "set trace-user -- Set the user name to use for current and future trace runs",
        "set trust-readonly-sections -- Set mode for reading from readonly sections",
        "set tui -- TUI configuration variables",
        "set tui active-border-mode -- Set the attribute mode to use for the active TUI window border"
        ]
    db = GdbCommandDb(helpText)
    print(db)

