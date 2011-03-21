/*
 * Logger.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-11-16
 */

#include "Logger.h"
#include "LogConsoleAppender.h"
#include "LogFileAppender.h"
#include <QDateTime>
#include <stdio.h>

// quasi singleton instance map
QMap<QString,Logger*> Logger::instanceMap;
// static definition for date and time format in output
QString Logger::dateFormat = QString("yyyy-MM-dd hh:mm:ss.zzz");
// Template for one log entry
QString Logger::logEntryTemplate = QString("[%1] (%2) %3 -\t%4");


Logger::Logger( const QString & loggerName ) {
//	printf("New Logger name=%s\n", loggerName.toUtf8().data());
	if ( !loggerName.isNull() && !loggerName.isEmpty() )
		name = loggerName;
	else
		name = QString::fromUtf8( ROOT_LOGGER );
	logLevel = LOGLEVEL_TRACE;
	defaultLoggingDirectory = QString::null;
}

Logger::~Logger() {
//	printf("Logger: destruktor name=%s\n", name.toUtf8().data() );
	if ( !listAppenders.isEmpty() ) {
		QListIterator<LogAppender*> it(listAppenders);
		while ( it.hasNext() ) {
			delete it.next();
		}
		listAppenders.clear();
	}
}

/* ******** Static singleton / factory API ********** */

// Mr Proper
void Logger::closeAllLogger() {
	QMapIterator<QString, Logger*> it(instanceMap);
	while (it.hasNext()) {
		it.next();
		delete (it.value());
	}
}

Logger * Logger::getLogger() {
	QString key(ROOT_LOGGER);
	if ( instanceMap.contains( key ) ) {
		return instanceMap.value( key );
	}
	Logger * loggerInst = new Logger( key );
	instanceMap.insert( key, loggerInst );
	return loggerInst;
}
Logger * Logger::getLogger( const char* loggerName ) {
	QString key(ROOT_LOGGER);
	if ( loggerName )
		key = QString(loggerName);
	if ( instanceMap.contains( key ) ) {
		return instanceMap.value( key );
	}
	Logger * loggerInst = new Logger( key );
	instanceMap.insert( key, loggerInst );
	return loggerInst;
}

Logger * Logger::getLogger( const QString & loggerName ) {
	QString key(ROOT_LOGGER);
	if ( !loggerName.isNull() && !loggerName.isEmpty() )
		key = loggerName;
	if ( instanceMap.contains( key ) ) {
		return instanceMap.value( key );
	}
	Logger * loggerInst = new Logger( key );
	instanceMap.insert( key, loggerInst );
	return loggerInst;
}

void Logger::setDefaultLoggingDirectory( const QString & directoryName ) {
	defaultLoggingDirectory = directoryName;
}

/* ******** Logging API ********** */

void Logger::trace( const char* text ) {
	log( LOGLEVEL_TRACE, text );
}
void Logger::trace( const QString &text ) {
	log( LOGLEVEL_TRACE, text );
}
bool Logger::isTraceEnabled() {
	if ( logLevel > LOGLEVEL_DEBUG )
		return true;
	return false;
}
void Logger::debug( const char* text ) {
	log( LOGLEVEL_DEBUG, text );
}
void Logger::debug( const QString &text ) {
	log( LOGLEVEL_DEBUG, text );
}
bool Logger::isDebugEnabled() {
	if ( logLevel > LOGLEVEL_INFO )
		return true;
	return false;
}
void Logger::info( const char* text ) {
	log( LOGLEVEL_INFO, text );
}
void Logger::info( const QString &text ) {
	log( LOGLEVEL_INFO, text );
}
bool Logger::isInfoEnabled() {
	if ( logLevel > LOGLEVEL_WARN )
		return true;
	return false;
}
void Logger::warn( const char* text ) {
	log( LOGLEVEL_WARN, text );
}
void Logger::warn( const QString &text ) {
	log( LOGLEVEL_WARN, text );
}
bool Logger::isWarnEnabled() {
	if ( logLevel > LOGLEVEL_ERROR )
		return true;
	return false;
}
void Logger::error( const char* text ) {
	log( LOGLEVEL_ERROR, text );
}
void Logger::error( const QString &text ) {
	log( LOGLEVEL_ERROR, text );
}

void Logger::log( eLogLevel level, const QString & text ) {
//	printf("log! level=%i/%i listAppenders.size=%i text=%s\n", level, logLevel, listAppenders.size(),
//			text.toUtf8().data() );
	if ( level <= logLevel && !listAppenders.isEmpty() ) {
		logMutex.lock();	// protect multithreaded use from here ***
		const QDateTime & dt = QDateTime::currentDateTime();
		QString logString = logEntryTemplate.arg( dt.toString( dateFormat ), name, logLevelToString( level ), text );

		QListIterator<LogAppender*> it(listAppenders);
		while ( it.hasNext() ) {
			LogAppender * appender = it.next();
			appender->write( logString );
		}
		logMutex.unlock();	// protect multithreaded use until here ***
	}
}

QString Logger::logLevelToString( eLogLevel level ) {
	switch ( level ) {
	case LOGLEVEL_DEBUG:
		return QString("DEBUG");
	case LOGLEVEL_ERROR:
		return QString("ERROR");
	case LOGLEVEL_INFO:
		return QString("INFO");
	case LOGLEVEL_TRACE:
		return QString("TRACE");
	case LOGLEVEL_WARN:
		return QString("WARN");
	}
	return "UNKNOWN";
}

void Logger::setLogLevel( eLogLevel level ) {
	logLevel = level;
}
void Logger::addConsoleAppender() {
	listAppenders.append( new LogConsoleAppender() );
}
void Logger::addFileAppender( const QString & filename, bool appendFile ) {
	listAppenders.append( new LogFileAppender( defaultLoggingDirectory, filename, appendFile ) );
}
void Logger::addFileAppender( const QString & dirname, const QString & filename, bool appendFile ) {
	listAppenders.append( new LogFileAppender( dirname, filename, appendFile ) );
}

void Logger::enableConsoleLogging( bool enable ) {
	LogConsoleAppender::setEnable( enable );
}
void Logger::enableFileLogging( bool enable ) {
	LogFileAppender::setEnable( enable );
}
