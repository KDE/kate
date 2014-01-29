# -*- coding: utf-8 -*-
#
# This file is part of Kate's Expand plugin.
#
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

'''Completion model for User Defined expand Functions (UDFs)'''

import inspect

from PyKDE4.kdecore import i18nc

from libkatepate.autocomplete import AbstractCodeCompletionModel

from .udf import getExpansionsFor


class ExpandsCompletionModel(AbstractCodeCompletionModel):
    '''Completion model for User Defined expand Functions (UDFs)'''

    TITLE_AUTOCOMPLETION = i18nc('@label:listbox', 'Expands Available')
    GROUP_POSITION = AbstractCodeCompletionModel.GroupPosition.GLOBAL

    def completionInvoked(self, view, word, invocationType):
        self.reset()
        # NOTE Do not allow automatic popup cuz most of expanders are short
        # and it will annoying when typing code...
        if invocationType == 0:
            return

        expansions = getExpansionsFor(view.document().mimeType())
        for exp, fn_tuple in expansions.items():
            # Try to get a function description (very first line)
            if hasattr(fn_tuple[0], '__description__'):
                description = fn_tuple[0].__description__.strip()
            else:
                description = None
            if hasattr(fn_tuple[0], '__details__'):
                details_text = fn_tuple[0].__details__.strip()
            else:
                details_text = None

            # Get function parameters
            fp = inspect.getargspec(fn_tuple[0])
            args = fp[0]
            params=''
            if len(args) != 0:
                params = ', '.join(args)
            if fp[1] is not None:
                if len(params):
                    params += ', '
                params += '[{}]'.format(fp[1])

            # Append to result completions list
            self.resultList.append(
                self.createItemAutoComplete(
                    text=exp
                  , description=description
                  , details = details_text
                  , args='({})'.format(params)
                  )
              )

    def reset(self):
        self.resultList = []
