/****************************************************************************
** Meta object code from reading C++ file 'QcTreeWidget.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../QtCollider/widgets/QcTreeWidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QcTreeWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcTreeWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       2,   99, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   22,   22,   22, 0x05,
      23,   22,   22,   22, 0x05,
      43,   22,   22,   22, 0x05,

 // methods: signature, parameters, type, tag, flags
      64,   96,  109,   22, 0x02,
     131,   22,  109,   22, 0x02,
     165,   22,  200,   22, 0x02,
     204,  247,  109,   22, 0x02,
     259,  309,  109,   22, 0x02,
     327,   22,   22,   22, 0x02,
     361,   22,  392,   22, 0x02,
     404,  447,   22,   22, 0x02,
     456,  447,   22,   22, 0x02,
     499,  447,   22,   22, 0x02,
     546,  584,  592,   22, 0x02,
     601,  447,   22,   22, 0x02,
     656,  584,   22,   22, 0x02,
     700,  715,   22,   22, 0x02,

 // properties: name, type, flags
     733,  392, 0x0009510b,
     741,  109, 0x0009510b,

       0        // eod
};

static const char qt_meta_stringdata_QcTreeWidget[] = {
    "QcTreeWidget\0action()\0\0itemPressedAction()\0"
    "currentItemChanged()\0"
    "item(QcTreeWidget::ItemPtr,int)\0"
    "parent,index\0QcTreeWidget::ItemPtr\0"
    "parentItem(QcTreeWidget::ItemPtr)\0"
    "indexOfItem(QcTreeWidget::ItemPtr)\0"
    "int\0addItem(QcTreeWidget::ItemPtr,VariantList)\0"
    "parent,data\0"
    "insertItem(QcTreeWidget::ItemPtr,int,VariantList)\0"
    "parent,index,data\0removeItem(QcTreeWidget::ItemPtr)\0"
    "strings(QcTreeWidget::ItemPtr)\0"
    "VariantList\0setText(QcTreeWidget::ItemPtr,int,QString)\0"
    ",column,\0setColor(QcTreeWidget::ItemPtr,int,QColor)\0"
    "setTextColor(QcTreeWidget::ItemPtr,int,QColor)\0"
    "itemWidget(QcTreeWidget::ItemPtr,int)\0"
    ",column\0QWidget*\0"
    "setItemWidget(QcTreeWidget::ItemPtr,int,QObjectProxy*)\0"
    "removeItemWidget(QcTreeWidget::ItemPtr,int)\0"
    "sort(int,bool)\0column,descending\0"
    "columns\0currentItem\0"
};

void QcTreeWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcTreeWidget *_t = static_cast<QcTreeWidget *>(_o);
        switch (_id) {
        case 0: _t->action(); break;
        case 1: _t->itemPressedAction(); break;
        case 2: _t->currentItemChanged(); break;
        case 3: { QcTreeWidget::ItemPtr _r = _t->item((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QcTreeWidget::ItemPtr*>(_a[0]) = _r; }  break;
        case 4: { QcTreeWidget::ItemPtr _r = _t->parentItem((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QcTreeWidget::ItemPtr*>(_a[0]) = _r; }  break;
        case 5: { int _r = _t->indexOfItem((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 6: { QcTreeWidget::ItemPtr _r = _t->addItem((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< const VariantList(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QcTreeWidget::ItemPtr*>(_a[0]) = _r; }  break;
        case 7: { QcTreeWidget::ItemPtr _r = _t->insertItem((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const VariantList(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< QcTreeWidget::ItemPtr*>(_a[0]) = _r; }  break;
        case 8: _t->removeItem((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1]))); break;
        case 9: { VariantList _r = _t->strings((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< VariantList*>(_a[0]) = _r; }  break;
        case 10: _t->setText((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 11: _t->setColor((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QColor(*)>(_a[3]))); break;
        case 12: _t->setTextColor((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QColor(*)>(_a[3]))); break;
        case 13: { QWidget* _r = _t->itemWidget((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QWidget**>(_a[0]) = _r; }  break;
        case 14: _t->setItemWidget((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< QObjectProxy*(*)>(_a[3]))); break;
        case 15: _t->removeItemWidget((*reinterpret_cast< const QcTreeWidget::ItemPtr(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 16: _t->sort((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcTreeWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcTreeWidget::staticMetaObject = {
    { &QTreeWidget::staticMetaObject, qt_meta_stringdata_QcTreeWidget,
      qt_meta_data_QcTreeWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcTreeWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcTreeWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcTreeWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcTreeWidget))
        return static_cast<void*>(const_cast< QcTreeWidget*>(this));
    return QTreeWidget::qt_metacast(_clname);
}

int QcTreeWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeWidget::qt_metacall(_c, _id, _a);
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
        case 0: *reinterpret_cast< VariantList*>(_v) = columns(); break;
        case 1: *reinterpret_cast< QcTreeWidget::ItemPtr*>(_v) = currentItem(); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setColumns(*reinterpret_cast< VariantList*>(_v)); break;
        case 1: setCurrentItem(*reinterpret_cast< QcTreeWidget::ItemPtr*>(_v)); break;
        }
        _id -= 2;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcTreeWidget::action()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QcTreeWidget::itemPressedAction()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void QcTreeWidget::currentItemChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
