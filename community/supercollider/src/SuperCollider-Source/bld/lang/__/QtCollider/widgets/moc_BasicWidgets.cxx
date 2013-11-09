/****************************************************************************
** Meta object code from reading C++ file 'BasicWidgets.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/BasicWidgets.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BasicWidgets.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcSimpleWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       1,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
      15,   26, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcSimpleWidget[] = {
    "QcSimpleWidget\0background\0QColor\0"
};

void QcSimpleWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcSimpleWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcSimpleWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcSimpleWidget,
      qt_meta_data_QcSimpleWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcSimpleWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcSimpleWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcSimpleWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcSimpleWidget))
        return static_cast<void*>(const_cast< QcSimpleWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcSimpleWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
     if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QColor*>(_v) = background(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
static const uint qt_meta_data_QcDefaultWidget[] = {

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

static const char qt_meta_stringdata_QcDefaultWidget[] = {
    "QcDefaultWidget\0"
};

void QcDefaultWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcDefaultWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcDefaultWidget::staticMetaObject = {
    { &QcSimpleWidget::staticMetaObject, qt_meta_stringdata_QcDefaultWidget,
      qt_meta_data_QcDefaultWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcDefaultWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcDefaultWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcDefaultWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcDefaultWidget))
        return static_cast<void*>(const_cast< QcDefaultWidget*>(this));
    return QcSimpleWidget::qt_metacast(_clname);
}

int QcDefaultWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcSimpleWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QcHLayoutWidget[] = {

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

static const char qt_meta_stringdata_QcHLayoutWidget[] = {
    "QcHLayoutWidget\0"
};

void QcHLayoutWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcHLayoutWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcHLayoutWidget::staticMetaObject = {
    { &QcSimpleWidget::staticMetaObject, qt_meta_stringdata_QcHLayoutWidget,
      qt_meta_data_QcHLayoutWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcHLayoutWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcHLayoutWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcHLayoutWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcHLayoutWidget))
        return static_cast<void*>(const_cast< QcHLayoutWidget*>(this));
    return QcSimpleWidget::qt_metacast(_clname);
}

int QcHLayoutWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcSimpleWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QcVLayoutWidget[] = {

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

static const char qt_meta_stringdata_QcVLayoutWidget[] = {
    "QcVLayoutWidget\0"
};

void QcVLayoutWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcVLayoutWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcVLayoutWidget::staticMetaObject = {
    { &QcSimpleWidget::staticMetaObject, qt_meta_stringdata_QcVLayoutWidget,
      qt_meta_data_QcVLayoutWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcVLayoutWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcVLayoutWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcVLayoutWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcVLayoutWidget))
        return static_cast<void*>(const_cast< QcVLayoutWidget*>(this));
    return QcSimpleWidget::qt_metacast(_clname);
}

int QcVLayoutWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcSimpleWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QcCustomPainted[] = {

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

static const char qt_meta_stringdata_QcCustomPainted[] = {
    "QcCustomPainted\0"
};

void QcCustomPainted::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcCustomPainted::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcCustomPainted::staticMetaObject = {
    { &QcCanvas::staticMetaObject, qt_meta_stringdata_QcCustomPainted,
      qt_meta_data_QcCustomPainted, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcCustomPainted::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcCustomPainted::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcCustomPainted::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcCustomPainted))
        return static_cast<void*>(const_cast< QcCustomPainted*>(this));
    return QcCanvas::qt_metacast(_clname);
}

int QcCustomPainted::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcCanvas::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
