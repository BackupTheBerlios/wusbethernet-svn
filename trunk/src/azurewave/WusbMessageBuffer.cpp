/*
 * WusbMessageBuffer.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-09
 */

#include "WusbMessageBuffer.h"
#include "WusbStack.h"
#include "../utils/Logger.h"
#include "WusbHelperLib.h"
#include <QByteArray>
#include <QMetaType>

bool WusbMessageBuffer::isFirstInstance = true;

WusbMessageBuffer::WusbMessageBuffer( WusbStack * owner, int mtuSize )
: QThread( (QObject*) owner ) {
	parentRef = owner;
	logger = owner->getLogger();
	lastMessageWasIncomplete = false;
	incompleteMessage.contentURB = NULL;
	if ( mtuSize < 1500 ) mtuSize = 1500;
	lastReceivedPacket = new QByteArray();
	lastReceivedPacket->reserve( mtuSize );

	// for usage in event queueing we have to register TypeOfMessage in QT-Metatype system
	// XXX this is not threadsafe...
	if ( WusbMessageBuffer::isFirstInstance ) {
		qRegisterMetaType<WusbMessageBuffer::eTypeOfMessage>("WusbMessageBuffer::eTypeOfMessage");
		WusbMessageBuffer::isFirstInstance = false;
	}

	start();	// start thread process
}

WusbMessageBuffer::~WusbMessageBuffer() {
}

void WusbMessageBuffer::receive( const QByteArray & bytes ) {
	// Sanity check
	if ( bytes.isNull() || bytes.isEmpty() ) return;
	// smaller packets are impossible in current WUSB/AzureWave protocol (minimal header takes 4 bytes)
	if ( bytes.length() < 4 ) return;

	// Status message
	if ( bytes.length() == 4 ) {
		// Device status message (open/close/etc.)
		if ( bytes[0] == 0x0 && bytes[1] == 0x0 && bytes[3] == 0x0 ) {
			// special treatment for case: 00 00 10 00
			uint8_t typeByte  = bytes[2];
			switch( typeByte ) {
			case 0x04:
				// close device
				logger->info("Received 'device closed' confirm message");
				emit statusMessage( DEVICE_CLOSE_SUCCESS );
				return;
				break;
			case 0x12:
				// open device
				logger->info("Received 'device opened' confirm message");
				emit statusMessage( DEVICE_OPEN_SUCCESS );
				return;
				break;
			case 0x10:
				// alive message with transaction counter == 0 (?) -> almost impossible...
				emit statusMessage( DEVICE_ALIVE );
				return;
				break;
			default:
				// ??? strange things are going on (error message from hub?)
				if ( logger->isWarnEnabled() )
					logger->warn(QString( "Received unknown message from hub: %1").arg( WusbHelperLib::messageToString( bytes ) ) );
				return;
				break;
			}
		}
		if ( bytes[2] == 0x10 ) {

			emit statusMessage( DEVICE_ALIVE );
		}
		return;
	} // 4 bytes messages

	if ( bytes == (*lastReceivedPacket) ) {
		// Duplicate packet received!
		if ( logger->isDebugEnabled() )
			logger->debug(QString("Duplicate packet received with length %1").arg( QString::number(bytes.length() ) ) );
		return;
	}
	lastReceivedPacket->clear();
	lastReceivedPacket->append( bytes );

	if ( logger->isDebugEnabled() )
		logger->debug(QString("Received message from hub: %1").arg(WusbHelperLib::messageToString(bytes,0)) );

	if ( lastMessageWasIncomplete && incompleteMessage.contentURB ) {
		struct WusbMessageBuffer::answerMessageParts contMsg = splitContinuedMessage( bytes );
		if ( contMsg.isCorrect ) {
			emit informPacketMeta( contMsg.receiverTAN, contMsg.TAN, contMsg.packetNum );
			incompleteMessage.contentURB->append( bytes.mid( 4 ) );	// append all URB data to buffer
			if ( incompleteMessage.contentLength == incompleteMessage.contentURB->size() ) {
				// message completed - emit all data and continue normal
				emit urbMessage( incompleteMessage.contentURB );
				lastMessageWasIncomplete = false;
			} else if (incompleteMessage.contentLength < incompleteMessage.contentURB->size() ) {
				// Houston we have a problem...
				logger->warn(QString::fromAscii("Received more data than expected! (%1 > %2)").arg(
						QString::number( incompleteMessage.contentURB->size() ), QString::number( incompleteMessage.contentLength ) ) );
			}
		}
		// if message is not correct, we cannot do anything ?
		return;
	}

	// Extract essential message details
	struct WusbMessageBuffer::answerMessageParts message = splitMessage( bytes );

	if ( message.isCorrect && message.isComplete ) {
		lastMessageWasIncomplete = false;
		emit informPacketMeta( message.receiverTAN, message.TAN, message.packetNum );
		emit urbMessage( message.contentURB );
	} else {
		if ( logger->isDebugEnabled() )
			logger->debug(QString("Message is not complete! Correct=%1 Complete=%2").arg(
					message.isCorrect? QString("true") : QString("false"),
							message.isComplete? QString("true") : QString("false")
			) );
		if ( message.isCorrect ) {
			// message is incomplete - we need to wait for more data to complete message
			lastMessageWasIncomplete = true;
			incompleteMessage = message;
		}
		// if message is incorrect - we cannot do anything
	}
}


struct WusbMessageBuffer::answerMessageParts WusbMessageBuffer::splitMessage( const QByteArray & bytes ) {
	struct WusbMessageBuffer::answerMessageParts retValue;
	retValue.isCorrect = false;
	if ( bytes.length() < WUSB_AZUREWAVE_RECEIVE_HEADER_LEN ) return retValue;
	if ( bytes[2] != 0x10 ) return retValue;

	retValue.senderTAN   = bytes[0];
	retValue.receiverTAN = bytes[1];
	retValue.TAN = bytes[3];

	unsigned int packetNum = 0;
	packetNum |= (bytes[8] & 0x00ff);
	packetNum |= ((bytes[9] & 0x00ff) << 8);
	packetNum |= ((bytes[10] & 0x00ff)<< 16);
	packetNum |= ((bytes[11] & 0x00ff)<< 24);
	retValue.packetNum = packetNum;

	int contentLength = 0;
	contentLength |= (bytes[23] & 0x00ff);
	contentLength |= ((bytes[22] & 0x00ff) << 8);
	contentLength |= ((bytes[21] & 0x00ff) << 16);
	retValue.contentLength = contentLength;

	if ( bytes.length() - WUSB_AZUREWAVE_RECEIVE_HEADER_LEN < contentLength )
		retValue.isComplete = false;	// USB URB is not entire contained in network packet!
	else
		retValue.isComplete = true;

	if (logger->isDebugEnabled() )
		logger->debug(QString("Received message: TAN1=%1 TAN2=%2 TAN3=%3 packetCount=%4 length=%5").arg(
				QString::number(retValue.senderTAN,16), QString::number(retValue.receiverTAN,16),QString::number(retValue.TAN,16),
				QString::number(retValue.packetNum&0xffffffff,16),  QString::number( contentLength ) ) );

	QByteArray *content = new QByteArray();
	content->reserve( contentLength );
	content->append( bytes.right( bytes.length() - WUSB_AZUREWAVE_RECEIVE_HEADER_LEN ) );
	retValue.contentURB = content;

	retValue.isCorrect = true;
	return retValue;
}


struct WusbMessageBuffer::answerMessageParts WusbMessageBuffer::splitContinuedMessage( const QByteArray & bytes ) {
	struct WusbMessageBuffer::answerMessageParts retValue;
	retValue.isCorrect = false;
	if ( bytes.length() < 4 ) return retValue;
	if ( bytes[2] != 0x10 ) return retValue;

	retValue.senderTAN   = bytes[0];
	retValue.receiverTAN = bytes[1];
	retValue.TAN = bytes[3];
	retValue.isCorrect = true;
	return retValue;
}


void WusbMessageBuffer::run() {
	// start event processing
	exec();
}
