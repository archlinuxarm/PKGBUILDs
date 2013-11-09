/****************************************************************************
** Meta object code from reading C++ file 'sc_editor.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/code_editor/sc_editor.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sc_editor.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__ScCodeEditor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      20,   54,   54,   54, 0x0a,
      55,   76,   54,   54, 0x0a,
      79,   54,   54,   54, 0x0a,
      88,   54,   54,   54, 0x0a,
     112,   54,   54,   54, 0x0a,
     135,   54,   54,   54, 0x0a,
     151,   54,   54,   54, 0x0a,
     171,   54,   54,   54, 0x0a,
     187,   54,   54,   54, 0x0a,
     213,   54,   54,   54, 0x0a,
     235,   54,   54,   54, 0x0a,
     252,   54,   54,   54, 0x0a,
     273,   54,  293,   54, 0x0a,
     298,   54,   54,   54, 0x0a,
     315,   54,   54,   54, 0x0a,
     332,   54,   54,   54, 0x0a,
     347,   54,   54,   54, 0x0a,
     364,   54,   54,   54, 0x0a,
     383,   54,   54,   54, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__ScCodeEditor[] = {
    "ScIDE::ScCodeEditor\0"
    "applySettings(Settings::Manager*)\0\0"
    "setSpaceIndent(bool)\0on\0indent()\0"
    "triggerAutoCompletion()\0triggerMethodCallAid()\0"
    "toggleComment()\0gotoPreviousBlock()\0"
    "gotoNextBlock()\0selectBlockAroundCursor()\0"
    "selectCurrentRegion()\0gotoNextRegion()\0"
    "gotoPreviousRegion()\0openDocumentation()\0"
    "bool\0openDefinition()\0findReferences()\0"
    "evaluateLine()\0evaluateRegion()\0"
    "evaluateDocument()\0matchBrackets()\0"
};

void ScIDE::ScCodeEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ScCodeEditor *_t = static_cast<ScCodeEditor *>(_o);
        switch (_id) {
        case 0: _t->applySettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 1: _t->setSpaceIndent((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->indent(); break;
        case 3: _t->triggerAutoCompletion(); break;
        case 4: _t->triggerMethodCallAid(); break;
        case 5: _t->toggleComment(); break;
        case 6: _t->gotoPreviousBlock(); break;
        case 7: _t->gotoNextBlock(); break;
        case 8: _t->selectBlockAroundCursor(); break;
        case 9: _t->selectCurrentRegion(); break;
        case 10: _t->gotoNextRegion(); break;
        case 11: _t->gotoPreviousRegion(); break;
        case 12: { bool _r = _t->openDocumentation();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 13: _t->openDefinition(); break;
        case 14: _t->findReferences(); break;
        case 15: _t->evaluateLine(); break;
        case 16: _t->evaluateRegion(); break;
        case 17: _t->evaluateDocument(); break;
        case 18: _t->matchBrackets(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::ScCodeEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::ScCodeEditor::staticMetaObject = {
    { &GenericCodeEditor::staticMetaObject, qt_meta_stringdata_ScIDE__ScCodeEditor,
      qt_meta_data_ScIDE__ScCodeEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::ScCodeEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::ScCodeEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::ScCodeEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__ScCodeEditor))
        return static_cast<void*>(const_cast< ScCodeEditor*>(this));
    return GenericCodeEditor::qt_metacast(_clname);
}

int ScIDE::ScCodeEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = GenericCodeEditor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
