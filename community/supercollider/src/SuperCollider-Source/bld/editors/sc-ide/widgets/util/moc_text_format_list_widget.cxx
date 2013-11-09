/****************************************************************************
** Meta object code from reading C++ file 'text_format_list_widget.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/util/text_format_list_widget.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'text_format_list_widget.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__TextFormatListWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      28,   41,   47,   47, 0x05,

 // slots: signature, parameters, type, tag, flags
      48,   47,   47,   47, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__TextFormatListWidget[] = {
    "ScIDE::TextFormatListWidget\0clicked(int)\0"
    "index\0\0onDoubleClicked(QModelIndex)\0"
};

void ScIDE::TextFormatListWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TextFormatListWidget *_t = static_cast<TextFormatListWidget *>(_o);
        switch (_id) {
        case 0: _t->clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->onDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::TextFormatListWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::TextFormatListWidget::staticMetaObject = {
    { &QTreeView::staticMetaObject, qt_meta_stringdata_ScIDE__TextFormatListWidget,
      qt_meta_data_ScIDE__TextFormatListWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::TextFormatListWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::TextFormatListWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::TextFormatListWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__TextFormatListWidget))
        return static_cast<void*>(const_cast< TextFormatListWidget*>(this));
    return QTreeView::qt_metacast(_clname);
}

int ScIDE::TextFormatListWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeView::qt_metacall(_c, _id, _a);
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
void ScIDE::TextFormatListWidget::clicked(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
