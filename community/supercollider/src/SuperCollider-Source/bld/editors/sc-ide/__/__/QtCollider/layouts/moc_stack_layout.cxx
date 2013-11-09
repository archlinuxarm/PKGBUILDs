/****************************************************************************
** Meta object code from reading C++ file 'stack_layout.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../QtCollider/layouts/stack_layout.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'stack_layout.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QtCollider__StackLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       3,   14, // properties
       1,   23, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
      24,   37, 0x02095103,
      41,   54, 0x0009510b,
      67,   37, 0x02095001,

 // enums: name, flags, count, data
      54, 0x0,    2,   27,

 // enum data: key, value
      73, uint(QtCollider::StackLayout::StackOne),
      82, uint(QtCollider::StackLayout::StackAll),

       0        // eod
};

static const char qt_meta_stringdata_QtCollider__StackLayout[] = {
    "QtCollider::StackLayout\0currentIndex\0"
    "int\0stackingMode\0StackingMode\0count\0"
    "StackOne\0StackAll\0"
};

void QtCollider::StackLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QtCollider::StackLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QtCollider::StackLayout::staticMetaObject = {
    { &QLayout::staticMetaObject, qt_meta_stringdata_QtCollider__StackLayout,
      qt_meta_data_QtCollider__StackLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QtCollider::StackLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QtCollider::StackLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QtCollider::StackLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtCollider__StackLayout))
        return static_cast<void*>(const_cast< StackLayout*>(this));
    return QLayout::qt_metacast(_clname);
}

int QtCollider::StackLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLayout::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = currentIndex(); break;
        case 1: *reinterpret_cast< StackingMode*>(_v) = stackingMode(); break;
        case 2: *reinterpret_cast< int*>(_v) = count(); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setCurrentIndex(*reinterpret_cast< int*>(_v)); break;
        case 1: setStackingMode(*reinterpret_cast< StackingMode*>(_v)); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
