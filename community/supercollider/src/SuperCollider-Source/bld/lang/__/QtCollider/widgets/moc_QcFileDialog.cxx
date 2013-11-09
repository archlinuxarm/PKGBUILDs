/****************************************************************************
** Meta object code from reading C++ file 'QcFileDialog.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcFileDialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcFileDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcFileDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       3,   34, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   35,   42,   42, 0x05,
      43,   42,   42,   42, 0x05,

 // slots: signature, parameters, type, tag, flags
      54,   42,   42,   42, 0x08,
      61,   77,   42,   42, 0x08,

 // constructors: signature, parameters, type, tag, flags
      81,  103,   42,   42, 0x0e,
     123,  141,   42,   42, 0x2e,
     150,   42,   42,   42, 0x2e,

       0        // eod
};

static const char qt_meta_stringdata_QcFileDialog[] = {
    "QcFileDialog\0accepted(VariantList)\0"
    "result\0\0rejected()\0show()\0onFinished(int)\0"
    "res\0QcFileDialog(int,int)\0fileMode,acceptMode\0"
    "QcFileDialog(int)\0fileMode\0QcFileDialog()\0"
};

void QcFileDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { QcFileDialog *_r = new QcFileDialog((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        case 1: { QcFileDialog *_r = new QcFileDialog((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        case 2: { QcFileDialog *_r = new QcFileDialog();
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcFileDialog *_t = static_cast<QcFileDialog *>(_o);
        switch (_id) {
        case 0: _t->accepted((*reinterpret_cast< VariantList(*)>(_a[1]))); break;
        case 1: _t->rejected(); break;
        case 2: _t->show(); break;
        case 3: _t->onFinished((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcFileDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcFileDialog::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QcFileDialog,
      qt_meta_data_QcFileDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcFileDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcFileDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcFileDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcFileDialog))
        return static_cast<void*>(const_cast< QcFileDialog*>(this));
    return QObject::qt_metacast(_clname);
}

int QcFileDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void QcFileDialog::accepted(VariantList _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QcFileDialog::rejected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
