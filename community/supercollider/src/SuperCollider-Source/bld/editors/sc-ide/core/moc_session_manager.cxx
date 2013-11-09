/****************************************************************************
** Meta object code from reading C++ file 'session_manager.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/core/session_manager.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'session_manager.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__SessionManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   51,   59,   59, 0x05,
      60,   51,   59,   59, 0x05,
      91,   59,   59,   59, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SessionManager[] = {
    "ScIDE::SessionManager\0"
    "saveSessionRequest(Session*)\0session\0"
    "\0switchSessionRequest(Session*)\0"
    "currentSessionNameChanged()\0"
};

void ScIDE::SessionManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SessionManager *_t = static_cast<SessionManager *>(_o);
        switch (_id) {
        case 0: _t->saveSessionRequest((*reinterpret_cast< Session*(*)>(_a[1]))); break;
        case 1: _t->switchSessionRequest((*reinterpret_cast< Session*(*)>(_a[1]))); break;
        case 2: _t->currentSessionNameChanged(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::SessionManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SessionManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__SessionManager,
      qt_meta_data_ScIDE__SessionManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SessionManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SessionManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SessionManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SessionManager))
        return static_cast<void*>(const_cast< SessionManager*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::SessionManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::SessionManager::saveSessionRequest(Session * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::SessionManager::switchSessionRequest(Session * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ScIDE::SessionManager::currentSessionNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
