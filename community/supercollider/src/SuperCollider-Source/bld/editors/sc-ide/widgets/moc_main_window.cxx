/****************************************************************************
** Meta object code from reading C++ file 'main_window.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../editors/sc-ide/widgets/main_window.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScIDE__MainWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      49,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      18,   45,   53,   53, 0x05,
      54,   53,   53,   53, 0x25,

 // slots: signature, parameters, type, tag, flags
      76,   53,   53,   53, 0x0a,
      89,   53,   53,   53, 0x0a,
     112,   53,   53,   53, 0x0a,
     133,   53,   53,   53, 0x0a,
     147,   53,   53,   53, 0x0a,
     162,   53,   53,   53, 0x0a,
     177,   53,   53,   53, 0x0a,
     194,   53,   53,   53, 0x0a,
     213,   53,   53,   53, 0x0a,
     230,   53,   53,   53, 0x0a,
     246,   53,   53,   53, 0x0a,
     266,   53,   53,   53, 0x0a,
     280,   53,   53,   53, 0x0a,
     295,   53,   53,   53, 0x0a,
     313,   53,   53,   53, 0x0a,
     332,   53,   53,   53, 0x0a,
     346,   53,   53,   53, 0x0a,
     361,  388,   53,   53, 0x0a,
     395,   53,   53,   53, 0x08,
     413,   53,   53,   53, 0x08,
     440,  464,   53,   53, 0x08,
     472,  464,   53,   53, 0x08,
     494,   53,   53,   53, 0x08,
     544,  593,   53,   53, 0x08,
     638,  679,   53,   53, 0x08,
     701,   53,   53,   53, 0x08,
     710,   53,   53,   53, 0x08,
     746,   53,   53,   53, 0x08,
     785,   53,   53,   53, 0x08,
     807,   53,   53,   53, 0x08,
     830,   53,   53,   53, 0x08,
     858,   53,   53,   53, 0x08,
     888,   53,   53,   53, 0x08,
     908,   53,   53,   53, 0x08,
     927,   53,   53,   53, 0x08,
     950,   53,   53,   53, 0x08,
     982,   53,   53,   53, 0x08,
    1001,   53,   53,   53, 0x08,
    1029,   53,   53,   53, 0x08,
    1040,   53,   53,   53, 0x08,
    1059,   53,   53,   53, 0x08,
    1090,   53,   53,   53, 0x08,
    1112,   53,   53,   53, 0x08,
    1146,   53,   53,   53, 0x08,
    1180,   53,   53,   53, 0x08,
    1206,   53,   53,   53, 0x08,
    1218,   53,   53,   53, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScIDE__MainWindow[] = {
    "ScIDE::MainWindow\0evaluateCode(QString,bool)\0"
    ",silent\0\0evaluateCode(QString)\0"
    "newSession()\0saveCurrentSessionAs()\0"
    "openSessionsDialog()\0newDocument()\0"
    "openDocument()\0saveDocument()\0"
    "saveDocumentAs()\0saveAllDocuments()\0"
    "reloadDocument()\0closeDocument()\0"
    "closeAllDocuments()\0showCmdLine()\0"
    "showFindTool()\0showReplaceTool()\0"
    "showGoToLineTool()\0hideToolBox()\0"
    "showSettings()\0showStatusMessage(QString)\0"
    "string\0openStartupFile()\0"
    "openUserSupportDirectory()\0"
    "switchSession(Session*)\0session\0"
    "saveSession(Session*)\0"
    "onInterpreterStateChanged(QProcess::ProcessState)\0"
    "onServerStatusReply(int,int,int,int,float,float)\0"
    "ugens,synths,groups,synthDefs,avgCPU,peakCPU\0"
    "onServerRunningChanged(bool,QString,int)\0"
    "running,hostName,port\0onQuit()\0"
    "onCurrentDocumentChanged(Document*)\0"
    "onDocumentChangedExternally(Document*)\0"
    "onDocDialogFinished()\0updateRecentDocsMenu()\0"
    "onRecentDocAction(QAction*)\0"
    "onOpenSessionAction(QAction*)\0"
    "updateWindowTitle()\0toggleFullScreen()\0"
    "lookupImplementation()\0"
    "lookupImplementationForCursor()\0"
    "lookupReferences()\0lookupReferencesForCursor()\0"
    "openHelp()\0openHelpAboutIDE()\0"
    "lookupDocumentationForCursor()\0"
    "lookupDocumentation()\0"
    "applySettings(Settings::Manager*)\0"
    "storeSettings(Settings::Manager*)\0"
    "showSwitchSessionDialog()\0showAbout()\0"
    "showAboutQT()\0"
};

void ScIDE::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->evaluateCode((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: _t->evaluateCode((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->newSession(); break;
        case 3: _t->saveCurrentSessionAs(); break;
        case 4: _t->openSessionsDialog(); break;
        case 5: _t->newDocument(); break;
        case 6: _t->openDocument(); break;
        case 7: _t->saveDocument(); break;
        case 8: _t->saveDocumentAs(); break;
        case 9: _t->saveAllDocuments(); break;
        case 10: _t->reloadDocument(); break;
        case 11: _t->closeDocument(); break;
        case 12: _t->closeAllDocuments(); break;
        case 13: _t->showCmdLine(); break;
        case 14: _t->showFindTool(); break;
        case 15: _t->showReplaceTool(); break;
        case 16: _t->showGoToLineTool(); break;
        case 17: _t->hideToolBox(); break;
        case 18: _t->showSettings(); break;
        case 19: _t->showStatusMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->openStartupFile(); break;
        case 21: _t->openUserSupportDirectory(); break;
        case 22: _t->switchSession((*reinterpret_cast< Session*(*)>(_a[1]))); break;
        case 23: _t->saveSession((*reinterpret_cast< Session*(*)>(_a[1]))); break;
        case 24: _t->onInterpreterStateChanged((*reinterpret_cast< QProcess::ProcessState(*)>(_a[1]))); break;
        case 25: _t->onServerStatusReply((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< float(*)>(_a[5])),(*reinterpret_cast< float(*)>(_a[6]))); break;
        case 26: _t->onServerRunningChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 27: _t->onQuit(); break;
        case 28: _t->onCurrentDocumentChanged((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 29: _t->onDocumentChangedExternally((*reinterpret_cast< Document*(*)>(_a[1]))); break;
        case 30: _t->onDocDialogFinished(); break;
        case 31: _t->updateRecentDocsMenu(); break;
        case 32: _t->onRecentDocAction((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 33: _t->onOpenSessionAction((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 34: _t->updateWindowTitle(); break;
        case 35: _t->toggleFullScreen(); break;
        case 36: _t->lookupImplementation(); break;
        case 37: _t->lookupImplementationForCursor(); break;
        case 38: _t->lookupReferences(); break;
        case 39: _t->lookupReferencesForCursor(); break;
        case 40: _t->openHelp(); break;
        case 41: _t->openHelpAboutIDE(); break;
        case 42: _t->lookupDocumentationForCursor(); break;
        case 43: _t->lookupDocumentation(); break;
        case 44: _t->applySettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 45: _t->storeSettings((*reinterpret_cast< Settings::Manager*(*)>(_a[1]))); break;
        case 46: _t->showSwitchSessionDialog(); break;
        case 47: _t->showAbout(); break;
        case 48: _t->showAboutQT(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ScIDE::MainWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ScIDE::MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_ScIDE__MainWindow,
      qt_meta_data_ScIDE__MainWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ScIDE::MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ScIDE::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ScIDE::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScIDE__MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int ScIDE::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 49)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 49;
    }
    return _id;
}

// SIGNAL 0
void ScIDE::MainWindow::evaluateCode(const QString & _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
