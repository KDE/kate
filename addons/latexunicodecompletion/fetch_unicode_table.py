#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>
# SPDX-License-Identifier: LGPL-2.0-or-later


JULIA_UNICODE_DOCUMENTATION_URL = "https://docs.julialang.org/en/v1/manual/unicode-input/"
CONTAINER_ID = "documenter-page"
OUTFNAME = "completiontrie.h"

from urllib import request
from html.parser import HTMLParser

class JuliaUnicodeCompletionsParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.table = []
        self._in_container = False
        self._in_table = False
        self._in_header = False
        self._in_body = False
        self._in_cell = False
        self._finished = False

        self._current_row = None

    def handle_starttag(self, tag, attrs):
        if self._finished:
            return
        if not self._in_container:
            for a in attrs:
                if a[0] == "id" and a[1] == CONTAINER_ID:
                    self._in_container = True
                    break
        elif not self._in_table and tag == "table":
            self._in_table = True
        elif self._in_table:
            if tag == "tr":
                if not self._in_header and not self._in_body:
                    self._in_header = True
                else:
                    self._in_body = True
                    self._current_row = []
            elif tag == "td" and self._in_body:
                self._in_cell = True

    def handle_data(self, data):
        if self._finished:
            return
        if self._in_body:
            self._current_row.append(data)

    def handle_endtag(self, tag):
        if self._finished:
            return
        if self._in_body:
            if tag == "tr":
                self.table.append(tuple(self._current_row))
                self._current_row = []
            elif tag == "table":
                self._finished = True

parser = JuliaUnicodeCompletionsParser()
with request.urlopen(JULIA_UNICODE_DOCUMENTATION_URL) as page:
    parser.feed(page.read().decode(page.headers.get_content_charset()))
parser.close()

parser.table.sort(key=lambda x: x[2])
with open(OUTFNAME, "w") as out:
    out.write("""\
#include <tsl/htrie_map.h>
#include <QString>
struct Completion {
    QString codepoint;
    QString chars;
    QString name;
};

static const tsl::htrie_map<char, Completion> completiontrie({
""")

    for i, completion in enumerate(parser.table):
        latexsym = completion[2].replace("\\", "\\\\")
        if i > 0:
            out.write(",")
        out.write(f"{{\n\"{latexsym}\",\n{{\n"
                  f"    QStringLiteral(\"{completion[0]}\"),\n"
                  f"    QStringLiteral(u\"{completion[1]}\"),\n"
                  f"    QStringLiteral(\"{completion[3]}\")\n}}\n}}\n")
    out.write("""\
});
""")
