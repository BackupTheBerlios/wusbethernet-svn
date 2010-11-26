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
#include "USBdeviceInfoProducer.h"
#include "azurewave/HubDevice.h"
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
	currentJob = JOBTYPE_NOWORK;
	buffer = QByteArray();
	stack = NULL;
	deviceQueryEngine = NULL;
	// If this is first time use of this class, we have to register the exitcode enum for signaling
	if ( USBconnectionWorker::firstInstance )
		qRegisterMetaType<USBconnectionWorker::WorkDoneExitCode>("USBconnectionWorker::WorkDoneExitCode");
	USBconnectionWorker::firstInstance = false;

	logger = Logger::getLogger( QString("USBConn") + QString::number(deviceRef->connectionPortNum) );
}

USBconnectionWorker::~USBconnectionWorker() {
	buffer.clear();
	if ( stack )
		delete stack;
	if ( deviceQueryEngine )
		delete deviceQueryEngine;
}

Logger * USBconnectionWorker::getLogger() {
	return logger;
}

const QString & USBconnectionWorker::getResultString() {
	return resultString;
}

USBconnectionWorker::WorkDoneExitCode USBconnectionWorker::getLastExitCode() {
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
	stack = new WusbStack( this, destinationIP, destinationPt );
	connect( stack, SIGNAL(receivedURB(const QByteArray &)), this, SLOT(receivedURB(const QByteArray &)));

	bool openSuccess = stack->openConnection();
	bool closeSuccess = true;
	if ( logger->isDebugEnabled() )
		logger->debug( QString("Open Connection result = %1").arg( openSuccess? "true": "false" ) );
	if ( openSuccess ) {
		while( deviceQueryEngine->hasNextURB() ) {
			QByteArray bytesToSend = deviceQueryEngine->nextURB();
			USBdeviceInfoProducer::RequestMetaData meta = deviceQueryEngine->getMetaData();
			if ( bytesToSend.isNull() || !meta.isValid ) continue;
			stack->sendURB( bytesToSend, meta.dataTransfer, meta.dataDirection, meta.endpoint, meta.expectedReturnLength );
			if ( waitForIncomingURB( 1000 ) ) {
				deviceQueryEngine->processAnswerURB( buffer );
				buffer.clear();
			}
		}
		busyWaiting( 2000 );
		bool closeSuccess = stack->closeConnection();
		if ( logger->isDebugEnabled() )
			logger->debug (QString::fromLatin1("Close Connection result = %1").arg( closeSuccess? "true": "false" ) );
	}
	if ( ! openSuccess || !closeSuccess )
		lastExitCode = WORK_DONE_FAILED;
	else
		lastExitCode = WORK_DONE_SUCCESS;
	resultString = deviceQueryEngine->getHTMLReport();
	currentJob = JOBTYPE_NOWORK;
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
	switch ( currentJob ) {
	case JOBTYPE_QUERY_DEVICE:
		queryDeviceInternal();
		break;
	default:
		break;
	}
	logger->info("Job finished!");
	emit workIsDone(lastExitCode, usbDeviceRef);
	logger->info("Job finished2");
}
