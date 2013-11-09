/****************************************************************************
** Meta object code from reading C++ file 'QcSlider.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcSlider.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcSlider.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcSlider[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
      11,   34, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
       9,   18,   18,   18, 0x05,
      19,   18,   18,   18, 0x05,

 // slots: signature, parameters, type, tag, flags
      37,   55,   18,   18, 0x0a,
      62,   55,   18,   18, 0x0a,

 // properties: name, type, flags
      80,   91, 0x06095103,
      98,   91, 0x06095103,
     108,   91, 0x06095103,
     117,   91, 0x06095103,
     122,   91, 0x06095001,
     132,   91, 0x06095103,
     138,  150, 0x02095103,
     154,  150, 0x02095103,
     167,  179, 0x43095103,
     186,  179, 0x43095103,
     197,  179, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcSlider[] = {
    "QcSlider\0action()\0\0preAction(double)\0"
    "increment(double)\0factor\0decrement(double)\0"
    "shiftScale\0double\0ctrlScale\0altScale\0"
    "step\0pixelStep\0value\0orientation\0int\0"
    "handleLength\0grooveColor\0QColor\0"
    "focusColor\0knobColor\0"
};

void QcSlider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcSlider *_t = static_cast<QcSlider *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->preAction((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 2: _t->increment((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 3: _t->decrement((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcSlider::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcSlider::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcSlider,
      qt_meta_data_QcSlider, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcSlider::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcSlider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcSlider::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcSlider))
        return static_cast<void*>(const_cast< QcSlider*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcSlider*>(this));
    if (!strcmp(_clname, "QcAbstractStepValue"))
        return static_cast< QcAbstractStepValue*>(const_cast< QcSlider*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcSlider*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcSlider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 1: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 2: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 3: *reinterpret_cast< double*>(_v) = step(); break;
        case 4: *reinterpret_cast< double*>(_v) = pixelStep(); break;
        case 5: *reinterpret_cast< double*>(_v) = value(); break;
        case 6: *reinterpret_cast< int*>(_v) = orientation(); break;
        case 7: *reinterpret_cast< int*>(_v) = handleLength(); break;
        case 8: *reinterpret_cast< QColor*>(_v) = grooveColor(); break;
        case 9: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        case 10: *reinterpret_cast< QColor*>(_v) = knobColor(); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setShiftScale(*reinterpret_cast< double*>(_v)); break;
        case 1: setCtrlScale(*reinterpret_cast< double*>(_v)); break;
        case 2: setAltScale(*reinterpret_cast< double*>(_v)); break;
        case 3: setStep(*reinterpret_cast< double*>(_v)); break;
        case 5: setValue(*reinterpret_cast< double*>(_v)); break;
        case 6: setOrientation(*reinterpret_cast< int*>(_v)); break;
        case 7: setHandleLength(*reinterpret_cast< int*>(_v)); break;
        case 8: setGrooveColor(*reinterpret_cast< QColor*>(_v)); break;
        case 9: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
        case 10: setKnobColor(*reinterpret_cast< QColor*>(_v)); break;
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

// SIGNAL 0
void QcSlider::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QcSlider::preAction(double _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
