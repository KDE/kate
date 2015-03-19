from PyQt4.QtScript import QScriptEngine, QScriptValue, QScriptValueIterator

import re
try:
    from collections.abc import Mapping, Iterable
except ImportError:
    from collections import Mapping, Iterable

def iter_js_obj(js_obj):
    """QScriptValueIterator to python iterator emitting tuples of (str, QScriptValue)"""
    it = QScriptValueIterator(js_obj)

    while it.hasNext():
        it.next()
        yield (it.name(), it.value())


def iter_js_arr(js_arr):
    """iterate array in order"""
    length = js_arr.property('length').toUInt32()

    for i in range(length):
        yield js_arr.property(i)


class PyJSEngine(QScriptEngine):
    def py2js(self, py_obj):
        """python object to QScriptValue"""
        if isinstance(py_obj, Mapping):
            ob = self.newObject()
            for name, value in py_obj.items():
                ob.setProperty(name, self.py2js(value))
            return ob
        elif isinstance(py_obj, Iterable) and not isinstance(py_obj, str):
            a = self.newArray()
            for i, item in enumerate(py_obj):
                a.setProperty(i, self.py2js(item))
            return a
        return QScriptValue(self, py_obj)

    def js_call(self, js_function):
        """js function to python function acting on python types, returning a python type"""
        def c(*args):
            js_val = js_function.call(self.globalObject(), self.py2js(args))
            return js_val.toVariant()
        return c


class JSModule:
    """Loads a JSModule from filepath into engine"""
    def __init__(self, engine, filepath, objname):
        self.engine = engine

        with open(filepath) as s:
            code = fix_js(s.read())

        self.engine.evaluate(code, s.name)
        if self.engine.hasUncaughtException():
            raise ValueError('file can’t be evaluated', self.engine.uncaughtException().toVariant())

        self.obj = self.engine.globalObject().property(objname)

    def __call__(self, *args):
        call = self.engine.js_call(self.obj)
        return call(*args)

    def __getitem__(self, key):
        value = self.obj.property(key)
        if value.isFunction():
            return self.engine.js_call(value)
        return value.toVariant()


# some ECMAScript ReservedWords which can be used as object property names.
_keywords = 'continue|break|function|var|delete|if|then|else'  # extend as appropriate
_keyword_in_okey = re.compile(r'\b({})\s*:'.format(_keywords))
_keyword_in_attr = re.compile(r'\.({})\b'.format(_keywords))


def fix_js(code):
    """Fixes JS so that Qt’s engine can digest it.

    The problem is that unquoted object keys like `{key: 'value'}`,
    as well as member accessors like `object.member` may be IdentifierNames,
    while Qt’s implementation expects them not to be ReservedWords.

        >>> fix_js('var foo = { function: function() {} }')
        '{ "function": function() {} }'
        >>> fix_js('foo.function()')
        'foo["function"]()'

    * http://www.ecma-international.org/ecma-262/5.1/#sec-7.6
    * http://www.ecma-international.org/ecma-262/5.1/#sec-11.1.5
    * http://www.ecma-international.org/ecma-262/5.1/#sec-11.2.1
    """
    code = _keyword_in_okey.sub(r'"\1":', code)
    code = _keyword_in_attr.sub(r'["\1"]', code)

    return code

if __name__ == '__main__':
    import sys
    from pprint import pprint
    from PyQt4.QtGui import QApplication

    app = QApplication(sys.argv)

    se = PyJSEngine()

    JSLint = JSModule(se, 'fulljslint.js', 'JSLINT')

    with open(sys.argv[1]) as s:
        lint_ok = JSLint(s.read(), {})

    if lint_ok:
        print('OK')
    else:
        errors = JSLint['errors']
        pprint(errors)
