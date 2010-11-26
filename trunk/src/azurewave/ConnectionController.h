/*
 * ConnectionController.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-06-15
 */

#ifndef CONNECTIONCONTROLLER_H_
#define CONNECTIONCONTROLLER_H_

#include "HubDevice.h"
#include <QHash>
#include <QList>
#include <QHostAddress>

class QUdpSocket;
class QByteArray;
class QString;
class QTimer;
class QTreeWidget;
class QMenu;
class QAction;
class Logger;

/**
 *
 */
class ConnectionController : public QObject {
	Q_OBJECT
public:
	/**
	 * Contructor for opening an listening socket to receive discover messages from
	 * a remote network HUB.
	 */
	ConnectionController( int portNum, QObject *parent = 0 );
	~ConnectionController();

	void setDiscoveryInterval( int intervallMultiplicator );
	/**
	 * Start network discovery procedure.
	 */
	void start();
	/**
	 * Stop (if previously started) the network discory procedure.
	 */
	void stop();
	bool isRunning();

	void setVisualTreeWidget( QTreeWidget * widget );
	void drawVisualTree();
	QMenu* widgetItemContextMenu( QTreeWidgetItem * );
private:
	int remoteDiscoveryPortNum;
	/** All IP-addresses to send discovery probes */
	QList<QHostAddress> discoverySendAddresses;

	QUdpSocket *udpSocket;
	QTimer *broadcastTimer;
	int timerSchedules;
	int socketPortNum;
	int configSocketPortNum;
	short currentTan;
	QHash<QString, HubDevice*> knownDevicesByIP;
	int discoveryIntervall;
	int timerIntervall;
	Logger * logger;

	QTreeWidget * visualRepresentationTree;
	QTreeWidgetItem* currentSelectedTreeWidget;

	/**
	 * Creates a UDP receiver socket (server-socket) which listens to
	 * broadcast annoucement messages from network devices.
	 * @param	port	port number for the server socket (defaults to 1550)
	 * @return			<code>true</code> if server socket could be enabled; else <code>false</code>
	 */
	void processAnnouncementMessage( const QHostAddress & sender, const QByteArray & bytes );
	/**
	 * Contructs the message to be send to any USB-hub device for discovery.
	 * The message consists of a header part (<tt>0x00 TAN 0x03 0xe9</tt>) and a
	 * XML coded payload with an discovery announcement.<br>
	 * @param	tan		Transaction-Number
	 */
	QByteArray * createDiscoveryMessage( short tan );

	/**
	 * Opens a socket for sending discovery broadcasts to any USB-hub device on network.
	 */
	bool openSocket();

	/**
	 * Checks the received message if it matches protocol and if adds IP to
	 * list of known addresses. Structures for communication with network
	 * hub are initialized (<tt>HubDevice</tt>) and visual representation
	 * is updated.
	 */
	bool checkDiscoveryMessageAnswerHeader( const QByteArray & bytes );
	/** Check configuration for IP-addresses for discovery protocol */
	void retrieveAllDiscoveryAddresses();
private slots:
	void processPendingDatagrams();
	void sendDiscoveryAnnouncement();
	void contextMenuAction_Connect();
	void contextMenuAction_Disconnect();
	void contextMenuAction_QueryDevice();
};

// Q_DECLARE_METATYPE( const void * )

#endif /* CONNECTIONCONTROLLER_H_ */
