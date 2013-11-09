/****************************************************************************
** Meta object code from reading C++ file 'QcSlider2D.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcSlider2D.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcSlider2D.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcSlider2D[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       9,   64, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,   20,   20,   20, 0x05,
      21,   20,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
      33,   52,   20,   20, 0x0a,
      59,   20,   20,   20, 0x2a,
      72,   52,   20,   20, 0x0a,
      91,   20,   20,   20, 0x2a,
     104,   52,   20,   20, 0x0a,
     123,   20,   20,   20, 0x2a,
     136,   52,   20,   20, 0x0a,
     155,   20,   20,   20, 0x2a,

 // properties: name, type, flags
     168,  175, 0x06095103,
     182,  175, 0x06095103,
     189,  175, 0x06095103,
     200,  175, 0x06095103,
     210,  175, 0x06095103,
     219,  175, 0x06095103,
     224,  236, 0x43095103,
     243,  236, 0x43095103,
     254,  236, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcSlider2D[] = {
    "QcSlider2D\0action()\0\0randomize()\0"
    "incrementX(double)\0factor\0incrementX()\0"
    "decrementX(double)\0decrementX()\0"
    "incrementY(double)\0incrementY()\0"
    "decrementY(double)\0decrementY()\0xValue\0"
    "double\0yValue\0shiftScale\0ctrlScale\0"
    "altScale\0step\0grooveColor\0QColor\0"
    "focusColor\0knobColor\0"
};

void QcSlider2D::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcSlider2D *_t = static_cast<QcSlider2D *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->randomize(); break;
        case 2: _t->incrementX((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 3: _t->incrementX(); break;
        case 4: _t->decrementX((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->decrementX(); break;
        case 6: _t->incrementY((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 7: _t->incrementY(); break;
        case 8: _t->decrementY((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 9: _t->decrementY(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcSlider2D::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcSlider2D::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcSlider2D,
      qt_meta_data_QcSlider2D, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcSlider2D::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcSlider2D::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcSlider2D::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcSlider2D))
        return static_cast<void*>(const_cast< QcSlider2D*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcSlider2D*>(this));
    if (!strcmp(_clname, "QcAbstractStepValue"))
        return static_cast< QcAbstractStepValue*>(const_cast< QcSlider2D*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcSlider2D*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcSlider2D::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< double*>(_v) = xValue(); break;
        case 1: *reinterpret_cast< double*>(_v) = yValue(); break;
        case 2: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 3: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 4: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 5: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 6: *reinterpret_cast< QColor*>(_v) = grooveColor(); break;
        case 7: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        case 8: *reinterpret_cast< QColor*>(_v) = knobColor(); break;
        }
        _id -= 9;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setXValue(*reinterpret_cast< double*>(_v)); break;
        case 1: setYValue(*reinterpret_cast< double*>(_v)); break;
        case 2: setShiftScale(*reinterpret_cast< double*>(_v)); break;
        case 3: setCtrlScale(*reinterpret_cast< double*>(_v)); break;
        case 4: setAltScale(*reinterpret_cast< double*>(_v)); break;
        case 5: setStep(*reinterpret_cast< double*>(_v)); break;
        case 6: setGrooveColor(*reinterpret_cast< QColor*>(_v)); break;
        case 7: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
        case 8: setKnobColor(*reinterpret_cast< QColor*>(_v)); break;
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

// SIGNAL 0
void QcSlider2D::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QcSlider2D::randomize()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
