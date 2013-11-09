/****************************************************************************
** Meta object code from reading C++ file 'QObjectProxy.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../QtCollider/QObjectProxy.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QObjectProxy.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QtCollider__ProxyToken[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_QtCollider__ProxyToken[] = {
    "QtCollider::ProxyToken\0"
};

void QtCollider::ProxyToken::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QtCollider::ProxyToken::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QtCollider::ProxyToken::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QtCollider__ProxyToken,
      qt_meta_data_QtCollider__ProxyToken, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QtCollider::ProxyToken::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QtCollider::ProxyToken::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QtCollider::ProxyToken::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtCollider__ProxyToken))
        return static_cast<void*>(const_cast< ProxyToken*>(this));
    return QObject::qt_metacast(_clname);
}

int QtCollider::ProxyToken::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QObjectProxy[] = {

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
      13,   26,   26,   26, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_QObjectProxy[] = {
    "QObjectProxy\0invalidate()\0\0"
};

void QObjectProxy::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QObjectProxy *_t = static_cast<QObjectProxy *>(_o);
        switch (_id) {
        case 0: _t->invalidate(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QObjectProxy::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QObjectProxy::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QObjectProxy,
      qt_meta_data_QObjectProxy, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QObjectProxy::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QObjectProxy::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QObjectProxy::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QObjectProxy))
        return static_cast<void*>(const_cast< QObjectProxy*>(this));
    return QObject::qt_metacast(_clname);
}

int QObjectProxy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
