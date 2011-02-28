/*
 * WusbReceiverThread.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-09-29
 */

#include "WusbReceiverThread.h"

WusbReceiverThread::WusbReceiverThread( QObject * parent )
: QThread( parent ) {
	start();
}

WusbReceiverThread::~WusbReceiverThread() {
}

void WusbReceiverThread::run() {
	exec();
}
