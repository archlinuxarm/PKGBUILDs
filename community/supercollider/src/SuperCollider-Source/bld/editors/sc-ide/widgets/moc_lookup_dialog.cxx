/****************************************************************************
** Meta object code from reading C++ file 'lookup_dialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/lookup_dialog.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lookup_dialog.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__GenericLookupDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      27,   47,   48,   47, 0x0a,
      53,   47,   47,   47, 0x09,
      77,   47,   47,   47, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__GenericLookupDialog[] = {
    "ScIDE::GenericLookupDialog\0"
    "openDocumentation()\0\0bool\0"
    "onAccepted(QModelIndex)\0performQuery()\0"
};

void ScIDE::GenericLookupDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        GenericLookupDialog *_t = static_cast<GenericLookupDialog *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->openDocumentation();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: _t->onAccepted((*reinterpret_cast< QModelIndex(*)>(_a[1]))); break;
        case 2: _t->performQuery(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::GenericLookupDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::GenericLookupDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ScIDE__GenericLookupDialog,
      qt_meta_data_ScIDE__GenericLookupDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::GenericLookupDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::GenericLookupDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::GenericLookupDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__GenericLookupDialog))
        return static_cast<void*>(const_cast< GenericLookupDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ScIDE::GenericLookupDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
static const uint qt_meta_data_ScIDE__LookupDialog[] = {

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
      20,   35,   35,   35, 0x08,
      36,   35,   35,   35, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__LookupDialog[] = {
    "ScIDE::LookupDialog\0performQuery()\0\0"
    "onAccepted(QModelIndex)\0"
};

void ScIDE::LookupDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LookupDialog *_t = static_cast<LookupDialog *>(_o);
        switch (_id) {
        case 0: _t->performQuery(); break;
        case 1: _t->onAccepted((*reinterpret_cast< QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::LookupDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::LookupDialog::staticMetaObject = {
    { &GenericLookupDialog::staticMetaObject, qt_meta_stringdata_ScIDE__LookupDialog,
      qt_meta_data_ScIDE__LookupDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::LookupDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::LookupDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::LookupDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__LookupDialog))
        return static_cast<void*>(const_cast< LookupDialog*>(this));
    return GenericLookupDialog::qt_metacast(_clname);
}

int ScIDE::LookupDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = GenericLookupDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
static const uint qt_meta_data_ScIDE__SymbolReferenceRequest[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SymbolReferenceRequest[] = {
    "ScIDE::SymbolReferenceRequest\0"
};

void ScIDE::SymbolReferenceRequest::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::SymbolReferenceRequest::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SymbolReferenceRequest::staticMetaObject = {
    { &ScRequest::staticMetaObject, qt_meta_stringdata_ScIDE__SymbolReferenceRequest,
      qt_meta_data_ScIDE__SymbolReferenceRequest, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SymbolReferenceRequest::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SymbolReferenceRequest::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SymbolReferenceRequest::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SymbolReferenceRequest))
        return static_cast<void*>(const_cast< SymbolReferenceRequest*>(this));
    return ScRequest::qt_metacast(_clname);
}

int ScIDE::SymbolReferenceRequest::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ScRequest::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_ScIDE__ReferencesDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      24,   43,   43,   43, 0x08,
      44,   43,   43,   43, 0x08,
      59,   98,   43,   43, 0x08,
     119,  134,  147,   43, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ReferencesDialog[] = {
    "ScIDE::ReferencesDialog\0requestCancelled()\0"
    "\0performQuery()\0onResposeFromLanguage(QString,QString)\0"
    "command,responseData\0parse(QString)\0"
    "responseData\0QStandardItemModel*\0"
};

void ScIDE::ReferencesDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ReferencesDialog *_t = static_cast<ReferencesDialog *>(_o);
        switch (_id) {
        case 0: _t->requestCancelled(); break;
        case 1: _t->performQuery(); break;
        case 2: _t->onResposeFromLanguage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: { QStandardItemModel* _r = _t->parse((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QStandardItemModel**>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ReferencesDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ReferencesDialog::staticMetaObject = {
    { &LookupDialog::staticMetaObject, qt_meta_stringdata_ScIDE__ReferencesDialog,
      qt_meta_data_ScIDE__ReferencesDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ReferencesDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ReferencesDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ReferencesDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ReferencesDialog))
        return static_cast<void*>(const_cast< ReferencesDialog*>(this));
    return LookupDialog::qt_metacast(_clname);
}

int ScIDE::ReferencesDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = LookupDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
