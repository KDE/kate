# -*- coding: utf-8 -*-
"""Python autocompletion using David Halter’s Jedi library

Autocompletion in python you want? Become a [Jedi] you must!

[Worried] Lord Rossum is, but worried should not be you. For the easy way not always the dark path is.


[Jedi]:    https://github.com/davidhalter/jedi
[Worried]: https://github.com/davidhalter/jedi#a-little-history

TODO Disable automatic completion popup. Or make it configurable at least...
    It damn slooow actually for ost projects... and damn stupid to start to
    complit in comments and stings... so most of the time it just annoying
    than doing a real (helpful) job...

TODO Completion model needs to be deregistered on plugin unload!
"""

from collections import defaultdict

from PyQt4.QtCore import Qt, QModelIndex
from PyKDE4.kdecore import i18nc
from PyKDE4.kdeui import KIcon
from PyKDE4.ktexteditor import KTextEditor

from jedi.api import Script

import kate

from libkatepate.autocomplete import CodeCompletionBase

# alias for easier typing
CCM = KTextEditor.CodeCompletionModel


TYPE2ICON = defaultdict(lambda: 'unknown')  # Jedi completion types to KIcon converter
TYPE2ICON.update({
    'import':    'code-block',
    'module':    'code-context',
    'class':     'code-class',
    'function':  'code-function',
    'statement': 'code-variable',
    'param':     'code-typedef',
})



def _func_param_strings(call_def):  # TODO: do kwargs instead; #TODO: support kwarg-only
    """Create strings that form a parameter list

    e.g. for fun(a, b, c); completing after “fun(a,” yields 'fun(a, ' and ', c)'
    """
    params = [p.get_code().replace('\n', '') for p in call_def.params]

    before_params, param, after_params = [], '', []
    i = call_def.index
    try:
        before_params, param, after_params = params[:i], params[i], params[i + 1:]
    except IndexError:
        pass
    kv = param.split('=')
    param = kv[0] + '='

    before_params.append(param)
    before = '{}({}'.format(call_def.call_name, ', '.join(before_params))

    after_params.insert(0, '')
    after = ', '.join(after_params) + ')'

    return before, after


class JediCompletionModel(CodeCompletionBase):
    TITLE_AUTOCOMPLETION = i18nc('@label:listbox', 'Python Jedi autocomplete')
    MIMETYPES = ['text/x-python']
    """Code Completion model using Jedi.

    I chose not to use libkatepate’s AbstractCodeCompletionModel due to it being optimized
    for several traits that we don’t need, especially line and word extraction
    """

    def __init__(self, parent):
        """Script and completion list, the only properties we need"""
        super(JediCompletionModel, self).__init__(parent)
        self.script = None


    def completionInvoked(self, view, range_, invocation_type):
        """For the lack of a better event, we create a script here and remember its completions"""
        # TODO Check if cursor positioned in a comment or string and
        # DO NOT EVEN TRY TO COMPLETE SOMETHING IN THAT CASE!
        # Unfortunately, due a (still opened) BUG https://bugs.kde.org/show_bug.cgi?id=247896#c5
        # it is impossible to get that info nowadays :(
        # Fortunately, pate plugin can be hacked to provide required method...
        doc = view.document()
        if not doc.mimeType() in self.MIMETYPES:
            # Reset the model (and loose all completions!) if current document
            # is not suitable!!! (so data() will return nothing)
            self.resultList.clear()
            return

        cursor = view.cursorPosition()
        self.script = Script(doc.text(), cursor.line() + 1, cursor.column(), doc.url().toLocalFile())

        self.resultList = self.script.complete()


    def data(self, index, role):
        """Basically a 2D-lookup-table for all the things a code completion model can do"""
        if not index.parent().isValid():
            return self.TITLE_AUTOCOMPLETION

        item = self.resultList[index.row()]
        col = index.column()

        if role == Qt.DecorationRole and col == CCM.Icon:
            return KIcon(TYPE2ICON[item.type.lower()]).pixmap(16, 16)

        if role == Qt.DisplayRole:
            call_def = self.script.get_in_function_call()
            if call_def:
                before, after = _func_param_strings(call_def)

            if col == CCM.Prefix:
                return before if call_def else None
            elif col == CCM.Name:
                return item.word
            elif col == CCM.Postfix:
                return after if call_def else item.description
            #elif col == CCM.Arguments:
                # TODO: what could we use it for?
        elif col == CCM.Name:
            return self.roles.get(role)


ccm = JediCompletionModel(kate.application)


@kate.viewCreated
def init(view):
    if view is None:
        return

    # TODO THAT IS WRONG! Do not even register a completer
    # for views w/ incompatible mime type! Subscribe to
    # document events to be notified if highlighting or URI (name)
    # gets changed, and try to register if doc became looks like a
    # _real_ Python code...
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(ccm)


#
# Resources to read
#

#model: http://qt-project.org/doc/qt-4.8/qabstractitemmodel.html
#index: http://qt-project.org/doc/qt-4.8/qmodelindex.html#details
#colns: http://api.kde.org/4.10-api/kde-baseapps-apidocs/kate/ktexteditor/html/classKTextEditor_1_1CodeCompletionModel.html#a3bd60270a94fe2001891651b5332d42b
#roles: http://qt-project.org/doc/qt-4.8/qt.html#ItemDataRole-enum
#       http://api.kde.org/4.10-api/kde-baseapps-apidocs/kate/ktexteditor/html/classKTextEditor_1_1CodeCompletionModel.html#ab709029fde377c02a760593aa5f4ac63
#props: http://www.purinchu.net/kdelibs-apidocs/interfaces/ktexteditor/html/codecompletionmodel_8h_source.html#l00098

#jedi: https://github.com/davidhalter/jedi
#plugin api: http://jedi.jedidjah.ch/en/latest/docs/plugin-api.html
