/*
 * LogConsoleAppender.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-11-16
 */

#ifndef LOGCONSOLEAPPENDER_H_
#define LOGCONSOLEAPPENDER_H_

#include "LogAppender.h"
#include <QMutex>

class Logger;

class LogConsoleAppender: public LogAppender {
	friend class Logger;
public:
	virtual ~LogConsoleAppender();
	static void setEnable( bool enable );
protected:
	LogConsoleAppender();
	virtual void write( const QString & text );
private:
	static bool loggingEnabled;
	QMutex syncOfOutput;
};

#endif /* LOGCONSOLEAPPENDER_H_ */
