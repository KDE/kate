#!/bin/sh
# Copyright (C) 2008 Sebastian Pipping <webmaster@hartwork.org>
grep 'RegExpr' * | grep --color=auto -E 'String=\s*(["'"'"'])(%?([a-zA-Z"'"'"'#_!§&/=:;<>-]|\\[\\\]\[()\{\}.$^+*?]|[()|])|[0-9])*%?\1'
