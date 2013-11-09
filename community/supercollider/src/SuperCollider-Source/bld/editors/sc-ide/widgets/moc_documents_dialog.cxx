/****************************************************************************
** Meta object code from reading C++ file 'documents_dialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/documents_dialog.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'documents_dialog.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__DocumentsDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      23,   35,   35,   35, 0x0a,
      36,   35,   35,   35, 0x0a,
      49,   35,   35,   35, 0x08,
      64,   35,   35,   35, 0x08,
      81,   35,   35,   35, 0x08,
      98,   35,   35,   35, 0x08,
     114,   35,   35,   35, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__DocumentsDialog[] = {
    "ScIDE::DocumentsDialog\0selectAll()\0\0"
    "selectNone()\0saveSelected()\0"
    "reloadSelected()\0ignoreSelected()\0"
    "closeSelected()\0onDocumentChangedExternally(Document*)\0"
};

void ScIDE::DocumentsDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DocumentsDialog *_t = static_cast<DocumentsDialog *>(_o);
        switch (_id) {
        case 0: _t->selectAll(); break;
        case 1: _t->selectNone(); break;
        case 2: _t->saveSelected(); break;
        case 3: _t->reloadSelected(); break;
        case 4: _t->ignoreSelected(); break;
        case 5: _t->closeSelected(); break;
        case 6: _t->onDocumentChangedExternally((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::DocumentsDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::DocumentsDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ScIDE__DocumentsDialog,
      qt_meta_data_ScIDE__DocumentsDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::DocumentsDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::DocumentsDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::DocumentsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__DocumentsDialog))
        return static_cast<void*>(const_cast< DocumentsDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ScIDE::DocumentsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
