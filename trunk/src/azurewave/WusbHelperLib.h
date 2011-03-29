/*
 * WusbHelperLib.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-11
 */

#ifndef WUSBHELPERLIB_H_
#define WUSBHELPERLIB_H_

#include <QByteArray>
#include <QMutex>
#include <stdint.h>

class QString;

class WusbHelperLib {
public:
	/** Initialize the packet counter with an value calculated from system uptime and program runtime */
	static void initPacketCounter();

	static unsigned int getIncrementedPacketCounter();

	static void appendTransactionHeader( QByteArray & buffer, int sendTAN, int recTAN, int tanCount );
	static void appendMarker55Header( QByteArray & buffer, uint8_t param1 = 0, uint8_t intervalVal = 0 );
	static unsigned int appendPacketIDHeader( QByteArray & buffer );
	static void appendPacketLength( QByteArray & buffer, unsigned int lenValue );


	/** Debug function: Print at most <em>length</em> bytes in hex notation into result string. */
	static QString messageToString( const QByteArray & bytes, int lengthToPrint = 4 );
	/** Debug function: Print at most <em>length</em> bytes in hex notation into result string. */
	static QString messageToString( const uint8_t* bytes, int lengthToPrint = 1 );

private:
	static QMutex packetCountMutex;
	static unsigned int packetCounter;
};

#endif /* WUSBHELPERLIB_H_ */
