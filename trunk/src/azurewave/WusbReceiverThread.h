/*
 * WusbReceiverThread.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-09-29
 */

#ifndef WUSBRECEIVERTHREAD_H_
#define WUSBRECEIVERTHREAD_H_

#include <QThread>

class WusbReceiverThread : public QThread {
public:
	WusbReceiverThread( QObject * parent );
	virtual ~WusbReceiverThread();
	void run();
};

#endif /* WUSBRECEIVERTHREAD_H_ */
