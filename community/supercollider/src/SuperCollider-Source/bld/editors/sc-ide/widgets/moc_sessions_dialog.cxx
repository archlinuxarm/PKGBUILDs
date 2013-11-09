/****************************************************************************
** Meta object code from reading C++ file 'sessions_dialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/sessions_dialog.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sessions_dialog.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__SessionsDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      22,   38,   38,   38, 0x08,
      39,   38,   38,   38, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SessionsDialog[] = {
    "ScIDE::SessionsDialog\0removeCurrent()\0"
    "\0renameCurrent()\0"
};

void ScIDE::SessionsDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SessionsDialog *_t = static_cast<SessionsDialog *>(_o);
        switch (_id) {
        case 0: _t->removeCurrent(); break;
        case 1: _t->renameCurrent(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::SessionsDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SessionsDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ScIDE__SessionsDialog,
      qt_meta_data_ScIDE__SessionsDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SessionsDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SessionsDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SessionsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SessionsDialog))
        return static_cast<void*>(const_cast< SessionsDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ScIDE::SessionsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
