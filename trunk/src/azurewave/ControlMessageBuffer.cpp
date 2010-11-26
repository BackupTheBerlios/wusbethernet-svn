/*
 * ControlMessageBuffer.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-07-14
 */

#include "ControlMessageBuffer.h"
#include "HubDevice.h"
#include "../utils/Logger.h"
#include "../BasicUtils.h"
#include <qbytearray.h>


ControlMessageBuffer::ControlMessageBuffer( HubDevice * recipientClass ) {
	recipientForData = recipientClass;
	logger = recipientClass->getLogger();
	// reserve internal buffer
	internalBuffer = new QByteArray();
	internalBuffer->reserve( DEFAULT_BUFFER_SIZE );
	bytesLeftInCurrentTask = 0;
	currentMessageTask = TOM_NOP;
}

ControlMessageBuffer::~ControlMessageBuffer() {
	delete internalBuffer;
}

void ControlMessageBuffer::receive( const QByteArray & bytes ) {
/*
	logger->trace(QString("ControlMessageBuffer - Received %1 bytes; internalBuffer.size = %2; currentTask = %3; "
			"bytesLeftInCurrentTask = %4").arg(
					QString::number(bytes.size()),
					QString::number(internalBuffer->size()),
					typeOfMessageToString( currentMessageTask ),
					QString::number( bytesLeftInCurrentTask )
					) );
	logger->trace( QString("ControlMessageBuffer - Header: %1 %2 %3 %4 %5 %6 %7").arg(
			QString::number(bytes[0]&0xff,16),
			QString::number(bytes[1]&0xff,16),
			QString::number(bytes[2]&0xff,16),
			QString::number(bytes[3]&0xff,16),
			QString::number(bytes[4]&0xff,16),
			QString::number(bytes[5]&0xff,16),
			bytes.length()> 6? QString::number(bytes[6],16) : QString("-")
	));
*/
	// put all data into internal buffer
	internalBuffer->append( bytes );
	// if there is no task to complete from last invocation - get type of task and length:
	if ( currentMessageTask == TOM_NOP || bytesLeftInCurrentTask == 0 ) {
		// no current message / task to complete
		// -> get task & length from buffer
		retrieveMessageLengthAndType();
	}

	// if there is no success: abort
	if ( currentMessageTask == TOM_NOP ) {
		bytesLeftInCurrentTask = 0;
		return;	// just to be sure
	}
	// now we have some bytes in buffer and we should know about type of message
	//   size of message + header smaller or equal buffer size
	while ( currentMessageTask != TOM_NOP && (bytesLeftInCurrentTask + 6) <= internalBuffer->size() ) {
		int buffSize = internalBuffer->size();

		// send complete message but without header to ConnectionController/recipient
		QByteArray message = internalBuffer->mid( 6, bytesLeftInCurrentTask );

		recipientForData->receiveData( currentMessageTask, message );

		// Erase all data from buffer and reset status if there is nothing left in buffer
		// -> buffer contains only current task data and nothing more
		if ( bytesLeftInCurrentTask + 6 == buffSize ) {
			currentMessageTask = TOM_NOP;
			bytesLeftInCurrentTask = 0;
			internalBuffer->clear();
			// finished
		} else {
			// there is still data in buffer:
			// -> copy remaining data to front of buffer
			int startPos = bytesLeftInCurrentTask + 6;
			for ( int i = startPos, j = 0; i < buffSize; i++, j++ )
				(*internalBuffer)[j] = (*internalBuffer)[i];

			internalBuffer->chop( startPos );	// truncate buffer after current stored data

			// Sanity check and strange error correction
			if ( internalBuffer->size() <= 2 ) {
				logger->warn(QString("ControlMessageBuffer - dropping %1 bytes in buffer").arg( QString::number(buffSize)));
				currentMessageTask = TOM_NOP;
				bytesLeftInCurrentTask = 0;
				internalBuffer->clear();
			} else {
				retrieveMessageLengthAndType();
			}
		}
	}
}

void ControlMessageBuffer::retrieveMessageLengthAndType() {
	currentMessageTask = getTypeOfMessage( *internalBuffer );
	if ( currentMessageTask == TOM_NOP ) {
		// Ohoh... there is probably nonsense data in buffer or synchronization lost
		if ( internalBuffer->size() < 7 ) {
			logger->warn( QString("ControlMessageBuffer: Sync lost or unknown type of message from network: (len < 7)") );
		} else {
			logger->warn( QString::fromAscii("ControlMessageBuffer: Sync lost or unknown type of message from network: %1").arg(
					messageToString( (*internalBuffer), 7 ) ) );
		}
		internalBuffer->clear();
		return;
	}
	bytesLeftInCurrentTask = getLengthOfMessage( *internalBuffer );
	if ( bytesLeftInCurrentTask < 0 ) {
		bytesLeftInCurrentTask = 0;
		currentMessageTask = TOM_NOP;
		return; // try again with some more content
	}
}

ControlMessageBuffer::TypeOfMessage ControlMessageBuffer::getTypeOfMessage( const QByteArray & bytes ) {
	if ( bytes.size() < 4 ) return TOM_NOP;
	// First 2 bytes are always: 0x77 0x77  OR  0x66 0x66
	// 0x66 -> "request"
	// 0x77 -> "answer"
	unsigned char byte  = bytes[0];
	if ( byte != 0x77 && byte != 0x66 ) return TOM_NOP;
	byte  = bytes[1];
	if ( byte != 0x77 && byte != 0x66 ) return TOM_NOP;
	// Next byte is type of message
	byte  = bytes[2];
	switch ( byte ) {
	case 0x65:
		return TOM_SERVERINFO;
	case 0x66:
		// unknown / never seen
		return TOM_MSG66;
	case 0x67:
		return TOM_STATUSCHANGED;
	case 0x68:
		return TOM_IMPORTINFO;
	case 0x69:
		return TOM_UNIMPORT;
	case 0x6a:
		return TOM_ALIVE;
	}
	return TOM_NOP;
}

int ControlMessageBuffer::getLengthOfMessage( const QByteArray & bytes ) {
	// PRE: header is correct!

	// if less than 6 bytes in buffer we cannot determine anything here!
	if ( bytes.size() < 6 ) return -1;

	// unclear if byte 3 belongs to length... (never seen anything different)
	// By now: byte 3, 4 and 5 are giving the total length of the message
	unsigned char lenbyte0  = bytes[3];
	unsigned char lenbyte1  = bytes[4];
	unsigned char lenbyte2  = bytes[5];

	int retVal = (lenbyte0 << 16);
	retVal |= (lenbyte1 << 8);
	retVal |= lenbyte2;
	return retVal;
}

QString ControlMessageBuffer::messageToString( const QByteArray & bytes, int lengthToPrint ) {
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

QString ControlMessageBuffer::typeOfMessageToString( ControlMessageBuffer::TypeOfMessage type ) {
	switch( type ) {
	case TOM_SERVERINFO:
		return QString("SERVERINFO");
	case TOM_MSG66:
		return QString("MSG66");
	case TOM_STATUSCHANGED:
		return QString("STATUSCHANGED");
	case TOM_IMPORTINFO:
		return QString("IMPORTINFO");
	case TOM_UNIMPORT:
		return QString("UNIMPORT");
	case TOM_ALIVE:
		return QString("ALIVE");
	case TOM_NOP:
		return QString("NOP");
	default:
		return QString("UNKNOWN (%1)").arg(QString::number((int)type) );
	}
}
