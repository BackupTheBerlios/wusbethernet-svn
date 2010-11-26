/*
 * ConnectionController.cpp
 *
 * @author		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version		$Id$
 * @created		2010-06-15
 */
#include "ConnectionController.h"
#include "../ConfigManager.h"
#include "../BasicUtils.h"
#include <QtNetwork>
#include <QtGui>
#include <QString>
#include <QHash>

ConnectionController::ConnectionController( int portNum, QObject *parent )
: QObject( parent ) {
	remoteDiscoveryPortNum = 16708;	// TODO config
	timerSchedules = 0;
	udpSocket = NULL;
	currentTan = (short) -1;
	timerIntervall = 2000;	// TODO config
	visualRepresentationTree = NULL;
	currentSelectedTreeWidget = NULL;

	if ( portNum < 10 ) {
		configSocketPortNum = portNum = 1500;
	} else
		configSocketPortNum = portNum;
	logger = Logger::getLogger( "DISCOVERY" );
};


ConnectionController::~ConnectionController() {
	stop();
}

void ConnectionController::start() {
	discoveryIntervall = 5;

	retrieveAllDiscoveryAddresses();

	// Try to open socket: This will try to use ports 1500 to 1519
	bool socketIsOpen = false;
	int tryCounter = 0;
	socketPortNum = configSocketPortNum;
	do {
		socketIsOpen = openSocket();
		tryCounter++;
		socketPortNum++;
	} while( !socketIsOpen && tryCounter < 20 );

	if ( !socketIsOpen ) {
		logger->error( QString::fromLatin1("Cannot open UDP socket on ports %1 to %2 - Giving up!").
				arg( QString::number(configSocketPortNum), QString::number(socketPortNum)) );
		return;
	}

	broadcastTimer = new QTimer(this);
	connect(broadcastTimer, SIGNAL(timeout()), this, SLOT(sendDiscoveryAnnouncement()));
	broadcastTimer->start(timerIntervall);
}

void ConnectionController::stop() {
	if ( broadcastTimer ) {
		broadcastTimer->stop();
		disconnect( broadcastTimer, SIGNAL(timeout()), this, SLOT(sendDiscoveryAnnouncement()) );
		delete broadcastTimer;
		broadcastTimer = NULL;
	}
	if ( udpSocket ) {
		disconnect( udpSocket, SIGNAL( readyRead() ), this, SLOT(processPendingDatagrams() ) );
		udpSocket->close();
		delete udpSocket;
		udpSocket = NULL;
	}
}

bool ConnectionController::isRunning() {
	return (broadcastTimer != NULL && broadcastTimer->isActive());
}


bool ConnectionController::openSocket() {
	// Clean up socket if necessary
	if ( udpSocket ) {
		disconnect( udpSocket, SIGNAL( readyRead() ), this, SLOT(processPendingDatagrams() ) );
		udpSocket->close();
		udpSocket = NULL;
	}
	// open the socket and binding to an interface
	udpSocket = new QUdpSocket( this );
	if ( !udpSocket->bind( QHostAddress::Any, socketPortNum ) ) {
		logger->warn(QString::fromLatin1("Cannot bind to interface on port: %1 - %2!").arg(
				QString::number( socketPortNum ), udpSocket->errorString() ) );
		return false;
	}

	// connecting a callback to get any
	connect( udpSocket, SIGNAL( readyRead() ),
			this, SLOT(processPendingDatagrams()));
	logger->info(QString::fromLatin1("Created socket bound to port %1").arg(
			QString::number( socketPortNum ) ));
	return true;
}

void ConnectionController::retrieveAllDiscoveryAddresses() {
	ConfigManager & confMgr = ConfigManager::getInstance();
	int discoveryTypeConf = confMgr.getIntValue( "azurewave.discovery.type", 1 );
	switch( discoveryTypeConf ) {
	case 0:
		// broadcast to global network (255.255.255.255)
		discoverySendAddresses.append( QHostAddress::Broadcast );
		break;
	case 1: {
		// broadcast to local network (e.g. 192.168.1.255 a.k.a. TCP/IP broadcast)
		QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
		if ( !allInterfaces.isEmpty() ) {
			QListIterator<QNetworkInterface> itIF( allInterfaces );
			while ( itIF.hasNext() ) {
				const QNetworkInterface & qnif = itIF.next();
				QNetworkInterface::InterfaceFlags flags = qnif.flags();
				if ( flags.testFlag( QNetworkInterface::IsUp) &&
						flags.testFlag( QNetworkInterface::CanBroadcast ) &&
						!flags.testFlag( QNetworkInterface::IsLoopBack ) &&
						!flags.testFlag( QNetworkInterface::IsPointToPoint ) ) {
					// found potential network interface
					QList<QNetworkAddressEntry> allAddresses = qnif.addressEntries();
					if ( allAddresses.isEmpty() ) continue;
					QListIterator<QNetworkAddressEntry> itAddr( allAddresses );
					while ( itAddr.hasNext() ) {
						const QNetworkAddressEntry & ntae = itAddr.next();
						QHostAddress hostAddr = ntae.broadcast();
						if ( !hostAddr.isNull() ) discoverySendAddresses.append( hostAddr );
					}
				}
			}
		}
		break;
	}
	case 2:
		// dedicated IP-address given by configuration
		if ( confMgr.haveKey( "azurewave.discovery.addresses" ) ) {
			const QString & confAddresses = confMgr.getStringValue( "azurewave.discovery.addresses" );
			if ( confAddresses.isNull() || confAddresses.isEmpty() )
				break;
			QStringList addList = confAddresses.split( QRegExp("[,; ]"), QString::SkipEmptyParts );
			if ( !addList.isEmpty() ) {
				QStringListIterator it(addList);
				while ( it.hasNext() ) {
					QString addr = it.next().simplified();
					if ( !addr.isNull() && !addr.isEmpty() ) {
						// TODO DNS lookup with QHostName (or similar?)
						QHostAddress qha = QHostAddress( addr );
						if ( !qha.isNull() )
							discoverySendAddresses.append( qha );
					}
				}
			}
		}
		break;
	default:
		discoverySendAddresses.append( QHostAddress::Broadcast );
	}

	// fallback...
	if ( discoverySendAddresses.isEmpty() )
		discoverySendAddresses.append( QHostAddress::Broadcast );

	QListIterator<QHostAddress> itSendAddr( discoverySendAddresses );
	while( itSendAddr.hasNext() ) {
		logger->debug( QString::fromLatin1("Azurewave discovery address: %1").arg( itSendAddr.next().toString() ) );
	}
}


void ConnectionController::sendDiscoveryAnnouncement() {
	if ( !udpSocket ) return;

	if ( timerSchedules % discoveryIntervall != 0 ) {
		timerSchedules++;
		return;
	}
	timerSchedules++;

	currentTan++;
	if ( currentTan >= 0xff ) currentTan = 0;

	QByteArray *datagram = createDiscoveryMessage( currentTan );

	QListIterator<QHostAddress> itSendAddr( discoverySendAddresses );
	while( itSendAddr.hasNext() ) {
		udpSocket->writeDatagram(datagram->data(), datagram->size(),
				itSendAddr.next(), remoteDiscoveryPortNum );
	}

	delete datagram;
//	printf("Sent datagram with %i bytes\n", (int) retVal );
}


void ConnectionController::processPendingDatagrams()
{
	if ( !udpSocket ) return;
	while ( udpSocket->hasPendingDatagrams() ) {
		QByteArray datagram;
		datagram.resize( udpSocket->pendingDatagramSize() );
		QHostAddress sender;
		quint16 senderPort;

		qint64 bytesRead = udpSocket->readDatagram(datagram.data(), datagram.size(),
				&sender, &senderPort);

		if ( bytesRead > 0 )
			processAnnouncementMessage( sender, datagram );
	}
}

/**
 * Creates a UDP receiver socket (server-socket) which listens to
 * broadcast annoucement messages from network devices.
 * @param	port	port number for the server socket (defaults to 1550)
 * @return			<code>true</code> if server socket could be enabled; else <code>false</code>
 */
void ConnectionController::processAnnouncementMessage( const QHostAddress & sender, const QByteArray & bytes ) {

	if ( !knownDevicesByIP.contains( sender.toString() ) ) {
		if ( !checkDiscoveryMessageAnswerHeader( bytes ) ) {
			logger->warn( QString::fromLatin1("Wrong checksum found in discovery reply from network!\n"
					"Originating IP: %1  Length: %2  First 7 bytes received: %3").arg(
							sender.toString(), QString::number(bytes.size()), messageToString( bytes,7) ) );
		} else {
			setDiscoveryInterval( 50 );

			// creating a new HUB device stack
			HubDevice * device = new HubDevice(sender, this );
			QByteArray payload = bytes.right( bytes.size() - 8 );
			device->setXMLdiscoveryData( bytes.size() -7, payload ); // submit complete payload without header

			knownDevicesByIP[ sender.toString() ] = device;
			drawVisualTree();
//			printf("Received %i bytes from %s\n", bytes.length(), sender.toString().toLatin1().data() );
		}
	}
}

bool ConnectionController::checkDiscoveryMessageAnswerHeader( const QByteArray & bytes ) {
	if ( bytes.size() < 10 ) return false;
	// Answer: 01 03(==TAN) 00 00 03 e9 00 00
	char byte1  = bytes[0];
	// XXX don't know why sometimes 0x00 and othertimes 0x01 is returned from hub
	if ( byte1 != 0x01 && byte1 != 0x0 ) {
		return false;
	}
	unsigned char byte2 = (unsigned char) bytes[1];
	if ( byte2 != currentTan ) {
		return false;
	}
	char byte3  = bytes[2];
	if ( byte3 != 0x00 ) {
		return false;
	}
	char byte4 = bytes[3];
	if ( byte4 != 0x00 ) {
		return false;
	}

	char byte5 = bytes[4];
	if ( byte5 != 0x03 ) {
		return false;
	}
	unsigned char byte6 = bytes[5];
	if ( byte6 != (unsigned char) 0xe9 ) {
		return false;
	}

	char byte7  = bytes[6];
	if ( byte7 != 0x00 ) return false;
	char byte8 = bytes[7];
	if ( byte8 != 0x00 ) return false;

	return true;
}

QByteArray * ConnectionController::createDiscoveryMessage( short tan ) {
	// The XML fragment to send to USB hub
	// XXX unclear why VendorID (vid) and ProductID (pid) are part of this message?!
	QString str = QString("<discover><pid type=\"int\">4000</pid><vid type=\"int\">0</vid></discover>");
	// Prepending Request-Header
	QByteArray *buffer = new QByteArray();
	buffer->append('\0');

	if ( tan > 0xff ) tan = 0xff; // just to be sure...
	buffer->append( (char) tan );

	buffer->append( 0x03 );
	buffer->append( 0xe9 );
	buffer->append( str );
	return buffer;
}

void ConnectionController::setDiscoveryInterval( int intervallMultiplicator ) {
	discoveryIntervall = intervallMultiplicator;
}


/* ***** methods for graphical representation ***** */


void ConnectionController::setVisualTreeWidget( QTreeWidget * widget ) {
	visualRepresentationTree = widget;
	visualRepresentationTree->setColumnCount(1);
	visualRepresentationTree->setIndentation(50);
	visualRepresentationTree->setRootIsDecorated( false );
	drawVisualTree();
}

void ConnectionController::drawVisualTree() {
	if ( ! visualRepresentationTree ) return;

	QList<QTreeWidgetItem *> items;
	QHashIterator<QString, HubDevice*>  it( knownDevicesByIP );
	while ( it.hasNext() ) {
		it.next();
		items.append( (it.value())->getQTreeWidgetItem( visualRepresentationTree ) );
	}
	visualRepresentationTree->insertTopLevelItems( 0, items );
}

QMenu * ConnectionController::widgetItemContextMenu( QTreeWidgetItem * witem ) {
	// produce different menus for the hub itself and all devices
	//  - the hub has no parent item (cause it is top on hierarchie)
	if ( witem->parent() ) {
		// Context menu for devices
		QMenu *menu = new QMenu;
		currentSelectedTreeWidget = witem;

		QAction * menuAction = new QAction( tr("Connect"), menu );
		connect(menuAction, SIGNAL(triggered()), this, SLOT(contextMenuAction_Connect()));
		menuAction->setEnabled( false );
		menu->addAction( menuAction );

		menuAction = new QAction( tr("Disconnect"), menu );
		connect(menuAction, SIGNAL(triggered()), this, SLOT(contextMenuAction_Disconnect()));
		menuAction->setEnabled( false );
		menu->addAction( menuAction );

		menu->addSeparator();

		menuAction = new QAction( tr("Query device info"), menu );
		connect(menuAction, SIGNAL(triggered()), this, SLOT(contextMenuAction_QueryDevice()));
		menu->addAction( menuAction );

		return menu;
	} else {
		// context menu for hub
		QMenu *menu = new QMenu;
		currentSelectedTreeWidget = witem;

		QAction * menuAction = new QAction( tr("Connect all"), menu );
		connect(menuAction, SIGNAL(triggered()), this, SLOT(contextMenuAction_Connect()));
		menuAction->setEnabled( false );
		menu->addAction( menuAction );

		menuAction = new QAction( tr("Disconnect all"), menu );
		connect(menuAction, SIGNAL(triggered()), this, SLOT(contextMenuAction_Disconnect()));
		menuAction->setEnabled( false );
		menu->addAction( menuAction );

		return menu;
	}
}

void ConnectionController::contextMenuAction_Connect( ) {
//	printf("contextMenu Action = Connect\n" );
	if ( currentSelectedTreeWidget ) {
		logger->debug(QString::fromLatin1("Connect item = %1").arg( currentSelectedTreeWidget->text(0) ) );
	}

	currentSelectedTreeWidget = NULL;
}

void ConnectionController::contextMenuAction_Disconnect() {
	logger->debug(QString::fromLatin1("Disconnect item = %1").arg( currentSelectedTreeWidget->text(0) ) );
	if ( currentSelectedTreeWidget ) {
		logger->debug(QString::fromLatin1("Diconnect item = %1").arg( currentSelectedTreeWidget->text(0) ) );
	}

	currentSelectedTreeWidget = NULL;

}
void ConnectionController::contextMenuAction_QueryDevice() {
	logger->info( QString("ContextMenu Action = Query Device: %1").arg(
			currentSelectedTreeWidget? currentSelectedTreeWidget->text(0) : QString("(none)") ) );
	if ( currentSelectedTreeWidget && !currentSelectedTreeWidget->data(0,Qt::UserRole).isNull() ) {
		USBTechDevice* refUSBDevice = currentSelectedTreeWidget->data(0,Qt::UserRole).value<USBTechDevice*>();
		if ( refUSBDevice ) {
			logger->info(QString("Item = '%1'  ID = %2  Hub = %3").arg(
					currentSelectedTreeWidget->text(0),
					refUSBDevice->deviceID,
					refUSBDevice->parentHub->toString() ) );
			refUSBDevice->parentHub->queryDevice( refUSBDevice );
		} else
			logger->error("Context menu action on <null> device? - action aborted!");
	} else
		logger->error("Context menu action: cannot find corresponding tree item! - action aborted!");

	currentSelectedTreeWidget = NULL;
}
