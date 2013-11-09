/****************************************************************************
** Meta object code from reading C++ file 'layouts.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/layouts/layouts.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'layouts.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcHBoxLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       1,   44, // properties
       0,    0, // enums/sets
       1,   47, // constructors
       0,       // flags
       0,       // signalCount

 // methods: signature, parameters, type, tag, flags
      13,   34,   39,   39, 0x02,
      40,   34,   39,   39, 0x02,
      64,   84,   39,   39, 0x02,
      98,  128,   39,   39, 0x02,
     138,  160,   39,   39, 0x02,
     164,  196,   39,   39, 0x02,

 // properties: name, type, flags
     200,  208, 0x0009510b,

 // constructors: signature, parameters, type, tag, flags
     220,  246,   39,   39, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_QcHBoxLayout[] = {
    "QcHBoxLayout\0addItem(VariantList)\0"
    "data\0\0insertItem(VariantList)\0"
    "setStretch(int,int)\0index,stretch\0"
    "setStretch(QObjectProxy*,int)\0p,stretch\0"
    "setAlignment(int,int)\0i,a\0"
    "setAlignment(QObjectProxy*,int)\0p,a\0"
    "margins\0VariantList\0QcHBoxLayout(VariantList)\0"
    "items\0"
};

void QcHBoxLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { QcHBoxLayout *_r = new QcHBoxLayout((*reinterpret_cast< const VariantList(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcHBoxLayout *_t = static_cast<QcHBoxLayout *>(_o);
        switch (_id) {
        case 0: _t->addItem((*reinterpret_cast< const VariantList(*)>(_a[1]))); break;
        case 1: _t->insertItem((*reinterpret_cast< const VariantList(*)>(_a[1]))); break;
        case 2: _t->setStretch((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->setStretch((*reinterpret_cast< QObjectProxy*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->setAlignment((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->setAlignment((*reinterpret_cast< QObjectProxy*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcHBoxLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcHBoxLayout::staticMetaObject = {
    { &QcBoxLayout<QHBoxLayout>::staticMetaObject, qt_meta_stringdata_QcHBoxLayout,
      qt_meta_data_QcHBoxLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcHBoxLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcHBoxLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcHBoxLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcHBoxLayout))
        return static_cast<void*>(const_cast< QcHBoxLayout*>(this));
    return QcBoxLayout<QHBoxLayout>::qt_metacast(_clname);
}

int QcHBoxLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcBoxLayout<QHBoxLayout>::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< VariantList*>(_v) = margins(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setMargins(*reinterpret_cast< VariantList*>(_v)); break;
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
static const uint qt_meta_data_QcVBoxLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       1,   44, // properties
       0,    0, // enums/sets
       1,   47, // constructors
       0,       // flags
       0,       // signalCount

 // methods: signature, parameters, type, tag, flags
      13,   34,   39,   39, 0x02,
      40,   34,   39,   39, 0x02,
      64,   84,   39,   39, 0x02,
      98,  128,   39,   39, 0x02,
     138,  160,   39,   39, 0x02,
     164,  196,   39,   39, 0x02,

 // properties: name, type, flags
     200,  208, 0x0009510b,

 // constructors: signature, parameters, type, tag, flags
     220,  246,   39,   39, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_QcVBoxLayout[] = {
    "QcVBoxLayout\0addItem(VariantList)\0"
    "data\0\0insertItem(VariantList)\0"
    "setStretch(int,int)\0index,stretch\0"
    "setStretch(QObjectProxy*,int)\0p,stretch\0"
    "setAlignment(int,int)\0i,a\0"
    "setAlignment(QObjectProxy*,int)\0p,a\0"
    "margins\0VariantList\0QcVBoxLayout(VariantList)\0"
    "items\0"
};

void QcVBoxLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { QcVBoxLayout *_r = new QcVBoxLayout((*reinterpret_cast< const VariantList(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcVBoxLayout *_t = static_cast<QcVBoxLayout *>(_o);
        switch (_id) {
        case 0: _t->addItem((*reinterpret_cast< const VariantList(*)>(_a[1]))); break;
        case 1: _t->insertItem((*reinterpret_cast< const VariantList(*)>(_a[1]))); break;
        case 2: _t->setStretch((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->setStretch((*reinterpret_cast< QObjectProxy*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->setAlignment((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->setAlignment((*reinterpret_cast< QObjectProxy*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcVBoxLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcVBoxLayout::staticMetaObject = {
    { &QcBoxLayout<QVBoxLayout>::staticMetaObject, qt_meta_stringdata_QcVBoxLayout,
      qt_meta_data_QcVBoxLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcVBoxLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcVBoxLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcVBoxLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcVBoxLayout))
        return static_cast<void*>(const_cast< QcVBoxLayout*>(this));
    return QcBoxLayout<QVBoxLayout>::qt_metacast(_clname);
}

int QcVBoxLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcBoxLayout<QVBoxLayout>::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< VariantList*>(_v) = margins(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setMargins(*reinterpret_cast< VariantList*>(_v)); break;
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
static const uint qt_meta_data_QcGridLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       3,   59, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // methods: signature, parameters, type, tag, flags
      13,   34,   43,   43, 0x02,
      44,   67,   43,   43, 0x02,
      78,  104,   43,   43, 0x02,
     118,  144,   43,   43, 0x02,
     150,  182,   43,   43, 0x02,
     186,  204,  208,   43, 0x02,
     212,  232,  208,   43, 0x02,
     236,  261,   43,   43, 0x02,
     267,  294,   43,   43, 0x02,

 // properties: name, type, flags
     300,  308, 0x0009510b,
     320,  208, 0x02095103,
     336,  208, 0x02095103,

       0        // eod
};

static const char qt_meta_stringdata_QcGridLayout[] = {
    "QcGridLayout\0addItem(VariantList)\0"
    "dataList\0\0setRowStretch(int,int)\0"
    "row,factor\0setColumnStretch(int,int)\0"
    "column,factor\0setAlignment(int,int,int)\0"
    "r,c,a\0setAlignment(QObjectProxy*,int)\0"
    "p,a\0minRowHeight(int)\0row\0int\0"
    "minColumnWidth(int)\0col\0"
    "setMinRowHeight(int,int)\0row,h\0"
    "setMinColumnWidth(int,int)\0col,w\0"
    "margins\0VariantList\0verticalSpacing\0"
    "horizontalSpacing\0"
};

void QcGridLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcGridLayout *_t = static_cast<QcGridLayout *>(_o);
        switch (_id) {
        case 0: _t->addItem((*reinterpret_cast< const VariantList(*)>(_a[1]))); break;
        case 1: _t->setRowStretch((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->setColumnStretch((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->setAlignment((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->setAlignment((*reinterpret_cast< QObjectProxy*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: { int _r = _t->minRowHeight((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 6: { int _r = _t->minColumnWidth((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 7: _t->setMinRowHeight((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 8: _t->setMinColumnWidth((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcGridLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcGridLayout::staticMetaObject = {
    { &QcLayout<QGridLayout>::staticMetaObject, qt_meta_stringdata_QcGridLayout,
      qt_meta_data_QcGridLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcGridLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcGridLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcGridLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcGridLayout))
        return static_cast<void*>(const_cast< QcGridLayout*>(this));
    return QcLayout<QGridLayout>::qt_metacast(_clname);
}

int QcGridLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QcLayout<QGridLayout>::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< VariantList*>(_v) = margins(); break;
        case 1: *reinterpret_cast< int*>(_v) = verticalSpacing(); break;
        case 2: *reinterpret_cast< int*>(_v) = horizontalSpacing(); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setMargins(*reinterpret_cast< VariantList*>(_v)); break;
        case 1: setVerticalSpacing(*reinterpret_cast< int*>(_v)); break;
        case 2: setHorizontalSpacing(*reinterpret_cast< int*>(_v)); break;
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
static const uint qt_meta_data_QcStackLayout[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       1,   19, // properties
       0,    0, // enums/sets
       1,   22, // constructors
       0,       // flags
       0,       // signalCount

 // methods: signature, parameters, type, tag, flags
      14,   46,   58,   58, 0x02,

 // properties: name, type, flags
      59,   67, 0x0009510b,

 // constructors: signature, parameters, type, tag, flags
      79,  106,   58,   58, 0x0e,

       0        // eod
};

static const char qt_meta_stringdata_QcStackLayout[] = {
    "QcStackLayout\0insertWidget(int,QObjectProxy*)\0"
    "index,proxy\0\0margins\0VariantList\0"
    "QcStackLayout(VariantList)\0items\0"
};

void QcStackLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::CreateInstance) {
        switch (_id) {
        case 0: { QcStackLayout *_r = new QcStackLayout((*reinterpret_cast< const VariantList(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;
        }
    } else if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcStackLayout *_t = static_cast<QcStackLayout *>(_o);
        switch (_id) {
        case 0: _t->insertWidget((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QObjectProxy*(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcStackLayout::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcStackLayout::staticMetaObject = {
    { &QcLayout<QtCollider::StackLayout>::staticMetaObject, qt_meta_stringdata_QcStackLayout,
      qt_meta_data_QcStackLayout, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcStackLayout::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcStackLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcStackLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcStackLayout))
        return static_cast<void*>(const_cast< QcStackLayout*>(this));
    typedef QcLayout<QtCollider::StackLayout> QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int QcStackLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef QcLayout<QtCollider::StackLayout> QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< VariantList*>(_v) = margins(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setMargins(*reinterpret_cast< VariantList*>(_v)); break;
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
QT_END_MOC_NAMESPACE
