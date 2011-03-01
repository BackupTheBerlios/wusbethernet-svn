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
#include <stdio.h>

VirtualUSBdevice::VirtualUSBdevice( TI_USB_VHCI * connectorSource, int portID ) {
	connector = connectorSource;
	this->portID = portID;
	logger = Logger::getLogger("VHCI");
}

VirtualUSBdevice::~VirtualUSBdevice() {

}

void VirtualUSBdevice::processURB(
		void * refData, uint16_t transferFlags, uint8_t endPointNo,
		TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
		QByteArray * urbData )
{
	printf("processURB\n");
	connector->giveBackAnswerURB( refData, portID, false, NULL );
}
