/**
 * VirtualUSBdevice.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-03-01
 */

#include "VirtualUSBdevice.h"
#include "../utils/Logger.h"
#include "../TI_USB_VHCI.h"
#include "../TI_WusbStack.h"
#include "../BasicUtils.h"
#include <stdio.h>

VirtualUSBdevice::VirtualUSBdevice( TI_USB_VHCI * connectorSource, int portID ) {
	connector = connectorSource;
	this->portID = portID;
	logger = Logger::getLogger("TEST");
	logger->info("Virtual USB device initialized...");
}

VirtualUSBdevice::~VirtualUSBdevice() {

}


QByteArray * VirtualUSBdevice::getDescriptorData( int type, int index, int len ) {
	QByteArray * retValue = NULL;
	switch ( type ) {
	case 1:
		logger->info("GET_DESCRIPTOR: Device");
		retValue = new QByteArray(18,'\0');
		(*retValue)[0] = 18;	// descriptor length
		(*retValue)[1] = 1;		// type: device descriptor
		(*retValue)[2] = 0x10;	// bcd usb release number (USB 1.1)
		(*retValue)[3] = 0x01;	//  "
		(*retValue)[4] = 0;		// device class: per interface
		(*retValue)[5] = 0;		// device sub class
		(*retValue)[6] = 0;		// device protocol
		(*retValue)[7] = 8;	// max packet size	(64)
		(*retValue)[8] = 0xad;	// vendor id
		(*retValue)[9] = 0xde;	//  "
		(*retValue)[10] = 0xef;	// product id
		(*retValue)[11] = 0xbe;	//  "
		(*retValue)[12] = 0x38;	// bcd device release number
		(*retValue)[13] = 0x11;	//  "
		(*retValue)[14] = 2;	// manufacturer string
		(*retValue)[15] = 1;	// product string
		(*retValue)[16] = 0;	// serial number string
		(*retValue)[17] = 1;	// number of configurations
		break;
	case 2:
		logger->info(QString("GET_DESCRIPTOR: Configuration (len = %1)").arg( QString::number(len) ) );
		if ( len <= 9 )
			retValue = new QByteArray(9,'\0');
		else
			retValue = new QByteArray(18,'\0');
		(*retValue)[0] =  9;		// descriptor length
		(*retValue)[1] =  2;		// type: configuration descriptor
		(*retValue)[2] = 18;	// total descriptor length (configuration+interface)
		(*retValue)[3] =  0;	//  "
		(*retValue)[4] =  1;	// number of interfaces
		(*retValue)[5] =  1;	// configuration index
		(*retValue)[6] =  0;	// configuration string
		(*retValue)[7] =  0x80;	// attributes: none
		(*retValue)[8] =  0;	// max power

		if ( len <= 9 )
			break;
		(*retValue)[9] =  9;	// descriptor length
		(*retValue)[10] =  4;	// type: interface
		(*retValue)[11] =  0;	// interface number
		(*retValue)[12] =  0;	// alternate setting
		(*retValue)[13] =  0;	// number of endpoints
		(*retValue)[14] =  0;	// interface class
		(*retValue)[15] =  0;	// interface sub class
		(*retValue)[16] =  0;	// interface protocol
		(*retValue)[17] =  0;	// interface string
		break;
	case 3:
		logger->info(QString("GET_DESCRIPTOR: String (type = %1)").arg( QString::number(index) ) );
		switch ( index ) {
		case 0:
			// language code list
			retValue = new QByteArray(4,'\0');
			(*retValue)[0] = 4;	// descriptor length
			(*retValue)[1] = 3;	// type: string
			(*retValue)[2] = 0x09; // LSB lang id: en_US
			(*retValue)[3] = 0x04; // MSB lang id: en_US
			break;
		case 1:
			// string on index 1
			retValue = new QByteArray(26,'\0');
			(*retValue)[0] =  26;	// length
			(*retValue)[1] =   3;	// type: string
			(*retValue)[2] =  'H';
			(*retValue)[3] =   0;
			(*retValue)[4] =  'e';
			(*retValue)[5] =   0;
			(*retValue)[6] =  'l';
			(*retValue)[7] =   0;
			(*retValue)[8] =  'l';
			(*retValue)[9] =   0;
			(*retValue)[10] = 'o';
			(*retValue)[11] =  0;
			(*retValue)[12] = ' ';
			(*retValue)[13] =  0;
			(*retValue)[14] = 'W';
			(*retValue)[15] =  0;
			(*retValue)[16] = 'o';
			(*retValue)[17] =  0;
			(*retValue)[18] = 'r';
			(*retValue)[19] =  0;
			(*retValue)[20] = 'l';
			(*retValue)[21] =  0;
			(*retValue)[22] = 'd';
			(*retValue)[23] =  0;
			(*retValue)[24] = '!';
			(*retValue)[25] =  0;
			break;
		case 2:
			// string on index 2
			retValue = new QByteArray(10,'\0');
			(*retValue)[0] =  10;	// length
			(*retValue)[1] =   3;	// type: string
			(*retValue)[2] =  'A';
			(*retValue)[3] =   0;
			(*retValue)[4] =  'C';
			(*retValue)[5] =   0;
			(*retValue)[6] =  'M';
			(*retValue)[7] =   0;
			(*retValue)[8] =  'E';
			(*retValue)[9] =   0;
			break;
		}
	}
	return retValue;
};


void VirtualUSBdevice::processURB(
		void * refData, uint16_t transferFlags, uint8_t endPointNo,
		TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
		QByteArray * urbData, int expectedReturnLength )
{
	printf("XXX processURB\n");

	if ( transferType != TI_WusbStack::CONTROL_TRANSFER && endPointNo != 0 ) {
		if ( logger->isDebugEnabled() )
			logger->debug(QString("VirtualDevice: Send STALL on URB packet - XferType=%1 Direction=%2 endPtNo=%3").arg(
					TI_WusbStack::transferTypeToString(transferType),
					TI_WusbStack::dataDirectionToString(dDirection),
					QString::number(endPointNo) ));
		connector->giveBackAnswerURB( refData, false, NULL );
		return;
	}
	if ( !urbData ) {
		if ( logger->isDebugEnabled() )
			logger->debug(QString("VirtualDevice: Send STALL on URB packet: no data!s - XferType=%1 Direction=%2 endPtNo=%3").arg(
					TI_WusbStack::transferTypeToString(transferType),
					TI_WusbStack::dataDirectionToString(dDirection),
					QString::number(endPointNo) ));
		connector->giveBackAnswerURB( refData, false, NULL );
		return;
	}

	logger->debug(QString("VirtualDevice: packet: %1").arg( messageToString( *urbData, 8 )) );

	uint8_t requestType = urbData->at(0);
	uint8_t request     = urbData->at(1);
	if ( requestType == 0x0 ) {
		logger->debug(QString("VirtualDevice: IN packet requestType=%1 request=%2").
				arg(QString::number(requestType),QString::number(request) ) );
		connector->giveBackAnswerURB( refData, true, NULL );
		return;
	}
	if ( requestType == 0x80 ) {
		if ( request == 0x06 ) {
			// GET_DESCRIPTOR
//			int wValue  = bytesToShort( urbData->at(2), urbData->at(3) );
			int wLength = bytesToShort( urbData->at(6), urbData->at(7) );
//			int wIndex  = bytesToShort( urbData->at(4), urbData->at(5) );
			QByteArray * returnData = getDescriptorData( urbData->at(3), urbData->at(2), wLength );
			if ( returnData )
				connector->giveBackAnswerURB( refData, true, returnData );
			else
				connector->giveBackAnswerURB( refData, false, NULL );

			if ( urbData ) delete urbData;
			return;
		}
	}

	connector->giveBackAnswerURB( refData, false, NULL );
	if ( urbData ) delete urbData;
}




uint16_t VirtualUSBdevice::bytesToShort( uint8_t byteLSB, uint8_t byteMSB ) {
	uint16_t sh = 0;
	sh |= byteLSB;
	sh |= ((byteMSB << 8) & 0xff00);
	return sh;
}
