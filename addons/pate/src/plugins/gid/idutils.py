#
# Copyright 2012, 2013, Shaheed Haque <srhaque@theiet.org>.
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

# Python implementation of a (subset of) the reading functionality GNU idutils,
# i.e. of lid(1). Parts of this work are derived in detail from the source code
# in lid.c, and where that is the case, the variables names and structure have
# been modelled after the original to ease maintenance.
#
# The remainder attempts to be more Pythonic in style.

from __future__ import print_function
__title__ = "idutils"
__author__ = "Shaheed Haque <srhaque@theiet.org>"
__license__ = "LGPL"

import array
import mmap
import os
import struct
import sys

def toStr(_bytes):
    if sys.version_info.major >= 3:
        return _bytes.decode("utf-8")
    else:
        return unicode(_bytes)

def toBytes(name):
    if sys.version_info.major >= 3:
        if isinstance(name, str):
            return bytearray(name.encode("utf-8"))
        else:
            return bytearray(name)
    else:
        if isinstance(name, unicode):
            return bytearray(name.encode("utf-8"))
        else:
            return bytearray(name)

class _FileLinkDb():
    """Read the file link database in an ID file.

    The database is an array where each item comprises:

        - A NULL terminated string name.

        - a flags byte.

        - a 3 byte parent index which identifies the parent directory's location
        in the array. A root element points to itself.
    """

    class Flags():
        """File link flags."""
        FL_CMD_LINE_ARG        = 1 << 0
        FL_USED            = 1 << 1
        # has a corresponding member_file entry
        FL_MEMBER        = 1 << 2
        FL_SCAN_ME        = 1 << 3
        FL_SYM_LINK        = 1 << 4
        FL_TYPE_DIR        = 1 << 5
        FL_TYPE_FILE        = 1 << 6
        FL_PRUNE        = 1 << 7
        FL_TYPE_MASK    = FL_TYPE_DIR | FL_TYPE_FILE

        def __init__(self, flags):
            self.flags = flags

        def isCmdLineArg(self):
            return ((self.flags & _FileLinkDb.Flags.FL_CMD_LINE_ARG) != 0)

        def isUsed(self):
            return ((self.flags & _FileLinkDb.Flags.FL_USED) != 0)

        def isMember(self):
            return ((self.flags & _FileLinkDb.Flags.FL_MEMBER) != 0)

        def isSymLink(self):
            return ((self.flags & _FileLinkDb.Flags.FL_SYM_LINK) != 0)

        def isDir(self):
            return ((self.flags & _FileLinkDb.Flags.FL_TYPE_MASK) == _FileLinkDb.Flags.FL_TYPE_DIR)

        def isFile(self):
            return ((self.flags & _FileLinkDb.Flags.FL_TYPE_MASK) == _FileLinkDb.Flags.FL_TYPE_FILE)

        def isPrune(self):
            return ((self.flags & _FileLinkDb.Flags.FL_PRUNE) != 0)

        def __repr__(self):
            result = "("
            if self.isCmdLineArg():
                result += "isCmdLineArg, "
            if self.isUsed():
                result += "isUsed, "
            if self.isMember():
                result += "isMember, "
            if self.isSymLink():
                result += "isSymLink, "
            if self.isDir():
                result += "isDir, "
            if self.isFile():
                result += "isFile, "
            if self.isPrune():
                result += "isPrune, "
            if len(result) > 1:
                return result[:-2] + ")"
            else:
                return "()"

    # The raw bytearray.
    rawData = None
    # Number of items.
    maxItems = None
    # Offsets into rawData for each item.
    offsets = None
    # Index into offsets for each Member item.
    memberOffsets = None

    def __init__(self, rawData, maxItems):
        self.rawData = rawData
        self.maxItems = maxItems
        self.offsets = array.array('L')
        self.memberOffsets = array.array('L')
        lastItem = len(self.rawData) - 2
        i = 0;
        j = 0;
        while i < lastItem:
            if self.rawData[i] == 0:
                #print "file",len(self.offsets),str(self.rawData[j:i])
                flags = self.Flags(self.rawData[i + 1])
                if flags.isMember():
                    #
                    # Remember the offset used for a member file.
                    #
                    self.memberOffsets.append(len(self.offsets))
                self.offsets.append(j)
                i += 5
                j = i
            else:
                i += 1
        if len(self.offsets) != self.maxItems:
            raise IOError("Read {} file links instead of {}".format(len(self.offsets), self.maxItems))

    def file(self, N):
        """Retrieve the full file in the form (name, flags, parentIndex)."""
        parentName = None
        name, flags, parentIndex = self._fileLink(N)
        while N != parentIndex:
            N = parentIndex
            parentName, parentFlags, parentIndex = self._fileLink(N)
            name = parentName + '/' + name
        #
        # Strip the root value if it is the same as the separator.
        #
        if parentName == '/':
            name = name[len(parentName):]
        return name, self.Flags(flags)

    def _fileLink(self, N):
        """Retrieve the Nth file link item in the form (name, flags, parentIndex)."""
        i = self.offsets[N]
        j = i
        while self.rawData[i] != 0:
            i += 1
        name = self.rawData[j:i]
        i += 1
        flags = self.rawData[i]
        parentIndex = self.rawData[i + 3]
        parentIndex = (parentIndex << 8) + self.rawData[i + 2]
        parentIndex = (parentIndex << 8) + self.rawData[i + 1]
        return toStr(name), flags, parentIndex

class _TokenDb():
    """Read the file link database in an ID file.

    The database is an array where each item comprises:

        - A NULL terminated string name.

        - a flags byte.

        - A number of hits (i.e. number of instances of the token found).

        - A file list where the token was seen (see @ref _FileLinkDb).
    """

    class TokenData():
        """Memory mapped access to token data."""
        def __init__(self, f):
            """Initialise with a file object."""
            self.mapped = mmap.mmap(f.fileno(), 0, access = mmap.ACCESS_READ)
            self.offset = f.tell()
            #
            # The file has two "excess" bytes at the end.
            #
            self.length = os.fstat(f.fileno()).st_size - self.offset - 2

        def __getitem__(self, key):
            try:
                # single element
                if sys.hexversion > 0x03000000:
                    return self.mapped[self.offset + key]
                else:
                    return ord(self.mapped[self.offset + key])
            except TypeError:
                # A slice
                return bytearray(self.mapped[self.offset + key.start:self.offset + key.stop])

        def __len__(self):
            return self.length

    class Flags():
        """Token flags."""
        # 1 = hits are stored as a vector
        # 0 = hits are stored as a 8-way tree of bits
        # mkid chooses whichever is more compact.
        # vector is more compact for tokens with few hits
        TOK_VECTOR        = 0x01
        # occurs as a number
        TOK_NUMBER        = 0x02
        # occurs as a name
        TOK_NAME        = 0x04
        # occurs in a string
        TOK_STRING        = 0x08
        # occurs as a literal
        TOK_LITERAL        = 0x10
        # occurs in a comment
        TOK_COMMENT        = 0x20
        TOK_UNUSED        = 0x40
        # count is two bytes
        TOK_SHORT_COUNT    = 0x80

        def __init__(self, flags):
            self.flags = flags

        def isVector(self):
            return ((self.flags & _TokenDb.Flags.TOK_VECTOR) != 0)

        def isNumber(self):
            return ((self.flags & _TokenDb.Flags.TOK_NUMBER) != 0)

        def isName(self):
            return ((self.flags & _TokenDb.Flags.TOK_NAME) != 0)

        def isString(self):
            return ((self.flags & _TokenDb.Flags.TOK_STRING) != 0)

        def isLiteral(self):
            return ((self.flags & _TokenDb.Flags.TOK_LITERAL) != 0)

        def isComment(self):
            return ((self.flags & _TokenDb.Flags.TOK_COMMENT) != 0)

        def isUnused(self):
            return ((self.flags & _TokenDb.Flags.TOK_UNUSED) != 0)

        def isShortCount(self):
            return ((self.flags & _TokenDb.Flags.TOK_SHORT_COUNT) != 0)

        def __repr__(self):
            result = "("
            if self.isVector():
                result += "isVector, "
            if self.isNumber():
                result += "isNumber, "
            if self.isName():
                result += "isName, "
            if self.isString():
                result += "isString, "
            if self.isLiteral():
                result += "isLiteral, "
            if self.isComment():
                result += "isComment, "
            if self.isUnused():
                result += "isUnused, "
            if self.isShortCount():
                result += "isShortCount, "
            if len(result) > 1:
                return result[:-2] + ")"
            else:
                return "()"

    def __init__(self, file, maxItems, maxFiles, fileLinkDb):
        # The raw bytearray.
        self.rawData = self.TokenData(file)
        # Number of items.
        self.maxItems = maxItems
        self.fileLinkDb = fileLinkDb
        #
        # Determine absolute name of the directory name to which database
        # constituent files are relative.
        #
        #members_0 = read_id_file(idh.idh_file_name, &idh);
        #
        # More than enough...
        #
        bitsVecSize = (maxFiles + 7) // 4
        self.bitsVec = array.array('B')
        for i in range(bitsVecSize):
            self.bitsVec.append(0)
        self.tree8Levels = self._tree8_count_levels(maxFiles)
        self.reservedFlinkSlots = 3
        self.flinkv_0 = array.array('L')
        for i in range(maxFiles + self.reservedFlinkSlots + 2):
            self.flinkv_0.append(0)

    def binarySearch(self, name, prefixMatchMode):
        """Return the (offset, name, usage) for a name.
        The look up is performed using a binary chop, and it is hoped that the
        in-memory rawData makes this fast enough to use for auto-completion
        logic in an editor.

            * name - the name to match.

            * prefixMatchMode - True if prefix matching is enabled.

        The returned usage is as per @ref _usage().

        See idutils lid.c.
        """
        token = toBytes(name)
        token.append(0)
        tokenSize = len(token)
        start = 0
        end = len(self.rawData)
        anchor_offset = None
        order = None

        while start < end:
            offset = start + (end - start) // 2
            offset = self._skip_past_00(offset)
            if offset >= end:
                offset = start

            # Compare the token names.
            for i in range(tokenSize):
                if self.rawData[offset + i] != token[i]:
                    break
            order = token[i] - self.rawData[offset + i]

            # Did we match? If not, in which direction do we move?
            if order < 0:
                end = offset
                #
                # The rawData was longer than the token, so this could also be
                # a prefix match.
                #
                if prefixMatchMode:
                    #
                    # Prefix matched if we got to the end of the token...
                    #
                    if i == (tokenSize - 1):
                        anchor_offset = offset;
            elif order > 0:
                try:
                    start = self._skip_past_00(offset)
                except IndexError:
                    break;
            else:
                break

        if order != 0:
            #
            # String not found. Use the prefix match if we have one.
            #
            if anchor_offset:
                offset = anchor_offset
                #
                # Return the prefix-matched name.
                #
                while self.rawData[offset + i] != 0:
                    i += 1
                name = toStr(self.rawData[offset:offset + i])
            else:
                raise IndexError("Not found: " + name)

        #
        # Fetch the binary blob results, starting after the zero termination.
        #
        blobStart = offset + len(name) + 1
        try:
            blobEnd = self._skip_past_00(blobStart)
        except IndexError:
            blobEnd = len(self.rawData)
        return offset, name, self._usage(blobStart, blobEnd - 2)

    def nextPrefixMatch(self, startingOffset, name, withUsage = False):
        """Return the next (offset, name, usage) matching a name after the startingOffset.

        The startingOffset is assumed to have been returned by a previous call
        to @ref binarySearch, or this function. If the prefix is not matched,
        IndexError is thrown.
        """
        offset = self._skip_past_00(startingOffset)
        token = toBytes(name)
        token.append(0)
        tokenSize = len(token)

        # Compare the token names.
        for i in range(tokenSize):
            if self.rawData[offset + i] != token[i]:
                break
        order = token[i] - self.rawData[offset + i]

        # Did we match?
        if order < 0:
            #
            # The rawData was longer than the token, so this could also be
            # a prefix match.
            #
            # Prefix matched if we got to the end of the token...
            #
            if i != (tokenSize - 1):
                raise IndexError("Not found: " + name)
        elif order > 0:
            raise IndexError("Not found: " + name)
        else:
            pass

        #
        # Return the prefix-matched name.
        #
        while self.rawData[offset + i] != 0:
            i += 1
        name = toStr(self.rawData[offset:offset + i])
        #
        # Are we done?
        #
        if not withUsage:
            return offset, name
        #
        # Fetch the binary blob results, starting after the zero termination.
        #
        blobStart = offset + len(name) + 1
        try:
            blobEnd = self._skip_past_00(blobStart)
        except IndexError:
            blobEnd = len(self.rawData)
        return offset, name, self._usage(blobStart, blobEnd - 2)

    def _skip_past_00(self, offset):
        """Skip till we see a pair of zeros.

        See idutils idread.c.
        """
        while True:
            while True:
                offset += 1
                if self.rawData[offset] == 0:
                    break;
            offset += 1
            if self.rawData[offset] == 0:
                break;
        return offset + 1

    def _usage(self, start, end):
        """Return the flags, number of hits, and file list for the token."""
        flags = self.Flags(self.rawData[start])
        start += 1
        if flags.isShortCount():
            count = self.rawData[start + 1]
            count = (count << 8) + self.rawData[start]
            start += 2
        else:
            count = self.rawData[start]
            start += 1
        return flags, count, self._tree8_to_flinkv(start, end)

    def _tree8_to_flinkv(self, start, end):
        for i in range(len(self.bitsVec)):
            self.bitsVec[i] = 0
        bv, start = self._tree8_to_bits(0, start, end, self.tree8Levels)
        #if start != end:
            #raise IOError("Start {} != {}".format(start, end))
        return self._bits_to_flinkv()

    def _bits_to_flinkv(self):
        """Return a list of file links."""
        bv = 0
        flinkv = self.reservedFlinkSlots
        members = 0
        end = len(self.fileLinkDb.memberOffsets)

        working = True
        while working:
            hits = None
            bit = None
            #
            # Skip empty bytes.
            #
            while self.bitsVec[bv] == 0:
                bv += 1
                members += 8
                if members >= end:
                    working = False
                    break
            if not working:
                break
            #
            # Decode the non-empty byte.
            #
            hits = self.bitsVec[bv]
            bv += 1
            bit = 1
            while (bit & 0xff) != 0:
                if (bit & hits) != 0:
                    self.flinkv_0[flinkv] = self.fileLinkDb.memberOffsets[members]
                    #print("FOUND MEMBER", self.flinkv_0[flinkv])
                    flinkv += 1
                members += 1
                if members >= end:
                    working = False
                    break
                bit = bit << 1
            if not working:
                break
        #
        # Terminate the list of files.
        #
        self.flinkv_0[flinkv] = 0
        return self.flinkv_0[self.reservedFlinkSlots:flinkv]

    def _tree8_to_bits(self, bv, start, end, level):
        """Recursively convert the tree8 into a bit vector."""
        hits = self.rawData[start]
        start += 1
        level -= 1
        if level > 0:
            incr = 1 << ((level - 1) * 3)
            bit = 1
            while (bit & 0xff) != 0:
                if (bit & hits) != 0:
                    bv, start = self._tree8_to_bits(bv, start, end, level)
                else:
                    bv += incr
                bit = bit << 1
        else:
            self.bitsVec[bv] |= hits
            bv += 1
        return bv, start

    def _tree8_count_levels(self, cardinality):
        levels = 1
        cardinality -= 1
        cardinality = cardinality >> 3
        while cardinality != 0:
            levels += 1
            cardinality = cardinality >> 3
        return levels

class Lookup():
    _IO_TYPE_FIX = ">"
    _IO_TYPE_INT = "<"

    def __init__(self):
        self.file = None
        #
        # Header information.
        #
        self.headerSize = None
        self.magic = None
        self.pad = None
        self.version = None
        self.flags = None
        self.fileLinks = None
        self.files = None
        self.tokens = None
        self.bufSize = None
        self.vecSize = None
        self.tokensOffset = None
        self.flinksOffset = None
        self.endOffset = None
        self.maxLink = None
        self.maxPath = None
        #
        # File links database.
        #
        self.fileLinkDb = None
        #
        # _TokenDb database.
        #
        self.tokenDb = None

    def setFile(self, name):
        self.file = open(name.encode("utf-8"), "rb")
        self.file.seek(0, os.SEEK_SET)
        self.headerSize = 0
        self.magic = self._read(2, Lookup._IO_TYPE_FIX)
        self.pad = self._read(1, Lookup._IO_TYPE_FIX)
        self.version = self._read(1, Lookup._IO_TYPE_FIX)
        self.flags = self._read(2, Lookup._IO_TYPE_INT)
        self.fileLinks = self._read(4, Lookup._IO_TYPE_INT)
        self.files = self._read(4, Lookup._IO_TYPE_INT)
        self.tokens = self._read(4, Lookup._IO_TYPE_INT)
        self.bufSize = self._read(4, Lookup._IO_TYPE_INT)
        self.vecSize = self._read(4, Lookup._IO_TYPE_INT)
        self.tokensOffset = self._read(4, Lookup._IO_TYPE_INT)
        self.flinksOffset = self._read(4, Lookup._IO_TYPE_INT)
        self.endOffset = self._read(4, Lookup._IO_TYPE_INT)
        self.maxLink = self._read(2, Lookup._IO_TYPE_INT)
        self.maxPath = self._read(2, Lookup._IO_TYPE_INT)
        #
        # Initialise file link reading.
        #
        data = self._readByteArray(self.flinksOffset, self.tokensOffset)
        self.fileLinkDb = _FileLinkDb(data, self.fileLinks)
        #
        # Initialise token reading.
        #
        self.tokenDb = _TokenDb(self.file, self.tokens, self.files, self.fileLinkDb)
        self.file.close()

    def _read(self, size, type):
        if size == 1:
            data = struct.unpack(type + "B", self.file.read(size))
        elif size == 2:
            data = struct.unpack(type + "H", self.file.read(size))
        elif size == 4:
            data = struct.unpack(type + "I", self.file.read(size))
        else:
            raise IOError
        if len(data) == 0:
            raise IOError
        self.headerSize += size
        return data[0]

    def _readByteArray(self, start, end):
        self.file.seek(start, os.SEEK_SET)
        data = bytearray(end - start)
        bytesRead = self.file.readinto(data)
        if bytesRead < len(data):
            raise IOError("Read {} bytes instead of {}".format(bytesRead, len(data)))
        return data

    def __repr__(self):
        return "headerSize {}, magic {:#0x}, version {}, flags {:#0x}, fileLinks {}, " \
                "files {}, tokens {}, bufSize {}, vecSize {}, tokensOffset {}, " \
                "flinksOffset {}, endOffset {}, maxLink {}, maxPath {}".format(
                self.headerSize, self.magic, self.version, self.flags, self.fileLinks,
                self.files, self.tokens, self.bufSize, self.vecSize, self.tokensOffset,
                self.flinksOffset, self.endOffset, self.maxLink, self.maxPath)

    def literalSearch(self, token):
        """Return the (tokenFlags, hitCount, [(fileName, fileFlags)...]) for a token."""
        offset, name, usage = self.tokenDb.binarySearch(token, False)
        tokenFlags, hitCount, fileIds = usage
        return tokenFlags, hitCount, [self.fileLinkDb.file(fileId) for fileId in fileIds]

    def prefixSearchFirst(self, token):
        """Return the first (offset, name) matching a prefix token."""
        offset, name, usage = self.tokenDb.binarySearch(token, True)
        return offset, name

    def prefixSearchNext(self, offset, token):
        """Return the (offset, name) matching the prefix token after the given offset."""
        return self.tokenDb.nextPrefixMatch(offset, token)

if __name__ == "__main__":
    dataSource = Lookup()
    dataSource.setFile("ID")
    #tokenFlags, hitCount, files = dataSource.literalSearch("fib_route_looped")
    tokenFlags, hitCount, files = dataSource.literalSearch("cef_dpss_open")
    print(tokenFlags, hitCount, files)

