from PyQt4.QtScript import QScriptEngine, QScriptValue, QScriptValueIterator

from collections.abc import Mapping, Iterable

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
            self.engine.evaluate(s.read(), s.name)

        self.obj = self.engine.globalObject().property(objname)

    def __call__(self, *args):
        call = self.engine.js_call(self.obj)
        return call(*args)

    def __getitem__(self, key):
        value = self.obj.property(key)
        if value.isFunction():
            return self.engine.js_call(value)
        return value.toVariant()

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