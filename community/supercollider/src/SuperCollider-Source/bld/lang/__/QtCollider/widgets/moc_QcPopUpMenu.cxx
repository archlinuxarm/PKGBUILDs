/****************************************************************************
** Meta object code from reading C++ file 'QcPopUpMenu.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcPopUpMenu.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcPopUpMenu.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcPopUpMenu[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       2,   34, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   21,   21,   21, 0x05,

 // slots: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x08,
      36,   21,   21,   21, 0x08,
      49,   21,   21,   21, 0x08,

 // properties: name, type, flags
      64,   70, 0x0009510b,
      82,  102, 0x01095103,

       0        // eod
};

static const char qt_meta_stringdata_QcPopUpMenu[] = {
    "QcPopUpMenu\0action()\0\0doAction(int)\0"
    "setChanged()\0clearChanged()\0items\0"
    "VariantList\0reactivationEnabled\0bool\0"
};

void QcPopUpMenu::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcPopUpMenu *_t = static_cast<QcPopUpMenu *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->doAction((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->setChanged(); break;
        case 3: _t->clearChanged(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcPopUpMenu::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcPopUpMenu::staticMetaObject = {
    { &QComboBox::staticMetaObject, qt_meta_stringdata_QcPopUpMenu,
      qt_meta_data_QcPopUpMenu, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcPopUpMenu::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcPopUpMenu::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcPopUpMenu::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcPopUpMenu))
        return static_cast<void*>(const_cast< QcPopUpMenu*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcPopUpMenu*>(this));
    return QComboBox::qt_metacast(_clname);
}

int QcPopUpMenu::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QComboBox::qt_metacall(_c, _id, _a);
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
        case 0: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 1: *reinterpret_cast< bool*>(_v) = reactivationEnabled(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setItems(*reinterpret_cast< VariantList*>(_v)); break;
        case 1: setReactivationEnabled(*reinterpret_cast< bool*>(_v)); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcPopUpMenu::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
