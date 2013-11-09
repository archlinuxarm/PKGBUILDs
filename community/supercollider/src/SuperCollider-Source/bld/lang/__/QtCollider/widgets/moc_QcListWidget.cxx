/****************************************************************************
** Meta object code from reading C++ file 'QcListWidget.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcListWidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcListWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcListWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       4,   29, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   22,   22,   22, 0x05,
      23,   22,   22,   22, 0x05,

 // slots: signature, parameters, type, tag, flags
      39,   22,   22,   22, 0x08,

 // properties: name, type, flags
      62,   68, 0x0009510b,
      80,   68, 0x0009510b,
      87,   98, 0x02095003,
     102,   68, 0x0009510b,

       0        // eod
};

static const char qt_meta_stringdata_QcListWidget[] = {
    "QcListWidget\0action()\0\0returnPressed()\0"
    "onCurrentItemChanged()\0items\0VariantList\0"
    "colors\0currentRow\0int\0selection\0"
};

void QcListWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcListWidget *_t = static_cast<QcListWidget *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->returnPressed(); break;
        case 2: _t->onCurrentItemChanged(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcListWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcListWidget::staticMetaObject = {
    { &QListWidget::staticMetaObject, qt_meta_stringdata_QcListWidget,
      qt_meta_data_QcListWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcListWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcListWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcListWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcListWidget))
        return static_cast<void*>(const_cast< QcListWidget*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcListWidget*>(this));
    return QListWidget::qt_metacast(_clname);
}

int QcListWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QListWidget::qt_metacall(_c, _id, _a);
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
        case 0: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 1: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 2: *reinterpret_cast< int*>(_v) = currentRow(); break;
        case 3: *reinterpret_cast< VariantList*>(_v) = selection(); break;
        }
        _id -= 4;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setItems(*reinterpret_cast< VariantList*>(_v)); break;
        case 1: setColors(*reinterpret_cast< VariantList*>(_v)); break;
        case 2: setCurrentRowWithoutAction(*reinterpret_cast< int*>(_v)); break;
        case 3: setSelection(*reinterpret_cast< VariantList*>(_v)); break;
        }
        _id -= 4;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 4;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcListWidget::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QcListWidget::returnPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
