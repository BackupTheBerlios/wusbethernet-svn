/*
 * WusbHelperLib.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	11.10.2010
 */

#include "WusbHelperLib.h"
#include "../utils/LogWriter.h"
#include "../ConfigManager.h"
#include "../BasicUtils.h"
#include <stdlib.h>
#include <QMutexLocker>
#include <QString>
#include <time.h>

// init static variables
QMutex WusbHelperLib::packetCountMutex;
unsigned int WusbHelperLib::packetCounter = 0x01010101;

void WusbHelperLib::initPacketCounter() {
	// initialize the packet counter
	WusbHelperLib::packetCounter = ConfigManager::getInstance().getIntValue( "main.startTimestamp", WusbHelperLib::packetCounter );
}

unsigned int WusbHelperLib::getIncrementedPacketCounter() {
	QMutexLocker locker(&WusbHelperLib::packetCountMutex);
//	LogWriter::debug(QString(" packetCounter = %1").arg( QString::number( packetCounter ) ) );
	packetCounter++;
	unsigned int retVal = WusbHelperLib::packetCounter;
	return retVal;
}

void WusbHelperLib::appendTransactionHeader( QByteArray & buffer, int sendTAN, int recTAN, int tanCount ) {
	buffer.append( (unsigned char) sendTAN );
	buffer.append( (unsigned char) recTAN );
	buffer.append( 0x10 );
	buffer.append( (unsigned char) tanCount );
}
void WusbHelperLib::appendMarker55Header( QByteArray & buffer, unsigned char sendFlag ) {
	buffer.append( 0x55 );
	buffer.append( 0x55 );
	buffer.append( '\0' );
	buffer.append( sendFlag );
}
void WusbHelperLib::appendPacketCountHeader( QByteArray & buffer ) {
	// The packet counter should be written with LSB first!
	unsigned int counter = getIncrementedPacketCounter();
	buffer.append(  (counter & 0x000000ff) );
	buffer.append( ((counter & 0x0000ff00) >> 8 ) );
	buffer.append( ((counter & 0x00ff0000) >> 16 ) );
	buffer.append( ((counter & 0xff000000) >> 24 ) );
}
void WusbHelperLib::appendPacketLength( QByteArray & buffer, unsigned int lenValue ) {
	// The length of packet is written with MSB first! (contrary to packet counter)
	buffer.append(  '\0' );	// always?!
	buffer.append( ((lenValue & 0x00ff0000) >> 16 ) );
	buffer.append( ((lenValue & 0x0000ff00) >> 8 ) );
	buffer.append(  (lenValue & 0x000000ff) );
}

QString WusbHelperLib::messageToString( const QByteArray & bytes, int lengthToPrint ) {
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
QString WusbHelperLib::messageToString( const unsigned char* bytes, int lengthToPrint ) {
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
