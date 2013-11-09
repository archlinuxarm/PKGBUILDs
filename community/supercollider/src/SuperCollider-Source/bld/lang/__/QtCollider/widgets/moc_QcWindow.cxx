/****************************************************************************
** Meta object code from reading C++ file 'QcWindow.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       1,   14, // constructors
       0,       // flags
       0,       // signalCount

 // constructors: signature, parameters, type, tag, flags
       9,   44,   71,   71, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_QcWindow[] = {
    "QcWindow\0QcWindow(QString,QRectF,bool,bool)\0"
    "title,geom,resizable,frame\0\0"
};

void QcWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { QcWindow *_r = new QcWindow((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QRectF(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    }
    Q_UNUSED(_o);
}

const QMetaObjectExtraData QcWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcWindow::staticMetaObject = {
    { &QcCustomPainted::staticMetaObject, qt_meta_stringdata_QcWindow,
      qt_meta_data_QcWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcWindow))
        return static_cast<void*>(const_cast< QcWindow*>(this));
    return QcCustomPainted::qt_metacast(_clname);
}

int QcWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcCustomPainted::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QcScrollWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       1,   14, // constructors
       0,       // flags
       0,       // signalCount

 // constructors: signature, parameters, type, tag, flags
      15,   56,   83,   83, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_QcScrollWindow[] = {
    "QcScrollWindow\0QcScrollWindow(QString,QRectF,bool,bool)\0"
    "title,geom,resizable,frame\0\0"
};

void QcScrollWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { QcScrollWindow *_r = new QcScrollWindow((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QRectF(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    }
    Q_UNUSED(_o);
}

const QMetaObjectExtraData QcScrollWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcScrollWindow::staticMetaObject = {
    { &QcScrollArea::staticMetaObject, qt_meta_stringdata_QcScrollWindow,
      qt_meta_data_QcScrollWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcScrollWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcScrollWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcScrollWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcScrollWindow))
        return static_cast<void*>(const_cast< QcScrollWindow*>(this));
    return QcScrollArea::qt_metacast(_clname);
}

int QcScrollWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcScrollArea::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
