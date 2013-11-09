/************************************************************************
*
* Copyright 2010-2012 Jakob Leben (jakob.leben@gmail.com)
*
* This file is part of SuperCollider Qt GUI.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
************************************************************************/

#include "primitives.h"
#include "../Slot.h"
#include "../QWidgetProxy.h"
#include "../Common.h"

#include <PyrKernel.h>
#include <SCBase.h>

#include <QWidget>
#include <QThread>
#include <QApplication>
#include <QDrag>
#include <QMimeData>

// WARNING these primitives have to always execute asynchronously, or Cocoa language client will
// hang.

namespace QtCollider {

QC_LANG_PRIMITIVE( QWidget_SetFocus, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  QWidgetProxy *proxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy( r ) );

  QApplication::postEvent( proxy, new SetFocusEvent( IsTrue(a) ) );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_BringFront, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *proxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy( r ) );

  QApplication::postEvent( proxy,
                           new QEvent( (QEvent::Type) QtCollider::Event_Proxy_BringFront ) );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_Refresh, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *proxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy( r ) );

  if( !proxy->compareThread() ) return QtCollider::wrongThreadError();

  proxy->refresh();

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_MapToGlobal, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *proxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy( r ) );

  if( !proxy->compareThread() ) return QtCollider::wrongThreadError();

  QWidget *w = proxy->widget();
  if( !w ) return errNone;

  QPoint pt( Slot::toPoint( a ).toPoint() );
  pt = w->mapToGlobal( pt );
  Slot::setPoint( r, pt );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_SetLayout, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  if( !isKindOfSlot( a, SC_CLASS(QLayout) ) ) return errWrongType;

  QWidgetProxy *wProxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy(r) );

  if( !wProxy->compareThread() ) return QtCollider::wrongThreadError();

  QObjectProxy *lProxy = Slot::toObjectProxy( a );
  wProxy->setLayout( lProxy );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_GetAlwaysOnTop, 0, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *wProxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy(r) );

  if( QThread::currentThread() != wProxy->thread() ) return errFailed;

  SetBool( r, wProxy->alwaysOnTop() );
  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_SetAlwaysOnTop, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *wProxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy(r) );

  QApplication::postEvent( wProxy, new SetAlwaysOnTopEvent( IsTrue(a) ) );

  return errNone;
}

struct MimeData : public QMimeData {
  virtual ~MimeData() {
    qcDebugMsg(1,"Drag data object destroyed, clearing QView.currentDrag.");

    QtCollider::lockLang();

    PyrClass *classView = getsym("View")->u.classobj;
    PyrSymbol *symClearDrag = getsym("prClearCurrentDrag");
    if( !classView || !symClearDrag ) return;

    QtCollider::runLang( classView, symClearDrag );

    QtCollider::unlockLang();
  }
};

QC_LANG_PRIMITIVE( QWidget_StartDrag, 3, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
    qcDebugMsg(1, "Starting drag...");

  QWidgetProxy *wProxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy(r) );
  if( !wProxy->compareThread() ) return QtCollider::wrongThreadError();

  PyrSlot *data = a+1;
  QString str = Slot::toString(a+2);
  QString label = Slot::toString(a);

  QMimeData *mime = new QtCollider::MimeData;

  mime->setData( "application/supercollider", QByteArray() );

  QColor color( Slot::toColor(data) );
  if( color.isValid() )
    mime->setColorData( QVariant(color) );

  if( !str.isEmpty() )
    mime->setText( str );

  wProxy->setDragData( mime, label );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_SetGlobalEventEnabled, 2, PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  if( NotInt( a+0 ) ) return errWrongType;
  int event = Slot::toInt(a+0);
  bool enabled = IsTrue(a+1);
  if( !enabled && !IsFalse(a+1) ) return errWrongType;

  QWidgetProxy::setGlobalEventEnabled( (QWidgetProxy::GlobalEvent) event, enabled );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_SetAcceptsMouse, 1,  PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *proxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy(r) );
  if( !proxy->compareThread() ) return QtCollider::wrongThreadError();

  QWidget *w = proxy->widget();
  if( !w ) return errNone;

  bool accept = IsTrue(a);
  w->setAttribute( Qt::WA_TransparentForMouseEvents, !accept );

  return errNone;
}

QC_LANG_PRIMITIVE( QWidget_AcceptsMouse, 1,  PyrSlot *r, PyrSlot *a, VMGlobals *g ) {
  QWidgetProxy *proxy = qobject_cast<QWidgetProxy*>( Slot::toObjectProxy(r) );
  if( !proxy->compareThread() ) return QtCollider::wrongThreadError();

  QWidget *w = proxy->widget();
  if( !w ) return errNone;

  bool accept = !w->testAttribute( Qt::WA_TransparentForMouseEvents );
  SetBool(r, accept);

  return errNone;
}

void defineQWidgetPrimitives()
{
  LangPrimitiveDefiner definer;
  definer.define<QWidget_SetFocus>();
  definer.define<QWidget_BringFront>();
  definer.define<QWidget_Refresh>();
  definer.define<QWidget_MapToGlobal>();
  definer.define<QWidget_SetLayout>();
  definer.define<QWidget_GetAlwaysOnTop>();
  definer.define<QWidget_SetAlwaysOnTop>();
  definer.define<QWidget_StartDrag>();
  definer.define<QWidget_SetGlobalEventEnabled>();
  definer.define<QWidget_AcceptsMouse>();
  definer.define<QWidget_SetAcceptsMouse>();
}

} // namespace QtCollider
