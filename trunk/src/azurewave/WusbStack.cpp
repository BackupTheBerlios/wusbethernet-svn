/*
 * WusbStack.cpp
 * Basic functionality to wrap raw URB messages into WUSB messages and send/receive messages
 * on network. Ensures transport to remote hub and keeps alive connection by sending idle messages.
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-09-28
 */

#include "WusbStack.h"
#include "WusbReceiverThread.h"
#include "WusbHelperLib.h"
#include "../BasicUtils.h"
#include "../USBconnectionWorker.h"
#include "../utils/Logger.h"
#include "../TI_USB_VHCI.h"
#include <QCoreApplication>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTimer>
#include <QLinkedList>
#include <unistd.h>
#include <time.h>

/** flag, if this is the first instantiation of this class.
 * On first instance some supplemental init code is needed. */
bool WusbStack::isFirstInstance = true;

WusbStack::WusbStack( Logger *parentLogger, const QHostAddress & destinationAddress, int destinationPort )
: TI_WusbStack() {
	// XXX this is not threadsafe
	if ( WusbStack::isFirstInstance ) {
		WusbHelperLib::initPacketCounter();
		WusbStack::isFirstInstance = false;
	}

	logger = parentLogger;
	state = STATE_DISCONNECTED;
	destAddress = QHostAddress( destinationAddress );
	destPort = destinationPort;
	haveAnswer = false;
	udpSocket = NULL;
	maxMTU = WUSB_AZUREWAVE_NETWORK_DEFAULT_MTU;	// TODO make this configurable or auto detect
	currentSendTransactionNum = -1;
	currentReceiveTransactionNum = 0;
	currentTransactionNum = 0xff;
	sendPacketCounter = 0;
	lastPacketSendTimeMillis = 0L;
	lastPacketReceiveTimeMillis = 0L;
	lastSendAlivePacket = 0L;
	connectionKeeperTimer = NULL;
	urbReceiver = NULL;
	packetRefData = NULL;
	packetRefDataByPacketID.clear();

	// init receive message buffer
	messageBuffer = new WusbMessageBuffer( this, maxMTU );

	connect( messageBuffer, SIGNAL(statusMessage(WusbMessageBuffer::eTypeOfMessage,uint8_t,uint8_t,uint8_t )),
			this, SLOT(processStatusMessage(WusbMessageBuffer::eTypeOfMessage,uint8_t,uint8_t,uint8_t)),
			Qt::QueuedConnection );
	connect( messageBuffer, SIGNAL(urbMessage( unsigned int, QByteArray* )),
			this, SLOT(processURBmessage( unsigned int, QByteArray* )), Qt::QueuedConnection );
	connect( messageBuffer, SIGNAL(informPacketMeta( int, int, unsigned int )),
			this, SLOT(informReceivedPacket( int, int, unsigned int )), Qt::QueuedConnection );

	connect( this, SIGNAL(receivedUDPdata(const QByteArray &)),
			messageBuffer, SLOT(receive(const QByteArray &)), Qt::QueuedConnection);

	if ( logger->isInfoEnabled() )
		logger->info(QString("WusbStack init completed for destination: %1:%2").
				arg(destAddress.toString(), QString::number(destPort)) );
}

WusbStack::~WusbStack() {
	closeConnection();
	if ( messageBuffer ) delete messageBuffer;
}

Logger * WusbStack::getLogger() {
	return logger;
}

bool WusbStack::openSocket() {
	if ( logger->isDebugEnabled() )
		logger->debug("Open connection..." );
	// Clean up socket if necessary
	if ( udpSocket ) {
		disconnect( udpSocket, SIGNAL( readyRead() ), this, SLOT(processPendingData() ) );
		udpSocket->close();
		udpSocket = NULL;
	}
	// open the socket and binding to an interface
	udpSocket = new QUdpSocket( this );
	if ( !udpSocket->bind( QHostAddress::Any, 0, QUdpSocket::DefaultForPlatform ) ) {	// XXX binding to a specific port is not necessary !!!???
		logger->warn(QString("Cannot bind to interface: %1").arg(udpSocket->errorString()) );
		return false;
	}

	// connecting a callback to get informed if data is available
	connect( udpSocket, SIGNAL( readyRead() ),
			this, SLOT(processPendingData()));
	return true;
}

bool WusbStack::writeToSocket( const QByteArray & buffer ) {
//	logger->debug(QString("Want to write %1 bytes to socket").arg( QString::number(buffer.size())));
	sendBufferMutex.lock();

	int bufSize = buffer.size();
	qint64 retVal = udpSocket->writeDatagram( buffer, destAddress, destPort );
	if ( logger->isTraceEnabled() )
		logger->trace(QString("Wrote %1 bytes (of %2) on network").arg( QString::number( retVal ), QString::number( bufSize ) ) );

	sendBufferMutex.unlock();
	return true;
}


bool WusbStack::openDevice() {
	if ( !udpSocket || state == STATE_OPENED || state == STATE_DISCONNECTED || state == STATE_FAILED ) return false;

	haveAnswer = false;

	// construct the "open device connection" message: 00 00 02 00
	QByteArray datagram = QByteArray(4, '\0' );
	datagram[2] = 0x2;
	// and write this to socket
	return writeToSocket( datagram );
}

bool WusbStack::closeDevice() {
	if ( !udpSocket || state == STATE_CLOSED || state == STATE_DISCONNECTED || state == STATE_FAILED ) return false;

	haveAnswer = false;
	stopTimer();

	// construct the "close device connection" message: 00 00 04 00
	QByteArray datagram = QByteArray(4, '\0' );
	datagram[2] = 0x4;
	// and write this to socket
	return writeToSocket( datagram );
}

void WusbStack::stopTimer() {
	// stop timer (if running)
	if (connectionKeeperTimer) {
		connectionKeeperTimer->stop();
		disconnect( connectionKeeperTimer, SIGNAL(timeout()), this, SLOT(timerInterrupt()) );
		delete connectionKeeperTimer;
		connectionKeeperTimer = NULL;
	}
}


bool WusbStack::openConnection() {
	if ( state == STATE_CONNECTED || state == STATE_OPENED ) return false;
	// open/create socket and connect to device
	if ( openSocket() ) {
		state = STATE_CONNECTED;	// State -> connected
		// start protocol thread handler
		if ( openDevice() ) {
			int waitCount = 0;
			while( waitCount < 300 && state != STATE_OPENED ) {
				QCoreApplication::processEvents();	// process pending events
				sleepThread( 10L );
				waitCount++;
			}
			if ( state == STATE_OPENED ) {
				connectionKeeperTimer = new QTimer(this);
				connect(connectionKeeperTimer, SIGNAL(timeout()), this, SLOT(timerInterrupt()));
				connectionKeeperTimer->start( WUSB_AZUREWAVE_TIMER_INTERVAL );
				return true;
			} else
				return false;
		} else {
			state = STATE_FAILED;
			return false;
		}
	}
	return false;
}

bool WusbStack::closeConnection() {
	if ( state == STATE_DISCONNECTED ) return false;

	// close connection to device
	if ( closeDevice() ) {
		// give network device some time to acknoledge
		int waitCount = 0;
		while( waitCount < 100 && state != STATE_CLOSED && state != STATE_FAILED ) {
			QCoreApplication::processEvents();	// process pending events
			sleepThread( 50L );
			waitCount++;
		}
	}

	// and close the socket
	if ( udpSocket ) {
		disconnect( udpSocket, SIGNAL( readyRead() ), this, SLOT(processPendingData() ) );
		if ( udpSocket->isOpen() )
			udpSocket->close();
		delete udpSocket;
		udpSocket = NULL;
	}
	return true;
}

void WusbStack::processPendingData() {
	if ( !udpSocket ) return;
	while ( udpSocket->hasPendingDatagrams() ) {
		QByteArray datagram = QByteArray();
		datagram.resize( udpSocket->pendingDatagramSize() );

		QHostAddress sender;
		quint16 senderPort;
		qint64 bytesRead = udpSocket->readDatagram(datagram.data(), datagram.size(),
				&sender, &senderPort);

		if ( bytesRead > 0 ) {
			if ( logger->isTraceEnabled() )
				logger->trace(QString("Received %1 bytes from network: %2").arg( QString::number(bytesRead), WusbHelperLib::messageToString( datagram, bytesRead )) );

			if ( datagram.size() > maxMTU ) {
				// remote device sent faster than we could receive or process
				// -> multiple messages are concatuated in buffer
				//    -> process all parts individual
				int idx = 0;
				do {
					emit receivedUDPdata( datagram.mid(idx, maxMTU) );
					idx += maxMTU;
				} while( idx < datagram.size() );
			} else
				emit receivedUDPdata( datagram );
		}
	}
}

void WusbStack::socketError(QAbstractSocket::SocketError socketError) {
	// TODO check/process socket error condition
	// since communication is based on UDP - unclear if/when this error may occur
	// -> reboot of remote hub and receive of "port unreachable" ICMP?
	logger->error( QString::fromAscii("Socket error in communication with remote hub %1:%2 - error: %3 (%4)").arg(
			destAddress.toString(), QString::number(destPort), QString::number(socketError), udpSocket->errorString() ) );
}

void WusbStack::timerInterrupt() {
	// This is called every 100ms to check if idle/keep-alive messages are neccessary to send
	long long now = currentTimeMillis();
	long long lastPacketTime = qMax( lastPacketSendTimeMillis, lastPacketReceiveTimeMillis );

	// sanity check
	if ( !udpSocket || state != STATE_OPENED || lastPacketSendTimeMillis == 0L ) return;

//	if ( lastPacketReceiveTimeMillis > now || now - lastPacketReceiveTimeMillis > )

	// Since more than 100ms nothing sent? (and workaround for clock warping...)
	// We need to send idle (or receive OK) messages:
	// - when no real communcation is needed (a.k.a. keep alive)  (2 secs)
	// - when we received a message from hub but have no new message to send (receive ack) (>100ms)
	if ( lastPacketTime > now || now - lastPacketTime > WUSB_AZUREWAVE_TIMER_SEND_ACK ) {
		if ( lastSendAlivePacket < lastPacketTime )
			sendAcknowledgeReplyMessage();
		else if ( now - lastSendAlivePacket >= WUSB_AZUREWAVE_TIMER_SEND_KEEP_ALIVE )
			sendIdleMessage();
	}
}

bool WusbStack::sendURB (
		void * refData,
		const char * urbData,
		int urbDataLen,
		eDataTransferType dataTransferType,
		eDataDirectionType directionType,
		uint8_t endpoint,
		uint16_t transferFlags,
		uint8_t intervalVal,
		int receiveLength ) {

	// just delegate this to methode below...
	QByteArray * urbBytes = new QByteArray( urbData, urbDataLen );
	return sendURB( refData, urbBytes, dataTransferType, directionType, endpoint, transferFlags, intervalVal, receiveLength );
}

bool WusbStack::sendURB (
		void * refData,
		QByteArray * urbData,
		eDataTransferType dataTransferType,
		eDataDirectionType directionType,
		uint8_t endpoint,
		uint16_t transferFlags,
		uint8_t intervalVal,
		int receiveLength ) {

	int packetLen =	urbData->length() + WUSB_AZUREWAVE_SEND_HEADER_LEN;	// +28 bytes header

	QLinkedList<QByteArray> messageSplit;
	if ( packetLen > maxMTU ) {
		// we need to split the message!
		// only the first message part will be complete (with all header parts)
		// subsequent message are composed of transaction header (with incremented send no.) and payload

		packetLen = maxMTU;

		// first packet: length = MTU - header
		messageSplit.append( urbData->left( maxMTU - WUSB_AZUREWAVE_SEND_HEADER_LEN ) );
		// subsequent packets:
		bool done = false;
		int idx = maxMTU - WUSB_AZUREWAVE_SEND_HEADER_LEN;	// first position of next packet
		int len = maxMTU - WUSB_AZUREWAVE_SEND_SUBSQ_HEADER_LEN;	// length of packet
		do {
			messageSplit.append( urbData->mid( idx, len ) );
			if ( idx + len >= urbData->length() )	// last packet
				done = true;
			idx += len;
		} while ( !done );
	}


	// Buffer to send (first packet == longest packet)
	QByteArray buffer = QByteArray();
	buffer.reserve( packetLen );

	currentSendTransactionNum = ( currentSendTransactionNum +1 ) % 256;
	if ( sendPacketCounter == 0 )
		currentTransactionNum = 0xff;
	else
		currentTransactionNum = ( currentTransactionNum +1 ) % 256;
//		currentTransactionNum = qMin(currentSendTransactionNum, currentReceiveTransactionNum);

	WusbHelperLib::appendTransactionHeader( buffer, currentSendTransactionNum, currentReceiveTransactionNum, currentTransactionNum );
	WusbHelperLib::appendMarker55Header( buffer, 0, intervalVal );	// XXX parameter1 unknown
	unsigned int packetID = WusbHelperLib::appendPacketIDHeader( buffer );

	// storing the packetID of prepared (and hopefully sent) packet
	if ( refData && dataTransferType != ISOCHRONOUS_TRANSFER )
		packetRefDataByPacketID[packetID] = refData;

	uint8_t xferDirectionValue = 0;
	switch( dataTransferType ) {
		case CONTROL_TRANSFER:
			xferDirectionValue = 0x80;
			break;
		case BULK_TRANSFER:
			xferDirectionValue = 0xc0;	// XXX
			break;
		case INTERRUPT_TRANSFER:
			xferDirectionValue = 0x40;
			break;
		case ISOCHRONOUS_TRANSFER:
			// isochronous transfer is enabled by transfer flag (?)
			xferDirectionValue = 0;
			break;
	}
	// TODO data direction, endpoint and special treatment of some transfer types
//	if ( directionType == TI_WusbStack::DATADIRECTION_OUT || dataTransferType == CONTROL_TRANSFER )
//		buffer.append( 0x80 >> endpoint );

	buffer.append( xferDirectionValue );

	// Endpoint address
	uint16_t endptShrt = (endpoint << 7);
	buffer.append( (endptShrt & 0xff00) >> 8 );
	buffer.append( endptShrt & 0xff );

	if ( directionType == TI_WusbStack::DATADIRECTION_IN )
		buffer.append( 0x80 );
	else
		buffer.append('\0' );
	/*
	// XXX WTF?
	if ( directionType == TI_WusbStack::DATADIRECTION_IN && dataTransferType != CONTROL_TRANSFER )
		buffer.append( 0x80 );
	else
		buffer.append( '\0' );
	if ( directionType == TI_WusbStack::DATADIRECTION_IN )
		buffer.append( 0x80 );
	else
		buffer.append('\0' );sendPacketCounter
	*/

	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( (transferFlags & 0xff00) >> 8 );
	buffer.append( transferFlags & 0xff );

	WusbHelperLib::appendPacketLength( buffer, receiveLength );
	WusbHelperLib::appendPacketLength( buffer, urbData->length() );

	// The first (eventually the only) packet with URB
	if ( messageSplit.isEmpty() )
		buffer.append( *urbData );
	else
		buffer.append( messageSplit.first() );

	if ( logger->isTraceEnabled() )
		logger->trace( WusbHelperLib::messageToString( buffer, buffer.length() ) );

	if ( sendPacketCounter == 0 )
		currentTransactionNum = 0;
	sendPacketCounter++;
	if ( logger->isDebugEnabled() )
		logger->debug( QString("Send to hub: ID=%1 tan1=%2 tan2=%3 tan=%4 countMsg=%5").arg(
				QString::number(packetID,16), QString::number(currentSendTransactionNum,16),
				QString::number(currentReceiveTransactionNum,16), QString::number(currentTransactionNum,16),
				messageSplit.isEmpty()? QString::number(1) : QString::number(messageSplit.size()) ) );
	if ( messageSplit.isEmpty() ) {
		delete urbData;
		lastPacketSendTimeMillis = currentTimeMillis();
		return writeToSocket( buffer );
	} else {
		QLinkedListIterator<QByteArray> it( messageSplit );
		it.next();	// omit the first enty since that message is already processed.
		bool res = writeToSocket( buffer );
		sendPacketCounter++;
		lastPacketSendTimeMillis = currentTimeMillis();
		while ( it.hasNext() ) {
			buffer.clear();
			currentSendTransactionNum = (currentSendTransactionNum +1 ) % 256;
			WusbHelperLib::appendTransactionHeader( buffer, currentSendTransactionNum, currentReceiveTransactionNum, currentTransactionNum );
			buffer.append( it.next() );
			res = writeToSocket( buffer );
			lastPacketSendTimeMillis = currentTimeMillis();
		}
		delete urbData;
		return res;
	}
}

/*
 * Slot for incoming (send to network) URBs.
 */
void WusbStack::processURB(
		void * refData, uint16_t transferFlags, uint8_t endPointNo,
		TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
		QByteArray * urbData, uint8_t intervalVal, int expectedReceiveLength ) {

	// the last packet
	packetRefData = refData;
	sendURB( refData, urbData, transferType, dDirection, endPointNo, transferFlags, intervalVal, expectedReceiveLength );
}

void WusbStack::registerURBreceiver( TI_USB_VHCI * urbSink ) {
	urbReceiver = urbSink;
}

void WusbStack::sendAcknowledgeReplyMessage() {
	int tempSendTransactionNum = (currentSendTransactionNum +1 ) % 256;
	int tempTransactionNum = 0;
	if ( sendPacketCounter == 0 )
		tempTransactionNum = 0xff;
	else
		tempTransactionNum = qMin(currentSendTransactionNum, currentReceiveTransactionNum);

	lastPacketSendTimeMillis = currentTimeMillis();
	sendIdleMessage( tempSendTransactionNum, currentReceiveTransactionNum, tempTransactionNum );
}

bool WusbStack::sendIdleMessage( uint8_t sendTAN, uint8_t recTAN, uint8_t tan ) {
	if ( state != WusbStack::STATE_OPENED ) return false;

	QByteArray buffer = QByteArray( 4, 0x10 );
	if ( sendTAN > 0 ) {
		buffer[0] = sendTAN;
		buffer[1] = recTAN;
		buffer[3] = tan;
	} else {
		buffer[0] = currentSendTransactionNum;
		buffer[1] = currentReceiveTransactionNum;
		buffer[3] = currentTransactionNum;
	}

	lastSendAlivePacket = currentTimeMillis();
	return writeToSocket( buffer );
}

void WusbStack::processStatusMessage( WusbMessageBuffer::eTypeOfMessage typeMsg, uint8_t tan1, uint8_t tan2, uint8_t tan3 ) {
	switch( typeMsg ) {
	case WusbMessageBuffer::DEVICE_OPEN_SUCCESS:
		lastPacketReceiveTimeMillis = currentTimeMillis();
		logger->info(QString("Status message: OPEN_SUCCESS") );
		state = STATE_OPENED;
		break;
	case WusbMessageBuffer::DEVICE_CLOSE_SUCCESS:
		lastPacketReceiveTimeMillis = currentTimeMillis();
		logger->info(QString("Status message: CLOSE_SUCCESS") );
		state = STATE_CLOSED;
		break;
	case WusbMessageBuffer::DEVICE_ALIVE:
		lastPacketReceiveTimeMillis = currentTimeMillis();
		if ( logger->isDebugEnabled() )
			logger->debug(QString("Status message: DEVICE_ALIVE") );
		break;
	case WusbMessageBuffer::DEVICE_STALL:
		lastPacketReceiveTimeMillis = currentTimeMillis();
		// an error occured!
		logger->warn(QString("Status message: DEVICE_STALL") );
		// -> send error message to message receiver
		state = STATE_FAILED;
		if ( urbReceiver && packetRefData ) {
			// Procedure for passing URB to OS integration module
			urbReceiver->giveBackAnswerURB( packetRefData, false, NULL );
			packetRefData = NULL;	// XXX this may be not true for isochronous transfer!
		}
		break;
	case WusbMessageBuffer::DEVICE_RECEIVED_DUP:
		lastPacketReceiveTimeMillis = currentTimeMillis();
		sendIdleMessage( tan2, tan1+1, tan2);
		break;
	default:
		// ???
		logger->info(QString("Status message: %1").arg( typeMsg ));
		break;
	}
}
/*
 * Receive URB from network (MessageBuffer).
 */
void WusbStack::processURBmessage( unsigned int packetID, QByteArray * urbBytes ) {
	if ( logger->isTraceEnabled() )
		logger->trace(QString("Received URB: %1").
				arg( WusbHelperLib::messageToString( *urbBytes, urbBytes->length() ) ));

	lastPacketReceiveTimeMillis = currentTimeMillis();

	if ( urbReceiver ) {
		// Procedure for passing URB to OS integration module
		if ( packetID && packetRefDataByPacketID.contains( packetID ) ) {
			// packet lookup positiv -> use reference data from hash
			urbReceiver->giveBackAnswerURB( packetRefDataByPacketID[ packetID ], true, urbBytes );
			packetRefDataByPacketID.remove( packetID );
		} else {
			logger->warn(QString("Fallback of packet ref data (ID = 0x%1)").arg( QString::number( packetID, 16) ) );
//			urbReceiver->giveBackAnswerURB( packetRefData, true, urbBytes );
			packetRefData = NULL;	// XXX this may be not true for isochronous transfer!
		}
	} else {
		// Procedure for passing URB to internal processing (more or less debug code)
		emit receivedURB( *urbBytes );
		delete urbBytes;
	}
}

void WusbStack::informReceivedPacket( int newReceiverTAN, int lastSessionTAN, unsigned int packetCounter ) {
	if ( logger->isTraceEnabled() )
		logger->trace(QString("WusbStack::informReceivedPacket: %1 %2 %3").arg(
				QString::number(newReceiverTAN),QString::number(lastSessionTAN),QString::number(packetCounter,16) ) );
	// XXX this is wrong in some cases (dup messages, retransmit etc.)
	currentReceiveTransactionNum = newReceiverTAN;

}

/* ************** Debug / printf methods ************** */

const QString WusbStack::stateToString() {
	switch ( state ) {
	case STATE_DISCONNECTED:
		return QString("DISCONNECTED");
	case STATE_CONNECTED:
		return QString("CONNECTED");
	case STATE_OPENED:
		return QString("OPENED");
	case STATE_CLOSED:
		return QString("CLOSED");
	case STATE_FAILED:
		return QString("FAILED");
	default:
		return QString("UNKNOWN");
	}
}
