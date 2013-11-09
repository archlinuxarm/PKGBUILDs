/****************************************************************************
** Meta object code from reading C++ file 'editor_page.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/settings/editor_page.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'editor_page.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__Settings__EditorPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      28,   43,   43,   43, 0x0a,
      44,   43,   43,   43, 0x0a,
      60,   43,   43,   43, 0x08,
      85,   43,   43,   43, 0x08,
     109,   43,   43,   43, 0x08,
     129,   43,   43,   43, 0x08,
     152,   43,   43,   43, 0x08,
     178,   43,   43,   43, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__Settings__EditorPage[] = {
    "ScIDE::Settings::EditorPage\0load(Manager*)\0"
    "\0store(Manager*)\0onCurrentTabChanged(int)\0"
    "onMonospaceToggle(bool)\0updateFontPreview()\0"
    "updateTextFormatEdit()\0updateTextFormatDisplay()\0"
    "updateTextFormatDisplayCommons()\0"
};

void ScIDE::Settings::EditorPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        EditorPage *_t = static_cast<EditorPage *>(_o);
        switch (_id) {
        case 0: _t->load((*reinterpret_cast< Manager*(*)>(_a[1]))); break;
        case 1: _t->store((*reinterpret_cast< Manager*(*)>(_a[1]))); break;
        case 2: _t->onCurrentTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onMonospaceToggle((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->updateFontPreview(); break;
        case 5: _t->updateTextFormatEdit(); break;
        case 6: _t->updateTextFormatDisplay(); break;
        case 7: _t->updateTextFormatDisplayCommons(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::Settings::EditorPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::Settings::EditorPage::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__Settings__EditorPage,
      qt_meta_data_ScIDE__Settings__EditorPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::Settings::EditorPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::Settings::EditorPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::Settings::EditorPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__Settings__EditorPage))
        return static_cast<void*>(const_cast< EditorPage*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::Settings::EditorPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
