/*
 * TI_USBhub.h
 * Abstract interface to define all functions of a network attached USB hub.
 * TODO not finished yet...
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-01-04
 */

#ifndef TI_USBHUB_H_
#define TI_USBHUB_H_

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QList>


class Logger;
class HubDevice;
class USBconnectionWorker;
class QTreeWidgetItem;

/**
 * Represents the "interface" setting/configuration for an USB device.<br>
 * NOTE: Some parameters (like "alternative setting" etc) are not available!
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
		portNum = -1;
		isValid = false;
		owned = false;
		claimedByName = QString("n/a");
		claimedByIP = QString("n/a");
		parentHub = parent;
		connWorker = NULL;
		lastOperationErrorCode = -1;
		connectionPortNum = -1;
		nextJobID = JA_NONE;
		usageHint = 0;
	}

	/** State of device */
	enum ePlugStatus {
		PS_Plugged,
		PS_Unplugged,
		PS_Claimed,
		PS_NotAvailable
	};


	/** Job to do on device (by user interaction) */
	enum eJobAssignment {
		JA_NONE,
		JA_INTERNAL_QUERY_DEVICE,
		JA_CONNECT_DEVICE
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
	HubDevice * parentHub; // TODO refactoring to TI_USBhub

	/** Reference to active connection worker */
	USBconnectionWorker * connWorker;

	/** Next job to do (by user interaction) */
	eJobAssignment nextJobID;

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
	/** Errorcode of last import/unimport operation (from network hub) */
	int lastOperationErrorCode;
	/** Port number (on network / UDP) for connection */
	int connectionPortNum;

	/** Connected USB-port number */
	int portNum;
	/** USB-device ID (hex) - this ID is produced by usb hub and should be
	 * unique at any given time for a specific device */
	QString deviceID;
	/** Status of device (mostly "plugged") */
	ePlugStatus status;

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
	/** Usage hint from newer firmware (> 1.0.18) */
	int usageHint;
};

Q_DECLARE_METATYPE( USBTechDevice* )



class TI_USBhub : public QObject {
	Q_OBJECT
public:
	TI_USBhub( QObject * parent = 0 ) : QObject( parent ) {};
	virtual ~TI_USBhub() {};

	/**
	 * Return state if this network hub device is still alive or unreachable.
	 * This only takes into account if the hub device is reachable over network and
	 * answers correctly to corresponding requests.
	 * @return State of network hub device
	 */
	virtual bool isAlive() = 0;

	/**
	 * Returns reference to used logger.
	 */
	virtual Logger * getLogger() = 0;


	/**
	 * Debug output: gives brief information about state of device.
	 */
	QString toString();

public slots:
	/**
	 * Handle reply from user to question/info.
	 */
	virtual void userInfoReply( const QString & key, const QString & reply, int answerBits ) = 0;

};

#endif /* TI_USBHUB_H_ */
