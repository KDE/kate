/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created: Wed Jun 29 23:35:13 2011
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../tree_alg/mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWindow[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x08,
      44,   11,   11,   11, 0x08,
      76,   11,   11,   11, 0x08,
     112,  103,   11,   11, 0x08,
     142,   11,   11,   11, 0x08,
     169,   11,   11,   11, 0x08,
     196,   11,   11,   11, 0x08,
     223,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0on_checkBox_3_stateChanged(int)\0"
    "on_checkBox_2_stateChanged(int)\0"
    "on_pushButton_6_released()\0newState\0"
    "on_checkBox_stateChanged(int)\0"
    "on_pushButton_5_released()\0"
    "on_pushButton_4_released()\0"
    "on_pushButton_3_released()\0"
    "on_pushButton_2_released()\0"
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: on_checkBox_3_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: on_checkBox_2_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: on_pushButton_6_released(); break;
        case 3: on_checkBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: on_pushButton_5_released(); break;
        case 5: on_pushButton_4_released(); break;
        case 6: on_pushButton_3_released(); break;
        case 7: on_pushButton_2_released(); break;
        default: ;
        }
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
