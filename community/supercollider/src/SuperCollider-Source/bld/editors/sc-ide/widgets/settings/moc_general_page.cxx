/****************************************************************************
** Meta object code from reading C++ file 'general_page.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../editors/sc-ide/widgets/settings/general_page.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'general_page.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__Settings__GeneralPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      29,   44,   44,   44, 0x0a,
      45,   44,   44,   44, 0x0a,
      61,   96,   44,   44, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__Settings__GeneralPage[] = {
    "ScIDE::Settings::GeneralPage\0"
    "load(Manager*)\0\0store(Manager*)\0"
    "onStartSessionNameChanged(QString)\0"
    "text\0"
};

void ScIDE::Settings::GeneralPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        GeneralPage *_t = static_cast<GeneralPage *>(_o);
        switch (_id) {
        case 0: _t->load((*reinterpret_cast< Manager*(*)>(_a[1]))); break;
        case 1: _t->store((*reinterpret_cast< Manager*(*)>(_a[1]))); break;
        case 2: _t->onStartSessionNameChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::Settings::GeneralPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::Settings::GeneralPage::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ScIDE__Settings__GeneralPage,
      qt_meta_data_ScIDE__Settings__GeneralPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::Settings::GeneralPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::Settings::GeneralPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::Settings::GeneralPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__Settings__GeneralPage))
        return static_cast<void*>(const_cast< GeneralPage*>(this));
    return QWidget::qt_metacast(_clname);
}

int ScIDE::Settings::GeneralPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
