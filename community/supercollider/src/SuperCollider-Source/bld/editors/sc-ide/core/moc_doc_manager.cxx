/****************************************************************************
** Meta object code from reading C++ file 'doc_manager.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/core/doc_manager.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'doc_manager.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__Document[] = {

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
      16,   37,   37,   37, 0x05,

 // slots: signature, parameters, type, tag, flags
      38,   37,   37,   37, 0x0a,
      72,   37,   37,   37, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__Document[] = {
    "ScIDE::Document\0defaultFontChanged()\0"
    "\0applySettings(Settings::Manager*)\0"
    "resetDefaultFont()\0"
};

void ScIDE::Document::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Document *_t = static_cast<Document *>(_o);
        switch (_id) {
        case 0: _t->defaultFontChanged(); break;
        case 1: _t->applySettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 2: _t->resetDefaultFont(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::Document::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::Document::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__Document,
      qt_meta_data_ScIDE__Document, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::Document::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::Document::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::Document::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__Document))
        return static_cast<void*>(const_cast< Document*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::Document::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void ScIDE::Document::defaultFontChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_ScIDE__DocumentManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   49,   81,   81, 0x05,
      82,   81,   81,   81, 0x05,
     100,   81,   81,   81, 0x05,
     117,  148,   81,   81, 0x05,
     169,  196,   81,   81, 0x25,
     201,   81,   81,   81, 0x25,
     224,   81,   81,   81, 0x05,
     253,   81,   81,   81, 0x05,

 // slots: signature, parameters, type, tag, flags
     270,  297,  352,   81, 0x0a,
     362,  384,  352,   81, 0x2a,
     427,  445,  352,   81, 0x2a,
     472,  486,  352,   81, 0x2a,
     491,   81,   81,   81, 0x0a,
     506,   81,   81,   81, 0x0a,
     540,  486,   81,   81, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__DocumentManager[] = {
    "ScIDE::DocumentManager\0opened(Document*,int,int)\0"
    ",cursorPosition,selectionLength\0\0"
    "closed(Document*)\0saved(Document*)\0"
    "showRequest(Document*,int,int)\0"
    ",pos,selectionLength\0showRequest(Document*,int)\0"
    ",pos\0showRequest(Document*)\0"
    "changedExternally(Document*)\0"
    "recentsChanged()\0open(QString,int,int,bool)\0"
    "path,initialCursorPosition,selectionLength,addToRecent\0"
    "Document*\0open(QString,int,int)\0"
    "path,initialCursorPosition,selectionLength\0"
    "open(QString,int)\0path,initialCursorPosition\0"
    "open(QString)\0path\0clearRecents()\0"
    "storeSettings(Settings::Manager*)\0"
    "onFileChanged(QString)\0"
};

void ScIDE::DocumentManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DocumentManager *_t = static_cast<DocumentManager *>(_o);
        switch (_id) {
        case 0: _t->opened((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 1: _t->closed((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 2: _t->saved((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 3: _t->showRequest((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->showRequest((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->showRequest((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 6: _t->changedExternally((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 7: _t->recentsChanged(); break;
        case 8: { Document* _r = _t->open((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< Document**>(_a[0]) = _r; }  break;
        case 9: { Document* _r = _t->open((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< Document**>(_a[0]) = _r; }  break;
        case 10: { Document* _r = _t->open((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< Document**>(_a[0]) = _r; }  break;
        case 11: { Document* _r = _t->open((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< Document**>(_a[0]) = _r; }  break;
        case 12: _t->clearRecents(); break;
        case 13: _t->storeSettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 14: _t->onFileChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::DocumentManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::DocumentManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__DocumentManager,
      qt_meta_data_ScIDE__DocumentManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::DocumentManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::DocumentManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::DocumentManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__DocumentManager))
        return static_cast<void*>(const_cast< DocumentManager*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::DocumentManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::DocumentManager::opened(Document * _t1, int _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::DocumentManager::closed(Document * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ScIDE::DocumentManager::saved(Document * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ScIDE::DocumentManager::showRequest(Document * _t1, int _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 6
void ScIDE::DocumentManager::changedExternally(Document * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ScIDE::DocumentManager::recentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, 0);
}
QT_END_MOC_NAMESPACE
