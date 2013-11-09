/****************************************************************************
** Meta object code from reading C++ file 'QcButton.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcButton.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcButton.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcButton[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       3,   24, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
       9,   21,   21,   21, 0x05,

 // slots: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x08,

 // properties: name, type, flags
      33,   40, 0x0009510b,
      52,   58, 0x02095103,
      62,   73, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcButton[] = {
    "QcButton\0action(int)\0\0doAction()\0"
    "states\0VariantList\0value\0int\0focusColor\0"
    "QColor\0"
};

void QcButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcButton *_t = static_cast<QcButton *>(_o);
        switch (_id) {
        case 0: _t->action((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->doAction(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcButton::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcButton::staticMetaObject = {
    { &QPushButton::staticMetaObject, qt_meta_stringdata_QcButton,
      qt_meta_data_QcButton, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcButton::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcButton::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcButton))
        return static_cast<void*>(const_cast< QcButton*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcButton*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcButton*>(this));
    return QPushButton::qt_metacast(_clname);
}

int QcButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPushButton::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 1: *reinterpret_cast< int*>(_v) = getValue(); break;
        case 2: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setStates(*reinterpret_cast< VariantList*>(_v)); break;
        case 1: setValue(*reinterpret_cast< int*>(_v)); break;
        case 2: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
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

// SIGNAL 0
void QcButton::action(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
