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
			int endpoint,
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
	virtual bool sendURB( const QByteArray & urbData,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			int endpoint,
			int receiveLength ) = 0;

signals:
	/**
	 * Signal to be emmited when a
	 */
	void receivedURB( const QByteArray & );

};


#endif	/* TI_WUSBSTACK_H_ */
