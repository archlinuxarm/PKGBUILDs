/****************************************************************************
** Meta object code from reading C++ file 'key_sequence_edit.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/util/key_sequence_edit.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'key_sequence_edit.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__KeySequenceEdit[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   40,   40,   40, 0x05,

 // slots: signature, parameters, type, tag, flags
      41,   40,   40,   40, 0x0a,
      49,   40,   40,   40, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__KeySequenceEdit[] = {
    "ScIDE::KeySequenceEdit\0editingStarted()\0"
    "\0reset()\0finishEditing()\0"
};

void ScIDE::KeySequenceEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        KeySequenceEdit *_t = static_cast<KeySequenceEdit *>(_o);
        switch (_id) {
        case 0: _t->editingStarted(); break;
        case 1: _t->reset(); break;
        case 2: _t->finishEditing(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::KeySequenceEdit::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::KeySequenceEdit::staticMetaObject = {
    { &QLineEdit::staticMetaObject, qt_meta_stringdata_ScIDE__KeySequenceEdit,
      qt_meta_data_ScIDE__KeySequenceEdit, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::KeySequenceEdit::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::KeySequenceEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::KeySequenceEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__KeySequenceEdit))
        return static_cast<void*>(const_cast< KeySequenceEdit*>(this));
    return QLineEdit::qt_metacast(_clname);
}

int ScIDE::KeySequenceEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::KeySequenceEdit::editingStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
