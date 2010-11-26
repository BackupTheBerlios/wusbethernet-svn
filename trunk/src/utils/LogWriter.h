/*
 * LogWriter.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	15.07.2010
 */

#ifndef LOGWRITER_H_
#define LOGWRITER_H_

class QString;

class LogWriter {
public:
	LogWriter();
	virtual ~LogWriter();
	static void trace( const char* text );
	static void trace( const QString &text );
	static void debug( const char* text );
	static void debug( const QString &text );
	static void info( const char* text );
	static void info( const QString &text );
	static void warn( const char* text );
	static void warn( const QString &text );
	static void error( const char* text );
	static void error( const QString &text );
private:
	static QString dateFormat;
	static QString getFormatedDateTime();
};

#endif /* LOGWRITER_H_ */
