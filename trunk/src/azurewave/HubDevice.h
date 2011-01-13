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
#include <QList>
#include <QHostAddress>

#define DEFAULT_DEVICE_CONTROL_PORT 21827

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
 * Represents the "interface" setting/configuration for an USB device.<br>
 * NOTE: Some parameters (like "alternative setting" etc.) are not available!
 */
class USBTechInterface {
public:
	USBTechInterface() { isValid = false; }
	/** Flag: This interface is valid */
	bool isValid;
	/** Interface number<br>
	 *  Indicates the index of the interface descriptor.
	 *  This should be zero based, and incremented once
	 *  for each new interface descriptor.*/
	int if_number;
	/** interface class<br>
	 *  <tt>bInterfaceClass</tt>, <tt>bInterfaceSubClass</tt> and <tt>bInterfaceProtocol</tt>
	 *   can be used to specify supported classes (e.g. HID, communications,
	 *   mass storage etc.)
	 *   This allows many devices to use class drivers preventing the need to write
	 *   specific drivers for your device. (All assigned by USB Org) */
	int if_class;
	/** interface sub-class */
	int if_subClass;
	/** interface protocol number */
	int if_protocol;
	/** number of endpoints for this interface */
	int if_numEndpoints;
};

/**
 * This represents the complete USB device specification (delivered by network device)
 */
class USBTechDevice {
public:
	inline USBTechDevice( HubDevice * parent ) {
		product = QString("dev");
		visualTreeWidgetItem = (QTreeWidgetItem *) 0;
		sortNumber = 0;
		portNum = 0;
		isValid = false;
		owned = false;
		claimedByName = QString("n/a");
		claimedByIP = QString("n/a");
		parentHub = parent;
		connWorker = NULL;
		lastOperationErrorCode = -1;
		connectionPortNum = -1;
		nextJobID = -1;
	}

	/** State of device */
	enum PlugStatus {
		PS_Plugged,
		PS_Unplugged,
		PS_Claimed,
		PS_NotAvailable
	};

	/**
	 * Creates an empty and invalid instance.<br>
	 * This method is useful when an <em>invalid</em> instance is
	 * needed e.g. as return value.
	 */
	static inline USBTechDevice & invalid() {
		// XXX use of some kind of autopointer to keep memory...
		USBTechDevice *d = new USBTechDevice(0);
		d->isValid = false;
		return *d;
	}

	/** Reference to hub on which this device is connected */
	HubDevice * parentHub;

	/** Reference to active connection worker */
	USBconnectionWorker * connWorker;

	int nextJobID;

	/** Flag: This device is valid */
	bool isValid;
	/** Sorting number for display purposes */
	int sortNumber;
	/** Reference to a visual representation object of this device */
	QTreeWidgetItem * visualTreeWidgetItem;

	/** Flag: Device is claimed by ourself */
	bool owned;
	/** Name of computer which imported this device */
	QString claimedByName;
	/** IP address of computer which imported this device */
	QString claimedByIP;
	/** Errorcode of last import/unimport operation */
	int lastOperationErrorCode;
	/** Port number for connection */
	int connectionPortNum;

	/** Connected USB-port number */
	int portNum;
	/** USB-device ID (hex) - this ID is produced by usb hub and should be
	 * unique at any given time for a specific device */
	QString deviceID;
	/** Status of device (mostly "plugged") */
	PlugStatus status;

	/** USB Specification Number which device complies too.<br>
	 * The bcdUSB field reports the highest version of USB the device supports.
	 * The value is in binary coded decimal with a format of 0xJJMN where JJ is
	 * the major version number, M is the minor version number and N is the sub minor
	 * version number. e.g. USB 2.0 is reported as 0x0200, USB 1.1 as 0x0110 and USB 1.0 as 0x0100.
	 */
	QString bcdUSB;
	QString sbcdUSB;
	/** Device class<br>
	 * The <tt>bDeviceClass</tt>, <tt>bDeviceSubClass</tt> and <tt>bDeviceProtocol</tt> are used by the
	 * operating system to find a class driver for your device.
	 * Typically only the bDeviceClass is set at the device level.
	 * Most class specifications choose to identify itself at the interface level
	 * and as a result set the bDeviceClass as 0x00. This allows for the one device to
	 * support multiple classes. */
	int bClass;
	/** Device sub-class */
	int bSubClass;
	/** Device protocol */
	int bProtocol;

	/** List of all announced interface descriptions */
	QList<USBTechInterface> interfaceList;

	/** vendor ID of device (assigned by USB.org) */
	int idVendor;
	/** product ID of device (assigned by manufacturer) */
	int idProduct;
	/** USB device's release number, in binary-coded decimal (BCD) format.
	 * @see bcdUSB */
	QString bcdDevice;
	QString sbcdDevice;
	/** manufacturer of device (as given in configuration section) */
	QString manufacturer;
	/** product name (as given in configuration section) */
	QString product;
};

Q_DECLARE_METATYPE( USBTechDevice* )

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
	void receiveData( ControlMessageBuffer::TypeOfMessage type, const QByteArray & bytes );
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
	enum TypeOfAnswerFromDevice {
		NOP,
		ALIVE,
		SERVERINFO,
		STATUSCHANGED,
		IMPORTINFO
	};
	QString name;
	QString protocol;
	QString manufacturer;
	QString modelName;
	QString deviceName;
	QString firmwareVersion;
	QString firmwareDate;
	QList<USBTechDevice*> deviceList;

	QHostAddress ipAddress;
	ConnectionController *refController;
	long int lastSeenTimestamp;
	bool alive;
	int controlConnectionPortNum;
	bool wantServerInfoRequest;
	int errorCounter;
	int deviceNumber;

	int bytesToReadFromNetwork;
	QByteArray *bufferReadFromNetwork;
	TypeOfAnswerFromDevice messageTypeFromNetwork;

	QTimer *aliveTimer;
	QTcpSocket *controlConnectionSocket;
	ControlMessageBuffer *receiveBuffer;
	QTreeWidgetItem * visualTreeWidgetItem;
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
public slots:
	void sendAliveRequest();
	void readControlConnectionMessage();
	void notifyControlConnectionError(QAbstractSocket::SocketError socketError);
	void connectionWorkerJobDone( USBconnectionWorker::WorkDoneExitCode, USBTechDevice* );
	/**
	 * Handle reply from user to question/info.
	 */
	virtual void userInfoReply( const QString & key, const QString & reply, int answerBits );

signals:
	/**
	 * Signalize that something has happened, which requires user interaction.
	 */
	void userInfoMessage( const QString & key, const QString & message, int answerBits );
};

#endif /* HUBDEVICE_H_ */
