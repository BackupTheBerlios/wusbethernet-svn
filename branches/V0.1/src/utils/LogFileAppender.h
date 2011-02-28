/*
 * LogFileAppender.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-11-16
 */

#ifndef LOGFILEAPPENDER_H_
#define LOGFILEAPPENDER_H_

#include "LogAppender.h"
#include <QString>

class QFile;
class Logger;

class LogFileAppender: public LogAppender {
	friend class Logger;
public:
	virtual ~LogFileAppender();
	static void setEnable( bool enable );
protected:
	LogFileAppender( const QString & dirName, const QString & fileName, bool appendFile );
	virtual void write( const QString & text );
private:
	static bool loggingEnabled;
	QString filename;
	bool appendToFile;
	bool isOpen;
	QFile * fd;
	bool openLogFile();
	void closeLogFile();
};

#endif /* LOGFILEAPPENDER_H_ */
