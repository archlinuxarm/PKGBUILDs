/****************************************************************************
** Meta object code from reading C++ file 'view.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../QtCollider/widgets/soundfileview/view.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'view.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QcWaveform[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      32,   14, // methods
      23,  174, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,   29,   29,   29, 0x05,
      30,   29,   29,   29, 0x05,
      44,   29,   29,   29, 0x05,
      53,   29,   29,   29, 0x05,

 // slots: signature, parameters, type, tag, flags
      66,   81,   29,   29, 0x0a,
      90,  105,   29,   29, 0x0a,
     112,   29,   29,   29, 0x0a,
     125,  144,   29,   29, 0x0a,
     159,  176,   29,   29, 0x0a,
     182,  199,   29,   29, 0x0a,
     206,   81,   29,   29, 0x0a,
     227,   29,   29,   29, 0x0a,
     243,   29,   29,   29, 0x0a,
     257,  105,   29,   29, 0x0a,
     274,  291,   29,   29, 0x0a,
     299,   29,   29,   29, 0x0a,

 // methods: signature, parameters, type, tag, flags
     308,  322,   29,   29, 0x02,
     331,  353,   29,   29, 0x02,
     381,  415,   29,   29, 0x02,
     447,  477,   29,   29, 0x22,
     498,  524,   29,   29, 0x22,
     536,  558,   29,   29, 0x22,
     563,  585,   29,   29, 0x02,
     612,  630,   29,   29, 0x22,
     646,  199,   29,   29, 0x22,
     660,  524,   29,   29, 0x02,
     687,  702,  708,   29, 0x02,
     720,  750,   29,   29, 0x02,
     761,  795,   29,   29, 0x02,
     803,  795,   29,   29, 0x02,
     835,  866,   29,   29, 0x02,
     881,  911,   29,   29, 0x02,

 // properties: name, type, flags
     921,  934, 0x87095001,
     940,  951, 0x02095001,
     199,  951, 0x02095001,
     955,  966, 0x06095001,
     973,  966, 0x06095003,
     988,  934, 0x87095103,
     998,  951, 0x02095103,
    1015,  708, 0x00095009,
    1026,  934, 0x87095103,
    1032,  934, 0x87095103,
    1038, 1052, 0x01095103,
    1057, 1052, 0x01095103,
    1072,  951, 0x02095103,
    1087, 1052, 0x01095103,
    1099,  934, 0x87095103,
    1110,  934, 0x87095103,
    1125, 1052, 0x01095103,
    1139, 1150, 0x43095103,
    1157, 1150, 0x43095103,
    1167, 1150, 0x43095103,
    1176, 1150, 0x43095103,
    1188, 1150, 0x43095103,
    1198,  708, 0x0009510b,

       0        // eod
};

static const char qt_meta_stringdata_QcWaveform[] = {
    "QcWaveform\0loadProgress(int)\0\0"
    "loadingDone()\0action()\0metaAction()\0"
    "zoomTo(double)\0fraction\0zoomBy(double)\0"
    "factor\0zoomAllOut()\0zoomSelection(int)\0"
    "selectionIndex\0scrollTo(double)\0frame\0"
    "scrollBy(double)\0frames\0setScrollPos(double)\0"
    "scrollToStart()\0scrollToEnd()\0"
    "setYZoom(double)\0setXZoom(double)\0"
    "seconds\0redraw()\0load(QString)\0filename\0"
    "load(QString,int,int)\0filename,beginning,duration\0"
    "load(QVector<double>,int,int,int)\0"
    "data,offset,channels,samplerate\0"
    "load(QVector<double>,int,int)\0"
    "data,offset,channels\0load(QVector<double>,int)\0"
    "data,offset\0load(QVector<double>)\0"
    "data\0allocate(int,int,int)\0"
    "frames,channels,samplerate\0allocate(int,int)\0"
    "frames,channels\0allocate(int)\0"
    "write(QVector<double>,int)\0selection(int)\0"
    "index\0VariantList\0setSelection(int,VariantList)\0"
    "index,data\0setSelectionStart(int,sf_count_t)\0"
    "i,frame\0setSelectionEnd(int,sf_count_t)\0"
    "setSelectionEditable(int,bool)\0"
    "index,editable\0setSelectionColor(int,QColor)\0"
    "index,clr\0readProgress\0float\0startFrame\0"
    "int\0viewFrames\0double\0viewStartFrame\0"
    "scrollPos\0currentSelection\0selections\0"
    "yZoom\0xZoom\0cursorVisible\0bool\0"
    "cursorEditable\0cursorPosition\0gridVisible\0"
    "gridOffset\0gridResolution\0drawsWaveform\0"
    "background\0QColor\0peakColor\0rmsColor\0"
    "cursorColor\0gridColor\0waveColors\0"
};

void QcWaveform::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QcWaveform *_t = static_cast<QcWaveform *>(_o);
        switch (_id) {
        case 0: _t->loadProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->loadingDone(); break;
        case 2: _t->action(); break;
        case 3: _t->metaAction(); break;
        case 4: _t->zoomTo((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->zoomBy((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 6: _t->zoomAllOut(); break;
        case 7: _t->zoomSelection((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->scrollTo((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 9: _t->scrollBy((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 10: _t->setScrollPos((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 11: _t->scrollToStart(); break;
        case 12: _t->scrollToEnd(); break;
        case 13: _t->setYZoom((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 14: _t->setXZoom((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 15: _t->redraw(); break;
        case 16: _t->load((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->load((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 18: _t->load((*reinterpret_cast< const QVector<double>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 19: _t->load((*reinterpret_cast< const QVector<double>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 20: _t->load((*reinterpret_cast< const QVector<double>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 21: _t->load((*reinterpret_cast< const QVector<double>(*)>(_a[1]))); break;
        case 22: _t->allocate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 23: _t->allocate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 24: _t->allocate((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 25: _t->write((*reinterpret_cast< const QVector<double>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 26: { VariantList _r = _t->selection((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< VariantList*>(_a[0]) = _r; }  break;
        case 27: _t->setSelection((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< VariantList(*)>(_a[2]))); break;
        case 28: _t->setSelectionStart((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< sf_count_t(*)>(_a[2]))); break;
        case 29: _t->setSelectionEnd((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< sf_count_t(*)>(_a[2]))); break;
        case 30: _t->setSelectionEditable((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 31: _t->setSelectionColor((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QcWaveform::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QcWaveform::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcWaveform,
      qt_meta_data_QcWaveform, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QcWaveform::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QcWaveform::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QcWaveform::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QcWaveform))
        return static_cast<void*>(const_cast< QcWaveform*>(this));
    if (!strcmp(_clname, "QcHelper"))
        return static_cast< QcHelper*>(const_cast< QcWaveform*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcWaveform::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< float*>(_v) = loadProgress(); break;
        case 1: *reinterpret_cast< int*>(_v) = startFrame(); break;
        case 2: *reinterpret_cast< int*>(_v) = frames(); break;
        case 3: *reinterpret_cast< double*>(_v) = viewFrames(); break;
        case 4: *reinterpret_cast< double*>(_v) = viewStartFrame(); break;
        case 5: *reinterpret_cast< float*>(_v) = scrollPos(); break;
        case 6: *reinterpret_cast< int*>(_v) = currentSelection(); break;
        case 7: *reinterpret_cast< VariantList*>(_v) = selections(); break;
        case 8: *reinterpret_cast< float*>(_v) = yZoom(); break;
        case 9: *reinterpret_cast< float*>(_v) = xZoom(); break;
        case 10: *reinterpret_cast< bool*>(_v) = cursorVisible(); break;
        case 11: *reinterpret_cast< bool*>(_v) = cursorEditable(); break;
        case 12: *reinterpret_cast< int*>(_v) = cursorPosition(); break;
        case 13: *reinterpret_cast< bool*>(_v) = gridVisible(); break;
        case 14: *reinterpret_cast< float*>(_v) = gridOffset(); break;
        case 15: *reinterpret_cast< float*>(_v) = gridResolution(); break;
        case 16: *reinterpret_cast< bool*>(_v) = drawsWaveform(); break;
        case 17: *reinterpret_cast< QColor*>(_v) = background(); break;
        case 18: *reinterpret_cast< QColor*>(_v) = peakColor(); break;
        case 19: *reinterpret_cast< QColor*>(_v) = rmsColor(); break;
        case 20: *reinterpret_cast< QColor*>(_v) = cursorColor(); break;
        case 21: *reinterpret_cast< QColor*>(_v) = gridColor(); break;
        case 22: *reinterpret_cast< VariantList*>(_v) = waveColors(); break;
        }
        _id -= 23;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 4: scrollTo(*reinterpret_cast< double*>(_v)); break;
        case 5: setScrollPos(*reinterpret_cast< float*>(_v)); break;
        case 6: setCurrentSelection(*reinterpret_cast< int*>(_v)); break;
        case 8: setYZoom(*reinterpret_cast< float*>(_v)); break;
        case 9: setXZoom(*reinterpret_cast< float*>(_v)); break;
        case 10: setCursorVisible(*reinterpret_cast< bool*>(_v)); break;
        case 11: setCursorEditable(*reinterpret_cast< bool*>(_v)); break;
        case 12: setCursorPosition(*reinterpret_cast< int*>(_v)); break;
        case 13: setGridVisible(*reinterpret_cast< bool*>(_v)); break;
        case 14: setGridOffset(*reinterpret_cast< float*>(_v)); break;
        case 15: setGridResolution(*reinterpret_cast< float*>(_v)); break;
        case 16: setDrawsWaveform(*reinterpret_cast< bool*>(_v)); break;
        case 17: setBackground(*reinterpret_cast< QColor*>(_v)); break;
        case 18: setPeakColor(*reinterpret_cast< QColor*>(_v)); break;
        case 19: setRmsColor(*reinterpret_cast< QColor*>(_v)); break;
        case 20: setCursorColor(*reinterpret_cast< QColor*>(_v)); break;
        case 21: setGridColor(*reinterpret_cast< QColor*>(_v)); break;
        case 22: setWaveColors(*reinterpret_cast< VariantList*>(_v)); break;
        }
        _id -= 23;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 23;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 23;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 23;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 23;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 23;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 23;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void QcWaveform::loadProgress(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QcWaveform::loadingDone()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void QcWaveform::action()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void QcWaveform::metaAction()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
static const uint qt_meta_data_SoundCacheStream[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      17,   35,   35,   35, 0x05,
      36,   35,   35,   35, 0x05,

 // slots: signature, parameters, type, tag, flags
      50,   35,   35,   35, 0x08,
      70,   35,   35,   35, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_SoundCacheStream[] = {
    "SoundCacheStream\0loadProgress(int)\0\0"
    "loadingDone()\0onLoadProgress(int)\0"
    "onLoadingDone()\0"
};

void SoundCacheStream::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SoundCacheStream *_t = static_cast<SoundCacheStream *>(_o);
        switch (_id) {
        case 0: _t->loadProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->loadingDone(); break;
        case 2: _t->onLoadProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onLoadingDone(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SoundCacheStream::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SoundCacheStream::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SoundCacheStream,
      qt_meta_data_SoundCacheStream, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SoundCacheStream::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SoundCacheStream::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SoundCacheStream::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SoundCacheStream))
        return static_cast<void*>(const_cast< SoundCacheStream*>(this));
    if (!strcmp(_clname, "SoundStream"))
        return static_cast< SoundStream*>(const_cast< SoundCacheStream*>(this));
    return QObject::qt_metacast(_clname);
}

int SoundCacheStream::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void SoundCacheStream::loadProgress(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SoundCacheStream::loadingDone()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
static const uint qt_meta_data_SoundCacheLoader[] = {

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
      17,   35,   35,   35, 0x05,
      36,   35,   35,   35, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_SoundCacheLoader[] = {
    "SoundCacheLoader\0loadProgress(int)\0\0"
    "loadingDone()\0"
};

void SoundCacheLoader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SoundCacheLoader *_t = static_cast<SoundCacheLoader *>(_o);
        switch (_id) {
        case 0: _t->loadProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->loadingDone(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SoundCacheLoader::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SoundCacheLoader::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_SoundCacheLoader,
      qt_meta_data_SoundCacheLoader, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SoundCacheLoader::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SoundCacheLoader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SoundCacheLoader::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SoundCacheLoader))
        return static_cast<void*>(const_cast< SoundCacheLoader*>(this));
    return QThread::qt_metacast(_clname);
}

int SoundCacheLoader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
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
void SoundCacheLoader::loadProgress(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SoundCacheLoader::loadingDone()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
