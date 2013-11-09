/****************************************************************************
** Meta object code from reading C++ file 'QcKnob.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcKnob.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcKnob.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcKnob[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       8,   19, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
       7,   16,   16,   16, 0x05,

 // properties: name, type, flags
      17,   28, 0x06095103,
      35,   28, 0x06095103,
      45,   28, 0x06095103,
      54,   28, 0x06095103,
      60,   65, 0x02095103,
      69,   28, 0x06095103,
      74,   83, 0x01095103,
      88,   99, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcKnob[] = {
    "QcKnob\0action()\0\0shiftScale\0double\0"
    "ctrlScale\0altScale\0value\0mode\0int\0"
    "step\0centered\0bool\0focusColor\0QColor\0"
};

void QcKnob::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcKnob *_t = static_cast<QcKnob *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcKnob::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcKnob::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcKnob,
      qt_meta_data_QcKnob, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcKnob::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcKnob::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcKnob::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcKnob))
        return static_cast<void*>(const_cast< QcKnob*>(this));
    if (!strcmp(_clname, "QcAbstractStepValue"))
        return static_cast< QcAbstractStepValue*>(const_cast< QcKnob*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcKnob*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcKnob::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        case 0: *reinterpret_cast< double*>(_v) = shiftScale(); break;
        case 1: *reinterpret_cast< double*>(_v) = ctrlScale(); break;
        case 2: *reinterpret_cast< double*>(_v) = altScale(); break;
        case 3: *reinterpret_cast< double*>(_v) = value(); break;
        case 4: *reinterpret_cast< int*>(_v) = mode(); break;
        case 5: *reinterpret_cast< double*>(_v) = step(); break;
        case 6: *reinterpret_cast< bool*>(_v) = centered(); break;
        case 7: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        }
        _id -= 8;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setShiftScale(*reinterpret_cast< double*>(_v)); break;
        case 1: setCtrlScale(*reinterpret_cast< double*>(_v)); break;
        case 2: setAltScale(*reinterpret_cast< double*>(_v)); break;
        case 3: setValue(*reinterpret_cast< double*>(_v)); break;
        case 4: setMode(*reinterpret_cast< int*>(_v)); break;
        case 5: setStep(*reinterpret_cast< double*>(_v)); break;
        case 6: setCentered(*reinterpret_cast< bool*>(_v)); break;
        case 7: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
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

// SIGNAL 0
void QcKnob::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
