/*
 * TI_WusbStack.h
 * Abstract interface defining all necessary functions to connect to a
 * specific USB device on a network attached USB server and send/transmit
 * <tt>URB</tt> packets.<br>
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-01-04
 */

#ifndef TI_WUSBSTACK_H_
#define TI_WUSBSTACK_H_

#include <QObject>
#include <stdint.h>

class TI_USB_VHCI;

class TI_WusbStack : public QObject {
	Q_OBJECT
public:
	enum eDataTransferType {
		CONTROL_TRANSFER,
		BULK_TRANSFER,
		INTERRUPT_TRANSFER,
		ISOCHRONOUS_TRANSFER
	};
	enum eDataDirectionType {
		DATADIRECTION_IN,
		DATADIRECTION_OUT,
		DATADIRECTION_CONTROL,
		DATADIRECTION_NONE
	};
	enum eStackState {
		STATE_DISCONNECTED,
		STATE_CONNECTED,
		STATE_OPENED,
		STATE_CLOSED,
		STATE_FAILED
	};


	TI_WusbStack( QObject * parent = 0 ) : QObject( parent ) {};
	virtual ~TI_WusbStack() {};

	/**
	 * Open connection to device
	 * @return <code>true</code> if no fatal errors (socket open, network error etc) occur.
	 */
	virtual bool openConnection() = 0;
	/**
	 * Closing connection to device and end up (finalize) network connection to device
	 * @return <code>true</code> if no fatal errors occur and connection is active.
	 */
	virtual bool closeConnection() = 0;
	/**
	 * Send USB request block (<em>URB</em>) to device. The URB is wrapped with
	 * headers and if necessary broken into smaller pieces for transport.<br>
	 * PRE: network connection to device initialized and not in error state.
	 * @param	urbData				URB data (raw data, NOT 0x0 terminated)
	 * @param	urbDataLen			Length of urb data
	 * @param	dataTransferType	Transfer type of URB
	 * @param	directionType		Direction of data transfer
	 * @parma	endpoint			Endpoint / Recipient
	 * @param	receiveLength		Length (in bytes) of expected respond from device
	 * @return	<code>true</code> if no fatal errors occur and connection is active.
	 */
	virtual bool sendURB( const char * urbData, int urbDataLen,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			uint8_t endpoint, uint16_t transferFlags,
			int receiveLength ) = 0;
	/**
	 * Send USB request block (<em>URB</em>) to device. The URB is wrapped with
	 * headers and if necessary broken into smaller pieces for transport.<br>
	 * PRE: network connection to device initialized and not in error state.
	 * @param	urbData
	 * @param	dataTransferType	Transfer type of URB
	 * @param	directionType		Direction of data transfer
	 * @parma	endpoint			Endpoint / Recipient
	 * @param	receiveLength		Length (in bytes) of expected respond from device
	 * @return	<code>true</code> if no fatal errors occur and connection is active.
	 */
	virtual bool sendURB( QByteArray * urbData,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			uint8_t endpoint, uint16_t transferFlags,
			int receiveLength ) = 0;

	/**
	 * Registers an URB receiver object.
	 * This object will get all URB replys from network hub instead of sending per signal.
	 */
	virtual void registerURBreceiver( TI_USB_VHCI * urbSink ) = 0;

	static QString transferTypeToString( eDataTransferType dataTransferType ) {
		switch ( dataTransferType ) {
		case CONTROL_TRANSFER:
			return QString::fromLatin1("ControlTransfer");
		case BULK_TRANSFER:
			return QString::fromLatin1("BulkTransfer");
		case ISOCHRONOUS_TRANSFER:
			return QString::fromLatin1("IsoTransfer");
		case INTERRUPT_TRANSFER:
			return QString::fromLatin1("InterruptTransfer");
		}
	};
	static QString dataDirectionToString( eDataDirectionType directionType ) {
		switch ( directionType ) {
		case DATADIRECTION_IN:
			return QString::fromLatin1("IN");
		case DATADIRECTION_OUT:
			return QString::fromLatin1("OUT");
		case DATADIRECTION_CONTROL:
			return QString::fromLatin1("CONTROL");
		case DATADIRECTION_NONE:
			return QString::fromLatin1("NONE");
		}
	}

public slots:
	virtual void processURB( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReceiveLength ) = 0;

signals:
	/**
	 * Signal to be emmited when an URB is received.
	 */
	void receivedURB( const QByteArray & );

};


#endif	/* TI_WUSBSTACK_H_ */
