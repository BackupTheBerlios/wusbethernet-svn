/**
 * HubDevice.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-06-21
 */

#include "HubDevice.h"
#include "XMLmessageDOMparser.h"
#include "ConnectionController.h"
#include "WusbStack.h"
#include "../TI_WusbStack.h"
#include "../ConfigManager.h"
#include "../Textinfoview.h"
#include "../utils/Logger.h"
#include "../BasicUtils.h"
#include <QtNetwork>
#include <QString>
#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QDateTime>
#include <QListIterator>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>


HubDevice::HubDevice( const QHostAddress & address, ConnectionController * controller, int devNumber ) {
	ipAddress = QHostAddress( address );
	refController = controller;
	// setup div names and values:
	visualTreeWidgetItem = NULL;
	deviceNumber = devNumber;
	name = "n/a";
	protocol = "n/a";
	manufacturer = "n/a";
	modelName = "n/a";
	deviceName = "n/a";
	firmwareVersion = "n/a";
	firmwareDate = "n/a";
	wantServerInfoRequest = false;
	errorCounter = 0;

	logger = Logger::getLogger( QString("HUB") + QString::number(devNumber) );
	logger->info(QString::fromLatin1("Network hub device found and at IP %1 - initiating communication").arg(
					address.toString()) );

	alive = false;	// initial value
	aliveTimer = NULL;
	controlConnectionSocket = NULL;

	controlConnectionPortNum = DEFAULT_DEVICE_CONTROL_PORT;	// TODO port number configurable?!
	aliveTimerInterval = DEFAULT_DEVICE_CONTROL_ALIVE_INTERVAL;	// TODO alive timer interval configurable!
	receiveBuffer = new ControlMessageBuffer( this );

	for ( int i = 0; i < 10; i++ ) {
		deviceList.append( new USBTechDevice(this) );
		deviceList[i]->isValid = false;
		deviceList[i]->sortNumber = i;
		deviceList[i]->visualTreeWidgetItem = NULL;
	}

	// open TCP control connection and get device information
	if ( ! openControlConnection( controlConnectionPortNum ) ) {
		alive = false;
		controlConnectionSocket = NULL;
	} else if ( queryDeviceInfo() ) {
		alive = true;
		startAliveTimer();
	}
}

HubDevice::~HubDevice() {
	if ( aliveTimer )
		aliveTimer->stop();
	if ( controlConnectionSocket ) {
		controlConnectionSocket->abort();
		controlConnectionSocket->close();
	}
}

Logger * HubDevice::getLogger() {
	return logger;
}

TI_WusbStack * HubDevice::createStackForDevice( const QString & deviceID ) {
	USBTechDevice & deviceRef = findDeviceByID( deviceID );
	if ( deviceRef.isValid ) {
		Logger * logger = Logger::getLogger( QString("USBConn") + QString::number( deviceRef.connectionPortNum) );
		return new WusbStack( logger, QHostAddress(ipAddress), deviceRef.connectionPortNum );
	} else
		return NULL;
}



void HubDevice::setXMLdiscoveryData( int len, const QByteArray & payloadData ) {
	ControlMsg_DiscoveryResponse *parseResult = XMLmessageDOMparser::parseDiscoveryMessage( payloadData );
	if ( parseResult ) {
		name = parseResult->name;
		delete parseResult;
	} else {
		logger->error( "Hub device: Could not parse discovery message from network!" );
		alive = false;
	}
	if ( logger->isTraceEnabled() )
		logger->trace(QString::fromLatin1("Raw string data[%1] = %2").arg(
				QString::number(len), messageToString( payloadData, payloadData.length() ) ) );
	logger->debug(QString::fromLatin1("Hub device name = %1").arg( name ) );
}

void HubDevice::startAliveTimer() {
	aliveTimer = new QTimer(this);
	connect(aliveTimer, SIGNAL(timeout()), this, SLOT(sendAliveRequest()));
	aliveTimer->start( aliveTimerInterval );
}

bool HubDevice::openControlConnection( int portNum ) {
	controlConnectionSocket = new QTcpSocket( this );
	controlConnectionSocket->setSocketOption( QAbstractSocket::LowDelayOption, 1 );

	logger->info( QString::fromAscii("Hub device: Open control connection to %1:%2").arg( ipAddress.toString(), QString::number(portNum) ) );
	controlConnectionSocket->connectToHost( ipAddress, portNum );
	connect(controlConnectionSocket, SIGNAL(readyRead()), this, SLOT(readControlConnectionMessage()));
	connect(controlConnectionSocket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(notifyControlConnectionError(QAbstractSocket::SocketError)));

	return controlConnectionSocket->waitForConnected( 1500 );
}


/* not needed anymore... */
int HubDevice::createClientSocket( const char *hostname, int localport, int peerport ) {
	struct sockaddr_in sa, socketName;
	struct hostent *hp;
	int sd;
	long addr;

	bzero( &sa, sizeof(sa) );
	if ( (addr = inet_addr(hostname)) != -1) { // hostname is given as IP-address
		bcopy(&addr, (char*) &sa.sin_addr, sizeof(addr));
		sa.sin_family = AF_INET;
	} else {
		if ( (hp = gethostbyname(hostname)) == NULL ) { // hostname is given as literal name
			return(-2);
		}
		bcopy(hp->h_addr, (char*) &sa.sin_addr, hp->h_length);
		sa.sin_family = hp->h_addrtype;
	}
	sa.sin_port = htons( peerport );
	printf("Verbinden mit %s ... ", inet_ntoa(sa.sin_addr));

	if ((sd = socket(sa.sin_family, SOCK_STREAM, 0)) < 0) { // create socket
		perror("hubConnect: opening socket");
		return( -1 );
	}

	int optval;
	// setting SO_REUSEADDR to socket
	optval = 1;
	setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	// disable nagle algorithm (since we are only sending small packets)
	optval = 1;
	setsockopt( sd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval );


	if ( localport > 1 ) {
		bzero((char*) &socketName, sizeof(socketName));
		socketName.sin_family = AF_INET;
		socketName.sin_port = htons( localport );
		socketName.sin_addr.s_addr = htonl(INADDR_ANY);

		if ( bind(sd, (struct sockaddr*) &socketName, sizeof(socketName)) < 0) {
			perror("hubConnect: Binding of socket failed");
			return( -3 );
		} else
			printf("Bind successfull\n");
	}

	if( ::connect( sd,(struct sockaddr*) &sa, sizeof(sa) ) < 0 ){
		perror("hubConnect: unable to connect");
		close(sd);
		return(-4);
	}
	return( sd );
}

void HubDevice::readControlConnectionMessage() {
	if (controlConnectionSocket->bytesAvailable() < 1)
		return;


	QByteArray bytesRead = controlConnectionSocket->readAll();
	receiveBuffer->receive( bytesRead );
}

void HubDevice::receiveData( ControlMessageBuffer::eTypeOfMessage type, const QByteArray & bytes ) {
	QString recData;
	if ( type != ControlMessageBuffer::TOM_NOP )
		errorCounter = 0;
	switch( type ) {
	case ControlMessageBuffer::TOM_ALIVE:
		logger->trace( "alive" );
		alive = true;
		lastSeenTimestamp = time(0);
		setToolTipText();
		if ( wantServerInfoRequest ) {
			wantServerInfoRequest = false;
			queryDeviceInfo();
		}
		break;
	case ControlMessageBuffer::TOM_SERVERINFO:
		lastSeenTimestamp = time(0);
		XMLmessageDOMparser::parseServerInfoMessage( bytes, this );
		refController->drawVisualTree();
		break;
	case ControlMessageBuffer::TOM_IMPORTINFO:
	{
		// Answer to an "import" request
		lastSeenTimestamp = time(0);
		USBTechDevice & usedDevice = XMLmessageDOMparser::parseImportResponseMessage( bytes, this );
		if ( usedDevice.isValid ) {
			if ( logger->isDebugEnabled() )
				logger->debug( QString::fromAscii("ImportInfo for device: %1 - Result: %2 - ConnectingToPort: %3").arg(
						usedDevice.deviceID,
						QString::number(usedDevice.lastOperationErrorCode),
						QString::number(usedDevice.connectionPortNum) ) );

			switch ( usedDevice.nextJobID ) {
			case USBTechDevice::JA_INTERNAL_QUERY_DEVICE:
				// device query operation
				queryDeviceJob( usedDevice );
				break;

			case USBTechDevice::JA_CONNECT_DEVICE:
				// connect to virtual USB port
				connectDeviceJob( usedDevice );
				break;
			}
		}
		break;
	}	// brackets are needed to keep compiler happy...
	case ControlMessageBuffer::TOM_UNIMPORT:
		// This is an outgoing AND incoming message!
		//  Message is send/received to signal a "release device" request
		// If a claimed-by-us device should be "unimported":
		// this is normaly done by closing the device on data channel...
		// -> Receive of this message should pop up a message box
		lastSeenTimestamp = time(0);
		if ( bytes.length() > 10 ) {
			ControlMsg_UnimportRequest * unimportMsg = XMLmessageDOMparser::parseUnimportMessage( bytes );
			if ( unimportMsg ) {
				if ( logger->isDebugEnabled() )
					logger->debug( QString("UnImportInfo from host: %1 for device %2").
							arg( unimportMsg->hostname, unimportMsg->deviceID )  );
				emit userInfoMessage( "unimport",
						tr("User on host %1 requesting access to device: <br><center>%2</center><br><b>Disconnect?</b>").
						arg( unimportMsg->hostname, unimportMsg->deviceID ), 2 );
				delete unimportMsg;
			}
		}
		break;
	case ControlMessageBuffer::TOM_STATUSCHANGED:
	{
		lastSeenTimestamp = time(0);
		QStringList sl = XMLmessageDOMparser::parseStatusChangedMessage( bytes, this );
		if ( logger->isDebugEnabled() )
			logger->debug( QString::fromAscii("Status changed for device(s): %1").arg( sl.join(", ") ) );
		refreshAllWidgetItemForDevice();
		wantServerInfoRequest = true;
		break;
	}	// keeps the compiler happy...
	case ControlMessageBuffer::TOM_MSG66:
		logger->debug( "Received message type 0x66!" );
		break;
	case ControlMessageBuffer::TOM_NOP:
		logger->debug( "NOP" );
	}
}


void HubDevice::notifyControlConnectionError( QAbstractSocket::SocketError socketError ) {
//	aliveTimer->stop();
	alive = false;
	if ( controlConnectionSocket ) {
		logger->warn(QString::fromLatin1("SocketError: %1").arg(controlConnectionSocket->errorString()) );
		disconnect( controlConnectionSocket, SIGNAL(readyRead()), this, SLOT(readControlConnectionMessage()) );
		disconnect(controlConnectionSocket, SIGNAL(error(QAbstractSocket::SocketError)),
				this, SLOT(notifyControlConnectionError(QAbstractSocket::SocketError)));
		controlConnectionSocket->abort();
		controlConnectionSocket->close();

		controlConnectionSocket = NULL;
		errorCounter++;
	}
}

bool HubDevice::queryDeviceInfo() {
	if ( !controlConnectionSocket )
		return false;
	// The XML fragment to send to USB hub
	QString str = QString("<getServerInfo/>");

	QByteArray buffer;
	// First: create header 66 66 65 00 00 10 3c 67 [..]
	buffer.append( 'f' ); // 0x66
	buffer.append( 'f' ); // 0x66
	buffer.append( 'e' ); // 0x65
	buffer.append( '\0' ); // 0x00
	buffer.append( '\0' ); // 0x00
	buffer.append( 0x10 ); // 0x10 == Length 16 bytes payload
	// Append XML query
	buffer.append( str );
	// write all to network
	qint64 bytesWritten = controlConnectionSocket->write( buffer );
	if ( bytesWritten <= 0 )
		return false;
	return true;
}

/**
 * Create and send an <tt>import</tt> message to hub to take ownership
 * of a specific device.
<code>
0000   66 66 68 00 00 9e 3c 69 6d 70 6f 72 74 3e 3c 68  ffh...<import><h
0010   6f 73 74 4e 61 6d 65 3e 64 75 73 73 65 6c 3c 2f  ostName>dussel</
0020   68 6f 73 74 4e 61 6d 65 3e 3c 64 65 76 69 63 65  hostName><device
0030   49 44 20 74 79 70 65 3d 22 68 65 78 22 3e 30 30  ID type="hex">00
0040   36 34 62 61 38 31 3c 2f 64 65 76 69 63 65 49 44  64ba81</deviceID
0050   3e 3c 69 64 56 65 6e 64 6f 72 20 74 79 70 65 3d  ><idVendor type=
0060   22 68 65 78 22 3e 30 39 30 63 3c 2f 69 64 56 65  "hex">090c</idVe
0070   6e 64 6f 72 3e 3c 69 64 50 72 6f 64 75 63 74 20  ndor><idProduct
0080   74 79 70 65 3d 22 68 65 78 22 3e 31 30 30 30 3c  type="hex">1000<
0090   2f 69 64 50 72 6f 64 75 63 74 3e 3c 2f 69 6d 70  /idProduct></imp
00a0   6f 72 74 3e                                      ort>
</code>
 */
bool HubDevice::sendImportDeviceMessage( const QString & deviceID, const QString & deviceVendor,const QString & deviceProd ) {
	if ( !controlConnectionSocket )
		return false;

	// get vendor/product-ID with trailing '0'
	QString vendorID = deviceVendor;
	if ( vendorID.length() < 4 )
		vendorID = vendorID.rightJustified(4, '0', true);
	QString prodID = deviceProd;
	if ( prodID.length() < 4 )
		prodID = prodID.rightJustified(4, '0', true);

	if ( logger->isInfoEnabled() )
		logger->info( QString("Sending import request for device: %1 (%2/%3) on host: %4").arg(
				deviceID, vendorID, prodID,
				ConfigManager::getInstance().getStringValue("hostname","localhost") ) );

	// The XML fragment to send to USB hub
	QString str = QString("<import>"
			"<hostName>%1</hostName>"
			"<deviceID type=\"hex\">%2</deviceID>"
			"<idVendor type=\"hex\">%3</idVendor>"
			"<idProduct type=\"hex\">%4</idProduct>"
			"</import>").arg(
					ConfigManager::getInstance().getStringValue("hostname","localhost"),
					deviceID, vendorID, prodID
					);
	QByteArray buffer;
	// First: create header 66 66 65 00 00 10 3c 67 [..]
	buffer.append( 'f' ); // 0x66
	buffer.append( 'f' ); // 0x66
	buffer.append( 'h' ); // 0x68
	int lenPayload = str.length();
	buffer.append( (lenPayload & 0x00ff0000) >> 16 );	// 3.byte of length
	buffer.append( (lenPayload & 0x0000ff00) >> 8 );	// 2.byte of length
	buffer.append( (lenPayload & 0x000000ff) );		 	// LSB byte of length
	// append XML query
	buffer.append( str );
	// write all to network
	qint64 bytesWritten = controlConnectionSocket->write( buffer );
	if ( bytesWritten <= 0 )
		return false;
	return true;
}

/**
 * Create and send an <tt>unimport</tt> message to hub to request
 * release of a specific device claimed by other host.
 * <code>
0000   66 66 69 00 00 58 3c 75 6e 69 6d 70 6f 72 74 3e  ffi..X<unimport>
0010   3c 68 6f 73 74 4e 61 6d 65 3e 64 75 73 73 65 6c  <hostName>dussel
0020   3c 2f 68 6f 73 74 4e 61 6d 65 3e 3c 64 65 76 69  </hostName><devi
0030   63 65 49 44 20 74 79 70 65 3d 22 68 65 78 22 3e  ceID type="hex">
0040   64 64 63 63 62 62 61 61 3c 2f 64 65 76 69 63 65  ddccbbaa</device
0050   49 44 3e 3c 2f 75 6e 69 6d 70 6f 72 74 3e        ID></unimport>
</code>
*/
bool HubDevice::sendUnimportMessage( const QString & deviceID, const QString & message ) {
	if ( !controlConnectionSocket )
		return false;
	ConfigManager & confMgr = ConfigManager::getInstance();

	if ( logger->isInfoEnabled() )
		logger->info( QString("Sending unimport request for device: %1 on host: %2").arg(
				deviceID,
				confMgr.getStringValue("hostname","localhost") ) );

	// optionally include username into unimport request (username@hostname)
	// NOTE: unknown if this brings trouble to specific firmware or client software releases???
	QString hostname;
	if ( confMgr.getBoolValue( "azurewave.devctrl.addUnimportUsername", false ) )
		hostname = QString("%1@%2").arg(
				confMgr.getStringValue("username","nobody"),
				confMgr.getStringValue("hostname","localhost") );
	else
		hostname = confMgr.getStringValue("hostname","localhost");

	// The XML fragment to send to USB hub
	QString str = QString("<unimport>"
			"<hostName>%1</hostName>"
			"<deviceID type=\"hex\">%2</deviceID>"
			"<message>blubba</message>"
			"</unimport>").arg(
					hostname,
					deviceID );
	QByteArray buffer;
	// First: create header 66 66 69 00 00 10 3c 67 [..]
	buffer.append( 'f' ); // 0x66
	buffer.append( 'f' ); // 0x66
	buffer.append( 'i' ); // 0x69
	int lenPayload = str.length();
	buffer.append( (lenPayload & 0x00ff0000) >> 16 );	// 3.byte of length
	buffer.append( (lenPayload & 0x0000ff00) >> 8 );	// 2.byte of length
	buffer.append( (lenPayload & 0x000000ff) );		 	// LSB byte of length
	// append XML query
	buffer.append( str );
	// write all to network
	qint64 bytesWritten = controlConnectionSocket->write( buffer );
	if ( bytesWritten <= 0 )
		return false;
	return true;
}

/**
 * Return state if this network hub device is still alive or unreachable.
 * This only takes into account if the hub device is reachable over network and
 * answers correctly to corresponding requests.
 * @return State of network hub device
 */
bool HubDevice::isAlive() {
	return alive;
}

void HubDevice::sendAliveRequest() {
	if ( !controlConnectionSocket || !controlConnectionSocket->isOpen() ) {
		if ( errorCounter > 3 )
			return;
		else {
			logger->warn( QString("Connection to hub lost - trying reconnect... (error counter = %1)").arg(
					QString::number(errorCounter) ) );
			if ( !openControlConnection( controlConnectionPortNum ) ) {
				logger->warn( QString("Reconnect failed (%1. try)").arg( QString::number(errorCounter) ) );
				return;
			}
			logger->warn( "Reconnect sucess!" );
		}
	}
	if ( logger->isDebugEnabled() )
		logger->debug("sending alive request...");
	// 66 66 6a 00 00 00
	QByteArray buffer;

	// First: create header
	buffer.append( 0x66 ); // 'f'
	buffer.append( 0x66 ); // 'f'
	buffer.append( 0x6a ); // 'j'
	buffer.append( '\0' );
	buffer.append( '\0' );
	buffer.append( '\0' );
	// write everything to network
	qint64 bytesWritten = controlConnectionSocket->write( buffer );
	if ( bytesWritten <= 0 ) {
		alive = false;
	}
}

USBTechDevice & HubDevice::findDeviceByID( const QString & deviceID ) {
//	LogWriter::info(QString("findDeviceByID: '%1'").arg( deviceID ));
	QListIterator<USBTechDevice*> it( deviceList );
	int idx = -1;
	while ( it.hasNext() ) {
		USBTechDevice * usbDev = it.next();
		idx++;
		if ( usbDev->isValid && !usbDev->deviceID.isNull() && usbDev->deviceID.compare( deviceID, Qt::CaseInsensitive ) == 0 ) {
			return *(deviceList[idx]);
		}
	}
	USBTechDevice * fooDev = new USBTechDevice( this );
	fooDev->isValid = false;
	return *fooDev;
}

QString HubDevice::toString() {
	QString retValue = QString("IP:%1").arg( ipAddress.toString() );
	return retValue;
}

/* ****** Methods to get a visual representation (tree) ****** */


QTreeWidgetItem * HubDevice::getQTreeWidgetItem( QTreeWidget * view ) {
	if ( !visualTreeWidgetItem ) {
		// the tree item for the hub itself
		QString infoStr = QString( "%1 (%2)" ).arg( name ).arg(ipAddress.toString());
		visualTreeWidgetItem = new QTreeWidgetItem( view, QStringList( infoStr ) );
		visualTreeWidgetItem->setIcon(0, QIcon(":/icons/images/usbHubIcon.png") );
		visualTreeWidgetItem->setData(0, Qt::UserRole, QVariant::fromValue<USBTechDevice*>( NULL ) );
	}
	setToolTipText();

	// remove all child items

	for ( int c = visualTreeWidgetItem->childCount() -1; c >= 0; c-- ) {
		QTreeWidgetItem * item = visualTreeWidgetItem->child( c );
		visualTreeWidgetItem->removeChild( item );
	}

	// and create/append items for each connected USB device
	QList<USBTechDevice*>::iterator it;
	for ( it = deviceList.begin(); it != deviceList.end(); ++it ) {
		USBTechDevice * usbDev = (*it);

		if ( usbDev->isValid ) {
			visualTreeWidgetItem->addChild( getQTreeWidgetItemForDevice( *usbDev ) );
		} else if ( usbDev->visualTreeWidgetItem )
			usbDev->visualTreeWidgetItem->setHidden( true );
	}

	visualTreeWidgetItem->setExpanded( true );
	return visualTreeWidgetItem;
}

QTreeWidgetItem * HubDevice::getQTreeWidgetItemForDevice( USBTechDevice & usbDevice ) {
	if ( !usbDevice.visualTreeWidgetItem ) {
		usbDevice.visualTreeWidgetItem = new QTreeWidgetItem( (QTreeWidget*)0, QStringList( usbDevice.product ) );
		usbDevice.visualTreeWidgetItem->setIcon(0, QIcon( getIconResourceForDevice( usbDevice ) ) );
		usbDevice.visualTreeWidgetItem->setCheckState( 0, Qt::Unchecked );
		// set the reference to specific device as "data" for this item
		usbDevice.visualTreeWidgetItem->setData(0, Qt::UserRole, QVariant::fromValue<USBTechDevice*>( &usbDevice ) );
	} else
		usbDevice.visualTreeWidgetItem->setHidden( false );


	int classID = usbDevice.bClass;
	int subClassID = usbDevice.bSubClass;
	if ( classID == 0 && !usbDevice.interfaceList.isEmpty() ) {
		classID = usbDevice.interfaceList[0].if_class;
		subClassID = usbDevice.interfaceList[0].if_subClass;
	}
	QString claimedText = "";	// Tooltip text (part of)
	QString usageHintText = ""; // Tooltip text for usage warning
	if ( usbDevice.usageHint != 0 ) {
		usageHintText = tr("<b><i>Warning:</i> <font color=\"red\">Usage maybe degraded</font></b><br>");
	}
	if ( usbDevice.status == USBTechDevice::PS_Claimed ) {
		claimedText = QString("<br>Claimed by: <em>%1 (%2)</em>").arg( usbDevice.claimedByName, usbDevice.claimedByIP );
		// text to display in tree widget (right to check box)
		// TODO checkout why its not working / how to display html formatted text
		usbDevice.visualTreeWidgetItem->setText(0, tr("%1 - Used by %2 (%3)").arg( usbDevice.product, usbDevice.claimedByName, usbDevice.claimedByIP ) );
		if ( usbDevice.owned ) {
			usbDevice.visualTreeWidgetItem->setBackgroundColor( 0, QColor( 0, 255, 0, 127 ) );
			usbDevice.visualTreeWidgetItem->setCheckState( 0, Qt::Checked );
		} else {
			usbDevice.visualTreeWidgetItem->setBackgroundColor( 0, QColor( 255, 0, 0, 127 ) );
			usbDevice.visualTreeWidgetItem->setCheckState( 0, Qt::Unchecked );
		}
	} else {
		if ( usbDevice.usageHint == 0 )
			usbDevice.visualTreeWidgetItem->setBackgroundColor( 0, QColor( Qt::white) );
		else
			usbDevice.visualTreeWidgetItem->setBackgroundColor( 0, QColor( Qt::lightGray ) );
		usbDevice.visualTreeWidgetItem->setCheckState( 0, Qt::Unchecked );
		usbDevice.visualTreeWidgetItem->setText(0, usbDevice.product );
	}
	usbDevice.visualTreeWidgetItem->setToolTip(0,
			tr("<html>%1<b>USB device: <em>%2</em></b><br>"
			"Manufacturer: <em>%3</em><br>"
			"Connected port: <em>%4</em><br>"
			"ID: <em>%5</em><br>"
			"Vendor/Product: <em>0x%6/0x%7</em><br>"
			"Version: <em>%8</em><br>"
			"USB type: <em>%9</em><br>"
			"USB class: <em>0x%10:0x%11</em><br>"
			"Num. interfaces: <em>%12</em>%13</html>").
			arg( usageHintText, usbDevice.product, usbDevice.manufacturer, QString::number(usbDevice.portNum),
					usbDevice.deviceID, QString::number(usbDevice.idVendor, 16),
					QString::number(usbDevice.idProduct, 16), usbDevice.sbcdDevice, usbDevice.sbcdUSB ).
					arg(QString::number(classID,16), QString::number(subClassID,16 ),
							QString::number( usbDevice.interfaceList.size()), claimedText ) );
	return usbDevice.visualTreeWidgetItem;
}

void HubDevice::refreshAllWidgetItemForDevice() {
	QList<USBTechDevice*>::iterator it;
	for ( it = deviceList.begin(); it != deviceList.end(); ++it ) {
		USBTechDevice * usbDev = (*it);

		if ( usbDev->isValid ) {
			logger->debug( QString("Refresh item for USBdev: %1").arg(usbDev->deviceID) );
			getQTreeWidgetItemForDevice( *usbDev );
		} else if ( usbDev->visualTreeWidgetItem ) {
			logger->debug("Setting hidden flag for USBdev");
			usbDev->visualTreeWidgetItem->setHidden( true );
		}
	}
}

void HubDevice::setToolTipText() {
	visualTreeWidgetItem->setToolTip( 0,
			tr("<html><b>Device: <em>%1</em></b><br>"
					"Model: <em>%2</em><br>"
					"Manufacturer: <em>%3</em><br>"
					"Version: <em>%4</em><br>"
					"&nbsp;&nbsp;&nbsp;&nbsp; <em>%5</em><br>"
					"Protocol: <em>%6</em><br>"
					"Contact: %7</html>").
					arg( deviceName,
							modelName,
							manufacturer,
							firmwareVersion, firmwareDate,
							protocol,
							QDateTime::fromTime_t( lastSeenTimestamp ).toString("hh:mm:ss") ) );
}


QString HubDevice::getIconResourceForDevice( USBTechDevice & usbDevice ) {
	// TODO retrieve type of device
	// Source: http://www.usb.org/developers/defined_class
	// -> default icon
	int usbDeviceClass = usbDevice.bClass;
	if ( usbDeviceClass == 0 && usbDevice.interfaceList.size() > 0 )
		usbDeviceClass = usbDevice.interfaceList[0].if_class;

	switch ( usbDeviceClass ) {
	case 0x01:
		// Audio
		return QString(":/icons/images/speaker.png");
	case 0x02:
		// CDC / Communications Device
		// TODO check if interpretation of network interface is correct
		return QString(":/icons/images/network-wireless.png");
	case 0x03:
		// HID / Human Interface Device
		// TODO distinguish between keyboard, mouse etc.
		return QString(":/icons/images/input-keyboard.png");
	case 0x05:
		// Physical Device (?)
		break;
	case 0x06:
		// Still Imaging Device (Scanner etc)
		break;
	case 0x07:
		// Printer
		return QString(":/icons/images/printer.png");
	case 0x08:
		// Mass Storage
		// TODO distinguish between harddrive, pen-drive, optical etc.
		return QString(":/icons/images/drive-removable-media-usb.png");
	case 0x09:
		// USB hub
		break;
	case 0x0a:
		// CDC Data
		break;
	case 0x0b:
		// Smart card
		return QString(":/icons/images/secure-card.png");
	case 0x0d:
		// Content Security (?)
		break;
	case 0x0e:
		// Video
		return QString(":/icons/images/camera-web.png");
	case 0x0f:
		// Personal Healthcare
		break;
	case 0xdc:
		// Diagnostic Device (?)
		break;
	case 0xe0:
		// Wireless controller (i.e. bluetooth device)
		return QString(":/icons/images/network-wireless.png");
	case 0xef:
		// Miscellaneous, often mobile devices with sync (Palm-Sync, Active Sync, etc.)
		return QString(":/icons/images/multimedia-player.png");
	}
	return QString(":/icons/images/icon_usb_logo.png");
}



/* ********** Callback methods / User initiated functions ********** */

void HubDevice::queryDevice( USBTechDevice * deviceRef ) {
	if ( ! deviceRef ) return;
	if ( ! deviceRef->isValid || deviceRef->owned || deviceRef->status != USBTechDevice::PS_Plugged ||
			(deviceRef->connWorker && deviceRef->connWorker->getLastExitCode() == USBconnectionWorker::WORK_DONE_STILL_RUNNING ) ) {
		logger->warn( QString("Query OP: Device not valid or not available (valid=%1, owned=%2, status=%3").arg(
				deviceRef->isValid? QString("true") : QString("false"),
				deviceRef->owned? QString("true") : QString("false"),
				QString::number( (int) deviceRef->status )
		) );
		return;
	}

	// Register connection on control channel
	sendImportDeviceMessage( deviceRef->deviceID,
			QString::number(deviceRef->idVendor, 16),
			QString::number(deviceRef->idProduct, 16) );

	deviceRef->nextJobID = USBTechDevice::JA_INTERNAL_QUERY_DEVICE;
}

void HubDevice::queryDeviceJob( USBTechDevice & deviceRef ) {
	if ( deviceRef.lastOperationErrorCode != 0 ) {
		logger->warn( "Could not use device cause claim-device-operation failed!" );
		return;
	}
	if ( !deviceRef.connWorker )
		deviceRef.connWorker = new USBconnectionWorker( this, &deviceRef );
	connect( deviceRef.connWorker, SIGNAL(workIsDone(USBconnectionWorker::eWorkDoneExitCode, USBTechDevice*)),
			this, SLOT(connectionWorkerJobDone(USBconnectionWorker::eWorkDoneExitCode, USBTechDevice*)), Qt::QueuedConnection );
	deviceRef.connWorker->queryDevice( QHostAddress(ipAddress), deviceRef.connectionPortNum );
	deviceRef.nextJobID = USBTechDevice::JA_NONE;
}

void HubDevice::connectDeviceJob( USBTechDevice & deviceRef ) {
	if ( deviceRef.lastOperationErrorCode != 0 ) {
		logger->warn( "Could not use device cause claim-device-operation failed!" );
		return;
	}
	if ( !deviceRef.connWorker )
		deviceRef.connWorker = new USBconnectionWorker( this, &deviceRef );
	connect( deviceRef.connWorker, SIGNAL(workIsDone(USBconnectionWorker::eWorkDoneExitCode, USBTechDevice*)),
			this, SLOT(connectionWorkerJobDone(USBconnectionWorker::eWorkDoneExitCode, USBTechDevice*)), Qt::QueuedConnection );
	deviceRef.connWorker->connectDevice( QHostAddress(ipAddress), deviceRef.connectionPortNum );
	deviceRef.nextJobID = USBTechDevice::JA_NONE;
}

void HubDevice::disconnectDevice( USBTechDevice * deviceRef ) {
	if ( ! deviceRef ) return;
	if ( ! deviceRef->isValid || deviceRef->status != USBTechDevice::PS_Claimed ||
			(deviceRef->connWorker && deviceRef->connWorker->getLastExitCode() == USBconnectionWorker::WORK_DONE_STILL_RUNNING ) ) {
		logger->warn( QString("Disconnect OP: Device not valid or not available (valid=%1, owned=%2, status=%3").arg(
				(deviceRef->isValid? QString("true") : QString("false")),
				(deviceRef->owned? QString("true") : QString("false")),
				QString::number( (int) deviceRef->status )
		) );
		return;
	}
	if ( deviceRef->status == USBTechDevice::PS_Claimed && deviceRef->owned ) {
		// TODO Device is claimed by us -> need to implement disconnect procedure!
		logger->warn( QString("Sorry: Device disconnect not implemented yet...") );
	} else if ( deviceRef->status == USBTechDevice::PS_Claimed && !deviceRef->owned ) {
		// send "unimport" message to hub
		sendUnimportMessage( deviceRef->deviceID, QString::null );
		// no direct answer from device expected!
		//  (sometimes a "status changed" message is emmitted -> processed in normal way
		deviceRef->nextJobID = USBTechDevice::JA_NONE;
	}
}

void HubDevice::connectDevice( USBTechDevice * deviceRef ) {
	if ( ! deviceRef ) return;
	if ( ! deviceRef->isValid || deviceRef->status != USBTechDevice::PS_Plugged ||
			(deviceRef->connWorker && deviceRef->connWorker->getLastExitCode() == USBconnectionWorker::WORK_DONE_STILL_RUNNING ) ) {
		logger->warn( QString("Connect OP: Device not valid or not available (valid=%1, owned=%2, status=%3").arg(
				(deviceRef->isValid? QString("true") : QString("false")),
				(deviceRef->owned? QString("true") : QString("false")),
				QString::number( (int) deviceRef->status )
		) );
		// Register connection on control channel
		sendImportDeviceMessage( deviceRef->deviceID,
				QString::number(deviceRef->idVendor, 16),
				QString::number(deviceRef->idProduct, 16) );

		deviceRef->nextJobID = USBTechDevice::JA_CONNECT_DEVICE;
		return;
	}

}

void HubDevice::connectionWorkerJobDone( USBconnectionWorker::eWorkDoneExitCode exitCode, USBTechDevice * deviceRef ) {
	if ( logger->isDebugEnabled() )
		logger->debug("JobDone Slot");
	const QString & resultString = deviceRef->connWorker->getResultString();
	if ( resultString.isNull() || exitCode == USBconnectionWorker::WORK_DONE_FAILED )
		logger->warn( "No result/failed from USB device operation!" );
	else {
		if ( logger->isTraceEnabled() )
			logger->trace( resultString );
		TextInfoView & tiv = TextInfoView::getInstance();
		tiv.showText( resultString );
	}
/*	disconnect( deviceRef.connWorker, SIGNAL(workIsDone(USBconnectionWorker::WorkDoneExitCode)),
			this, SLOT(connectionWorkerJobDone(USBconnectionWorker::WorkDoneExitCode)) );
	delete deviceRef.connWorker;
*/
}


void HubDevice::userInfoReply(QString const& key, QString const& message, int replyBits ) {

}
