/****************************************************************************
** Meta object code from reading C++ file 'highlighter.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/code_editor/highlighter.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'highlighter.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__SyntaxHighlighterGlobals[] = {

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
      32,   55,   55,   55, 0x05,

 // slots: signature, parameters, type, tag, flags
      56,   55,   55,   55, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__SyntaxHighlighterGlobals[] = {
    "ScIDE::SyntaxHighlighterGlobals\0"
    "syntaxFormatsChanged()\0\0"
    "applySettings(Settings::Manager*)\0"
};

void ScIDE::SyntaxHighlighterGlobals::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SyntaxHighlighterGlobals *_t = static_cast<SyntaxHighlighterGlobals *>(_o);
        switch (_id) {
        case 0: _t->syntaxFormatsChanged(); break;
        case 1: _t->applySettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::SyntaxHighlighterGlobals::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SyntaxHighlighterGlobals::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ScIDE__SyntaxHighlighterGlobals,
      qt_meta_data_ScIDE__SyntaxHighlighterGlobals, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SyntaxHighlighterGlobals::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SyntaxHighlighterGlobals::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SyntaxHighlighterGlobals::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SyntaxHighlighterGlobals))
        return static_cast<void*>(const_cast< SyntaxHighlighterGlobals*>(this));
    return QObject::qt_metacast(_clname);
}

int ScIDE::SyntaxHighlighterGlobals::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void ScIDE::SyntaxHighlighterGlobals::syntaxFormatsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
static const uint qt_meta_data_ScIDE__SyntaxHighlighter[] = {

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

static const char qt_meta_stringdata_ScIDE__SyntaxHighlighter[] = {
    "ScIDE::SyntaxHighlighter\0"
};

void ScIDE::SyntaxHighlighter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData ScIDE::SyntaxHighlighter::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::SyntaxHighlighter::staticMetaObject = {
    { &QSyntaxHighlighter::staticMetaObject, qt_meta_stringdata_ScIDE__SyntaxHighlighter,
      qt_meta_data_ScIDE__SyntaxHighlighter, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::SyntaxHighlighter::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::SyntaxHighlighter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::SyntaxHighlighter::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__SyntaxHighlighter))
        return static_cast<void*>(const_cast< SyntaxHighlighter*>(this));
    return QSyntaxHighlighter::qt_metacast(_clname);
}

int ScIDE::SyntaxHighlighter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSyntaxHighlighter::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
