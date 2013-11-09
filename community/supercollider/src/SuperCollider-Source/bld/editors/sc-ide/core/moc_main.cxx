/****************************************************************************
** Meta object code from reading C++ file 'main.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/core/main.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__SingleInstanceGuard[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      27,   48,   48,   48, 0x0a,
      49,   48,   48,   48, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SingleInstanceGuard[] = {
    "ScIDE::SingleInstanceGuard\0"
    "onNewIpcConnection()\0\0onIpcData()\0"
};

void ScIDE::SingleInstanceGuard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SingleInstanceGuard *_t = static_cast<SingleInstanceGuard *>(_o);
        switch (_id) {
        case 0: _t->onNewIpcConnection(); break;
        case 1: _t->onIpcData(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::SingleInstanceGuard::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SingleInstanceGuard::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__SingleInstanceGuard,
      qt_meta_data_ScIDE__SingleInstanceGuard, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SingleInstanceGuard::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SingleInstanceGuard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SingleInstanceGuard::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SingleInstanceGuard))
        return static_cast<void*>(const_cast< SingleInstanceGuard*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::SingleInstanceGuard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
static const uint qt_meta_data_ScIDE__Main[] = {

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
      12,   53,   53,   53, 0x05,
      54,   53,   53,   53, 0x05,

 // slots: signature, parameters, type, tag, flags
      95,   53,   53,   53, 0x0a,
     111,   53,   53,   53, 0x0a,
     127,   53,   53,   53, 0x0a,
     134,  168,   53,   53, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__Main[] = {
    "ScIDE::Main\0storeSettingsRequest(Settings::Manager*)\0"
    "\0applySettingsRequest(Settings::Manager*)\0"
    "storeSettings()\0applySettings()\0quit()\0"
    "onScLangResponse(QString,QString)\0,\0"
};

void ScIDE::Main::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Main *_t = static_cast<Main *>(_o);
        switch (_id) {
        case 0: _t->storeSettingsRequest((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 1: _t->applySettingsRequest((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 2: _t->storeSettings(); break;
        case 3: _t->applySettings(); break;
        case 4: _t->quit(); break;
        case 5: _t->onScLangResponse((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::Main::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::Main::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__Main,
      qt_meta_data_ScIDE__Main, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::Main::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::Main::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::Main::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__Main))
        return static_cast<void*>(const_cast< Main*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::Main::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void ScIDE::Main::storeSettingsRequest(Settings::Manager * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::Main::applySettingsRequest(Settings::Manager * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
