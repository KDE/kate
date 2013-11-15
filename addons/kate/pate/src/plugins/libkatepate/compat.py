# -*- coding: utf-8 -*-

"""
Contains common Python2-Python3 compatibility 
When extending this, please let yourself be inspired by mitsuhiko:
http://lucumr.pocoo.org/2013/5/21/porting-to-python-3-redux/

text_type is to be used instead of `unicode` in python 2:
e.g. my_str += text_type(my_exception)

string_types is to be used instead of basestring:
i.e. if isinstance(my_obj, string_types): ...
"""

import sys
PY2 = sys.version_info[0] == 2
if not PY2:
    text_type = str
    string_types = (str,)
    unichr = chr
else:
    text_type = unicode
    string_types = (str, unicode)
    unichr = unichr
