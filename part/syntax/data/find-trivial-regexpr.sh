#!/bin/sh
# Copyright (C) 2008 Sebastian Pipping <webmaster@hartwork.org>
grep 'RegExpr' * | grep --color=auto -E 'String=\s*(["'"'"'])'\
'(\^(\\s[*+]?)?)?'\
'('\
'%?'\
'('\
'[a-zA-Z"'"'"'#_!§/=:;<>-]'\
'|\\[\\\]\[()\{\}.$^+*?]'\
'|[()|]'\
'|\(\?:'\
'|&(amp|gt|lt|apos|quot);'\
')'\
'|[0-9]'\
')*%?'\
'\1'
