/****************************************************************************
** Meta object code from reading C++ file 'doc_list.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/doc_list.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'doc_list.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__DocumentListWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      26,   45,   45,   45, 0x05,

 // slots: signature, parameters, type, tag, flags
      46,   45,   45,   45, 0x0a,
      68,   94,   45,   45, 0x08,
      97,   45,   45,   45, 0x08,
     116,   45,   45,   45, 0x08,
     135,   45,   45,   45, 0x08,
     167,   45,   45,   45, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__DocumentListWidget[] = {
    "ScIDE::DocumentListWidget\0clicked(Document*)\0"
    "\0setCurrent(Document*)\0onOpen(Document*,int,int)\0"
    ",,\0onClose(Document*)\0onSaved(Document*)\0"
    "onModificationChanged(QObject*)\0"
    "onItemClicked(QListWidgetItem*)\0"
};

void ScIDE::DocumentListWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DocumentListWidget *_t = static_cast<DocumentListWidget *>(_o);
        switch (_id) {
        case 0: _t->clicked((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 1: _t->setCurrent((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 2: _t->onOpen((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 3: _t->onClose((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 4: _t->onSaved((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 5: _t->onModificationChanged((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        case 6: _t->onItemClicked((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::DocumentListWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::DocumentListWidget::staticMetaObject = {
    { &QListWidget::staticMetaObject, qt_meta_stringdata_ScIDE__DocumentListWidget,
      qt_meta_data_ScIDE__DocumentListWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::DocumentListWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::DocumentListWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::DocumentListWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__DocumentListWidget))
        return static_cast<void*>(const_cast< DocumentListWidget*>(this));
    return QListWidget::qt_metacast(_clname);
}

int ScIDE::DocumentListWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QListWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::DocumentListWidget::clicked(Document * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_ScIDE__DocumentsDocklet[] = {

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

static const char qt_meta_stringdata_ScIDE__DocumentsDocklet[] = {
    "ScIDE::DocumentsDocklet\0"
};

void ScIDE::DocumentsDocklet::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::DocumentsDocklet::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::DocumentsDocklet::staticMetaObject = {
    { &Docklet::staticMetaObject, qt_meta_stringdata_ScIDE__DocumentsDocklet,
      qt_meta_data_ScIDE__DocumentsDocklet, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::DocumentsDocklet::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::DocumentsDocklet::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::DocumentsDocklet::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__DocumentsDocklet))
        return static_cast<void*>(const_cast< DocumentsDocklet*>(this));
    return Docklet::qt_metacast(_clname);
}

int ScIDE::DocumentsDocklet::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Docklet::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
