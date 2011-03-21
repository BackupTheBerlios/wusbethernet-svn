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
#include "../BasicUtils.h"
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
		: TI_USB_VHCI( parent ) {
	instance = this;
	hcd = NULL;
	shouldRun = false;
	kernelInterfaceUsable = true; // default: everything should be ok


	numberOfPorts = 6;	// TODO config!
	portStatusList = new PortStatusData_t[numberOfPorts];
	for ( int i = 0; i < numberOfPorts; i++ ) {
		portStatusList[i].portOK = false;
		portStatusList[i].portInUse = false;
		portStatusList[i].portStatus = TI_USB_VHCI::PORTSTATE_UNKNOWN_STATE;
		portStatusList[i].portEnumeratedByHost = 0;
		portStatusList[i].initialConnectDeviceDescriptor = NULL;
		portStatusList[i].deviceInInitPhase = false;
		portStatusList[i].lastURBhandle = 0;
		portStatusList[i].packetCount = 0;
	}

	// synchronization mutex
	connectionRequestQueueMutex = new QMutex;
	deviceReplyDataQueueMutex = new QMutex;

	nextConnectionRequestDeferValue = 0L;

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

bool LinuxVHCIconnector::openInterface() {
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

void LinuxVHCIconnector::closeInterface() {
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

int LinuxVHCIconnector::connectDevice( USBTechDevice * device, int portID ) {
	if ( !hcd && !openInterface() ) {
		return -2;
	}
	if ( !hcd || !kernelInterfaceUsable ) {
		return -2;
	}
	if ( !shouldRun ) startWork();

	if ( portID < 1 )
		portID = getUnusedPort();

	if ( portID > 0 ) {
		struct DeviceConnectionData_t connRequest;
		connRequest.refDevice = device;
		connRequest.port = portID;
		connRequest.operationFlag = 1;
		short bcdUSB = USButils::decodeBCDToShort( connRequest.refDevice->bcdUSB );
		if ( bcdUSB >= 0x0200 )
			connRequest.dataRate = usb::data_rate_high;
		else
			connRequest.dataRate = usb::data_rate_full;

		QByteArray * deviceDescriptor = new QByteArray(18,'\0');	// device descriptor is always 18 bytes in length
		createDeviceDescriptorFromDeviceDescription( device, *deviceDescriptor );
		connRequest.initialDeviceDescriptor = deviceDescriptor;
		logger->debug(QString("Fake init descriptor: %1").arg( messageToString( *deviceDescriptor, 0 ) ) );

		logger->debug(QString("Enqueueing device connect request on port %1").
				arg( (portID > 0? QString::number(portID) : "(unspecified)") ) );

		connectionRequestQueueMutex->lock();

		portStatusList[portID-1].portInUse = true; // mark port as used
		portStatusList[portID-1].packetCount = 0;
		deviceConnectionRequestQueue.enqueue( connRequest );

		connectionRequestQueueMutex->unlock();
	}
	return portID;
}

bool LinuxVHCIconnector::disconnectDevice( int portID ) {
	if ( portID < 1 ) return false;
	struct DeviceConnectionData_t connRequest;
	connRequest.port = portID;
	connRequest.operationFlag = 2;

	logger->debug(QString("Enqueing device disconnect request on port %1").
			arg( QString::number(portID) ) );
	connectionRequestQueueMutex->lock();
	deviceConnectionRequestQueue.enqueue( connRequest );
	connectionRequestQueueMutex->unlock();
	return true;
}

int LinuxVHCIconnector::getAndReservePortID() {
	int portID = getUnusedPort();
	portStatusList[portID-1].portInUse = true;// mark port as used
	return portID;
}

int LinuxVHCIconnector::getUnusedPort() {
	for ( int i = 0; i < numberOfPorts; i++ ) {
		if ( portStatusList[i].portOK &&
				!portStatusList[i].portInUse &&
				portStatusList[i].portStatus == TI_USB_VHCI::PORTSTATE_POWERON )
			return i+1;
	}
	return -1;
}

void LinuxVHCIconnector::createDeviceDescriptorFromDeviceDescription( USBTechDevice * device, QByteArray & bytes ) {
	if ( !device ) return;
	// Creating an standard device descriptor
	bytes.reserve(18);	// reserving at least 18 bytes space
	bytes[0] = 18;		// overall length
	bytes[1] = 0x01;	// descriptor type (0x01 == device descriptor)
	short value = USButils::decodeBCDToShort( device->bcdUSB );
	bytes[2] = (value & 0xff);				// LSB bcdUSB == USB version
	bytes[3] = ((value & 0xff00) >> 8);		// MSB bcdUSB
	bytes[4] = (device->bClass & 0xff);		// device class
	bytes[5] = (device->bSubClass & 0x0ff);	// device subclass
	bytes[6] = (device->bProtocol & 0x0ff);	// device protocol
	bytes[7] = 0x40;						// XXX packet length
	bytes[8] = ((device->idVendor & 0xff00) >> 8);	// LSB vendor ID
	bytes[9] = (device->idVendor & 0xff);			// MSB vendor ID
	bytes[10] = ((device->idProduct & 0xff00) >> 8);	// LSB product ID
	bytes[11] = (device->idProduct & 0xff);			// MSB product ID
	value = USButils::decodeBCDToShort( device->bcdDevice );
	bytes[12] = (value & 0xff);				// LSB bcdDevice
	bytes[13] = ((value & 0xff00) >> 8);	// MSB bcdDevice
	bytes[14] = 0;							// String idx manufacturer
	bytes[15] = 0;							// String idx product
	bytes[16] = 0;							// String idx serial no
	bytes[17] = 1;							// No of configurations (faking to always 1)
}

void LinuxVHCIconnector::createURBfromInternalStruct( usb::urb * urbData, QByteArray & buffer, int portID ) {
	if ( urbData->is_control() ) {
		int lenData = urbData->get_buffer_actual();
		buffer.reserve(8 + lenData);	// reserve at least 8 bytes + length of data section
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

		// additional data to transfer: copy to buffer
		if ( lenData >  0 ) {
			uint8_t* dataBuf = urbData->get_buffer();
			buffer.append( (const char*) dataBuf, lenData );
		}


		if ( logger->isDebugEnabled() ) {
			QString xferModeStr;
			switch ( urbData->get_type() ){
			case usb::urb_type_isochronous:
				xferModeStr = "ISO"; break;
			case usb::urb_type_bulk:
				xferModeStr = "BULK"; break;
			case usb::urb_type_control:
				xferModeStr = "CRTL"; break;
			case usb::urb_type_interrupt:
				xferModeStr = "INTR"; break;
			default:
				xferModeStr = "UKN"; break;
			}
			QString dirStr = "?";
			if ( urbData->is_in() ) dirStr = "IN";
			else if ( urbData->is_out() ) dirStr = "OUT";
			logger->debug(QString("VHCIconn: URB %1 from system (len=%2, actual=%3, "
					"isoCount=%4, XferMode=%5, Dir=%6, Flags=0x%7 EndPt=%8): %9").arg(
							QString::number( portStatusList[portID-1].packetCount ),
							QString::number( urbData->get_buffer_length() ),
							QString::number( urbData->get_buffer_actual() ),
							QString::number( urbData->get_iso_packet_count() ),
							xferModeStr,
							dirStr,
							QString::number( urbData->get_flags(), 16),
							QString::number( urbData->get_endpoint_number() ),
							messageToString( buffer, buffer.length() ) ) );
		}
	} else {
		if ( logger->isDebugEnabled() ) {
			QString xferModeStr;
			switch ( urbData->get_type() ){
			case usb::urb_type_isochronous:
				xferModeStr = "ISO"; break;
			case usb::urb_type_bulk:
				xferModeStr = "BULK"; break;
			case usb::urb_type_control:
				xferModeStr = "CRTL"; break;
			case usb::urb_type_interrupt:
				xferModeStr = "INTR"; break;
			default:
				xferModeStr = "UKN"; break;
			}
			QString dirStr = "?";
			if ( urbData->is_in() ) dirStr = "IN";
			else if ( urbData->is_out() ) dirStr = "OUT";

			logger->debug(QString("XX VHCIconn: URB %1 from system (len=%2, actual=%3, "
					"isoCount=%4, XferMode=%5, Dir=%6, Flags=0x%7, EndPt=%8): %9").arg(
							QString::number( portStatusList[portID-1].packetCount ),
							QString::number( urbData->get_buffer_length() ),
							QString::number( urbData->get_buffer_actual() ),
							QString::number( urbData->get_iso_packet_count() ),
							xferModeStr,
							dirStr,
							QString::number( urbData->get_flags(), 16),
							QString::number( urbData->get_endpoint_number() ),
							messageToString( urbData->get_buffer(),
									(urbData->get_buffer_length()> 8? 10 : 8) )) );
		}
	}
		/*
		logger->debug(QString("XXX VHCIconn: URB from system (len=%1,%2): %3").arg(
				QString::number( urbData->get_iso_packet_length(0) ),
				QString::number( urbData->get_iso_packet_actual(0) ),
				messageToString( urbData->get_iso_packet_buffer(0),
						(urbData->get_iso_packet_length(0)> 8? 10 : 8) )) );
		*/
}

void LinuxVHCIconnector::giveBackAnswerURB( void * refData, bool isOK, QByteArray * urbData ) {
	printf("XXX giveBackAnswerURB isOK=%s!\n", (isOK?"1":"0") );

	usb::vhci::process_urb_work * refURB = reinterpret_cast<usb::vhci::process_urb_work*>(refData);
	if ( !refURB ) return;

	struct DeviceURBreplyData replyData;
	replyData.isOK = isOK;
	replyData.refURB = refURB;
	replyData.dataURB = urbData;

	deviceReplyDataQueueMutex->lock();
	deviceReplyDataQueue.enqueue( replyData );
	deviceReplyDataQueueMutex->unlock();
	printf("XXX giveBackAnswerURB finished!\n" );
	// wake up working thread
	workInProgressCondition->wakeAll();
}

bool LinuxVHCIconnector::processOutstandingConnectionRequests() {
	if ( deviceConnectionRequestQueue.isEmpty() ) return false;
	long long nowTimeStamp = 0L;
	if ( nextConnectionRequestDeferValue > 0L ) {
		nowTimeStamp = currentTimeMillis();
		if ( nextConnectionRequestDeferValue > nowTimeStamp )
			return false;
	}

	connectionRequestQueueMutex->lock();
	if ( deviceConnectionRequestQueue.isEmpty() ) return false;
	struct DeviceConnectionData_t connRequest = deviceConnectionRequestQueue.dequeue();
	connectionRequestQueueMutex->unlock();

	if ( connRequest.operationFlag == 2 ) {
		// disconnect operation
		if ( connRequest.port <= 0 ) return false;
		hcd->port_disconnect( connRequest.port );
		if ( portStatusList[connRequest.port -1].lastURBhandle )
			hcd->cancel_process_urb_work( portStatusList[connRequest.port -1].lastURBhandle );
		portStatusList[connRequest.port -1].portInUse = false;
	} else {
		// connect operation
		if ( connRequest.port <= 0 )
			connRequest.port = getUnusedPort();

		if ( connRequest.port > 0 ) {
			QString datarateStr = "none";
			switch ( connRequest.dataRate ) {
			case usb::data_rate_high:
				datarateStr = "high";
				break;
			case usb::data_rate_full:
				datarateStr = "full";
				break;
			case usb::data_rate_low:
				datarateStr = "low";
				break;
			}
			portStatusList[connRequest.port -1].initialConnectDeviceDescriptor = connRequest.initialDeviceDescriptor;
			portStatusList[connRequest.port -1].deviceInInitPhase = true;

			logger->info( QString("Connecting device on port %1 with datarate %2").arg(
					QString::number(connRequest.port), datarateStr ) );
			hcd->port_connect( connRequest.port, connRequest.dataRate );
		} else {
			// XXX connect later depends on additional work done in connectDevice method! -> still to do
			nextConnectionRequestDeferValue = currentTimeMillis() + 10000L;	// next try in 10 secs
			// if no port is available just put connection request job back into queue
			connectionRequestQueueMutex->lock();
			deviceConnectionRequestQueue.enqueue( connRequest );
			connectionRequestQueueMutex->unlock();

			return false;
		}
	}
	return true;
}

bool LinuxVHCIconnector::processOutstandingURBReplys() {
	if ( deviceReplyDataQueue.isEmpty() ) return false;
	deviceReplyDataQueueMutex->lock();
	if ( deviceReplyDataQueue.isEmpty() ) return false;
	while ( !deviceReplyDataQueue.isEmpty() ) {
		struct DeviceURBreplyData replyData = deviceReplyDataQueue.dequeue();
		int portID = replyData.refURB->get_port();
		printf("XXX processOutstandingURBReplys!\n");

		if ( portStatusList[portID-1].lastURBhandle )
			portStatusList[portID-1].lastURBhandle = 0;
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

					if ( logger->isDebugEnabled() )
						logger->debug(QString("VHCIconn: Replypacket (len=%1): %2").arg(
								QString::number( lenMax ),
								messageToString( buffer, lenMax )) );
				}
				logger->debug(QString("URB reply: URB %1 - Sending ACK, buffer length=%2").arg(
						QString::number( portStatusList[portID-1].packetCount ),
						QString::number(lenMax) ) );
				urbOrig->ack();
			} else {
				logger->debug(QString("URB reply: Sending STALL"));
				urbOrig->stall();
			}
			hcd->finish_work( replyData.refURB );
		} catch ( std::exception &ex ) {
			logger->error( QString::fromLatin1("Exception caught while passing USB reply to host - Error: %1").
					arg( QString(ex.what()) ) );
		}

		if ( replyData.dataURB )
			delete replyData.dataURB;
	}
	deviceReplyDataQueueMutex->unlock();

	return true;
}

void LinuxVHCIconnector::run() {
	bool cont(false);
	if ( !kernelInterfaceUsable ) return;	// nothing to do here!
	while ( applicationShouldRun && shouldRun ) {
		// Wait if last interaction with kernel still running
		if ( !cont ) {
			workInProgressMutex->lock();
			if ( isWorkInProgress ) {
				isWaitingForWork = true;
				workInProgressCondition->wait( workInProgressMutex, 150L );
				if ( !shouldRun ) return;
			} else
				isWorkInProgress = true;
			workInProgressMutex->unlock();
		}

		// process device connect / disconnect operations
		processOutstandingConnectionRequests();
		// process URB replys
		processOutstandingURBReplys();

		// TODO check for memory allocation error / exception
		// get something to do from host controller
		try {
			cont = hcd->next_work(&work);
		} catch ( std::exception &ex ) {
			logger->error(QString("Error: %1").arg(QString(ex.what())));
		}
		if ( work ) {
			if ( usb::vhci::port_stat_work* psw = dynamic_cast<usb::vhci::port_stat_work*>(work) ) {
				// Host changes status of port
				uint8_t portID = psw->get_port();
				if( psw->triggers_power_off() || psw->triggers_disable() || psw->triggers_suspend() ) {
					logger->info(QString("USB Port #%1 powered off").
							arg(QString::number(portID)));

					portStatusList[ portID -1 ].portStatus = TI_USB_VHCI::PORTSTATE_DISABLED;
					if ( portStatusList[ portID -1 ].portInUse ) {
						// port is in use by us!
						// TODO send signal and disconnect all devices
						//						logger->info(QString("port #%1 not in use?").arg( QString::number(portID) ) );
						portStatusList[portID-1].deviceInInitPhase = true;
					}
				}
				if ( psw->triggers_power_on() ) {
					logger->info(QString("USB Port #%1 powered on").
							arg(QString::number(portID)));
					portStatusList[ portID -1 ].portStatus = TI_USB_VHCI::PORTSTATE_POWERON;
					portStatusList[ portID -1 ].portOK = true;
					if ( portStatusList[psw->get_port()-1].portInUse ) {
						// port is in use by us!
						emit portStatusChanged( portID, PORTSTATE_POWERON );
					}
				}
				if ( psw->triggers_reset() ) {
					logger->info(QString("USB Port #%1 reset").
							arg(QString::number(portID)));
					// portStatusList[ portID -1 ] = PORTSTATE_RESET;
					if ( hcd->get_port_stat( portID ).get_connection() ) {
						logger->info(" - Completing reset on port");
						hcd->port_reset_done( portID );
					}
					emit portStatusChanged( portID, PORTSTATE_RESET );
				}

				if ( psw->triggers_resuming() ) {
					logger->info(QString("USB Port #%1 resuming").
							arg(QString::number(portID)));
					portStatusList[ portID -1 ].portStatus = TI_USB_VHCI::PORTSTATE_POWERON;
					if ( hcd->get_port_stat( portID ).get_connection() ) {
						hcd->port_resumed( portID );
					}
					emit portStatusChanged( portID, PORTSTATE_RESUME );
				}

				if ( psw->triggers_suspend() || psw->triggers_disable() ) {
					logger->info(QString("USB Port #%1 disabled / suspend").
							arg(QString::number(portID)));
					portStatusList[ portID -1 ].portStatus = TI_USB_VHCI::PORTSTATE_SUSPENDED;
					if ( portStatusList[psw->get_port()-1].portInUse ) {
						// port is in use by us!urbOrig->get_buffer()
						// TODO send signal and disconnect all devices
					}
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
						portStatusList[portID -1].lastURBhandle = urbData->get_handle();
						portStatusList[portID-1].packetCount++;

/*						// workaround for reinit of port
						if ( urbData->is_control() &&
								urbData->get_bmRequestType() == 0x00 &&
								urbData->get_bRequest() == 0x05 ) {
							portStatusList[portID-1].deviceInInitPhase = true;
						}
*/
						if ( portStatusList[portID-1].deviceInInitPhase ) {
							// if we are still in init phase all communication is performed here!
							if ( urbData->is_control() ) {
								// Check for "GET_DESCRIPTOR(device)" request
								if ( urbData->get_bmRequestType() == 0x80 &&
										urbData->get_bRequest() == 0x06 &&
										urbData->get_wValue() == 0x0100 ) {
									if ( portStatusList[portID-1].initialConnectDeviceDescriptor ) {
										if ( logger->isDebugEnabled() )
											logger->debug("Sending fake device descriptor...");
										// send device descriptor
										int lenMax = urbData->get_wLength();
										QByteArray * deviceDescriptor = portStatusList[portID-1].initialConnectDeviceDescriptor;
										if ( deviceDescriptor->length() < lenMax ) lenMax = deviceDescriptor->length();
										const char* replyRawData = deviceDescriptor->constData();
										std::copy( replyRawData, replyRawData + lenMax, urbData->get_buffer() );	// copy data
										urbData->set_buffer_actual( lenMax );
										urbData->ack();
										// clean up
										delete portStatusList[portID-1].initialConnectDeviceDescriptor;
										portStatusList[portID-1].initialConnectDeviceDescriptor = NULL;
										hcd->finish_work(work);
										continue;
									} else {
										// Problem!
										logger->error("Not sending fake device descriptor - ERROR");
//										urbData->stall();
									}
								} else if ( urbData->get_bmRequestType() == 0x00 &&
										urbData->get_bRequest() == 0x05 ) {
									// host is setting device enumeration number
									uint16_t devAddress = urbData->get_wValue();
									logger->info(QString("Host sets address %1 for device.").arg(QString::number(devAddress) ) );
									portStatusList[portID-1].portEnumeratedByHost = (uint8_t) devAddress;

									portStatusList[portID-1].deviceInInitPhase = false;
									urbData->ack();
									hcd->finish_work(work);
									continue;
								}
							} else
								portStatusList[portID-1].deviceInInitPhase = false;
						}
						// communicate with network stack

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
						createURBfromInternalStruct( urbData, *urbRawData, portID );
						switch ( portID ) {
						case 1:
							emit urbDataSend1( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 2:
							emit urbDataSend2( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 3:
							emit urbDataSend3( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 4:
							emit urbDataSend4( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 5:
							emit urbDataSend5( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 6:
							emit urbDataSend6( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 7:
							emit urbDataSend7( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						case 8:
							emit urbDataSend8( puw, xferFlags, endPtNo,
									xferType, dirType,
									urbRawData, urbData->get_buffer_length() );
							break;
						}
					} else
						printf("no URB data???\n");
				} else {
					logger->debug(QString("Got canceled URB for port %1").arg( QString::number(portID) ) );
				}
			}

			else if( usb::vhci::cancel_urb_work* cuw = dynamic_cast<usb::vhci::cancel_urb_work*>(work) ) {
				uint8_t portID = cuw->get_port();
				logger->info(QString("Cancel URB for port %1").arg( QString::number( portID ) ) );
				// assuming that canceled packet was last sent packet...
				portStatusList[portID -1].lastURBhandle = 0L;
				// TODO cancel URB in wusb stack
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
