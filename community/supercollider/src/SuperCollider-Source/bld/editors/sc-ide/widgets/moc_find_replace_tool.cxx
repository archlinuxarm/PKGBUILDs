/****************************************************************************
** Meta object code from reading C++ file 'find_replace_tool.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/find_replace_tool.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'find_replace_tool.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__TextFindReplacePanel[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      28,   39,   39,   39, 0x0a,
      40,   39,   39,   39, 0x0a,
      55,   39,   39,   39, 0x0a,
      65,   39,   39,   39, 0x0a,
      75,   39,   39,   39, 0x0a,
      88,   39,   39,   39, 0x08,
     108,   39,   39,   39, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__TextFindReplacePanel[] = {
    "ScIDE::TextFindReplacePanel\0findNext()\0"
    "\0findPrevious()\0findAll()\0replace()\0"
    "replaceAll()\0onFindFieldReturn()\0"
    "onFindFieldTextChanged()\0"
};

void ScIDE::TextFindReplacePanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TextFindReplacePanel *_t = static_cast<TextFindReplacePanel *>(_o);
        switch (_id) {
        case 0: _t->findNext(); break;
        case 1: _t->findPrevious(); break;
        case 2: _t->findAll(); break;
        case 3: _t->replace(); break;
        case 4: _t->replaceAll(); break;
        case 5: _t->onFindFieldReturn(); break;
        case 6: _t->onFindFieldTextChanged(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::TextFindReplacePanel::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::TextFindReplacePanel::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__TextFindReplacePanel,
      qt_meta_data_ScIDE__TextFindReplacePanel, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::TextFindReplacePanel::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::TextFindReplacePanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::TextFindReplacePanel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__TextFindReplacePanel))
        return static_cast<void*>(const_cast< TextFindReplacePanel*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::TextFindReplacePanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
