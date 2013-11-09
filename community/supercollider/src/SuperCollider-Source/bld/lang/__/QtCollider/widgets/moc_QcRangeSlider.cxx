/****************************************************************************
** Meta object code from reading C++ file 'QcRangeSlider.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcRangeSlider.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcRangeSlider.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcRangeSlider[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
      10,   29, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   23,   23,   23, 0x05,

 // slots: signature, parameters, type, tag, flags
      24,   42,   23,   23, 0x0a,
      49,   42,   23,   23, 0x0a,

 // properties: name, type, flags
      67,   79, 0x0009510b,
      95,  103, 0x06095103,
     110,  103, 0x06095103,
     118,  103, 0x06095103,
     129,  103, 0x06095103,
     139,  103, 0x06095103,
     148,  103, 0x06095103,
     153,  165, 0x43095103,
     172,  165, 0x43095103,
     183,  165, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcRangeSlider[] = {
    "QcRangeSlider\0action()\0\0increment(double)\0"
    "factor\0decrement(double)\0orientation\0"
    "Qt::Orientation\0loValue\0double\0hiValue\0"
    "shiftScale\0ctrlScale\0altScale\0step\0"
    "grooveColor\0QColor\0focusColor\0knobColor\0"
};

void QcRangeSlider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcRangeSlider *_t = static_cast<QcRangeSlider *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->increment((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 2: _t->decrement((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcRangeSlider::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcRangeSlider::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcRangeSlider,
      qt_meta_data_QcRangeSlider, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcRangeSlider::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcRangeSlider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcRangeSlider::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcRangeSlider))
        return static_cast<void*>(const_cast< QcRangeSlider*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcRangeSlider*>(this));
    if (!strcmp(_clname, "QcAbstractStepValue"))
        return static_cast< QcAbstractStepValue*>(const_cast< QcRangeSlider*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcRangeSlider*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcRangeSlider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        case 0: *reinterpret_cast< Qt::Orientation*>(_v) = orientation(); break;
        case 1: *reinterpret_cast< double*>(_v) = loValue(); break;
        case 2: *reinterpret_cast< double*>(_v) = hiValue(); break;
        case 3: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 4: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 5: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 6: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 7: *reinterpret_cast< QColor*>(_v) = grooveColor(); break;
        case 8: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        case 9: *reinterpret_cast< QColor*>(_v) = knobColor(); break;
        }
        _id -= 10;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setOrientation(*reinterpret_cast< Qt::Orientation*>(_v)); break;
        case 1: setLoValue(*reinterpret_cast< double*>(_v)); break;
        case 2: setHiValue(*reinterpret_cast< double*>(_v)); break;
        case 3: setShiftScale(*reinterpret_cast< double*>(_v)); break;
        case 4: setCtrlScale(*reinterpret_cast< double*>(_v)); break;
        case 5: setAltScale(*reinterpret_cast< double*>(_v)); break;
        case 6: setStep(*reinterpret_cast< double*>(_v)); break;
        case 7: setGrooveColor(*reinterpret_cast< QColor*>(_v)); break;
        case 8: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
        case 9: setKnobColor(*reinterpret_cast< QColor*>(_v)); break;
        }
        _id -= 10;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 10;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 10;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 10;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 10;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 10;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 10;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcRangeSlider::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
