/****************************************************************************
** Meta object code from reading C++ file 'QcScopeShm.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcScopeShm.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcScopeShm.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcScopeShm[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
      11,   29, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      11,   19,   19,   19, 0x0a,
      20,   19,   19,   19, 0x0a,
      27,   19,   19,   19, 0x08,

 // properties: name, type, flags
      41,   52, 0x02095103,
      56,   52, 0x02095103,
      69,   77, 0x87095103,
      83,   77, 0x87095103,
      91,   77, 0x87095103,
      97,   77, 0x87095103,
     103,   52, 0x02095103,
     109,  120, 0x0009510b,
     132,  143, 0x43095103,
     150,   52, 0x02095103,
     165,  173, 0x01095001,

       0        // eod
};

static const char qt_meta_stringdata_QcScopeShm[] = {
    "QcScopeShm\0start()\0\0stop()\0updateScope()\0"
    "serverPort\0int\0bufferNumber\0xOffset\0"
    "float\0yOffset\0xZoom\0yZoom\0style\0"
    "waveColors\0VariantList\0background\0"
    "QColor\0updateInterval\0running\0bool\0"
};

void QcScopeShm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcScopeShm *_t = static_cast<QcScopeShm *>(_o);
        switch (_id) {
        case 0: _t->start(); break;
        case 1: _t->stop(); break;
        case 2: _t->updateScope(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcScopeShm::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcScopeShm::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcScopeShm,
      qt_meta_data_QcScopeShm, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcScopeShm::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcScopeShm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcScopeShm::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcScopeShm))
        return static_cast<void*>(const_cast< QcScopeShm*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcScopeShm*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcScopeShm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = serverPort(); break;
        case 1: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 2: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 3: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 4: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 5: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 6: *reinterpret_cast< int*>(_v) = style(); break;
        case 7: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 8: *reinterpret_cast< QColor*>(_v) = background(); break;
        case 9: *reinterpret_cast< int*>(_v) = updateInterval(); break;
        case 10: *reinterpret_cast< bool*>(_v) = running(); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setServerPort(*reinterpret_cast< int*>(_v)); break;
        case 1: setBufferNumber(*reinterpret_cast< int*>(_v)); break;
        case 2: setXOffset(*reinterpret_cast< float*>(_v)); break;
        case 3: setYOffset(*reinterpret_cast< float*>(_v)); break;
        case 4: setXZoom(*reinterpret_cast< float*>(_v)); break;
        case 5: setYZoom(*reinterpret_cast< float*>(_v)); break;
        case 6: setStyle(*reinterpret_cast< int*>(_v)); break;
        case 7: setWaveColors(*reinterpret_cast< VariantList*>(_v)); break;
        case 8: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        case 9: setUpdateInterval(*reinterpret_cast< int*>(_v)); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 11;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
