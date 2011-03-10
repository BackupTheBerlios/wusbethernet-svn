/*
 * HubDevice.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-06-21
 */

#ifndef HUBDEVICE_H_
#define HUBDEVICE_H_

#include "../TI_USBhub.h"
#include "ControlMessageBuffer.h"
#include "../USBconnectionWorker.h"
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

#define DEFAULT_DEVICE_CONTROL_PORT				21827
#define DEFAULT_DEVICE_CONTROL_ALIVE_INTERVAL	3000

class QTimer;
class QByteArray;
class ConnectionController;
class ControlMessageBuffer;
class QTreeWidgetItem;
class QTreeWidget;
class Logger;

/**
 * Content of message from discovery process: <tt>discoveryResponse</tt>
 */
struct ControlMsg_DiscoveryResponse {
	QString name;
	QString hwid;
	int pid;
	int vid;
};

/**
 * Content of message from control channel: <tt>unimport</tt>
 */
struct ControlMsg_UnimportRequest {
	QString hostname;
	QString deviceID;
	QString message;
};


/**
 * Represents one network usb hub device.
 */
class HubDevice : public TI_USBhub {
	Q_OBJECT
friend class XMLmessageDOMparser;
public:
	HubDevice( const QHostAddress & address, ConnectionController * controller, int discoveredDeviceNumber = 0 );
	virtual ~HubDevice();
	void setXMLdiscoveryData( int len, const QByteArray & payloadData );
	/**
	 * Return state if this network hub device is still alive or unreachable.
	 * This only takes into account if the hub device is reachable over network and
	 * answers correctly to corresponding requests.
	 * @return State of network hub device
	 */
	bool isAlive();
	void receiveData( ControlMessageBuffer::eTypeOfMessage type, const QByteArray & bytes );
	QString toString();

	/**
	 * Returns a TreeWidgetItem to get a graphical representation of this USB hub.
	 */
	QTreeWidgetItem * getQTreeWidgetItem( QTreeWidget * view );

	/**
	 * Import (take ownership of) device and query device directly for device info.<br>
	 * This function is available only when no one is already connected to
	 * device.
	 */
	void queryDevice( USBTechDevice * deviceRef );

	/**
	 * Disconnect (release) a connected device.<br>
	 * This works for own connected (<em>claimed</em> and <em>owned</em>) devices
	 * as well as for devices connected to different hosts on LAN (<em>claimed</em>).<br>
	 * If a device is claimed by a different host a "disconnect request" will
	 * pop up on that computer. Unfortunatly there is only small influence on
	 * displayed message from here.
	 *
	 */
	void disconnectDevice( USBTechDevice * deviceRef );


	/**
	 * Connect a available device.<br>
	 * @param  deviceRef	Reference to an device on this hub
	 */
	void connectDevice( USBTechDevice * deviceRef );

	/**
	 * Finds a specific device by given device ID.
	 */
	USBTechDevice & findDeviceByID( const QString & deviceID );

	/**
	 * Returns reference to logger.
	 */
	Logger * getLogger();

	/**
	 * Factory method to create a WUSB stack for given device.
	 */
	TI_WusbStack * createStackForDevice( const QString & deviceID );

private:
	/** Value from discovery reply: Name of device */
	QString name;
	/** Value from server info: used protocol */
	QString protocol;
	/** Value from server info: manufacturer of device */
	QString manufacturer;
	/** Value from server info: model name of device */
	QString modelName;
	/** Value from server info: name of device (should be same as <tt>name</tt>...) */
	QString deviceName;
	/** Value from server info: version of firmware */
	QString firmwareVersion;
	/** Value from server info: date of firmware */
	QString firmwareDate;
	/** List of all reported / connected devices on hub */
	QList<USBTechDevice*> deviceList;

	/** IP address of this device in network */
	QHostAddress ipAddress;
	/** Reference to ConnectionController (parent) */
	ConnectionController *refController;
	/** Timestamp: last contact with device */
	long int lastSeenTimestamp;
	/** Status: Is Device alive? */
	bool alive;
	/** Configuration value for control connection port at hub */
	int controlConnectionPortNum;
	/** Configuration value for alive timer interval */
	int aliveTimerInterval;
	/** Flag from message processing: Request server info at next feasible time */
	bool wantServerInfoRequest;
	/** Counter for error conditions on control channel */
	int errorCounter;
	/** Device number, set from constructor. [not used yet] */
	int deviceNumber;

	/** Timer to send alive requests at regular interval (see <tt>aliveTimerInterval</tt>) */
	QTimer *aliveTimer;
	/** control connection socket */
	QTcpSocket *controlConnectionSocket;
	/** Reference to receive buffer */
	ControlMessageBuffer *receiveBuffer;
	/** Reference to tree widget item for visualization */
	QTreeWidgetItem * visualTreeWidgetItem;
	/** Reference to logger */
	Logger * logger;

	/**
	 * Send query to hub for list of all connected devices.
	 */
	bool queryDeviceInfo();
	/**
	 * Send "import" message to hub to claim ownership.
	 */
	bool sendImportDeviceMessage( const QString & deviceID, const QString & deviceVendor,const QString & deviceProd );
	/**
	 * Send "unimport" message to hub to request release of device by other host.
	 */
	bool sendUnimportMessage( const QString & deviceID, const QString & message );
	bool openControlConnection( int portNum );
	int createClientSocket( const char *hostname, int localport, int peerport );
	void startAliveTimer();

	QTreeWidgetItem * getQTreeWidgetItemForDevice( USBTechDevice & usbDevice );
	QString getIconResourceForDevice( USBTechDevice & usbDevice );
	void setToolTipText();
	void refreshAllWidgetItemForDevice();

	void queryDeviceJob(USBTechDevice & deviceRef);
	void connectDeviceJob( USBTechDevice & deviceRef );
public slots:
	void sendAliveRequest();
	void readControlConnectionMessage();
	void notifyControlConnectionError(QAbstractSocket::SocketError socketError);
	void connectionWorkerJobDone( USBconnectionWorker::eWorkDoneExitCode, USBTechDevice* );
	/**
	 * Handle reply from user to question/info.
	 */
	virtual void userInfoReply( const QString & key, const QString & reply, int answerBits );
	/**
	 * Relay an user info message from submodules to higher modules.
	 * This allows that a higher module (caller) to be unaware of implementation details and dependency.
	 */
	virtual void userInfoMessageRelay( const QString & key, const QString & message, int answerBits );
signals:
	/**
	 * Signalize that something has happened, which requires user interaction.
	 */
	void userInfoMessage( const QString & key, const QString & message, int answerBits );
};

#endif /* HUBDEVICE_H_ */
