/****************************************************************************
** Meta object code from reading C++ file 'post_window.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/post_window.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'post_window.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__PostWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      18,   42,   42,   42, 0x05,

 // slots: signature, parameters, type, tag, flags
      43,   57,   42,   42, 0x0a,
      62,   42,   42,   42, 0x0a,
      79,   91,   42,   42, 0x0a,
      97,   42,   42,   42, 0x2a,
     106,   91,   42,   42, 0x0a,
     119,   42,   42,   42, 0x2a,
     129,   42,   42,   42, 0x0a,
     141,   42,  161,   42, 0x0a,
     166,   42,   42,   42, 0x0a,
     183,   42,   42,   42, 0x0a,
     200,   42,   42,   42, 0x08,
     228,  246,   42,   42, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__PostWindow[] = {
    "ScIDE::PostWindow\0scrollToBottomRequest()\0"
    "\0post(QString)\0text\0scrollToBottom()\0"
    "zoomIn(int)\0steps\0zoomIn()\0zoomOut(int)\0"
    "zoomOut()\0resetZoom()\0openDocumentation()\0"
    "bool\0openDefinition()\0findReferences()\0"
    "onAutoScrollTriggered(bool)\0"
    "setLineWrap(bool)\0on\0"
};

void ScIDE::PostWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PostWindow *_t = static_cast<PostWindow *>(_o);
        switch (_id) {
        case 0: _t->scrollToBottomRequest(); break;
        case 1: _t->post((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->scrollToBottom(); break;
        case 3: _t->zoomIn((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->zoomIn(); break;
        case 5: _t->zoomOut((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->zoomOut(); break;
        case 7: _t->resetZoom(); break;
        case 8: { bool _r = _t->openDocumentation();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 9: _t->openDefinition(); break;
        case 10: _t->findReferences(); break;
        case 11: _t->onAutoScrollTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->setLineWrap((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::PostWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::PostWindow::staticMetaObject = {
    { &QPlainTextEdit::staticMetaObject, qt_meta_stringdata_ScIDE__PostWindow,
      qt_meta_data_ScIDE__PostWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::PostWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::PostWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::PostWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__PostWindow))
        return static_cast<void*>(const_cast< PostWindow*>(this));
    return QPlainTextEdit::qt_metacast(_clname);
}

int ScIDE::PostWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPlainTextEdit::qt_metacall(_c, _id, _a);
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
void ScIDE::PostWindow::scrollToBottomRequest()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_ScIDE__PostDocklet[] = {

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
      19,   43,   52,   52, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__PostDocklet[] = {
    "ScIDE::PostDocklet\0onFloatingChanged(bool)\0"
    "floating\0\0"
};

void ScIDE::PostDocklet::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PostDocklet *_t = static_cast<PostDocklet *>(_o);
        switch (_id) {
        case 0: _t->onFloatingChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::PostDocklet::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::PostDocklet::staticMetaObject = {
    { &Docklet::staticMetaObject, qt_meta_stringdata_ScIDE__PostDocklet,
      qt_meta_data_ScIDE__PostDocklet, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::PostDocklet::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::PostDocklet::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::PostDocklet::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__PostDocklet))
        return static_cast<void*>(const_cast< PostDocklet*>(this));
    return Docklet::qt_metacast(_clname);
}

int ScIDE::PostDocklet::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Docklet::qt_metacall(_c, _id, _a);
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
