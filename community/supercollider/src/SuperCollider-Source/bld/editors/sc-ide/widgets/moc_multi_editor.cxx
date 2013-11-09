/****************************************************************************
** Meta object code from reading C++ file 'multi_editor.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/multi_editor.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'multi_editor.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__MultiEditor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      19,   53,   53,   53, 0x05,

 // slots: signature, parameters, type, tag, flags
      54,   53,   53,   53, 0x0a,
      76,   53,   53,   53, 0x0a,
      95,   53,   53,   53, 0x0a,
     118,   53,   53,   53, 0x0a,
     135,   53,   53,   53, 0x0a,
     155,   53,   53,   53, 0x0a,
     173,   53,   53,   53, 0x0a,
     194,   53,   53,   53, 0x0a,
     212,  238,   53,   53, 0x08,
     277,   53,   53,   53, 0x08,
     296,  320,   53,   53, 0x08,
     352,  372,   53,   53, 0x28,
     388,   53,   53,   53, 0x28,
     404,   53,   53,   53, 0x08,
     422,  442,   53,   53, 0x08,
     448,  442,   53,   53, 0x08,
     473,   53,   53,   53, 0x08,
     516,   53,   53,   53, 0x08,
     547,   53,   53,   53, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__MultiEditor[] = {
    "ScIDE::MultiEditor\0currentDocumentChanged(Document*)\0"
    "\0setCurrent(Document*)\0showNextDocument()\0"
    "showPreviousDocument()\0switchDocument()\0"
    "splitHorizontally()\0splitVertically()\0"
    "removeCurrentSplit()\0removeAllSplits()\0"
    "onOpen(Document*,int,int)\0"
    ",initialCursorPosition,selectionLength\0"
    "onClose(Document*)\0show(Document*,int,int)\0"
    ",cursorPosition,selectionLenght\0"
    "show(Document*,int)\0,cursorPosition\0"
    "show(Document*)\0update(Document*)\0"
    "onCloseRequest(int)\0index\0"
    "onCurrentTabChanged(int)\0"
    "onCurrentEditorChanged(GenericCodeEditor*)\0"
    "onBoxActivated(CodeEditorBox*)\0"
    "onDocModified(QObject*)\0"
};

void ScIDE::MultiEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MultiEditor *_t = static_cast<MultiEditor *>(_o);
        switch (_id) {
        case 0: _t->currentDocumentChanged((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 1: _t->setCurrent((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 2: _t->showNextDocument(); break;
        case 3: _t->showPreviousDocument(); break;
        case 4: _t->switchDocument(); break;
        case 5: _t->splitHorizontally(); break;
        case 6: _t->splitVertically(); break;
        case 7: _t->removeCurrentSplit(); break;
        case 8: _t->removeAllSplits(); break;
        case 9: _t->onOpen((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 10: _t->onClose((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 11: _t->show((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 12: _t->show((*reinterpret_cast< Document*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 13: _t->show((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 14: _t->update((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 15: _t->onCloseRequest((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 16: _t->onCurrentTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: _t->onCurrentEditorChanged((*reinterpret_cast< GenericCodeEditor*(*)>(_a[1]))); break;
        case 18: _t->onBoxActivated((*reinterpret_cast< CodeEditorBox*(*)>(_a[1]))); break;
        case 19: _t->onDocModified((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::MultiEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::MultiEditor::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__MultiEditor,
      qt_meta_data_ScIDE__MultiEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::MultiEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::MultiEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::MultiEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__MultiEditor))
        return static_cast<void*>(const_cast< MultiEditor*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::MultiEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::MultiEditor::currentDocumentChanged(Document * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
