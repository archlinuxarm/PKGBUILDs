/****************************************************************************
** Meta object code from reading C++ file 'docklet.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/util/docklet.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'docklet.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__DockletToolBar[] = {

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

static const char qt_meta_stringdata_ScIDE__DockletToolBar[] = {
    "ScIDE::DockletToolBar\0"
};

void ScIDE::DockletToolBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::DockletToolBar::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::DockletToolBar::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__DockletToolBar,
      qt_meta_data_ScIDE__DockletToolBar, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::DockletToolBar::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::DockletToolBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::DockletToolBar::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__DockletToolBar))
        return static_cast<void*>(const_cast< DockletToolBar*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::DockletToolBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_ScIDE__Docklet[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   32,   32,   32, 0x0a,
      33,   32,   32,   32, 0x0a,
      50,   67,   32,   32, 0x0a,
      75,   32,   32,   32, 0x0a,
      82,   32,   32,   32, 0x0a,
      89,   32,   32,   32, 0x0a,
      97,   32,   32,   32, 0x0a,
     105,   32,   32,   32, 0x0a,
     113,  164,   32,   32, 0x08,
     173,   32,   32,   32, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__Docklet[] = {
    "ScIDE::Docklet\0toggleFloating()\0\0"
    "toggleDetached()\0setVisible(bool)\0"
    "visible\0show()\0hide()\0close()\0raise()\0"
    "focus()\0onFeaturesChanged(QDockWidget::DockWidgetFeatures)\0"
    "features\0updateDockAction()\0"
};

void ScIDE::Docklet::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Docklet *_t = static_cast<Docklet *>(_o);
        switch (_id) {
        case 0: _t->toggleFloating(); break;
        case 1: _t->toggleDetached(); break;
        case 2: _t->setVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->show(); break;
        case 4: _t->hide(); break;
        case 5: _t->close(); break;
        case 6: _t->raise(); break;
        case 7: _t->focus(); break;
        case 8: _t->onFeaturesChanged((*reinterpret_cast< QDockWidget::DockWidgetFeatures(*)>(_a[1]))); break;
        case 9: _t->updateDockAction(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::Docklet::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::Docklet::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__Docklet,
      qt_meta_data_ScIDE__Docklet, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::Docklet::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::Docklet::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::Docklet::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__Docklet))
        return static_cast<void*>(const_cast< Docklet*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::Docklet::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
