/*
 * Logger.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-11-16
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#define ROOT_LOGGER		"ROOT"

#include <QMap>
#include <QString>
#include <QMutex>

class LogAppender;

class Logger {
public:
	enum LogLevel {
		LOGLEVEL_ERROR = 0,
		LOGLEVEL_WARN,
		LOGLEVEL_INFO,
		LOGLEVEL_DEBUG,
		LOGLEVEL_TRACE
	};
	virtual ~Logger();

	/** Return <i>default</i> (a.k.a <tt>root</tt>) logger */
	static Logger * getLogger();
	/** Return logger by name. Returns <tt>root</tt> logger when loggerName is <code>NULL</code> */
	static Logger * getLogger( const char* loggerName );
	/** Return logger by name. Returns <tt>root</tt> logger when loggerName is <code>NULL</code> */
	static Logger * getLogger( const QString & loggerName );

	/** Closes all known loggers. This finalizes (closes) all file appenders and cleans up memory */
	static void closeAllLogger();

	/** Log <tt>text</tt> with level TRACE */
	void trace( const char* text );
	/** Log <tt>text</tt> with level TRACE */
	void trace( const QString &text );
	/** Return if loglevel TRACE is enabled or not */
	bool isTraceEnabled();
	/** Log <tt>text</tt> with level DEBUG */
	void debug( const char* text );
	/** Log <tt>text</tt> with level DEBUG */
	void debug( const QString &text );
	/** Return if loglevel DEBUG is enabled or not */
	bool isDebugEnabled();
	/** Log <tt>text</tt> with level INFO */
	void info( const char* text );
	/** Log <tt>text</tt> with level INFO */
	void info( const QString &text );
	/** Return if loglevel INFO is enabled or not */
	bool isInfoEnabled();
	/** Log <tt>text</tt> with level WARNING */
	void warn( const char* text );
	/** Log <tt>text</tt> with level WARNING */
	void warn( const QString &text );
	/** Return if loglevel WARN is enabled or not */
	bool isWarnEnabled();
	/** Log <tt>text</tt> with level ERROR */
	void error( const char* text );
	/** Log <tt>text</tt> with level ERROR */
	void error( const QString &text );

	/** Sets the new loglevel to given value */
	void setLogLevel( LogLevel level );
	/** Adds a console appender to logger */
	void addConsoleAppender();
	/** Adds a file appender with specified filename in default directory to logger */
	void addFileAppender( const QString & filename, bool appendFile = false );
	/** Adds a file appender with specified filename and directory to logger */
	void addFileAppender( const QString & dirname, const QString & filename, bool appendFile = false );

	void setDefaultLoggingDirectory( const QString & directoryName );

	/** Enable/Disable logging to console */
	static void enableConsoleLogging( bool enable );
	/** Enable/Disable logging to file */
	static void enableFileLogging( bool enable );
private:
	static QMap<QString,Logger*> instanceMap;
	static QString dateFormat;
	static QString logEntryTemplate;
	QString name;
	LogLevel logLevel;
	/** List of appenders */
	QList<LogAppender*> listAppenders;
	/** Thread mutex to protect simulanous usage */
	QMutex logMutex;

	QString defaultLoggingDirectory;

	/** Constructor with name of logger */
	Logger( const QString & name );
	/** Write <tt>text</tt> to all available appenders */
	void log( LogLevel level, const QString & text );

	/** Convert given log level to string */
	QString logLevelToString( LogLevel level );
};

#endif /* LOGGER_H_ */
