/****************************************************************************
** Meta object code from reading C++ file 'QcScope.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcScope.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcScope.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcScope[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       9,   19, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       8,   22,   22,   22, 0x08,

 // properties: name, type, flags
      23,   36, 0x02095103,
      40,   48, 0x87095103,
      54,   48, 0x87095103,
      62,   48, 0x87095103,
      68,   48, 0x87095103,
      74,   36, 0x02095103,
      80,   91, 0x0009510b,
     103,  114, 0x43095103,
     121,   36, 0x02095103,

       0        // eod
};

static const char qt_meta_stringdata_QcScope[] = {
    "QcScope\0updateScope()\0\0bufferNumber\0"
    "int\0xOffset\0float\0yOffset\0xZoom\0yZoom\0"
    "style\0waveColors\0VariantList\0background\0"
    "QColor\0updateInterval\0"
};

void QcScope::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcScope *_t = static_cast<QcScope *>(_o);
        switch (_id) {
        case 0: _t->updateScope(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcScope::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcScope::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcScope,
      qt_meta_data_QcScope, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcScope::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcScope::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcScope::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcScope))
        return static_cast<void*>(const_cast< QcScope*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcScope*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcScope::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 1: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 2: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 3: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 4: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 5: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 6: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 7: *reinterpret_cast< QColor*>(_v) = background(); break;
        case 8: *reinterpret_cast< int*>(_v) = updateInterval(); break;
        }
        _id -= 9;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setBufferNumber(*reinterpret_cast< int*>(_v)); break;
        case 1: setXOffset(*reinterpret_cast< float*>(_v)); break;
        case 2: setYOffset(*reinterpret_cast< float*>(_v)); break;
        case 3: setXZoom(*reinterpret_cast< float*>(_v)); break;
        case 4: setYZoom(*reinterpret_cast< float*>(_v)); break;
        case 5: setStyle(*reinterpret_cast< int*>(_v)); break;
        case 6: setWaveColors(*reinterpret_cast< VariantList*>(_v)); break;
        case 7: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        case 8: setUpdateInterval(*reinterpret_cast< int*>(_v)); break;
        }
        _id -= 9;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 9;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 9;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 9;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 9;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 9;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 9;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
