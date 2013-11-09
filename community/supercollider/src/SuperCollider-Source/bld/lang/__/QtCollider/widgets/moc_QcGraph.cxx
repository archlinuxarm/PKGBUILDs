/****************************************************************************
** Meta object code from reading C++ file 'QcGraph.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcGraph.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcGraph.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcGraphModel[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   39,   39,   39, 0x05,
      40,   39,   39,   39, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_QcGraphModel[] = {
    "QcGraphModel\0appended(QcGraphElement*)\0"
    "\0removed(QcGraphElement*)\0"
};

void QcGraphModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcGraphModel *_t = static_cast<QcGraphModel *>(_o);
        switch (_id) {
        case 0: _t->appended((*reinterpret_cast< QcGraphElement*(*)>(_a[1]))); break;
        case 1: _t->removed((*reinterpret_cast< QcGraphElement*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcGraphModel::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcGraphModel::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QcGraphModel,
      qt_meta_data_QcGraphModel, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcGraphModel::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcGraphModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcGraphModel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcGraphModel))
        return static_cast<void*>(const_cast< QcGraphModel*>(this));
    return QObject::qt_metacast(_clname);
}

int QcGraphModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void QcGraphModel::appended(QcGraphElement * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QcGraphModel::removed(QcGraphElement * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
static const uint qt_meta_data_QcGraph[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
      25,   99, // properties
       1,  174, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
       8,   17,   17,   17, 0x05,
      18,   17,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
      31,   48,   17,   17, 0x0a,
      64,   76,   17,   17, 0x2a,
      82,   76,   17,   17, 0x0a,
      96,   17,   17,   17, 0x0a,
     110,  144,   17,   17, 0x08,

 // methods: signature, parameters, type, tag, flags
     146,  179,   17,   17, 0x02,
     181,  179,   17,   17, 0x02,
     206,  179,   17,   17, 0x02,
     233,  179,   17,   17, 0x02,
     257,  179,   17,   17, 0x02,
     283,  179,   17,   17, 0x02,
     308,  179,   17,   17, 0x02,
     332,  350,   17,   17, 0x02,
     360,  375,   17,   17, 0x02,
     380,  403,   17,   17, 0x02,

 // properties: name, type, flags
     410,  416, 0x0009510b,
     428,  416, 0x0009510b,
      76,  436, 0x02095103,
     440,  436, 0x02095001,
     450,  416, 0x00095009,
     467,  436, 0x02095103,
     477,  436, 0x02095103,
     488,  436, 0x02095103,
     500,  511, 0x43095103,
     518,  511, 0x43095103,
     530,  511, 0x43095103,
     540,  511, 0x43095103,
     550,  511, 0x43095103,
     561,  511, 0x43095103,
     576,  586, 0x01095103,
     591,  586, 0x01095103,
     601,  607, 0x0009500b,
     620,  586, 0x01095103,
     629,  634, 0x06095103,
     641,  436, 0x02095103,
     655,  436, 0x02095103,
     671,  673, 0x87095003,
     679,  673, 0x87095003,
     681,  686, 0x1a095103,
     694,  586, 0x01095103,

 // enums: name, flags, count, data
     607, 0x0,    2,  178,

 // enum data: key, value
     701, uint(QcGraph::DotElements),
     713, uint(QcGraph::RectElements),

       0        // eod
};

static const char qt_meta_stringdata_QcGraph[] = {
    "QcGraph\0action()\0\0metaAction()\0"
    "select(int,bool)\0index,exclusive\0"
    "select(int)\0index\0deselect(int)\0"
    "deselectAll()\0onElementRemoved(QcGraphElement*)\0"
    "e\0connectElements(int,VariantList)\0,\0"
    "setStringAt(int,QString)\0"
    "setFillColorAt(int,QColor)\0"
    "setEditableAt(int,bool)\0"
    "setThumbHeightAt(int,int)\0"
    "setThumbWidthAt(int,int)\0"
    "setThumbSizeAt(int,int)\0setCurves(double)\0"
    "curvature\0setCurves(int)\0type\0"
    "setCurves(VariantList)\0curves\0value\0"
    "VariantList\0strings\0int\0lastIndex\0"
    "selectionIndexes\0thumbSize\0thumbWidth\0"
    "thumbHeight\0background\0QColor\0strokeColor\0"
    "fillColor\0gridColor\0focusColor\0"
    "selectionColor\0drawLines\0bool\0drawRects\0"
    "style\0ElementStyle\0editable\0step\0"
    "double\0selectionForm\0horizontalOrder\0"
    "x\0float\0y\0grid\0QPointF\0gridOn\0DotElements\0"
    "RectElements\0"
};

void QcGraph::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcGraph *_t = static_cast<QcGraph *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->metaAction(); break;
        case 2: _t->select((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: _t->select((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->deselect((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->deselectAll(); break;
        case 6: _t->onElementRemoved((*reinterpret_cast< QcGraphElement*(*)>(_a[1]))); break;
        case 7: _t->connectElements((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< VariantList(*)>(_a[2]))); break;
        case 8: _t->setStringAt((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->setFillColorAt((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        case 10: _t->setEditableAt((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 11: _t->setThumbHeightAt((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 12: _t->setThumbWidthAt((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 13: _t->setThumbSizeAt((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 14: _t->setCurves((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 15: _t->setCurves((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 16: _t->setCurves((*reinterpret_cast< const VariantList(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcGraph::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcGraph::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcGraph,
      qt_meta_data_QcGraph, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcGraph::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcGraph::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcGraph::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcGraph))
        return static_cast<void*>(const_cast< QcGraph*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcGraph*>(this));
    if (!strcmp(_clname, "QtCollider::Style::Client"))
        return static_cast< QtCollider::Style::Client*>(const_cast< QcGraph*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcGraph::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< VariantList*>(_v) = value(); break;
        case 1: *reinterpret_cast< VariantList*>(_v) = dummyVariantList(); break;
        case 2: *reinterpret_cast< int*>(_v) = index(); break;
        case 3: *reinterpret_cast< int*>(_v) = lastIndex(); break;
        case 4: *reinterpret_cast< VariantList*>(_v) = selectionIndexes(); break;
        case 5: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 6: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 7: *reinterpret_cast< int*>(_v) = dummyInt(); break;
        case 8: *reinterpret_cast< QColor*>(_v) = background(); break;
        case 9: *reinterpret_cast< QColor*>(_v) = strokeColor(); break;
        case 10: *reinterpret_cast< QColor*>(_v) = dummyColor(); break;
        case 11: *reinterpret_cast< QColor*>(_v) = gridColor(); break;
        case 12: *reinterpret_cast< QColor*>(_v) = focusColor(); break;
        case 13: *reinterpret_cast< QColor*>(_v) = selectionColor(); break;
        case 14: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 15: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 16: *reinterpret_cast< ElementStyle*>(_v) = elementStyle(); break;
        case 17: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        case 18: *reinterpret_cast< double*>(_v) = step(); break;
        case 19: *reinterpret_cast< int*>(_v) = selectionForm(); break;
        case 20: *reinterpret_cast< int*>(_v) = horizontalOrder(); break;
        case 21: *reinterpret_cast< float*>(_v) = currentX(); break;
        case 22: *reinterpret_cast< float*>(_v) = currentY(); break;
        case 23: *reinterpret_cast< QPointF*>(_v) = grid(); break;
        case 24: *reinterpret_cast< bool*>(_v) = dummyBool(); break;
        }
        _id -= 25;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setValue(*reinterpret_cast< VariantList*>(_v)); break;
        case 1: setStrings(*reinterpret_cast< VariantList*>(_v)); break;
        case 2: setIndex(*reinterpret_cast< int*>(_v)); break;
        case 5: setThumbSize(*reinterpret_cast< int*>(_v)); break;
        case 6: setThumbWidth(*reinterpret_cast< int*>(_v)); break;
        case 7: setThumbHeight(*reinterpret_cast< int*>(_v)); break;
        case 8: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        case 9: setStrokeColor(*reinterpret_cast< QColor*>(_v)); break;
        case 10: setFillColor(*reinterpret_cast< QColor*>(_v)); break;
        case 11: setGridColor(*reinterpret_cast< QColor*>(_v)); break;
        case 12: setFocusColor(*reinterpret_cast< QColor*>(_v)); break;
        case 13: setSelectionColor(*reinterpret_cast< QColor*>(_v)); break;
        case 14: setDrawLines(*reinterpret_cast< bool*>(_v)); break;
        case 15: setDrawRects(*reinterpret_cast< bool*>(_v)); break;
        case 16: setElementStyle(*reinterpret_cast< ElementStyle*>(_v)); break;
        case 17: setEditable(*reinterpret_cast< bool*>(_v)); break;
        case 18: setStep(*reinterpret_cast< double*>(_v)); break;
        case 19: setSelectionForm(*reinterpret_cast< int*>(_v)); break;
        case 20: setHorizontalOrder(*reinterpret_cast< int*>(_v)); break;
        case 21: setCurrentX(*reinterpret_cast< float*>(_v)); break;
        case 22: setCurrentY(*reinterpret_cast< float*>(_v)); break;
        case 23: setGrid(*reinterpret_cast< QPointF*>(_v)); break;
        case 24: setGridOn(*reinterpret_cast< bool*>(_v)); break;
        }
        _id -= 25;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 25;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 25;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 25;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 25;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 25;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 25;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcGraph::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QcGraph::metaAction()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
