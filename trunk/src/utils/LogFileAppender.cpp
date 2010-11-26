/*
 * LogFileAppender.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-11-16
 */

#include "LogFileAppender.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <iostream>

using namespace std;

// initialize global enable/disable flag
bool LogFileAppender::loggingEnabled = false;

LogFileAppender::LogFileAppender( const QString & dirName, const QString & fileName, bool appendFile ) {
	QString dir;
	if ( dirName.isNull() || dirName.isEmpty() )
		dir = QDir::tempPath() + "/" + "USBhubConnector/";
	else
		dir = dirName;
	dir = dir.replace('\\','/');
	if ( !dir.endsWith('/') )
		dir += '/';
	filename = dir + fileName;

	appendToFile = appendFile;
	fd = NULL;
	isOpen = false;
}

LogFileAppender::~LogFileAppender() {
	closeLogFile();
}

void LogFileAppender::write( const QString & text ) {
	if ( !loggingEnabled ) {
		if ( isOpen ) closeLogFile();
		return;
	}
	if ( !isOpen && !openLogFile() ) return;	// error
	if ( !fd ) return; // ???
//	qint64 result =
	fd->write( text.toUtf8().data() );
	fd->write( "\n" );
//	cout << "Wrote to logfile: " << filename.toUtf8().data() <<  " " << result << " bytes" << endl;
	fd->flush();
}

void LogFileAppender::setEnable( bool enable ) {
	LogFileAppender::loggingEnabled = enable;
}

bool LogFileAppender::openLogFile() {
	if ( isOpen ) return true;
	QFileInfo qfi(filename);
	if ( !qfi.exists() ) {
		QDir parentDir = qfi.dir();
		if ( !parentDir.exists() ) {
			bool erg = parentDir.mkpath( parentDir.absolutePath() );
			cout << "Create log directory: " << parentDir.absolutePath().toUtf8().data() <<
					" -> " << ( erg? "true" : "false" ) << endl;
		}
		if ( !parentDir.exists() ) {
			cout << "Cannot create parent log directory: " << parentDir.absolutePath().toUtf8().data() << endl;
			return false;
		}
	} else if ( !qfi.isWritable() ) {
		cout << "Cannot write log file: " << filename.toUtf8().data() << endl;
		return false;
	}
	fd = new QFile( filename );
	QIODevice::OpenMode om = QIODevice::WriteOnly | QIODevice::Text;
	if ( appendToFile )
		om |= QIODevice::Append;
	if ( !fd->open( om ) ) {
		cout << "Cannot open log file \"" << filename.toUtf8().data() << "\":" << fd->errorString().toUtf8().data() << endl;
		return false;
	}
//	cout << "Logfile: " << filename.toUtf8().data() <<  " is opened!" << endl;
	isOpen = true;
	return isOpen;
}

void LogFileAppender::closeLogFile() {
	if ( !isOpen || !fd ) {
		isOpen = false;
		return;
	}
	fd->flush();
	fd->close();
	fd = NULL;
	isOpen = false;
}
