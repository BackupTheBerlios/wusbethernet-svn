/**
 * LinuxVHCIconnector.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-02-09
 */

#ifndef LINUXVHCICONNECTOR_H_
#define LINUXVHCICONNECTOR_H_

#include "../../usb-vhci/libusb_vhci.h"
#include "../TI_WusbStack.h"
#include "../TI_USB_VHCI.h"
#include <QThread>
#include <QQueue>
#include <QMap>
#include <QByteArray>

class Logger;
class QMutex;
class QWaitCondition;
class USBTechDevice;


class LinuxVHCIconnector : public TI_USB_VHCI {
	Q_OBJECT
public:

	virtual ~LinuxVHCIconnector();
	/**
	 * Returns the singleton instance of this class.
	 * If no instance is loaded, a new instance will be created.<br>
	 * NOTE: Implementation is NOT threadsafe!
	 */
	static LinuxVHCIconnector* getInstance();
	/**
	 * Returns if an (singleton) instance of this class is available and
	 * if the kernel interface is initialized.<br>
	 */
	static bool isLoaded();

	/**
	 * Opens (Linux-) kernel <em>usb-vhci</em> interface. A (by configuration) specified
	 * number of ports (default: <tt>6</tt>) are allocated on a virtual usb hub.
	 * @return	<code>true</code> if device could be opened, <code>false</code> if not.
	 */
	virtual bool openInterface();
	/**
	 * Returns if kernel interface is connected.
	 */
	virtual bool isConnected();
	/**
	 * Closes the (Linux-) kernel <em>usb-vhci</em> interface.
	 */
	virtual void closeInterface();
	/**
	 * Connect given device to kernel.
	 * @param	device descriptor
	 * @return	port number / ID
	 */
	virtual int connectDevice( USBTechDevice * device, int portID = -1);

	/**
	 * Retrieves and reserves a free port-ID.<br>
	 * If port-ID is not needed anymore use <tt>disconnectDevice</tt> to
	 * free port-ID.
	 * @return	port number / ID
	 */
	virtual int getAndReservePortID();

	/**
	 * Disconnect device on given port from OS.
	 */
	bool disconnectDevice( int portID );
	/**
	 * Start working thread. The worker thread is needed to query kernel interface for new data.
	 */
	void startWork();
	/**
	 * Stop working thread.
	 */
	void stopWork();


	/**
	 * Passes back an answer to host for last request.
	 * @see TI_USB_VHCI
	 */
	virtual void giveBackAnswerURB( void * refData, bool isOK, QByteArray * urbData );

	/**
	 * Thread run loop.<br>
	 * For technical reasons this method must be declared <em>public</em>...
	 */
	void run();
protected:
	LinuxVHCIconnector( QObject* parent = NULL );

private:
	struct DeviceConnectionData_t {
		int operationFlag;
		int port;
		usb::data_rate dataRate;
		USBTechDevice * refDevice;
		QByteArray * initialDeviceDescriptor;
	};
	struct DeviceURBreplyData {
		bool isOK;
		usb::vhci::process_urb_work * refURB;
		QByteArray * dataURB;
	};
	/** Describes all status data for one virtual USB port. */
	struct PortStatusData_t {
		/** USB hub port is working at all */
		bool portOK;
		/** USB hub port is currently in use */
		bool portInUse;
		/** Port status (enables, powered, suspended etc. */
		TI_USB_VHCI::ePortStatus portStatus;
		/** When a device is connected the host enumerates the device with a number (0/1 - 127) */
		uint8_t portEnumeratedByHost;
		/** A device connector that is send to host for a freshly connected device.
		 * This cannot be performed by real device query because the host
		 * needs this prior to enumeration. */
		QByteArray * initialConnectDeviceDescriptor;
		/** Flag if a new connected device is still in "init phase"
		 * (the host will perform a port reset and send port enumeration data after that) */
		bool deviceInInitPhase;
		/** handle ID of last URB send from host */
		uint64_t lastURBhandle;
		/** packet counter for debug purpose (counting each send packet) */
		unsigned int packetCount;
	};

	/** Singleton instance */
	static LinuxVHCIconnector * instance;


	/** Reference to logger */
	Logger * logger;
	/** Total number of connected USB ports */
	int numberOfPorts;
	/** Flag if worker should run. (@see <tt>startWork()</tt>) */
	bool shouldRun;

	/** Array for each port of virtual hub with used status */
	bool* portInUseList;

	/** Array for each port with port status information */
	PortStatusData_t * portStatusList;

	/** Connection to (virtual) host controller device */
	usb::vhci::local_hcd * hcd;
	/** Current work to do on device */
	usb::vhci::work* work;
	/** Flag indicating that the kernel interface is usable */
	bool kernelInterfaceUsable;

	/** Mutex for synchronization of usb interaction with kernel */
	static QMutex * workInProgressMutex;
	/** Wait condition for synchronization of usb interaction with kernel */
	static QWaitCondition * workInProgressCondition;
	/** Flag indicating work with kernel interface is in progress */
	static bool isWorkInProgress;
	static bool isWaitingForWork;

	/** Timestamp indicating a time when a subsequent connection request can be performed
	 *  (this is necessary to limit connection requests) */
	long long nextConnectionRequestDeferValue;

	/** A queue for "device connect" requests */
	QQueue< struct DeviceConnectionData_t > deviceConnectionRequestQueue;
	/** Mutex to gard device connect operation queue */
	QMutex * connectionRequestQueueMutex;
	/** A queue for "URB reply from device" data */
	QQueue< struct DeviceURBreplyData >   deviceReplyDataQueue;
	/** Mutex to gard reply from device queue */
	QMutex * deviceReplyDataQueueMutex;



	/** Finds an unused port */
	int getUnusedPort();

	/** Internal callback of last packet state */
	static void signal_work_enqueued( void* arg, usb::vhci::hcd& from ) throw();

	/**
	 * Process queued device connect / disconnect operations.
	 * @see <tt>connectDevice</tt>
	 * @see <tt>disconnectDevice</tt>
	 */
	bool processOutstandingConnectionRequests();

	bool processOutstandingURBReplys();

	/**
	 * Creates device descriptor data from a USBTechDevice description.
	 * Device descriptor data will be written to given byte array.
	 * @param	device 	Reference of a connected USB device.
	 * @param	bytes	Reference to a allocated byte array.
	 */
	void createDeviceDescriptorFromDeviceDescription( USBTechDevice * device, QByteArray & bytes );

	void createURBfromInternalStruct( usb::urb * urbData, QByteArray & buffer, int portID = 0 );


signals:
	void portStatusChanged( uint8_t portID, ePortStatus portState );
	void urbDataSend1( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend2( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend3( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend4( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend5( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend6( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend7( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
	void urbDataSend8( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReturnLength );
};

#endif /* LINUXVHCICONNECTOR_H_ */
