/****************************************************************************
** Meta object code from reading C++ file 'QcNumberBox.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcNumberBox.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcNumberBox.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcNumberBox[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
      16,   64, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   26,   32,   32, 0x05,
      33,   32,   32,   32, 0x05,
      48,   32,   32,   32, 0x05,

 // slots: signature, parameters, type, tag, flags
      57,   75,   32,   32, 0x0a,
      82,   75,   32,   32, 0x0a,
     100,   32,   32,   32, 0x08,
     120,   32,   32,   32, 0x08,

 // methods: signature, parameters, type, tag, flags
     133,  151,   32,   32, 0x02,
     160,   32,   32,   32, 0x22,
     174,   32,   32,   32, 0x02,

 // properties: name, type, flags
     183,  194, 0x06095103,
     201,  194, 0x06095103,
     211,  194, 0x06095103,
     220,  194, 0x06095103,
     228,  194, 0x06095103,
     236,  245, 0x02095103,
     249,  245, 0x02095103,
     261,  245, 0x02095103,
     273,  194, 0x06095103,
     279,  245, 0x02095001,
     289,  294, 0x0a095003,
     302,  194, 0x06095103,
     307,  194, 0x06095103,
     318,  325, 0x01095103,
     330,  342, 0x43095003,
     349,  342, 0x43095003,

       0        // eod
};

static const char qt_meta_stringdata_QcNumberBox[] = {
    "QcNumberBox\0scrolled(int)\0steps\0\0"
    "valueChanged()\0action()\0increment(double)\0"
    "factor\0decrement(double)\0onEditingFinished()\0"
    "updateText()\0setInfinite(bool)\0positive\0"
    "setInfinite()\0setNaN()\0shiftScale\0"
    "double\0ctrlScale\0altScale\0minimum\0"
    "maximum\0decimals\0int\0maxDecimals\0"
    "minDecimals\0value\0valueType\0text\0"
    "QString\0step\0scrollStep\0scroll\0bool\0"
    "normalColor\0QColor\0editingColor\0"
};

void QcNumberBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcNumberBox *_t = static_cast<QcNumberBox *>(_o);
        switch (_id) {
        case 0: _t->scrolled((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->valueChanged(); break;
        case 2: _t->action(); break;
        case 3: _t->increment((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 4: _t->decrement((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->onEditingFinished(); break;
        case 6: _t->updateText(); break;
        case 7: _t->setInfinite((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->setInfinite(); break;
        case 9: _t->setNaN(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcNumberBox::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcNumberBox::staticMetaObject = {
    { &QLineEdit::staticMetaObject, qt_meta_stringdata_QcNumberBox,
      qt_meta_data_QcNumberBox, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcNumberBox::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcNumberBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcNumberBox::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcNumberBox))
        return static_cast<void*>(const_cast< QcNumberBox*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcNumberBox*>(this));
    if (!strcmp(_clname, "QcAbstractStepValue"))
        return static_cast< QcAbstractStepValue*>(const_cast< QcNumberBox*>(this));
    return QLineEdit::qt_metacast(_clname);
}

int QcNumberBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
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
        case 0: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 1: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 2: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 3: *reinterpret_cast< double*>(_v) = minimum(); break;
        case 4: *reinterpret_cast< double*>(_v) = maximum(); break;
        case 5: *reinterpret_cast< int*>(_v) = decimals(); break;
        case 6: *reinterpret_cast< int*>(_v) = maxDecimals(); break;
        case 7: *reinterpret_cast< int*>(_v) = minDecimals(); break;
        case 8: *reinterpret_cast< double*>(_v) = value(); break;
        case 9: *reinterpret_cast< int*>(_v) = valueType(); break;
        case 10: *reinterpret_cast< QString*>(_v) = text(); break;
        case 11: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 12: *reinterpret_cast< double*>(_v) = dummyFloat(); break;
        case 13: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 14: *reinterpret_cast< QColor*>(_v) = dummyColor(); break;
        case 15: *reinterpret_cast< QColor*>(_v) = dummyColor(); break;
        }
        _id -= 16;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setShiftScale(*reinterpret_cast< double*>(_v)); break;
        case 1: setCtrlScale(*reinterpret_cast< double*>(_v)); break;
        case 2: setAltScale(*reinterpret_cast< double*>(_v)); break;
        case 3: setMinimum(*reinterpret_cast< double*>(_v)); break;
        case 4: setMaximum(*reinterpret_cast< double*>(_v)); break;
        case 5: setDecimals(*reinterpret_cast< int*>(_v)); break;
        case 6: setMaxDecimals(*reinterpret_cast< int*>(_v)); break;
        case 7: setMinDecimals(*reinterpret_cast< int*>(_v)); break;
        case 8: setValue(*reinterpret_cast< double*>(_v)); break;
        case 10: setTextValue(*reinterpret_cast< QString*>(_v)); break;
        case 11: setStep(*reinterpret_cast< double*>(_v)); break;
        case 12: setScrollStep(*reinterpret_cast< double*>(_v)); break;
        case 13: setScroll(*reinterpret_cast< bool*>(_v)); break;
        case 14: setTextColor(*reinterpret_cast< QColor*>(_v)); break;
        case 15: setEditedTextColor(*reinterpret_cast< QColor*>(_v)); break;
        }
        _id -= 16;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 16;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 16;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 16;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 16;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 16;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 16;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcNumberBox::scrolled(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QcNumberBox::valueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void QcNumberBox::action()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
