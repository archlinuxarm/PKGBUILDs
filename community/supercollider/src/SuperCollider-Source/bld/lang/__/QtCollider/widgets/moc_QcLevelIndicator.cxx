/****************************************************************************
** Meta object code from reading C++ file 'QcLevelIndicator.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcLevelIndicator.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcLevelIndicator.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcLevelIndicator[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       8,   19, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      17,   31,   31,   31, 0x08,

 // properties: name, type, flags
      32,   38, 0x87095103,
      44,   38, 0x87095103,
      52,   38, 0x87095103,
      61,   38, 0x87095103,
      66,   75, 0x01095103,
      80,   86, 0x02095103,
      90,   86, 0x02095103,
     101,  113, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcLevelIndicator[] = {
    "QcLevelIndicator\0clipTimeout()\0\0value\0"
    "float\0warning\0critical\0peak\0drawPeak\0"
    "bool\0ticks\0int\0majorTicks\0grooveColor\0"
    "QColor\0"
};

void QcLevelIndicator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcLevelIndicator *_t = static_cast<QcLevelIndicator *>(_o);
        switch (_id) {
        case 0: _t->clipTimeout(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcLevelIndicator::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcLevelIndicator::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcLevelIndicator,
      qt_meta_data_QcLevelIndicator, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcLevelIndicator::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcLevelIndicator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcLevelIndicator::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcLevelIndicator))
        return static_cast<void*>(const_cast< QcLevelIndicator*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcLevelIndicator*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcLevelIndicator*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcLevelIndicator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        case 0: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 1: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 2: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 3: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 4: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 5: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 6: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 7: *reinterpret_cast< QColor*>(_v) = grooveColor(); break;
        }
        _id -= 8;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setValue(*reinterpret_cast< float*>(_v)); break;
        case 1: setWarning(*reinterpret_cast< float*>(_v)); break;
        case 2: setCritical(*reinterpret_cast< float*>(_v)); break;
        case 3: setPeak(*reinterpret_cast< float*>(_v)); break;
        case 4: setDrawPeak(*reinterpret_cast< bool*>(_v)); break;
        case 5: setTicks(*reinterpret_cast< int*>(_v)); break;
        case 6: setMajorTicks(*reinterpret_cast< int*>(_v)); break;
        case 7: setGrooveColor(*reinterpret_cast< QColor*>(_v)); break;
        }
        _id -= 8;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 8;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 8;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
