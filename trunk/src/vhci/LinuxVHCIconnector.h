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
#include <QThread>
#include <QQueue>
#include <QMap>
#include <QByteArray>

class Logger;
class QMutex;
class QWaitCondition;
class USBTechDevice;


class LinuxVHCIconnector : public QThread {
	Q_OBJECT
public:
	enum ePortStatus {
		PORTSTATE_UNKNOWN_STATE,
		PORTSTATE_POWEREDOFF,
		PORTSTATE_DISABLED,
		PORTSTATE_SUSPENDED,
		PORTSTATE_POWERON,
		PORTSTATE_RESET,
		PORTSTATE_RESUME
	};

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
	bool openKernelInterface();
	/**
	 * Returns if kernel interface is connected.
	 */
	bool isConnected();
	/**
	 * Closes the (Linux-) kernel <em>usb-vhci</em> interface.
	 */
	void closeKernelInterface();
	/**
	 * Connect given device to kernel.
	 * @param	device descriptor
	 * @return	port number
	 */
	int connectDevice( USBTechDevice * device );
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
	 */
	void giveBackAnswerURB( void * refData, int portID, bool isOK, QByteArray * urbData );

	/**
	 * Thread run loop.<br>
	 * For technical reasons this method must be declared <em>public</em>...
	 */
	void run();
protected:
	LinuxVHCIconnector( QObject* parent = NULL );

private:
	struct DeviceConnectionData {
		int operationFlag;
		int port;
		usb::data_rate dataRate;
		USBTechDevice * refDevice;
	};
	struct DeviceURBreplyData {
		bool isOK;
		usb::vhci::process_urb_work * refURB;
		QByteArray * dataURB;
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
	/** Array for each port with port status */
	ePortStatus* portStatusList;

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



	/** A queue for "device connect" requests */
	QQueue< struct DeviceConnectionData > deviceConnectionRequestQueue;
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

	void createURBfromInternalStruct( usb::urb * urbData, QByteArray & buffer );


signals:
	void portStatusChanged( uint8_t portID, ePortStatus portState );
	void urbDataSend1( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend2( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend3( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend4( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend5( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend6( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend7( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
	void urbDataSend8( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );
};

#endif /* LINUXVHCICONNECTOR_H_ */
