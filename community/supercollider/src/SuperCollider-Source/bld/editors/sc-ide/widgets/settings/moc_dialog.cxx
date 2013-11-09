/****************************************************************************
** Meta object code from reading C++ file 'dialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/settings/dialog.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dialog.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__Settings__Dialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   47,   47,   47, 0x05,
      48,   47,   47,   47, 0x05,

 // slots: signature, parameters, type, tag, flags
      70,   47,   47,   47, 0x0a,
      79,   47,   47,   47, 0x0a,
      88,   47,   47,   47, 0x0a,
      96,   47,   47,   47, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__Settings__Dialog[] = {
    "ScIDE::Settings::Dialog\0storeRequest(Manager*)\0"
    "\0loadRequest(Manager*)\0accept()\0"
    "reject()\0reset()\0apply()\0"
};

void ScIDE::Settings::Dialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Dialog *_t = static_cast<Dialog *>(_o);
        switch (_id) {
        case 0: _t->storeRequest((*reinterpret_cast< Manager*(*)>(_a[1]))); break;
        case 1: _t->loadRequest((*reinterpret_cast< Manager*(*)>(_a[1]))); break;
        case 2: _t->accept(); break;
        case 3: _t->reject(); break;
        case 4: _t->reset(); break;
        case 5: _t->apply(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::Settings::Dialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::Settings::Dialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ScIDE__Settings__Dialog,
      qt_meta_data_ScIDE__Settings__Dialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::Settings::Dialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::Settings::Dialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::Settings::Dialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__Settings__Dialog))
        return static_cast<void*>(const_cast< Dialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ScIDE::Settings::Dialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::Settings::Dialog::storeRequest(Manager * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::Settings::Dialog::loadRequest(Manager * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
