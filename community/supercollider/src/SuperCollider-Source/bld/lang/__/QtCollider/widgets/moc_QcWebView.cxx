/****************************************************************************
** Meta object code from reading C++ file 'QcWebView.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcWebView.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcWebView.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QtCollider__WebView[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       6,   74, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      20,   43,   43,   43, 0x05,
      44,   43,   43,   43, 0x05,
      69,   88,   43,   43, 0x05,
      93,  127,   43,   43, 0x05,

 // slots: signature, parameters, type, tag, flags
     130,  153,   43,   43, 0x0a,
     173,  191,   43,   43, 0x2a,
     202,   43,   43,   43, 0x08,
     222,   43,   43,   43, 0x08,

 // methods: signature, parameters, type, tag, flags
     237,  262,   43,   43, 0x02,
     275,  292,   43,   43, 0x22,
     297,  325,   43,   43, 0x02,
     332,  359,   43,   43, 0x02,

 // properties: name, type, flags
     388,  392, 0x0a095103,
     292,  392, 0x0a095001,
     400,  392, 0x0a095001,
     410,  431, 0x0009510b,
     462,  477, 0x01095103,
     482,  477, 0x01095003,

       0        // eod
};

static const char qt_meta_stringdata_QtCollider__WebView[] = {
    "QtCollider::WebView\0linkActivated(QString)\0"
    "\0reloadTriggered(QString)\0interpret(QString)\0"
    "code\0jsConsoleMsg(QString,int,QString)\0"
    ",,\0findText(QString,bool)\0searchText,reversed\0"
    "findText(QString)\0searchText\0"
    "onLinkClicked(QUrl)\0onPageReload()\0"
    "setHtml(QString,QString)\0html,baseUrl\0"
    "setHtml(QString)\0html\0evaluateJavaScript(QString)\0"
    "script\0setFontFamily(int,QString)\0"
    "genericFontFamily,fontFamily\0url\0"
    "QString\0plainText\0linkDelegationPolicy\0"
    "QWebPage::LinkDelegationPolicy\0"
    "delegateReload\0bool\0enterInterpretsSelection\0"
};

void QtCollider::WebView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        WebView *_t = static_cast<WebView *>(_o);
        switch (_id) {
        case 0: _t->linkActivated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->reloadTriggered((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->interpret((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->jsConsoleMsg((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 4: _t->findText((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 5: _t->findText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->onLinkClicked((*reinterpret_cast< const QUrl(*)>(_a[1]))); break;
        case 7: _t->onPageReload(); break;
        case 8: _t->setHtml((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->setHtml((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->evaluateJavaScript((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->setFontFamily((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

#ifdef Q_NO_DATA_RELOCATION
static const QMetaObjectAccessor qt_meta_extradata_QtCollider__WebView[] = {
        QWebPage::getStaticMetaObject,
#else
static const QMetaObject *qt_meta_extradata_QtCollider__WebView[] = {
        &QWebPage::staticMetaObject,
#endif //Q_NO_DATA_RELOCATION
    0
};

const QMetaObjectExtraData QtCollider::WebView::staticMetaObjectExtraData = {
    qt_meta_extradata_QtCollider__WebView,  qt_static_metacall 
};

const QMetaObject QtCollider::WebView::staticMetaObject = {
    { &QWebView::staticMetaObject, qt_meta_stringdata_QtCollider__WebView,
      qt_meta_data_QtCollider__WebView, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QtCollider::WebView::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QtCollider::WebView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QtCollider::WebView::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtCollider__WebView))
        return static_cast<void*>(const_cast< WebView*>(this));
    return QWebView::qt_metacast(_clname);
}

int QtCollider::WebView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWebView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = url(); break;
        case 1: *reinterpret_cast< QString*>(_v) = html(); break;
        case 2: *reinterpret_cast< QString*>(_v) = plainText(); break;
        case 3: *reinterpret_cast< QWebPage::LinkDelegationPolicy*>(_v) = linkDelegationPolicy(); break;
        case 4: *reinterpret_cast< bool*>(_v) = delegateReload(); break;
        case 5: *reinterpret_cast< bool*>(_v) = interpretSelection(); break;
        }
        _id -= 6;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setUrl(*reinterpret_cast< QString*>(_v)); break;
        case 3: setLinkDelegationPolicy(*reinterpret_cast< QWebPage::LinkDelegationPolicy*>(_v)); break;
        case 4: setDelegateReload(*reinterpret_cast< bool*>(_v)); break;
        case 5: setInterpretSelection(*reinterpret_cast< bool*>(_v)); break;
        }
        _id -= 6;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 6;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QtCollider::WebView::linkActivated(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QtCollider::WebView::reloadTriggered(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void QtCollider::WebView::interpret(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void QtCollider::WebView::jsConsoleMsg(const QString & _t1, int _t2, const QString & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
