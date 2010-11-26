/*
 * WusbStack.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-09-28
 */

#include "WusbStack.h"
#include "../utils/Logger.h"
#include "WusbReceiverThread.h"
#include "WusbHelperLib.h"
#include "../BasicUtils.h"
#include "../USBconnectionWorker.h"
#include <QtNetwork>
#include <QTimer>
#include <unistd.h>
#include <time.h>



WusbStack::WusbStack( USBconnectionWorker *parent, const QHostAddress & destinationAddress, int destinationPort )
: QObject() {
	logger = parent->getLogger();
	state = STATE_DISCONNECTED;
	destAddress = QHostAddress( destinationAddress );
	destPort = destinationPort;
	haveAnswer = false;
	udpSocket = NULL;
	maxMTU = 1472;	// TODO make this configurable or auto detect
	currentSendTransactionNum = -1;
	currentReceiveTransactionNum = 0;
	currentTransactionNum = 0xff;
	sendPacketCounter = 0;
	lastPacketSendTimeMillis = 0L;
	lastPacketReceiveTimeMillis = 0L;
	lastSendAlivePacket = 0L;
	connectionKeeperTimer = NULL;


	messageBuffer = new WusbMessageBuffer( this );

	connect( messageBuffer, SIGNAL(statusMessage(WusbMessageBuffer::TypeOfMessage)),
			this, SLOT(processStatusMessage(WusbMessageBuffer::TypeOfMessage)), Qt::QueuedConnection );
	connect( messageBuffer, SIGNAL(urbMessage( QByteArray * )),
			this, SLOT(processURBmessage( QByteArray * )), Qt::QueuedConnection );
	connect( messageBuffer, SIGNAL(informPacketMeta( int, int, int )),
			this, SLOT(informReceivedPacket( int, int, int )), Qt::QueuedConnection );

	connect( this, SIGNAL(receivedUDPdata(const QByteArray &)),
			messageBuffer, SLOT(receive(const QByteArray &)), Qt::QueuedConnection);

	logger->info(QString("WusbStack init completed for destination: %1:%2").arg(destAddress.toString(), QString::number(destPort)) );
}

WusbStack::~WusbStack() {
	closeConnection();
	if ( messageBuffer ) delete messageBuffer;
}

Logger * WusbStack::getLogger() {
	return logger;
}

bool WusbStack::openSocket() {
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
	sendBufferMutex.lock();

	int bufSize = buffer.size();
	qint64 retVal = udpSocket->writeDatagram( buffer, destAddress, destPort );
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
			logger->debug(QString("Received %1 bytes from network: %2").arg( QString::number(bytesRead), WusbHelperLib::messageToString( datagram, bytesRead )) );
/*			receiveBufferMutex.lock();
			receiveBuffer.append( datagram );
			receiveBufferMutex.unlock();
*/
			// XXX inform reader thread for new data
//			messageBuffer->receive( datagram );
			emit receivedUDPdata( datagram );
		}
	}
}

void WusbStack::socketError(QAbstractSocket::SocketError socketError) {
	// TODO check/process socket error condition
}

void WusbStack::timerInterrupt() {
	// This is called every 100ms to check if idle/keep-alive messages are neccessary to send
	long long now = currentTimeMillis();
	// sanity check
	if ( !udpSocket || state != STATE_OPENED || lastPacketSendTimeMillis == 0L ) return;

	// Since more than 200ms nothing sent? (and workaround for clock warping...)
	if ( lastPacketSendTimeMillis > now || now - lastPacketSendTimeMillis >= 100L ) {
		if ( now - lastSendAlivePacket > 1000L )
			sendIdleMessage();
	}
}

bool WusbStack::sendURB(
		const char * urbData,
		int urbDataLen,
		DataTransferType dataTransferType,
		DataDirectionType directionType,
		int endpoint,
		int receiveLength  ) {
	int packetLen =	urbDataLen + WUSB_AZUREWAVE_SEND_HEADER_LEN;	// 28 bytes header
	// TODO check if packetLen exceeds MTU size and split packet eventually
	if ( packetLen > maxMTU )
		packetLen = maxMTU;
	QByteArray buffer = QByteArray();
	buffer.reserve( packetLen );
	WusbHelperLib::appendTransactionHeader( buffer, 0, 0, 0xff );	// XXX
	WusbHelperLib::appendMarker55Header( buffer );
	WusbHelperLib::appendPacketCountHeader( buffer );

	buffer.append( 0x80 );
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( 0x80 );
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( '\0' );

	WusbHelperLib::appendPacketLength( buffer, receiveLength );
	WusbHelperLib::appendPacketLength( buffer, urbDataLen );

	// last the URB itself
	buffer.append( urbData, urbDataLen );

	logger->debug( messageToString( buffer, buffer.length() ) );
	return writeToSocket( buffer );
}
bool WusbStack::sendURB( const QByteArray & urbData, DataTransferType dataTransferType, DataDirectionType directionType, int endpoint, int receiveLength ) {
	int packetLen =	urbData.length() + WUSB_AZUREWAVE_SEND_HEADER_LEN;	// +28 bytes header
	// TODO check if packetLen exceeds MTU size and split packet eventually
	if ( packetLen > maxMTU )
		packetLen = maxMTU;
	QByteArray buffer = QByteArray();
	buffer.reserve( packetLen );

	currentSendTransactionNum = (currentSendTransactionNum +1 ) % 256;
	if ( sendPacketCounter == 0 )
		currentTransactionNum = 0xff;
	else
		currentTransactionNum = qMax(currentSendTransactionNum, currentReceiveTransactionNum);
	WusbHelperLib::appendTransactionHeader( buffer, currentSendTransactionNum, currentReceiveTransactionNum, currentTransactionNum );
	WusbHelperLib::appendMarker55Header( buffer );
	WusbHelperLib::appendPacketCountHeader( buffer );

	buffer.append( 0x80 );
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( 0x80 );
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( '\0' );

	WusbHelperLib::appendPacketLength( buffer, receiveLength );
	WusbHelperLib::appendPacketLength( buffer, urbData.length() );

	// last the URB itself
	buffer.append( urbData );

	logger->debug( WusbHelperLib::messageToString( buffer, buffer.length() ) );

	if ( sendPacketCounter == 0 )
		currentTransactionNum = 0;
	sendPacketCounter++;
	lastPacketSendTimeMillis = currentTimeMillis();
	return writeToSocket( buffer );
}

bool WusbStack::sendIdleMessage() {
	if ( state != WusbStack::STATE_OPENED ) return false;

	QByteArray buffer = QByteArray( 4, 0x10 );
	buffer[0] = currentSendTransactionNum;
	buffer[1] = currentReceiveTransactionNum;
	buffer[3] = currentTransactionNum;

	lastSendAlivePacket = currentTimeMillis();
	return writeToSocket( buffer );
}

void WusbStack::processStatusMessage( WusbMessageBuffer::TypeOfMessage typeMsg ) {
	switch( typeMsg ) {
	case WusbMessageBuffer::DEVICE_OPEN_SUCCESS:
		logger->info(QString("Status message: OPEN_SUCCESS") );
		state = STATE_OPENED;
		break;
	case WusbMessageBuffer::DEVICE_CLOSE_SUCCESS:
		logger->info(QString("Status message: CLOSE_SUCCESS") );
		state = STATE_CLOSED;
		break;
	case WusbMessageBuffer::DEVICE_ALIVE:
		logger->info(QString("Status message: DEVICE_ALIVE") );
		break;
	default:
		// ???
		logger->info(QString("Status message: %1").arg( typeMsg ));
		break;
	}
}
void WusbStack::processURBmessage( QByteArray * urbBytes ) {
	logger->trace(QString("Received URB: %1").arg( WusbHelperLib::messageToString( *urbBytes, urbBytes->length() ) ));

	emit receivedURB( *urbBytes );
}

void WusbStack::informReceivedPacket( int newReceiverTAN, int lastSessionTAN, int packetCounter ) {
	logger->trace(QString("WusbStack::informReceivedPacket: %1 %2 %3").arg(
			QString::number(newReceiverTAN),QString::number(lastSessionTAN),QString::number(packetCounter,16) ) );
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
