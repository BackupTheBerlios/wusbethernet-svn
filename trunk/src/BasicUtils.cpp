/*
 * BasicUtils.c
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-08
 */


#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <QStringList>
#include <QRegExp>
#include "BasicUtils.h"


void sleepThread( long millis ) {
	long seconds = millis / 1000L;
	long nanos = (millis % 1000L) * 1000000L;
	struct timespec t;
	t.tv_sec = seconds;
	t.tv_nsec = nanos;
	nanosleep(&t, NULL);
}

long long currentTimeMillis() {
	struct timeval tv;
	long long retValue;
	if ( gettimeofday( &tv, NULL ) ) {
		// error?
		perror("Problem getting timevalue by gettimeofday: " );
		return 0L;
	}
	// note: in einer Zeile geschrieben gibt folgendes Probleme! unklar?!
	retValue  = tv.tv_sec;
	retValue *= 1000L;	// since we want milliseconds
	retValue += tv.tv_usec / 1000L;
	return retValue;
}

QString flipIPaddress( const QString & address ) {
	if ( address.isNull() || address.isEmpty() ) return QString::null;
	QStringList list;
	list = address.split(QRegExp("[\\.:]"));
	QString retStr = QString();
	int len = list.length();
	for ( int i = len-1; i >= 0; i-- ) {
		retStr.append( list[i] );
		if ( i > 0 )
			retStr.append('.');
	}
	return retStr;
}

QString messageToString( const QByteArray & bytes, int lengthToPrint ) {
	if ( bytes.isNull() || bytes.isEmpty() ) return QString::null;
	if ( lengthToPrint <= 0 ) lengthToPrint = bytes.length();

	QString str( lengthToPrint * 2 + lengthToPrint -1, ' ' );
	int pointer = 0;
	for ( int i = 0; i < lengthToPrint && i < bytes.length(); i++ ) {
		unsigned char c = bytes[i];
		if ( c <= 0x0f ) {
			// fill with '0' if number < 0x0f
			str.replace( pointer, 1, "0" );
			str.replace( pointer+1, 1, QString::number( c, 16 ).toUpper() );
		} else
			str.replace( pointer, 2, QString::number( c, 16 ).toUpper() );
		pointer+=3;
	}
	return str;
}
QString messageToString( const unsigned char* bytes, int lengthToPrint ) {
	if ( ! bytes ) return QString("null");
	QString str( lengthToPrint * 2 + lengthToPrint -1, ' ' );
	int pointer = 0;
	for ( int i = 0; i < lengthToPrint; i++ ) {
		unsigned char c = bytes[i];
		if ( c <= 0x0f ) {
			// fill with '0' if number < 0x0f
			str.replace( pointer, 1, "0" );
			str.replace( pointer+1, 1, QString::number( c, 16 ).toUpper() );
		} else
			str.replace( pointer, 2, QString::number( c, 16 ).toUpper() );
		pointer+=3;
	}
	return str;
}
