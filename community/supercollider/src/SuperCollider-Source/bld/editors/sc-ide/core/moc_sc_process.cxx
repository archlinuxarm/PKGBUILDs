/****************************************************************************
** Meta object code from reading C++ file 'sc_process.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/core/sc_process.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sc_process.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__ScProcess[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      17,   33,   33,   33, 0x05,
      34,   33,   33,   33, 0x05,
      57,   83,   33,   33, 0x05,
      97,   33,   33,   33, 0x05,
     122,   33,   33,   33, 0x05,

 // slots: signature, parameters, type, tag, flags
     149,   33,   33,   33, 0x0a,
     165,   33,   33,   33, 0x0a,
     181,   33,   33,   33, 0x0a,
     196,   33,   33,   33, 0x0a,
     214,   33,   33,   33, 0x0a,
     238,   33,   33,   33, 0x0a,
     249,  276,   33,   33, 0x0a,
     297,  319,   33,   33, 0x2a,
     333,  379,   33,   33, 0x08,
     396,   33,   33,   33, 0x08,
     417,   33,   33,   33, 0x08,
     429,   33,   33,   33, 0x08,
     450,  496,   33,   33, 0x08,
     502,   33,   33,   33, 0x08,
     516,   33,   33,   33, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ScProcess[] = {
    "ScIDE::ScProcess\0scPost(QString)\0\0"
    "statusMessage(QString)\0response(QString,QString)\0"
    "selector,data\0classLibraryRecompiled()\0"
    "introspectionAboutToSwap()\0toggleRunning()\0"
    "startLanguage()\0stopLanguage()\0"
    "restartLanguage()\0recompileClassLibrary()\0"
    "stopMain()\0evaluateCode(QString,bool)\0"
    "commandString,silent\0evaluateCode(QString)\0"
    "commandString\0"
    "swapIntrospection(ScLanguage::Introspection*)\0"
    "newIntrospection\0onNewIpcConnection()\0"
    "onIpcData()\0finalizeConnection()\0"
    "onProcessStateChanged(QProcess::ProcessState)\0"
    "state\0onReadyRead()\0updateToggleRunningAction()\0"
};

void ScIDE::ScProcess::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScProcess *_t = static_cast<ScProcess *>(_o);
        switch (_id) {
        case 0: _t->scPost((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->statusMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->response((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->classLibraryRecompiled(); break;
        case 4: _t->introspectionAboutToSwap(); break;
        case 5: _t->toggleRunning(); break;
        case 6: _t->startLanguage(); break;
        case 7: _t->stopLanguage(); break;
        case 8: _t->restartLanguage(); break;
        case 9: _t->recompileClassLibrary(); break;
        case 10: _t->stopMain(); break;
        case 11: _t->evaluateCode((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 12: _t->evaluateCode((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->swapIntrospection((*reinterpret_cast< ScLanguage::Introspection*(*)>(_a[1]))); break;
        case 14: _t->onNewIpcConnection(); break;
        case 15: _t->onIpcData(); break;
        case 16: _t->finalizeConnection(); break;
        case 17: _t->onProcessStateChanged((*reinterpret_cast< QProcess::ProcessState(*)>(_a[1]))); break;
        case 18: _t->onReadyRead(); break;
        case 19: _t->updateToggleRunningAction(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ScProcess::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ScProcess::staticMetaObject = {
    { &QProcess::staticMetaObject, qt_meta_stringdata_ScIDE__ScProcess,
      qt_meta_data_ScIDE__ScProcess, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ScProcess::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ScProcess::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ScProcess::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ScProcess))
        return static_cast<void*>(const_cast< ScProcess*>(this));
    return QProcess::qt_metacast(_clname);
}

int ScIDE::ScProcess::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QProcess::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::ScProcess::scPost(QString const & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::ScProcess::statusMessage(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ScIDE::ScProcess::response(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ScIDE::ScProcess::classLibraryRecompiled()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void ScIDE::ScProcess::introspectionAboutToSwap()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
static const uint qt_meta_data_ScIDE__ScRequest[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      17,   43,   56,   56, 0x05,
      57,   56,   56,   56, 0x05,

 // slots: signature, parameters, type, tag, flags
      69,   56,   56,   56, 0x0a,
      78,  106,   56,   56, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ScRequest[] = {
    "ScIDE::ScRequest\0response(QString,QString)\0"
    "command,data\0\0cancelled()\0cancel()\0"
    "onResponse(QString,QString)\0"
    "responseId,responseData\0"
};

void ScIDE::ScRequest::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScRequest *_t = static_cast<ScRequest *>(_o);
        switch (_id) {
        case 0: _t->response((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->cancelled(); break;
        case 2: _t->cancel(); break;
        case 3: _t->onResponse((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ScRequest::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ScRequest::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__ScRequest,
      qt_meta_data_ScIDE__ScRequest, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ScRequest::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ScRequest::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ScRequest::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ScRequest))
        return static_cast<void*>(const_cast< ScRequest*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::ScRequest::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void ScIDE::ScRequest::response(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::ScRequest::cancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
static const uint qt_meta_data_ScIDE__ScIntrospectionParserWorker[] = {

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
      35,   68,   75,   75, 0x05,

 // slots: signature, parameters, type, tag, flags
      76,   93,   75,   75, 0x08,
      99,   75,   75,   75, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ScIntrospectionParserWorker[] = {
    "ScIDE::ScIntrospectionParserWorker\0"
    "done(ScLanguage::Introspection*)\0"
    "output\0\0process(QString)\0input\0quit()\0"
};

void ScIDE::ScIntrospectionParserWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScIntrospectionParserWorker *_t = static_cast<ScIntrospectionParserWorker *>(_o);
        switch (_id) {
        case 0: _t->done((*reinterpret_cast< ScLanguage::Introspection*(*)>(_a[1]))); break;
        case 1: _t->process((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->quit(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ScIntrospectionParserWorker::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ScIntrospectionParserWorker::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__ScIntrospectionParserWorker,
      qt_meta_data_ScIDE__ScIntrospectionParserWorker, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ScIntrospectionParserWorker::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ScIntrospectionParserWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ScIntrospectionParserWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ScIntrospectionParserWorker))
        return static_cast<void*>(const_cast< ScIntrospectionParserWorker*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::ScIntrospectionParserWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void ScIDE::ScIntrospectionParserWorker::done(ScLanguage::Introspection * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_ScIDE__ScIntrospectionParser[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      29,   59,   64,   64, 0x05,
      65,   64,   64,   64, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ScIntrospectionParser[] = {
    "ScIDE::ScIntrospectionParser\0"
    "newIntrospectionData(QString)\0data\0\0"
    "done(ScLanguage::Introspection*)\0"
};

void ScIDE::ScIntrospectionParser::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScIntrospectionParser *_t = static_cast<ScIntrospectionParser *>(_o);
        switch (_id) {
        case 0: _t->newIntrospectionData((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->done((*reinterpret_cast< ScLanguage::Introspection*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ScIntrospectionParser::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ScIntrospectionParser::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_ScIDE__ScIntrospectionParser,
      qt_meta_data_ScIDE__ScIntrospectionParser, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ScIntrospectionParser::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ScIntrospectionParser::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ScIntrospectionParser::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ScIntrospectionParser))
        return static_cast<void*>(const_cast< ScIntrospectionParser*>(this));
    return QThread::qt_metacast(_clname);
}

int ScIDE::ScIntrospectionParser::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::ScIntrospectionParser::newIntrospectionData(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::ScIntrospectionParser::done(ScLanguage::Introspection * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
