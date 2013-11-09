/****************************************************************************
** Meta object code from reading C++ file 'help_browser.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/help_browser.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'help_browser.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__LoadProgressIndicator[] = {

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
      29,   44,   48,   48, 0x0a,
      49,   48,   48,   48, 0x2a,
      57,   48,   48,   48, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__LoadProgressIndicator[] = {
    "ScIDE::LoadProgressIndicator\0"
    "start(QString)\0msg\0\0start()\0stop()\0"
};

void ScIDE::LoadProgressIndicator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LoadProgressIndicator *_t = static_cast<LoadProgressIndicator *>(_o);
        switch (_id) {
        case 0: _t->start((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->start(); break;
        case 2: _t->stop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::LoadProgressIndicator::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::LoadProgressIndicator::staticMetaObject = {
    { &QLabel::staticMetaObject, qt_meta_stringdata_ScIDE__LoadProgressIndicator,
      qt_meta_data_ScIDE__LoadProgressIndicator, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::LoadProgressIndicator::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::LoadProgressIndicator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::LoadProgressIndicator::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__LoadProgressIndicator))
        return static_cast<void*>(const_cast< LoadProgressIndicator*>(this));
    return QLabel::qt_metacast(_clname);
}

int ScIDE::LoadProgressIndicator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLabel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
static const uint qt_meta_data_ScIDE__HelpBrowser[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      19,   32,   32,   32, 0x05,

 // slots: signature, parameters, type, tag, flags
      33,   32,   32,   32, 0x0a,
      67,   32,   32,   32, 0x0a,
      76,   32,   32,   32, 0x0a,
      85,   32,   32,   32, 0x0a,
      95,   32,   32,   32, 0x0a,
     107,   32,   32,   32, 0x0a,
     127,  150,   32,   32, 0x0a,
     165,  183,   32,   32, 0x2a,
     188,   32,  208,   32, 0x0a,
     213,   32,   32,   32, 0x0a,
     230,   32,   32,   32, 0x0a,
     247,  276,   32,   32, 0x08,
     280,   32,   32,   32, 0x08,
     300,   32,   32,   32, 0x08,
     311,  341,   32,   32, 0x08,
     354,  390,   32,   32, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__HelpBrowser[] = {
    "ScIDE::HelpBrowser\0urlChanged()\0\0"
    "applySettings(Settings::Manager*)\0"
    "goHome()\0zoomIn()\0zoomOut()\0resetZoom()\0"
    "evaluateSelection()\0findText(QString,bool)\0"
    "text,backwards\0findText(QString)\0text\0"
    "openDocumentation()\0bool\0openDefinition()\0"
    "findReferences()\0onContextMenuRequest(QPoint)\0"
    "pos\0onLinkClicked(QUrl)\0onReload()\0"
    "onScResponse(QString,QString)\0"
    "command,data\0onJsConsoleMsg(QString,int,QString)\0"
    ",,\0"
};

void ScIDE::HelpBrowser::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        HelpBrowser *_t = static_cast<HelpBrowser *>(_o);
        switch (_id) {
        case 0: _t->urlChanged(); break;
        case 1: _t->applySettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 2: _t->goHome(); break;
        case 3: _t->zoomIn(); break;
        case 4: _t->zoomOut(); break;
        case 5: _t->resetZoom(); break;
        case 6: _t->evaluateSelection(); break;
        case 7: _t->findText((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 8: _t->findText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: { bool _r = _t->openDocumentation();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 10: _t->openDefinition(); break;
        case 11: _t->findReferences(); break;
        case 12: _t->onContextMenuRequest((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 13: _t->onLinkClicked((*reinterpret_cast< const QUrl(*)>(_a[1]))); break;
        case 14: _t->onReload(); break;
        case 15: _t->onScResponse((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 16: _t->onJsConsoleMsg((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::HelpBrowser::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::HelpBrowser::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__HelpBrowser,
      qt_meta_data_ScIDE__HelpBrowser, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::HelpBrowser::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::HelpBrowser::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::HelpBrowser::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__HelpBrowser))
        return static_cast<void*>(const_cast< HelpBrowser*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::HelpBrowser::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::HelpBrowser::urlChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_ScIDE__HelpBrowserFindBox[] = {

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
      26,   46,   61,   61, 0x05,
      62,   77,   61,   61, 0x25,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__HelpBrowserFindBox[] = {
    "ScIDE::HelpBrowserFindBox\0query(QString,bool)\0"
    "text,backwards\0\0query(QString)\0text\0"
};

void ScIDE::HelpBrowserFindBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        HelpBrowserFindBox *_t = static_cast<HelpBrowserFindBox *>(_o);
        switch (_id) {
        case 0: _t->query((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: _t->query((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::HelpBrowserFindBox::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::HelpBrowserFindBox::staticMetaObject = {
    { &QLineEdit::staticMetaObject, qt_meta_stringdata_ScIDE__HelpBrowserFindBox,
      qt_meta_data_ScIDE__HelpBrowserFindBox, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::HelpBrowserFindBox::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::HelpBrowserFindBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::HelpBrowserFindBox::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__HelpBrowserFindBox))
        return static_cast<void*>(const_cast< HelpBrowserFindBox*>(this));
    return QLineEdit::qt_metacast(_clname);
}

int ScIDE::HelpBrowserFindBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
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
void ScIDE::HelpBrowserFindBox::query(const QString & _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_ScIDE__HelpBrowserDocklet[] = {

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
      26,   47,   47,   47, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__HelpBrowserDocklet[] = {
    "ScIDE::HelpBrowserDocklet\0"
    "onInterpreterStart()\0\0"
};

void ScIDE::HelpBrowserDocklet::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        HelpBrowserDocklet *_t = static_cast<HelpBrowserDocklet *>(_o);
        switch (_id) {
        case 0: _t->onInterpreterStart(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::HelpBrowserDocklet::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::HelpBrowserDocklet::staticMetaObject = {
    { &Docklet::staticMetaObject, qt_meta_stringdata_ScIDE__HelpBrowserDocklet,
      qt_meta_data_ScIDE__HelpBrowserDocklet, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::HelpBrowserDocklet::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::HelpBrowserDocklet::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::HelpBrowserDocklet::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__HelpBrowserDocklet))
        return static_cast<void*>(const_cast< HelpBrowserDocklet*>(this));
    return Docklet::qt_metacast(_clname);
}

int ScIDE::HelpBrowserDocklet::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
