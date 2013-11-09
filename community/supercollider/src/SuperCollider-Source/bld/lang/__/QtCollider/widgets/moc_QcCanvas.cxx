/****************************************************************************
** Meta object code from reading C++ file 'QcCanvas.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcCanvas.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcCanvas.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcCanvas[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       5,   34, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
       9,   29,   29,   29, 0x05,

 // slots: signature, parameters, type, tag, flags
      30,   29,   29,   29, 0x0a,
      40,   29,   29,   29, 0x0a,
      48,   62,   29,   29, 0x0a,

 // properties: name, type, flags
      69,   84, 0x01095103,
      89,   84, 0x01095103,
     104,  114, 0x87095103,
     120,  131, 0x02095001,
     135,  146, 0x43095103,

       0        // eod
};

static const char qt_meta_stringdata_QcCanvas[] = {
    "QcCanvas\0painting(QPainter*)\0\0refresh()\0"
    "clear()\0animate(bool)\0toggle\0"
    "clearOnRefresh\0bool\0drawingEnabled\0"
    "frameRate\0float\0frameCount\0int\0"
    "background\0QColor\0"
};

void QcCanvas::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcCanvas *_t = static_cast<QcCanvas *>(_o);
        switch (_id) {
        case 0: _t->painting((*reinterpret_cast< QPainter*(*)>(_a[1]))); break;
        case 1: _t->refresh(); break;
        case 2: _t->clear(); break;
        case 3: _t->animate((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcCanvas::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcCanvas::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcCanvas,
      qt_meta_data_QcCanvas, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcCanvas::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcCanvas::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcCanvas::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcCanvas))
        return static_cast<void*>(const_cast< QcCanvas*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcCanvas::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = clearOnRefresh(); break;
        case 1: *reinterpret_cast< bool*>(_v) = drawingEnabled(); break;
        case 2: *reinterpret_cast< float*>(_v) = frameRate(); break;
        case 3: *reinterpret_cast< int*>(_v) = frameCount(); break;
        case 4: *reinterpret_cast< QColor*>(_v) = background(); break;
        }
        _id -= 5;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setClearOnRefresh(*reinterpret_cast< bool*>(_v)); break;
        case 1: setDrawingEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 2: setFrameRate(*reinterpret_cast< float*>(_v)); break;
        case 4: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        }
        _id -= 5;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 5;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcCanvas::painting(QPainter * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
