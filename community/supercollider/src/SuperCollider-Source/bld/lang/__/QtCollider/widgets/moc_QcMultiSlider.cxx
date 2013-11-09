/****************************************************************************
** Meta object code from reading C++ file 'QcMultiSlider.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcMultiSlider.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcMultiSlider.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcMultiSlider[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
      22,   39, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   25,   25,   25, 0x05,
      26,   25,   25,   25, 0x05,
      39,   25,   25,   25, 0x05,
      48,   25,   25,   25, 0x05,

 // slots: signature, parameters, type, tag, flags
      61,   25,   25,   25, 0x0a,

 // properties: name, type, flags
      72,   84, 0x02095103,
      88,   95, 0x0009510b,
     111,   95, 0x0009510b,
     121,  127, 0x06095103,
     134,  127, 0x06095103,
     139,   84, 0x02095103,
     145,   84, 0x02095103,
     159,  171, 0x0009510b,
     187,  195, 0x01095103,
     200,   84, 0x02095103,
     215,  230, 0x87095103,
     236,   84, 0x02095103,
     240,  195, 0x01095103,
     250,  195, 0x01095103,
     260,  195, 0x01095103,
     269,  195, 0x01095103,
     279,  195, 0x01095103,
     288,   84, 0x02095103,
     299,  310, 0x43095103,
     317,  310, 0x43095103,
     327,  310, 0x43095103,
     339,  310, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcMultiSlider[] = {
    "QcMultiSlider\0modified()\0\0interacted()\0"
    "action()\0metaAction()\0doAction()\0"
    "sliderCount\0int\0values\0QVector<double>\0"
    "reference\0value\0double\0step\0index\0"
    "selectionSize\0orientation\0Qt::Orientation\0"
    "elastic\0bool\0indexThumbSize\0valueThumbSize\0"
    "float\0gap\0drawLines\0drawRects\0isFilled\0"
    "highlight\0editable\0startIndex\0background\0"
    "QColor\0fillColor\0strokeColor\0focusColor\0"
};

void QcMultiSlider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcMultiSlider *_t = static_cast<QcMultiSlider *>(_o);
        switch (_id) {
        case 0: _t->modified(); break;
        case 1: _t->interacted(); break;
        case 2: _t->action(); break;
        case 3: _t->metaAction(); break;
        case 4: _t->doAction(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcMultiSlider::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcMultiSlider::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcMultiSlider,
      qt_meta_data_QcMultiSlider, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcMultiSlider::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcMultiSlider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcMultiSlider::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcMultiSlider))
        return static_cast<void*>(const_cast< QcMultiSlider*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcMultiSlider*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcMultiSlider*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcMultiSlider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = sliderCount(); break;
        case 1: *reinterpret_cast< QVector<double>*>(_v) = values(); break;
        case 2: *reinterpret_cast< QVector<double>*>(_v) = reference(); break;
        case 3: *reinterpret_cast< double*>(_v) = value(); break;
        case 4: *reinterpret_cast< double*>(_v) = step(); break;
        case 5: *reinterpret_cast< int*>(_v) = index(); break;
        case 6: *reinterpret_cast< int*>(_v) = selectionSize(); break;
        case 7: *reinterpret_cast< Qt::Orientation*>(_v) = orientation(); break;
        case 8: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 9: *reinterpret_cast< int*>(_v) = dummyFloat(); break;
        case 10: *reinterpret_cast< float*>(_v) = dummyFloat(); break;
        case 11: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 12: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 13: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 14: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 15: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 16: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 17: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 18: *reinterpret_cast< QColor*>(_v) = background(); break;
        case 19: *reinterpret_cast< QColor*>(_v) = fillColor(); break;
        case 20: *reinterpret_cast< QColor*>(_v) = strokeColor(); break;
        case 21: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        }
        _id -= 22;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setSliderCount(*reinterpret_cast< int*>(_v)); break;
        case 1: setValues(*reinterpret_cast< QVector<double>*>(_v)); break;
        case 2: setReference(*reinterpret_cast< QVector<double>*>(_v)); break;
        case 3: setValue(*reinterpret_cast< double*>(_v)); break;
        case 4: setStep(*reinterpret_cast< double*>(_v)); break;
        case 5: setIndex(*reinterpret_cast< int*>(_v)); break;
        case 6: setSelectionSize(*reinterpret_cast< int*>(_v)); break;
        case 7: setOrientation(*reinterpret_cast< Qt::Orientation*>(_v)); break;
        case 8: setElastic(*reinterpret_cast< bool*>(_v)); break;
        case 9: setIndexThumbSize(*reinterpret_cast< int*>(_v)); break;
        case 10: setValueThumbSize(*reinterpret_cast< float*>(_v)); break;
        case 11: setGap(*reinterpret_cast< int*>(_v)); break;
        case 12: setDrawLines(*reinterpret_cast< bool*>(_v)); break;
        case 13: setDrawRects(*reinterpret_cast< bool*>(_v)); break;
        case 14: setIsFilled(*reinterpret_cast< bool*>(_v)); break;
        case 15: setHighlight(*reinterpret_cast< bool*>(_v)); break;
        case 16: setEditable(*reinterpret_cast< bool*>(_v)); break;
        case 17: setStartIndex(*reinterpret_cast< int*>(_v)); break;
        case 18: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        case 19: setFillColor(*reinterpret_cast< QColor*>(_v)); break;
        case 20: setStrokeColor(*reinterpret_cast< QColor*>(_v)); break;
        case 21: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
        }
        _id -= 22;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 22;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 22;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 22;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 22;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 22;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 22;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcMultiSlider::modified()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QcMultiSlider::interacted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void QcMultiSlider::action()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void QcMultiSlider::metaAction()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
