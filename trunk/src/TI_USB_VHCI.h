/**
 * TI_USB_VHCI.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-03-01
 */

#ifndef TI_USB_VHCI_H_
#define TI_USB_VHCI_H_

#include <QThread>

class QByteArray;
class USBTechDevice;

class TI_USB_VHCI : public QThread {
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

	TI_USB_VHCI( QObject * parent = 0 ) : QThread( parent ) {};
	virtual ~TI_USB_VHCI() {};

	/**
	 * Returns if kernel interface is connected.
	 */
	virtual bool isConnected() = 0;
	/**
	 * Closes the (Linux-) kernel <em>usb-vhci</em> interface.
	 */
	virtual void closeInterface() = 0;
	/**
	 * Connect given device to kernel.
	 * @param	device	descriptor
	 * @param	portID	port number (if not given, a number will be automatically assigned)
	 * @return			port number
	 */
	virtual int connectDevice( USBTechDevice * device, int portID = -1 ) = 0;
	/**
	 * Disconnect device on given port from OS.
	 * @param	portID	port number
	 */
	virtual bool disconnectDevice( int portID ) = 0;
	/**
	 * Retrieves and reserves a free port-ID.<br>
	 * If port-ID is not needed / used use <tt>disconnectDevice</tt> to
	 * free port-ID.
	 * @return	port number / ID
	 */
	virtual int getAndReservePortID() = 0;
	/**
	 * Passes back an answer to host for last request.
	 * @param	refData		Pointer to reference data (was given when sending data packet)
	 * @param	isOK		If request was acknowledged by device
	 * @param	urbData		Byte array with raw URB data from device
	 */
	virtual void giveBackAnswerURB( void * refData, bool isOK, QByteArray * urbData ) = 0;
};

#endif /* TI_USB_VHCI_H_ */
