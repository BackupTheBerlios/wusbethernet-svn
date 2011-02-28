/*
 * LogConsoleAppender.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-11-16
 */

#include "LogConsoleAppender.h"
#include <QString>
using namespace std;
#include <iostream>
#include <cstdlib>


// initialize global enable/disable flag
bool LogConsoleAppender::loggingEnabled = true;

LogConsoleAppender::LogConsoleAppender() {
}

LogConsoleAppender::~LogConsoleAppender() {
}

void LogConsoleAppender::write( const QString & text ) {
	if ( loggingEnabled ) {
		syncOfOutput.lock();
		cout << text.toUtf8().data() << endl;
		syncOfOutput.unlock();
	}
}

void LogConsoleAppender::setEnable( bool enable ) {
	LogConsoleAppender::loggingEnabled = enable;
}
