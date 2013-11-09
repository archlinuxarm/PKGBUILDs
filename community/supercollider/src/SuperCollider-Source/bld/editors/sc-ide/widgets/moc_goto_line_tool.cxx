/****************************************************************************
** Meta object code from reading C++ file 'goto_line_tool.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/goto_line_tool.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'goto_line_tool.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__GoToLineTool[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      20,   35,   46,   46, 0x05,

 // slots: signature, parameters, type, tag, flags
      47,   63,   46,   46, 0x0a,
      67,   46,   46,   46, 0x0a,
      78,   46,   46,   46, 0x08,
      83,   46,   46,   46, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__GoToLineTool[] = {
    "ScIDE::GoToLineTool\0activated(int)\0"
    "lineNumber\0\0setMaximum(int)\0max\0"
    "setFocus()\0go()\0onEditingFinished()\0"
};

void ScIDE::GoToLineTool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        GoToLineTool *_t = static_cast<GoToLineTool *>(_o);
        switch (_id) {
        case 0: _t->activated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->setMaximum((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->setFocus(); break;
        case 3: _t->go(); break;
        case 4: _t->onEditingFinished(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::GoToLineTool::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::GoToLineTool::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__GoToLineTool,
      qt_meta_data_ScIDE__GoToLineTool, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::GoToLineTool::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::GoToLineTool::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::GoToLineTool::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__GoToLineTool))
        return static_cast<void*>(const_cast< GoToLineTool*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::GoToLineTool::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::GoToLineTool::activated(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
