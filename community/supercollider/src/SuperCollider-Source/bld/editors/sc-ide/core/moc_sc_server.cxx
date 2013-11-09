/****************************************************************************
** Meta object code from reading C++ file 'sc_server.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/core/sc_server.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sc_server.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__ScServer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      16,   53,   75,   75, 0x05,
      76,  124,   75,   75, 0x05,

 // slots: signature, parameters, type, tag, flags
     180,   75,   75,   75, 0x0a,
     187,   75,   75,   75, 0x0a,
     196,   75,   75,   75, 0x0a,
     203,   75,   75,   75, 0x0a,
     219,   75,   75,   75, 0x0a,
     232,   75,   75,   75, 0x0a,
     247,   75,   75,   75, 0x0a,
     274,  294,   75,   75, 0x0a,
     307,   75,   75,   75, 0x08,
     352,  385,   75,   75, 0x08,
     399,   75,   75,   75, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ScServer[] = {
    "ScIDE::ScServer\0runningStateChange(bool,QString,int)\0"
    "running,hostName,port\0\0"
    "updateServerStatus(int,int,int,int,float,float)\0"
    "ugenCount,synthCount,groupCount,defCount,avgCPU,peakCPU\0"
    "boot()\0reboot()\0quit()\0toggleRunning()\0"
    "showMeters()\0dumpNodeTree()\0"
    "dumpNodeTreeWithControls()\0"
    "queryAllNodes(bool)\0dumpControls\0"
    "onScLangStateChanged(QProcess::ProcessState)\0"
    "onScLangReponse(QString,QString)\0"
    "selector,data\0updateToggleRunningAction()\0"
};

void ScIDE::ScServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScServer *_t = static_cast<ScServer *>(_o);
        switch (_id) {
        case 0: _t->runningStateChange((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 1: _t->updateServerStatus((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< float(*)>(_a[5])),(*reinterpret_cast< float(*)>(_a[6]))); break;
        case 2: _t->boot(); break;
        case 3: _t->reboot(); break;
        case 4: _t->quit(); break;
        case 5: _t->toggleRunning(); break;
        case 6: _t->showMeters(); break;
        case 7: _t->dumpNodeTree(); break;
        case 8: _t->dumpNodeTreeWithControls(); break;
        case 9: _t->queryAllNodes((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->onScLangStateChanged((*reinterpret_cast< QProcess::ProcessState(*)>(_a[1]))); break;
        case 11: _t->onScLangReponse((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 12: _t->updateToggleRunningAction(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ScServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ScServer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__ScServer,
      qt_meta_data_ScIDE__ScServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ScServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ScServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ScServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ScServer))
        return static_cast<void*>(const_cast< ScServer*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::ScServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::ScServer::runningStateChange(bool _t1, QString const & _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScIDE::ScServer::updateServerStatus(int _t1, int _t2, int _t3, int _t4, float _t5, float _t6)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
