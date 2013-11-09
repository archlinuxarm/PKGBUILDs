/****************************************************************************
** Meta object code from reading C++ file 'QcTextEdit.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcTextEdit.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcTextEdit.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcTextEdit[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
      11,   24, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,   30,   35,   35, 0x05,

 // methods: signature, parameters, type, tag, flags
      36,   52,   35,   35, 0x02,

 // properties: name, type, flags
      63,   72, 0x0a095103,
      80,   95, 0x02095001,
      99,   95, 0x02095001,
     113,   72, 0x0a095003,
     128,   72, 0x0a095001,
     140,  149, 0x40095103,
     155,  165, 0x43095103,
     172,  183, 0x0009510b,
     195,  183, 0x0009510b,
     205,  183, 0x0009510b,
     215,  240, 0x01095003,

       0        // eod
};

static const char qt_meta_stringdata_QcTextEdit[] = {
    "QcTextEdit\0interpret(QString)\0code\0\0"
    "select(int,int)\0start,size\0document\0"
    "QString\0selectionStart\0int\0selectionSize\0"
    "selectedString\0currentLine\0textFont\0"
    "QFont\0textColor\0QColor\0rangeColor\0"
    "VariantList\0rangeFont\0rangeText\0"
    "enterInterpretsSelection\0bool\0"
};

void QcTextEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcTextEdit *_t = static_cast<QcTextEdit *>(_o);
        switch (_id) {
        case 0: _t->interpret((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->select((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcTextEdit::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcTextEdit::staticMetaObject = {
    { &QTextEdit::staticMetaObject, qt_meta_stringdata_QcTextEdit,
      qt_meta_data_QcTextEdit, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcTextEdit::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcTextEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcTextEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcTextEdit))
        return static_cast<void*>(const_cast< QcTextEdit*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcTextEdit*>(this));
    return QTextEdit::qt_metacast(_clname);
}

int QcTextEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = documentFilename(); break;
        case 1: *reinterpret_cast< int*>(_v) = selectionStart(); break;
        case 2: *reinterpret_cast< int*>(_v) = selectionSize(); break;
        case 3: *reinterpret_cast< QString*>(_v) = selectedString(); break;
        case 4: *reinterpret_cast< QString*>(_v) = currentLine(); break;
        case 5: *reinterpret_cast< QFont*>(_v) = dummyFont(); break;
        case 6: *reinterpret_cast< QColor*>(_v) = dummyColor(); break;
        case 7: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 8: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 9: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 10: *reinterpret_cast< bool*>(_v) = interpretSelection(); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setDocument(*reinterpret_cast< QString*>(_v)); break;
        case 3: replaceSelectedText(*reinterpret_cast< QString*>(_v)); break;
        case 5: setTextFont(*reinterpret_cast< QFont*>(_v)); break;
        case 6: setTextColor(*reinterpret_cast< QColor*>(_v)); break;
        case 7: setRangeColor(*reinterpret_cast< VariantList*>(_v)); break;
        case 8: setRangeFont(*reinterpret_cast< VariantList*>(_v)); break;
        case 9: setRangeText(*reinterpret_cast< VariantList*>(_v)); break;
        case 10: setInterpretSelection(*reinterpret_cast< bool*>(_v)); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 11;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcTextEdit::interpret(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
