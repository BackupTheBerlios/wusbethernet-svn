/*
 * USBconnectionWorker.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-15
 */

#ifndef USBCONNECTIONWORKER_H_
#define USBCONNECTIONWORKER_H_

#include <QThread>
#include <QString>
#include <QHostAddress>

class TI_WusbStack;
class WusbStack;
class USBdeviceInfoProducer;
class HubDevice;
class Logger;
struct USBTechDevice;

class USBconnectionWorker : public QThread {
	Q_OBJECT
friend class WusbStack;
public:
	/** Exitcode of worker thread */
	enum WorkDoneExitCode {
		WORK_DONE_STILL_RUNNING,
		WORK_DONE_SUCCESS,
		WORK_DONE_FAILED,
		WORK_DONE_EXITED_UNKNOWN
	};

	USBconnectionWorker( HubDevice * parent, USBTechDevice * deviceRef );
	~USBconnectionWorker();

	/**
	 * Query device information from given device by connecting to hub,
	 * request device info and disconnect.<br>
	 * A new thread will be created to perform given request. After all work is
	 * done, a signal <tt>workIsDone</tt> will be emitted and a resulting string
	 * (if any) is stored.
	 */
	void queryDevice( const QHostAddress & destinationAddress, int destinationPort );

	/**
	 * Return resulting string from last operation. If operation is
	 * not finished or no result is produced by operation <tt>QString::null</tt>
	 * is returned.
	 */
	const QString & getResultString();

	/**
	 * Return exit code of last operation. If an operation (thread) is still running,
	 * <tt>WORK_DONE_NOT_STILL_RUNNING</tt> is returned;
	 */
	WorkDoneExitCode getLastExitCode();

	/**
	 * Thread run loop.<br>
	 * For technical reasons this method must be declared <em>public</em>...
	 */
	void run();

protected:
	/**
	 * Returns reference to logger.
	 */
	Logger * getLogger();

private:
	enum JobType {
		JOBTYPE_NOWORK,
		JOBTYPE_QUERY_DEVICE
	};
	/** Some URBs used to query device */
	enum URBType {
		URB_DEVICE_DESCRIPTOR
	};
	USBTechDevice * usbDeviceRef;
	HubDevice * parentDevice;
	/** Flag if thread is currently running */
	bool isRunning;
	/** Storage for resulting string of an operation */
	QString resultString;
	/** Last exit code of job */
	WorkDoneExitCode lastExitCode;
	JobType currentJob;

	QByteArray buffer;
	TI_WusbStack *stack;
	Logger * logger;
	USBdeviceInfoProducer * deviceQueryEngine;

	/** Destination IP / hostname */
	QHostAddress destinationIP;
	/** Destination port */
	int destinationPt;

	static bool firstInstance;

	void queryDeviceInternal();
	bool waitForIncomingURB( int waitingTime );
	void busyWaiting( int waitMillis );
private slots:
	void receivedURB( const QByteArray &);
signals:
	void workIsDone(USBconnectionWorker::WorkDoneExitCode,USBTechDevice *);

};

#endif /* USBCONNECTIONWORKER_H_ */
