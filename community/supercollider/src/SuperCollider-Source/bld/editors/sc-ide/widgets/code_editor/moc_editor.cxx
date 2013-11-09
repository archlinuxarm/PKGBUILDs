/****************************************************************************
** Meta object code from reading C++ file 'editor.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/code_editor/editor.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'editor.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__GenericCodeEditor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      25,   59,   59,   59, 0x0a,
      60,   72,   59,   59, 0x0a,
      78,   59,   59,   59, 0x2a,
      87,   72,   59,   59, 0x0a,
     100,   59,   59,   59, 0x2a,
     110,   59,   59,   59, 0x0a,
     126,   59,   59,   59, 0x0a,
     150,   59,   59,   59, 0x0a,
     176,   59,   59,   59, 0x0a,
     198,   59,   59,   59, 0x0a,
     211,   59,   59,   59, 0x0a,
     226,   59,   59,   59, 0x0a,
     239,   59,   59,   59, 0x0a,
     254,   59,   59,   59, 0x0a,
     278,   59,   59,   59, 0x0a,
     298,   59,   59,   59, 0x09,
     313,  344,   59,   59, 0x09,
     346,   59,   59,   59, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__GenericCodeEditor[] = {
    "ScIDE::GenericCodeEditor\0"
    "applySettings(Settings::Manager*)\0\0"
    "zoomIn(int)\0steps\0zoomIn()\0zoomOut(int)\0"
    "zoomOut()\0resetFontSize()\0"
    "setShowWhitespace(bool)\0"
    "clearSearchHighlighting()\0"
    "toggleOverwriteMode()\0copyLineUp()\0"
    "copyLineDown()\0moveLineUp()\0moveLineDown()\0"
    "gotoPreviousEmptyLine()\0gotoNextEmptyLine()\0"
    "updateLayout()\0updateLineIndicator(QRect,int)\0"
    ",\0onDocumentFontChanged()\0"
};

void ScIDE::GenericCodeEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        GenericCodeEditor *_t = static_cast<GenericCodeEditor *>(_o);
        switch (_id) {
        case 0: _t->applySettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 1: _t->zoomIn((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->zoomIn(); break;
        case 3: _t->zoomOut((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->zoomOut(); break;
        case 5: _t->resetFontSize(); break;
        case 6: _t->setShowWhitespace((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->clearSearchHighlighting(); break;
        case 8: _t->toggleOverwriteMode(); break;
        case 9: _t->copyLineUp(); break;
        case 10: _t->copyLineDown(); break;
        case 11: _t->moveLineUp(); break;
        case 12: _t->moveLineDown(); break;
        case 13: _t->gotoPreviousEmptyLine(); break;
        case 14: _t->gotoNextEmptyLine(); break;
        case 15: _t->updateLayout(); break;
        case 16: _t->updateLineIndicator((*reinterpret_cast< QRect(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 17: _t->onDocumentFontChanged(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::GenericCodeEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::GenericCodeEditor::staticMetaObject = {
    { &QPlainTextEdit::staticMetaObject, qt_meta_stringdata_ScIDE__GenericCodeEditor,
      qt_meta_data_ScIDE__GenericCodeEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::GenericCodeEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::GenericCodeEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::GenericCodeEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__GenericCodeEditor))
        return static_cast<void*>(const_cast< GenericCodeEditor*>(this));
    return QPlainTextEdit::qt_metacast(_clname);
}

int ScIDE::GenericCodeEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPlainTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
