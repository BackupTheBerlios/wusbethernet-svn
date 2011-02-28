/*
 * LogWriter.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	15.07.2010
 */

#include "LogWriter.h"
#include <iostream>
#include <cstdlib>
#include <qstring.h>
#include <QDateTime>

using namespace std;

QString LogWriter::dateFormat = QString("yyyy-MM-dd hh:mm:ss");

LogWriter::LogWriter() {

}

LogWriter::~LogWriter() {
	// TODO Auto-generated destructor stub
}
void LogWriter::trace( const char* text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] TRACE - " << text << endl;
}
void LogWriter::trace( const QString& text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] TRACE - " << text.toUtf8().data() << endl;
}
void LogWriter::debug( const char* text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] DEBUG - " << text << endl;
}
void LogWriter::debug( const QString& text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] DEBUG - " << text.toUtf8().data() << endl;
}
void LogWriter::info( const char* text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] INFO - " << text << endl;
}
void LogWriter::info( const QString &text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] INFO - " << text.toUtf8().data() << endl;
}
void LogWriter::warn( const char* text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] WARN - " << text << endl;
}
void LogWriter::warn( const QString &text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] WARN - " << text.toUtf8().data() << endl;
}
void LogWriter::error( const char* text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] ERROR - " << text << endl;
}
void LogWriter::error( const QString &text ) {
	cout << "[" << getFormatedDateTime().toUtf8().data() << "] ERROR - " << text.toUtf8().data() << endl;
}

QString LogWriter::getFormatedDateTime() {
	const QDateTime & dt = QDateTime::currentDateTime();
	return dt.toString( dateFormat );
}
