/****************************************************************************
** Meta object code from reading C++ file 'session_switch_dialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/session_switch_dialog.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'session_switch_dialog.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__SessionSwitchDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      27,   61,   61,   61, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SessionSwitchDialog[] = {
    "ScIDE::SessionSwitchDialog\0"
    "onItemActivated(QListWidgetItem*)\0\0"
};

void ScIDE::SessionSwitchDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SessionSwitchDialog *_t = static_cast<SessionSwitchDialog *>(_o);
        switch (_id) {
        case 0: _t->onItemActivated((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::SessionSwitchDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SessionSwitchDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ScIDE__SessionSwitchDialog,
      qt_meta_data_ScIDE__SessionSwitchDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SessionSwitchDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SessionSwitchDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SessionSwitchDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SessionSwitchDialog))
        return static_cast<void*>(const_cast< SessionSwitchDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ScIDE::SessionSwitchDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
