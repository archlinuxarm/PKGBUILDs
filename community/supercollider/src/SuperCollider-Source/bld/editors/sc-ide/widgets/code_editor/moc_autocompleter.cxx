/****************************************************************************
** Meta object code from reading C++ file 'autocompleter.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/code_editor/autocompleter.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'autocompleter.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__AutoCompleter[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      21,   51,   69,   69, 0x08,
      70,   69,   69,   69, 0x08,
      88,  118,   69,   69, 0x08,
     125,   69,   69,   69, 0x08,
     148,   69,   69,   69, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__AutoCompleter[] = {
    "ScIDE::AutoCompleter\0onContentsChange(int,int,int)\0"
    "pos,removed,added\0\0onCursorChanged()\0"
    "onCompletionMenuFinished(int)\0result\0"
    "clearMethodCallStack()\0hideWidgets()\0"
};

void ScIDE::AutoCompleter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        AutoCompleter *_t = static_cast<AutoCompleter *>(_o);
        switch (_id) {
        case 0: _t->onContentsChange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 1: _t->onCursorChanged(); break;
        case 2: _t->onCompletionMenuFinished((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->clearMethodCallStack(); break;
        case 4: _t->hideWidgets(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::AutoCompleter::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::AutoCompleter::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__AutoCompleter,
      qt_meta_data_ScIDE__AutoCompleter, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::AutoCompleter::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::AutoCompleter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::AutoCompleter::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__AutoCompleter))
        return static_cast<void*>(const_cast< AutoCompleter*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::AutoCompleter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
