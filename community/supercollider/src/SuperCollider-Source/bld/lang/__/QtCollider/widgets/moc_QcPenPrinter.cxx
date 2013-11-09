/****************************************************************************
** Meta object code from reading C++ file 'QcPenPrinter.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcPenPrinter.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcPenPrinter.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcPenPrinter[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       4,   39, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   29,   29,   29, 0x05,
      30,   29,   29,   29, 0x05,

 // slots: signature, parameters, type, tag, flags
      42,   29,   29,   29, 0x08,
      49,   29,   29,   29, 0x08,
      57,   29,   29,   29, 0x08,

 // properties: name, type, flags
      67,   76, 0x13095001,
      82,   76, 0x13095001,
      92,  101, 0x02095001,
     105,  101, 0x02095001,

       0        // eod
};

static const char qt_meta_stringdata_QcPenPrinter[] = {
    "QcPenPrinter\0dialogDone(int)\0\0printFunc()\0"
    "show()\0print()\0newPage()\0pageRect\0"
    "QRect\0paperRect\0fromPage\0int\0toPage\0"
};

void QcPenPrinter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcPenPrinter *_t = static_cast<QcPenPrinter *>(_o);
        switch (_id) {
        case 0: _t->dialogDone((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->printFunc(); break;
        case 2: _t->show(); break;
        case 3: _t->print(); break;
        case 4: _t->newPage(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcPenPrinter::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcPenPrinter::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QcPenPrinter,
      qt_meta_data_QcPenPrinter, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcPenPrinter::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcPenPrinter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcPenPrinter::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcPenPrinter))
        return static_cast<void*>(const_cast< QcPenPrinter*>(this));
    return QObject::qt_metacast(_clname);
}

int QcPenPrinter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QRect*>(_v) = pageRect(); break;
        case 1: *reinterpret_cast< QRect*>(_v) = paperRect(); break;
        case 2: *reinterpret_cast< int*>(_v) = fromPage(); break;
        case 3: *reinterpret_cast< int*>(_v) = toPage(); break;
        }
        _id -= 4;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 4;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 4;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcPenPrinter::dialogDone(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QcPenPrinter::printFunc()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
