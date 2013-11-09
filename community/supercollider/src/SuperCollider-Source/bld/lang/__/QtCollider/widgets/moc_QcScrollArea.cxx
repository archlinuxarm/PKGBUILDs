/****************************************************************************
** Meta object code from reading C++ file 'QcScrollArea.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcScrollArea.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcScrollArea.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcScrollWidget[] = {

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

static const char qt_meta_stringdata_QcScrollWidget[] = {
    "QcScrollWidget\0"
};

void QcScrollWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData QcScrollWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcScrollWidget::staticMetaObject = {
    { &QcCanvas::staticMetaObject, qt_meta_stringdata_QcScrollWidget,
      qt_meta_data_QcScrollWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcScrollWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcScrollWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcScrollWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcScrollWidget))
        return static_cast<void*>(const_cast< QcScrollWidget*>(this));
    return QcCanvas::qt_metacast(_clname);
}

int QcScrollWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcCanvas::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QcScrollArea[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       3,   29, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   24,   24,   24, 0x05,

 // methods: signature, parameters, type, tag, flags
      25,   24,   24,   24, 0x02,
      50,   69,   24,   24, 0x02,

 // properties: name, type, flags
      71,   81, 0x01095103,
      86,   98, 0x14095001,
     105,  119, 0x1a095103,

       0        // eod
};

static const char qt_meta_stringdata_QcScrollArea[] = {
    "QcScrollArea\0scrolled()\0\0"
    "setWidget(QObjectProxy*)\0addChild(QWidget*)\0"
    "w\0hasBorder\0bool\0innerBounds\0QRectF\0"
    "visibleOrigin\0QPointF\0"
};

void QcScrollArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcScrollArea *_t = static_cast<QcScrollArea *>(_o);
        switch (_id) {
        case 0: _t->scrolled(); break;
        case 1: _t->setWidget((*reinterpret_cast< QObjectProxy*(*)>(_a[1]))); break;
        case 2: _t->addChild((*reinterpret_cast< QWidget*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcScrollArea::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcScrollArea::staticMetaObject = {
    { &QScrollArea::staticMetaObject, qt_meta_stringdata_QcScrollArea,
      qt_meta_data_QcScrollArea, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcScrollArea::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcScrollArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcScrollArea::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcScrollArea))
        return static_cast<void*>(const_cast< QcScrollArea*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcScrollArea*>(this));
    return QScrollArea::qt_metacast(_clname);
}

int QcScrollArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QScrollArea::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 1: *reinterpret_cast< QRectF*>(_v) = innerBounds(); break;
        case 2: *reinterpret_cast< QPointF*>(_v) = visibleOrigin(); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setHasBorder(*reinterpret_cast< bool*>(_v)); break;
        case 2: setVisibleOrigin(*reinterpret_cast< QPointF*>(_v)); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcScrollArea::scrolled()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
