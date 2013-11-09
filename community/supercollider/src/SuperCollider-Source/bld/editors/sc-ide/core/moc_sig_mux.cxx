/****************************************************************************
** Meta object code from reading C++ file 'sig_mux.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/core/sig_mux.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sig_mux.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__SignalMultiplexer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      25,   52,   62,   62, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SignalMultiplexer[] = {
    "ScIDE::SignalMultiplexer\0"
    "setCurrentObject(QObject*)\0newObject\0"
    "\0"
};

void ScIDE::SignalMultiplexer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SignalMultiplexer *_t = static_cast<SignalMultiplexer *>(_o);
        switch (_id) {
        case 0: _t->setCurrentObject((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::SignalMultiplexer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SignalMultiplexer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__SignalMultiplexer,
      qt_meta_data_ScIDE__SignalMultiplexer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SignalMultiplexer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SignalMultiplexer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SignalMultiplexer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SignalMultiplexer))
        return static_cast<void*>(const_cast< SignalMultiplexer*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::SignalMultiplexer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
