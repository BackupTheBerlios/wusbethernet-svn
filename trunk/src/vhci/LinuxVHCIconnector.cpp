/**
 * LinuxVHCIconnector.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-02-09
 */

#include "LinuxVHCIconnector.h"
#include "../utils/Logger.h"
#include "../TI_USBhub.h"
#include "../USButils.h"
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

extern bool applicationShouldRun;

// singleton instance initialization
LinuxVHCIconnector *LinuxVHCIconnector::instance = NULL;

// initialization of synchronization mechanism for kernel work
QMutex * LinuxVHCIconnector::workInProgressMutex = new QMutex;
QWaitCondition * LinuxVHCIconnector::workInProgressCondition = new QWaitCondition;
bool LinuxVHCIconnector::isWorkInProgress = false;
bool LinuxVHCIconnector::isWaitingForWork = false;


LinuxVHCIconnector::LinuxVHCIconnector( QObject* parent )
		: QThread( parent ) {
	instance = this;
	hcd = NULL;
	shouldRun = false;
	kernelInterfaceUsable = true; // default: everything should be ok


	numberOfPorts = 6;	// TODO config!
	portInUseList = new bool[numberOfPorts];
	portStatusList = new ePortStatus[numberOfPorts];
	for ( int i = 0; i < numberOfPorts; i++ ) {
		portInUseList[i] = false;
		portStatusList[i] = PORTSTATE_UNKNOWN_STATE;
	}

	// synchronization mutex
	connectionRequestQueueMutex = new QMutex;
	deviceReplyDataQueueMutex = new QMutex;

	// init logger with root-logger
	logger = Logger::getLogger("VHCI");
}

LinuxVHCIconnector::~LinuxVHCIconnector() {
	if ( shouldRun )
		stopWork();
	if ( hcd ) delete hcd;
	delete connectionRequestQueueMutex;
	delete deviceReplyDataQueueMutex;
	delete workInProgressMutex;
	delete workInProgressCondition;
	delete portInUseList;
	delete portStatusList;
}

LinuxVHCIconnector* LinuxVHCIconnector::getInstance() {
	if ( LinuxVHCIconnector::instance ) return LinuxVHCIconnector::instance;
	return new LinuxVHCIconnector();
}
bool LinuxVHCIconnector::isLoaded() {
	if ( LinuxVHCIconnector::instance && LinuxVHCIconnector::instance->hcd ) return true;
	return false;
}

bool LinuxVHCIconnector::isConnected() {
	return hcd != NULL;
}

void LinuxVHCIconnector::signal_work_enqueued(void* arg, usb::vhci::hcd& from) throw()
{
	workInProgressMutex->lock();
	isWorkInProgress = false;
	if ( isWaitingForWork ) {
		isWaitingForWork = false;
		workInProgressCondition->wakeAll();
	}
	workInProgressMutex->unlock();
}

bool LinuxVHCIconnector::openKernelInterface() {
	if ( hcd ) return true;
	try {
		// open interface
		hcd = new usb::vhci::local_hcd( numberOfPorts );
		// and connect work finished callback
		hcd->add_work_enqueued_callback( usb::vhci::hcd::callback( &signal_work_enqueued, NULL ) );
	} catch ( std::exception &ex ) {
		hcd = NULL;
		logger->error(QString::fromLatin1("Cannot open virtual host controller device: '%1' Error: %2").
				arg( QString(USB_VHCI_DEVICE_FILE), QString(ex.what()) ) );
		logger->error("Make sure kernel modules (usb-vhci-hcd AND usb-vhci-iocifc) are loaded!");
		kernelInterfaceUsable = false;
		return false;
	}
	if ( logger->isInfoEnabled() )
		logger->info(QString::fromLatin1("Opened virtual usb hcd interface: ID: %1 Bus#: %2").
				arg( QString::fromStdString( hcd->get_bus_id() ), QString::number(hcd->get_usb_bus_num()) ) );

	// finished
	return true;
}

void LinuxVHCIconnector::closeKernelInterface() {
	if ( !hcd ) return;
	// TODO check is devices are connected and disconnect them accordingly...
	delete hcd;
	hcd = NULL;
	kernelInterfaceUsable = true;
}

void LinuxVHCIconnector::startWork() {
	if ( shouldRun ) return;
	shouldRun = true;
	start();
}

void LinuxVHCIconnector::stopWork() {
	if ( !shouldRun ) return;

	// trying to stop worker thread
	shouldRun = false;
	if ( this->isRunning() ) {
		if ( !this->wait(200L) ) {
			workInProgressCondition->wakeAll();
			if ( !this->wait(500L) ) {
				workInProgressCondition->wakeAll();
			}
		}
	}
	if ( this->isRunning() ) {
		logger->warn("Cannot stop worker thread in kernel connection");
	}
}

int LinuxVHCIconnector::connectDevice( USBTechDevice * device ) {
	if ( !hcd && !openKernelInterface() ) {
		return -1;
	}
	if ( !hcd || !kernelInterfaceUsable ) return -1;
	if ( !shouldRun ) startWork();


	int port = getUnusedPort();
	if ( port > 0 ) {
		struct DeviceConnectionData connRequest;
		connRequest.refDevice = device;
		connRequest.port = port;
		connRequest.operationFlag = 1;
		short bcdUSB = USButils::decodeBCDToShort( connRequest.refDevice->bcdUSB );
		if ( bcdUSB >= 0x0200 )
			connRequest.dataRate = usb::data_rate_high;
		else
			connRequest.dataRate = usb::data_rate_full;

/*
		QByteArray deviceDescriptor = QByteArray(18,'\0');	// device descriptor is always 18 bytes in length

		deviceInitializationURBdata[port] = createDeviceDescriptorFromDeviceDescription( deviceDescriptor );
*/
		logger->debug(QString("Enqueing device connect request on port %1").
				arg( QString::number(port) ) );

		connectionRequestQueueMutex->lock();

		portInUseList[port-1] = true;	// mark port as used
		deviceConnectionRequestQueue.enqueue( connRequest );

		connectionRequestQueueMutex->unlock();
		return port;
	}
	return -2;
}

bool LinuxVHCIconnector::disconnectDevice( int portID ) {
	if ( portID < 1 ) return false;
	struct DeviceConnectionData connRequest;
	connRequest.port = portID;
	connRequest.operationFlag = 2;

	logger->debug(QString("Enqueing device disconnect request on port %1").
			arg( QString::number(portID) ) );
	connectionRequestQueueMutex->lock();
	deviceConnectionRequestQueue.enqueue( connRequest );
	connectionRequestQueueMutex->unlock();
	return true;
}


int LinuxVHCIconnector::getUnusedPort() {
	for ( int i = 0; i < numberOfPorts; i++ ) {
		if ( !portInUseList[i] &&
				portStatusList[i] == PORTSTATE_POWERON )
			return i+1;
	}
	return -1;
}

void LinuxVHCIconnector::createDeviceDescriptorFromDeviceDescription( USBTechDevice * device, QByteArray & bytes ) {
	if ( !device ) return;
	bytes[0] = 18;		// overall length
	bytes[1] = 0x01;	// descriptor type (0x01 == device descriptor)
	short value = USButils::decodeBCDToShort( device->bcdUSB );
	bytes[2] = ((value & 0xff00) >> 8);	// bcdUSB == USB version
	bytes[3] = (value & 0xff);
	bytes[4] = (device->bClass & 0xff);
	bytes[5] = (device->bSubClass & 0x0ff);
	bytes[6] = (device->bProtocol & 0x0ff);
	bytes[7] = 0x08;

}

void LinuxVHCIconnector::createURBfromInternalStruct( usb::urb * urbData, QByteArray & buffer ) {
	if ( urbData-> is_control() ) {
		buffer.reserve(8);	// reserve at least 8 bytes
		buffer[0] = urbData->get_bmRequestType();
		buffer[1] = urbData->get_bRequest();
		uint16_t val = urbData->get_wValue();
		buffer[2] = (val & 0xff);	// little endian -> big endian
		buffer[3] = (val >> 8) & 0xff;
		val = urbData->get_wIndex();
		buffer[4] = (val & 0xff);	// little endian -> big endian
		buffer[5] = (val >> 8) & 0xff;
		val = urbData->get_wLength();
		buffer[6] = (val & 0xff);	// little endian -> big endian
		buffer[7] = (val >> 8) & 0xff;
	}
}

void LinuxVHCIconnector::giveBackAnswerURB( void * refData, int portID, bool isOK, QByteArray * urbData ) {
	usb::vhci::process_urb_work * refURB = reinterpret_cast<usb::vhci::process_urb_work*>(refData);
	if ( !refURB ) return;

	struct DeviceURBreplyData replyData;
	replyData.isOK = isOK;
	replyData.refURB = refURB;
	replyData.dataURB = urbData;

	deviceReplyDataQueueMutex->lock();
	deviceReplyDataQueue.enqueue( replyData );
	deviceReplyDataQueueMutex->unlock();
}

bool LinuxVHCIconnector::processOutstandingConnectionRequests() {
	if ( deviceConnectionRequestQueue.isEmpty() ) return false;
	connectionRequestQueueMutex->lock();
	if ( deviceConnectionRequestQueue.isEmpty() ) return false;
	struct DeviceConnectionData connRequest = deviceConnectionRequestQueue.dequeue();
	connectionRequestQueueMutex->unlock();

	if ( connRequest.operationFlag == 2 ) {
		// disconnect operation
		if ( connRequest.port <= 0 ) return false;
		hcd->port_disconnect( connRequest.port );
		portInUseList[connRequest.port -1] = false;
	} else {
		// connect operation
		if ( connRequest.port <= 0 )
			connRequest.port = getUnusedPort();

		if ( connRequest.port > 0 ) {
			hcd->port_connect( connRequest.port, connRequest.dataRate );
		} else
			return false;
	}
	return true;
}

bool LinuxVHCIconnector::processOutstandingURBReplys() {
	if ( deviceReplyDataQueue.isEmpty() ) return false;
	deviceReplyDataQueueMutex->lock();
	if ( deviceReplyDataQueue.isEmpty() ) return false;
	while ( !deviceReplyDataQueue.isEmpty() ) {
		struct DeviceURBreplyData replyData = deviceReplyDataQueue.dequeue();

		try {
			usb::urb * urbOrig = replyData.refURB->get_urb();
			if ( replyData.isOK ) {
				int lenMax = urbOrig->get_buffer_length();
				if ( replyData.dataURB && replyData.dataURB->length() > 0 ) {
					uint8_t* buffer( urbOrig->get_buffer() );	// get buffer from URB struct
					if( replyData.dataURB->length() < lenMax ) lenMax = replyData.dataURB->length();
					const char* replyRawData = replyData.dataURB->constData();
					std::copy( replyRawData, replyRawData + lenMax, buffer );	// copy data
					urbOrig->set_buffer_actual( lenMax );
				}
				urbOrig->ack();
			} else
				urbOrig->stall();

			hcd->finish_work( replyData.refURB );
		} catch ( std::exception &ex ) {
			logger->error( QString::fromLatin1("Exception caught while passing USB reply to host - Error: %1").
					arg( QString(ex.what()) ) );
		}
	}
	deviceReplyDataQueueMutex->unlock();

	return true;
}

void LinuxVHCIconnector::run() {
	bool cont(false);
	if ( !kernelInterfaceUsable ) return;	// nothing to do here!
	while ( applicationShouldRun && shouldRun ) {
		// process device connect / disconnect operations
		processOutstandingConnectionRequests();
		// process URB replys
		processOutstandingURBReplys();

		// Wait if last interaction with kernel still running
		if ( !cont ) {
			workInProgressMutex->lock();
			if ( isWorkInProgress ) {
				isWaitingForWork = true;
				workInProgressCondition->wait( workInProgressMutex, 500L );
				if ( !shouldRun ) return;
			} else
				isWorkInProgress = true;
			workInProgressMutex->unlock();
		}

		// TODO check for memory allocation error / exception
		// get something to do from host controller
		cont = hcd->next_work(&work);
		if ( work ) {
			if ( usb::vhci::port_stat_work* psw = dynamic_cast<usb::vhci::port_stat_work*>(work) ) {
				// Host changes status of port
				uint8_t portID = psw->get_port();
				if( psw->triggers_power_off() || psw->triggers_disable() || psw->triggers_suspend() ) {
					logger->info(QString("Portstatus #%1 powered off / disabled / suspend").
							arg(QString::number(portID)));
					portStatusList[ portID -1 ] = PORTSTATE_DISABLED;
					if ( portInUseList[psw->get_port()-1] ) {
						// port is not in use by us!
						// TODO send signal and disconnect all devices
					}
				} else if ( psw->triggers_power_on() ) {
					logger->info(QString("Portstatus #%1 powered on").
							arg(QString::number(portID)));
					portStatusList[ portID -1 ] = PORTSTATE_POWERON;
					if ( portInUseList[psw->get_port()-1] ) {
						// port is in use by us!
						emit portStatusChanged( portID, PORTSTATE_POWERON );
					}
				} else if ( psw->triggers_reset() ) {
					logger->info(QString("Portstatus #%1 reset on").
							arg(QString::number(portID)));
					// portStatusList[ portID -1 ] = PORTSTATE_RESET;
					if ( hcd->get_port_stat( portID ).get_connection() ) {
						hcd->port_reset_done( portID );
					}
					emit portStatusChanged( portID, PORTSTATE_RESET );

				} else if ( psw->triggers_resuming() ) {
					logger->info(QString("Portstatus #%1 resuming").
							arg(QString::number(portID)));
					// portStatusList[ portID -1 ] = PORTSTATE_RESUME;
					if ( hcd->get_port_stat( portID ).get_connection() ) {
						hcd->port_resumed( portID );
					}
					emit portStatusChanged( portID, PORTSTATE_RESUME );
				}
				hcd->finish_work(work);
			} // portstatus

			else if( usb::vhci::process_urb_work* puw = dynamic_cast<usb::vhci::process_urb_work*>(work) ) {
				uint8_t portID = puw->get_port();
				if ( !puw->is_canceled() ) {
					logger->debug(QString("Process URB for port %1").arg( QString::number(portID) ) );
					// get URB data from work structure
					usb::urb* urbData = puw->get_urb();
					if ( urbData ) {
						// emit urb data to receiver
						uint8_t endPtNo = urbData->get_endpoint_number();	// which endpoint
						uint16_t xferFlags = urbData->get_flags();  // transfer flags
						TI_WusbStack::eDataDirectionType dirType =
								urbData->is_out()? TI_WusbStack::DATADIRECTION_OUT : TI_WusbStack::DATADIRECTION_IN;
						TI_WusbStack::eDataTransferType xferType = TI_WusbStack::CONTROL_TRANSFER;
						if ( urbData->is_control() )
							xferType = TI_WusbStack::CONTROL_TRANSFER;
						else if ( urbData->is_bulk() )
							xferType = TI_WusbStack::BULK_TRANSFER;
						else if ( urbData->is_interrupt() )
							xferType = TI_WusbStack::INTERRUPT_TRANSFER;
						else if ( urbData->is_isochronous() )
							xferType = TI_WusbStack::ISOCHRONOUS_TRANSFER;

						QByteArray * urbRawData = new QByteArray;
						createURBfromInternalStruct( urbData, *urbRawData );
						switch ( portID ) {
						case 1:
							emit urbDataSend1( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 2:
							emit urbDataSend2( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 3:
							emit urbDataSend3( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 4:
							emit urbDataSend4( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 5:
							emit urbDataSend5( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 6:
							emit urbDataSend6( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 7:
							emit urbDataSend7( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						case 8:
							emit urbDataSend8( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData );
							break;
						}

					}
				} else {
					logger->debug(QString("Got canceled URB for port %1").arg( QString::number(portID) ) );
				}
			}

			else if( usb::vhci::cancel_urb_work* cuw = dynamic_cast<usb::vhci::cancel_urb_work*>(work) ) {
				// TODO
				logger->info(QString("Cancel URB for port %1").arg( QString::number(cuw->get_port()) ) );
				hcd->finish_work(work);
			}

			else {
				// ???
				logger->warn("Unknown work object!?");
				hcd->finish_work(work);
			}
		}
	}
}

// EOC
