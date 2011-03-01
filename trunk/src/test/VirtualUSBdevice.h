/**
 * VirtualUSBdevice.h
 * Implements a simple USB device to test VHCI.
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-03-01
 */

#ifndef VIRTUALUSBDEVICE_H_
#define VIRTUALUSBDEVICE_H_

#include "../TI_WusbStack.h"
#include <QObject>
#include <QByteArray>
#include <stdint.h>

class Logger;
class TI_USB_VHCI;

class VirtualUSBdevice : public QObject {
	Q_OBJECT
public:
	VirtualUSBdevice( TI_USB_VHCI * connectorSource, int portID );
	virtual ~VirtualUSBdevice();

private:
	Logger * logger;
	TI_USB_VHCI * connector;
	int portID;

public slots:
	void processURB( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData );

};

#endif /* VIRTUALUSBDEVICE_H_ */
