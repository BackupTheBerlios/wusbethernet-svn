/*
 * USBconnectionWorker.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-15
 */

#include "USBconnectionWorker.h"
#include "USButils.h"
#include "utils/Logger.h"
#include "BasicUtils.h"
#include "TI_WusbStack.h"
#include "TI_USB_VHCI.h"
#include "USBdeviceInfoProducer.h"
#include "azurewave/HubDevice.h"
#include "vhci/LinuxVHCIconnector.h"
#include "test/VirtualUSBdevice.h"
#include <QMetaType>
#include <QCoreApplication>
#include <time.h>


bool USBconnectionWorker::firstInstance = true;

USBconnectionWorker::USBconnectionWorker( HubDevice * parent, USBTechDevice * deviceRef )
: QThread( (QObject*) parent ) {
	usbDeviceRef = deviceRef;
	parentDevice = parent;
	isRunning = false;
	resultString = QString();
	lastExitCode = WORK_DONE_EXITED_UNKNOWN;
	destinationIP = QHostAddress();
	destinationPt = 0;
	vhciPortID = -1;
	currentJob = JOBTYPE_NOWORK;
	buffer = QByteArray();
	stack = NULL;
	deviceQueryEngine = NULL;
	deviceUSBhostConnector = NULL;
	// If this is first time use of this class, we have to register the exitcode enum for signaling
	if ( USBconnectionWorker::firstInstance ) {
		qRegisterMetaType<USBconnectionWorker::eWorkDoneExitCode>("USBconnectionWorker::eWorkDoneExitCode");
		qRegisterMetaType<uint16_t>("uint16_t");
		qRegisterMetaType<uint8_t>("uint8_t");
		qRegisterMetaType<TI_WusbStack::eDataTransferType>("TI_WusbStack::eDataTransferType");
		qRegisterMetaType<TI_WusbStack::eDataDirectionType>("TI_WusbStack::eDataDirectionType");
	}
	USBconnectionWorker::firstInstance = false;

	if ( usbDeviceRef->portNum >= 0 )
		logger = Logger::getLogger( QString("USBConn") + QString::number( usbDeviceRef->portNum) );
	else
		logger = Logger::getLogger( QString("USBConn") );
//	logger->info("Hallo Leute");
}

USBconnectionWorker::~USBconnectionWorker() {
	buffer.clear();
	if ( stack )
		delete stack;
	if ( deviceQueryEngine )
		delete deviceQueryEngine;
	if ( deviceUSBhostConnector )
		delete deviceUSBhostConnector;
}

Logger * USBconnectionWorker::getLogger() {
	return logger;
}

const QString & USBconnectionWorker::getResultString() {
	return resultString;
}

USBconnectionWorker::eWorkDoneExitCode USBconnectionWorker::getLastExitCode() {
	return lastExitCode;
}

void USBconnectionWorker::queryDevice( const QHostAddress & destinationAddress, int destinationPort ) {
	destinationIP = destinationAddress;
	destinationPt = destinationPort;
	logger->info( QString("QueryDevice: %1:%2").arg(destinationAddress.toString(),
			QString::number( destinationPort ) ) );
//	destinationIP = QHostAddress("127.0.0.1");
//	destinationPt = 8002;
	currentJob = JOBTYPE_QUERY_DEVICE;
	if ( deviceQueryEngine )
		delete deviceQueryEngine;
	deviceQueryEngine = new USBdeviceInfoProducer();
	start();
}


void USBconnectionWorker::queryDeviceInternal() {
	if ( logger->isDebugEnabled() )
		logger->debug( "queryDeviceInternal");
	stack = parentDevice->createStackForDevice( usbDeviceRef->deviceID );
	connect( stack, SIGNAL(receivedURB(const QByteArray &)), this, SLOT(receivedURB(const QByteArray &)));

	bool openSuccess = stack->openConnection();
	bool closeSuccess = true;
	if ( logger->isDebugEnabled() )
		logger->debug( QString("Open Connection result = %1").arg( openSuccess? "true": "false" ) );
	if ( openSuccess ) {
		while( deviceQueryEngine->hasNextURB() ) {
			QByteArray * bytesToSend = new QByteArray(deviceQueryEngine->nextURB());
			USBdeviceInfoProducer::RequestMetaData meta = deviceQueryEngine->getMetaData();
			if ( bytesToSend->isNull() || !meta.isValid ) continue;
			stack->sendURB( NULL, bytesToSend, meta.dataTransfer, meta.dataDirection, meta.endpoint, 0, 0, meta.expectedReturnLength );
			if ( waitForIncomingURB( 1000 ) ) {
				deviceQueryEngine->processAnswerURB( buffer );
				buffer.clear();
			}
		}
		busyWaiting( 5000 );
		bool closeSuccess = stack->closeConnection();
		if ( logger->isDebugEnabled() )
			logger->debug (QString("Close Connection result = %1").arg( closeSuccess? "true": "false" ) );
	}
	if ( ! openSuccess || !closeSuccess )
		lastExitCode = WORK_DONE_FAILED;
	else
		lastExitCode = WORK_DONE_SUCCESS;
	resultString = deviceQueryEngine->getHTMLReport();
	currentJob = JOBTYPE_NOWORK;
}

void USBconnectionWorker::connectDevice( const QHostAddress & destinationAddress, int destinationPort ) {
	printf("USBconnectionWorker::connectDevice\n");
	destinationIP = destinationAddress;
	destinationPt = destinationPort;
	logger->info( QString("ConnectDevice: %1:%2").arg(
			destinationAddress.toString(),
			QString::number( destinationPort ) ) );

	deviceUSBhostConnector = LinuxVHCIconnector::getInstance();
	// Last check if kernel interface is available
	if ( ! deviceUSBhostConnector->isConnected() ) {
		if ( !deviceUSBhostConnector->openInterface() ) {
			logger->error("Cannot open OS interface to connect USB device - aborting operation!");
			currentJob = JOBTYPE_NOWORK;
			lastExitCode = WORK_DONE_FAILED;
			return;
		}
	}
	currentJob = JOBTYPE_CONNECT_DEVICE;
	start();
}

void USBconnectionWorker::connectDeviceInternal() {

	int portID = -1000;
	if ( (portID = deviceUSBhostConnector->connectDevice( usbDeviceRef )) < 1 ) {
		// problem with port, port number or similar
		logger->warn(QString("Cannot connect device to VHCI hub! (portID=%1)").arg( QString::number(portID) ));
		lastExitCode = WORK_DONE_FAILED;
		currentJob = JOBTYPE_NOWORK;
		vhciPortID = -1;
		emit userInfoMessage( "none", tr("<html>Cannot connect device!<br>"
				"No free port on virtual USB hub.<br>&nbsp;&nbsp;&nbsp;Try again later!</html>"), -2 );
		quit();	// stop thread
		return;
	}
	if ( logger->isInfoEnabled() )
		logger->info(QString("Connected on port %1").arg( QString::number( portID) ));
	vhciPortID = portID;

	// open network connection
	stack = parentDevice->createStackForDevice( usbDeviceRef->deviceID );
	stack->registerURBreceiver( deviceUSBhostConnector );

	bool openSuccess = stack->openConnection();
	if ( ! openSuccess ) {
		lastExitCode = WORK_DONE_FAILED;
		logger->warn("Could not open connection to hub/device!");
		deviceUSBhostConnector->disconnectDevice( portID );
		return;
	}

//	testDev = new VirtualUSBdevice( deviceUSBhostConnector, portID );

	switch ( portID ) {
	case 1:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend1(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 2:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend2(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 3:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend3(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 4:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend4(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 5:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend5(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 6:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend6(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 7:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend7(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	case 8:
		connect( deviceUSBhostConnector, SIGNAL( urbDataSend8(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				Qt::QueuedConnection );
		break;
	}

	lastExitCode = WORK_DONE_STILL_RUNNING;
	currentJob = JOBTYPE_NOWORK;
}

void USBconnectionWorker::disconnectDevice() {
	if ( stack ) {
		if ( !stack->closeConnection() ) {
			logger->warn("Stack connection not active???");
		}
	} else
		return;
	if ( deviceUSBhostConnector ) {
		deviceUSBhostConnector->disconnectDevice( vhciPortID );
	}

	switch ( vhciPortID ) {
	case 1:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend1(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)) );
		break;
	case 2:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend2(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)) );
		break;
	case 3:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend3(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)) );
		break;
	case 4:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend4(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)) );
		break;
	case 5:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend5(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)));
		break;
	case 6:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend6(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)));
		break;
	case 7:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend7(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)));
		break;
	case 8:
		disconnect( deviceUSBhostConnector, SIGNAL( urbDataSend8(
				void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,int)),
				stack,
				SLOT(processURB(void *,uint16_t,uint8_t,TI_WusbStack::eDataTransferType,TI_WusbStack::eDataDirectionType,QByteArray *,uint8_t,int)));
		break;
	}
	quit();	// exit event looping
}


bool USBconnectionWorker::waitForIncomingURB( int waitMillis ) {
	int waitCount = 0;
	int maxWaitCount = waitMillis / 5;
	if ( maxWaitCount <= 0 ) maxWaitCount = 1;
	while( waitCount < maxWaitCount && buffer.isEmpty() ) {
		QCoreApplication::processEvents();	// process pending events
		sleepThread( 5L );
		waitCount++;
	}
	return !buffer.isEmpty();
}

void USBconnectionWorker::busyWaiting( int waitMillis ) {
	int waitCount = 0;
	int maxWaitCount = waitMillis / 5;
	if ( maxWaitCount <= 0 ) maxWaitCount = 1;
	while( waitCount < maxWaitCount ) {
		QCoreApplication::processEvents();	// process pending events
		sleepThread( 5L );
		waitCount++;
	}
}

void USBconnectionWorker::receivedURB( const QByteArray & bytes ) {
	if ( logger->isDebugEnabled() )
		logger->debug(QString::fromLatin1("USBconnectionWorker::receivedURB - Got %1 bytes").arg( QString::number(bytes.length()) ) );
	buffer.append( bytes );
}

void USBconnectionWorker::run() {
	// Connect slot of HubDevice with "workIsDone" -
	// this is possible only here because signaling works only if connecting to runnning thread..
	// TODO make this configurable or switchable from calling object

//	connect( this, SIGNAL(workIsDone(USBconnectionWorker::WorkDoneExitCode,USBTechDevice &)),
//			parentDevice, SLOT(connectionWorkerJobDone(USBconnectionWorker::WorkDoneExitCode,USBTechDevice &)), Qt::QueuedConnection );


	lastExitCode = WORK_DONE_STILL_RUNNING;
	logger->info(QString("USBconnectionWorker::run() currentJob=%1").arg(QString::number(currentJob) ) );
	switch ( currentJob ) {
	case JOBTYPE_QUERY_DEVICE:
		queryDeviceInternal();
		break;
	case JOBTYPE_CONNECT_DEVICE:
		connectDeviceInternal();
		exec();
		lastExitCode = WORK_DONE_SUCCESS;
		break;
	default:
		break;
	}
	logger->info("Job finished!");
	emit workIsDone(lastExitCode, usbDeviceRef);
}
