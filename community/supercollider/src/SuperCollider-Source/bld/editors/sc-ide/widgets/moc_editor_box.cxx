/****************************************************************************
** Meta object code from reading C++ file 'editor_box.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/editor_box.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'editor_box.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__CodeEditorBox[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   56,   56,   56, 0x05,
      57,   83,   56,   56, 0x05,

 // slots: signature, parameters, type, tag, flags
      86,   56,   56,   56, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__CodeEditorBox[] = {
    "ScIDE::CodeEditorBox\0"
    "currentChanged(GenericCodeEditor*)\0\0"
    "activated(CodeEditorBox*)\0me\0"
    "onDocumentClosed(Document*)\0"
};

void ScIDE::CodeEditorBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CodeEditorBox *_t = static_cast<CodeEditorBox *>(_o);
        switch (_id) {
        case 0: _t->currentChanged((*reinterpret_cast< GenericCodeEditor*(*)>(_a[1]))); break;
        case 1: _t->activated((*reinterpret_cast< CodeEditorBox*(*)>(_a[1]))); break;
        case 2: _t->onDocumentClosed((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::CodeEditorBox::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::CodeEditorBox::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__CodeEditorBox,
      qt_meta_data_ScIDE__CodeEditorBox, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::CodeEditorBox::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::CodeEditorBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::CodeEditorBox::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__CodeEditorBox))
        return static_cast<void*>(const_cast< CodeEditorBox*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::CodeEditorBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void ScIDE::CodeEditorBox::currentChanged(GenericCodeEditor * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::CodeEditorBox::activated(CodeEditorBox * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
